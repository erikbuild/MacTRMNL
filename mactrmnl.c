/*
 * MacTRMNL: A simple TRMNL Client for Classic Mac OS.
 * mactrmnl.c
 * 
 * A Classic Mac OS application that connects to a server, receives a 1-bit BMP image,
 * and displays it in a fullscreen window. This code is designed to run on System 6.0.8 and later with MacTCP support.
 * It uses the MacTCP API to establish a TCP connection, receive BMP data, and draw it on the screen.
 * 
 * Written by Erik Reynolds
 * v20250702-1
 */
 
#include <QuickDraw.h>
#include <Windows.h>
#include <Files.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Fonts.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Desk.h>
#include <Devices.h>
#include <OSUtils.h>
#include <GestaltEqu.h>
#include <string.h>

// Local includes
#include "Logging.h"
#include "MacTCPHelper.h"

// Constants
#define kMaxBMPSize			65536L

#define kStartWindowWidth	300
#define kStartWindowHeight   200
#define kStartButtonWidth   80
#define kStartButtonHeight  20
#define kStartButtonTop		100

#define kStartButtonID      128
#define kAppleMenu			128
#define kFileMenu			129
#define	kEditMenu			130
#define kAboutDialogID      128
#define kSettingsDialogID   129

#define kAboutItem			1
#define kQuitItem			1


/* Globals */
WindowPtr		gWindow;
ControlHandle	gStartButton;
MenuHandle		gAppleMenu;
MenuHandle		gFileMenu;
MenuHandle		gEditMenu;
Boolean		    gEndProgram = false;

// Application globals  
WindowPtr mainWindow;

// Dialog item constants
#define kIPAddressItem 3
#define kPortItem 5
#define kStartButton 6
#define kCancelButton 7

// Logging globals
short gLogFileRefNum = 0;

/* Function Prototypes */
void Draw1BitBMPFromData(WindowPtr win, Ptr bmpData, long dataSize, Boolean centerImage);
Boolean ShowSettingsDialog(char *ipString, int *port);
void InitializeToolbox(void);
void SetUpMenus(void);
void ShowStartWindow(void);
void DrawCenteredText(Str255 text, short y);
void ShowAboutDialog(void);
void HandleMouseDown(EventRecord *event);
void HandleMenuChoice(long menuChoice);
void HandleAppleMenu(short item);
void HandleFileMenu(short item);


void Draw1BitBMPFromData(WindowPtr win, Ptr bmpData, long dataSize, Boolean centerImage) {
    int row, col;
    unsigned char *pixelData;
    int width, height, rowSize;
    int i;
    Rect destRect;
    unsigned char *headerBytes;
    unsigned char *infoBytes;
    long pixelOffset;
    int xOffset;
    int yOffset;
    Rect portRect;
    BitMap offBitMap;
    GrafPtr oldPort;
    Ptr offBaseAddr;
    int offRowBytes;
    Rect srcRect;
    int srcByte;
    unsigned char *srcPtr;
    unsigned char *destPtr;
    
    SetPort(win);
    
    // Check minimum size
    if (dataSize < 54) {  // Min size for headers
        LogError("Invalid BMP data - too small");
        return;
    }
    
    headerBytes = (unsigned char *)bmpData;
    
    // Check signature
    if (headerBytes[0] != 0x42 || headerBytes[1] != 0x4D) { 
        LogError("Not a valid BMP file - invalid signature");
        return; 
    }
    
    infoBytes = (unsigned char *)bmpData + 14;
    
    if (infoBytes[14] != 1) { 
        LogError("Not a 1-bit BMP file");
        return; 
    }
    
    // Read dimensions
    width = infoBytes[4] | (infoBytes[5] << 8) | (infoBytes[6] << 16) | (infoBytes[7] << 24);
    height = infoBytes[8] | (infoBytes[9] << 8) | (infoBytes[10] << 16) | (infoBytes[11] << 24);
    rowSize = ((width + 31) / 32) * 4;
    
    // Get pixel data offset
    pixelOffset = headerBytes[10] | (headerBytes[11] << 8) | (headerBytes[12] << 16) | (headerBytes[13] << 24);
    
    if (pixelOffset + (rowSize * height) > dataSize) {
        LogError("Incomplete BMP data");
        return;
    }
    
    pixelData = (unsigned char *)bmpData + pixelOffset;
    
    // Create offscreen bitmap
    GetPort(&oldPort);
    
    // Calculate row bytes for Mac bitmap (must be even)
    offRowBytes = ((width + 15) / 16) * 2;
    
    // Allocate memory for offscreen bitmap
    offBaseAddr = NewPtr(offRowBytes * height);
    if (offBaseAddr == NULL) {
        return;
    }
    
    // Clear the offscreen bitmap to white
    for (i = 0; i < offRowBytes * height; i++) {
        offBaseAddr[i] = 0xFF;
    }
    
    // Set up bitmap structure
    offBitMap.baseAddr = offBaseAddr;
    offBitMap.rowBytes = offRowBytes;
    offBitMap.bounds.top = 0;
    offBitMap.bounds.left = 0;
    offBitMap.bounds.bottom = height;
    offBitMap.bounds.right = width;
    
    // Convert BMP data to Mac bitmap format
    for (row = 0; row < height; row++) {
        srcPtr = pixelData + (height - 1 - row) * rowSize;
        destPtr = (unsigned char *)offBaseAddr + row * offRowBytes;
        
        // Process bytes
        for (col = 0; col < width; col += 8) {
            srcByte = col / 8;
            if (srcByte < rowSize) {
                // Invert bits since BMP and Mac have opposite conventions
                destPtr[col / 8] = ~srcPtr[srcByte];
            }
        }
    }
    
    // Calculate destination rectangle
    portRect = screenBits.bounds;
    if (centerImage) {
        xOffset = (portRect.right - width) / 2;
        yOffset = (portRect.bottom - height) / 2;
    } else {
        xOffset = 10;
        yOffset = 10;
    }
    
    destRect.top = yOffset;
    destRect.left = xOffset;
    destRect.bottom = yOffset + height;
    destRect.right = xOffset + width;
    
    srcRect = offBitMap.bounds;
    
    // Copy the bitmap to screen
    SetPort(win);
    CopyBits(&offBitMap, &win->portBits, &srcRect, &destRect, srcCopy, NULL);
    
    // Clean up
    DisposePtr(offBaseAddr);
    SetPort(oldPort);
}


// Main entry point
void main(void) {
    Rect winRect;
    EventRecord event;
    Pattern black;
    char key;
    OSErr err;
    ip_addr serverIP;
    Ptr bmpData = NULL;
    long dataSize;
    
    int serverPort = 1337;
    char ipString[20] = "10.0.1.26";
    
    // Initialize logging
	InitLogging();
	
    // Initialize everything
	InitializeToolbox();
	SetUpMenus();
	
    // Show the startup window
    ShowStartWindow();
	   
    // Initialize MacTCP
    err = InitMacTCP();
    if (err != noErr) {
        // Log specific error
        if (err == gestaltUnknownErr) {
            LogError("MacTCP not installed - need MacTCP");
        } else {
            LogError("MacTCP driver failed to open");
        }
        SysBeep(10);
        while (!Button()) { }  // Wait for mouse click
        CloseLog();
        ExitToShell();
    }
    
    // Log success
    LogInfo("MacTCP initialized successfully");
    
    // Get screen bounds and create full screen window
    winRect = screenBits.bounds;
    
    // Use plainDBox window type for borderless window
    mainWindow = NewWindow(NULL, &winRect, "\p", true, plainDBox, (WindowPtr)-1, false, 0);
    SetPort(mainWindow);
    
    // Fill screen with white background
    EraseRect(&winRect);
    
    ShowWindow(mainWindow);
    
    // Convert IP address string to ip_addr
    LogInfo("Parsing IP address...");
    err = ParseIPAddress(ipString, &serverIP);
    if (err != noErr) {
        LogError("IP parsing failed.");
        SysBeep(10);  // Alert user about parsing failure
        CleanupTCP();
        CloseLog();
        ExitToShell();
    } else {
        LogInfo("IP parsed successfully");
    }
    
    // Connect to server and receive BMP
    LogInfo("Connecting to server...");
    err = ConnectToServer(serverIP, serverPort, &tcpStream);
    if (err == noErr) {
        LogInfo("Connected! Receiving data...");
        err = ReceiveBMPData(tcpStream, &bmpData, &dataSize);
        if (err == noErr && bmpData != NULL && dataSize > 0) {
            LogInfo("Data received! Drawing image...");
            Draw1BitBMPFromData(mainWindow, bmpData, dataSize, true);
            // Keep bmpData for redraws
        } else {
            if (err != noErr) {
                LogError("Receive function failed");
            } else if (bmpData == NULL) {
                LogError("No buffer returned");
            } else {
                LogError("No data in buffer");
            }
            SysBeep(10);
            CleanupTCP();
            CloseLog();
            ExitToShell();
        }
    } else {
        LogError("Connection failed!");
        SysBeep(10);
        CleanupTCP();
        CloseLog();
        ExitToShell();
    }

    if (!Button()) {} // Wait for mouse click to continue
    
    // Basic event loop
    while (true) {
        if (WaitNextEvent(everyEvent, &event, 60, NULL)) {
            if (event.what == updateEvt) {
                BeginUpdate(mainWindow);
                if (bmpData != NULL) {
                    Draw1BitBMPFromData(mainWindow, bmpData, dataSize, true);
                }
                EndUpdate(mainWindow);
            } else if (event.what == keyDown) {
                key = (char)(event.message & charCodeMask);
                // Escape key or Cmd+Q quits
                if (key == 27 || ((event.modifiers & cmdKey) && (key == 'Q' || key == 'q'))) {
                    CleanupTCP();
                    if (bmpData != NULL) {
                        DisposePtr(bmpData);
                    }
                    CloseLog();
                    ExitToShell();
                }
            } else if (event.what == mouseDown) {
                // Click anywhere to exit full screen
                CleanupTCP();
                if (bmpData != NULL) {
                    DisposePtr(bmpData);
                }
                CloseLog();
                ExitToShell();
            }
        }
    }
}

/* Show the Startup Settings Dialog */
void ShowStartWindow(void) {
    Dialog(kSettingsDialogID, nil);
}

/* Initialize the Mac Toolbox */
void InitializeToolbox(void) {
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
}

/* Draw centered text */
void DrawCenteredText(Str255 text, short y) {
	short width;
	
	width = StringWidth(text);
	MoveTo(kStartWindowWidth - width / 2, y);
	DrawString(text);
}

/* Set Up Menus */
void SetUpMenus(void) {
	Handle menuBar;
	
	menuBar = GetNewMBar(128);
	
	if (menuBar == nil)
		return;
		
	SetMenuBar(menuBar);
	
	gAppleMenu = GetMHandle(kAppleMenu);
	gFileMenu = GetMHandle(kFileMenu);
	gEditMenu = GetMHandle(kEditMenu);
	
	if (gAppleMenu != nil)
		AddResMenu(gAppleMenu, 'DRVR');
		
	DrawMenuBar();
}

/* Handle MouseDown Events */
void HandleMouseDown(EventRecord *event) {
	WindowPtr whichWindow;
	short part;
	long menuChoice;
	ControlHandle control;
	
	part = FindWindow(event->where, &whichWindow);
	
	switch (part) {
		case inMenuBar:
			menuChoice = MenuSelect(event->where);
			HandleMenuChoice(menuChoice);
			break;
			
		case inContent:
			if (whichWindow == gWindow) {
				SetPort(gWindow);
				GlobalToLocal(&event->where);
				part = FindControl(event->where, gWindow, &control);
				if (part && TrackControl(control, event->where, nil)) {
			//		if (control == gRol????????START BUTTON) {
			//			DO THE THING();
			//		}
				}
			}
			break;
			
		case inDrag:
			if (whichWindow == gWindow) {
				DragWindow(whichWindow, event->where, &qd.screenBits.bounds);
			}
			break;
			
		case inGoAway:
			if (whichWindow == gWindow && TrackGoAway(whichWindow, event->where)) {
			    gEndProgram = true;
			}
			break;
	}
}

/* Handle Menu Selection */
void HandleMenuChoice(long menuChoice) {
	short menu, item;
	
	if (menuChoice != 0) {
		menu = HiWord(menuChoice);
		item = LoWord(menuChoice);
		
		switch (menu) {
			case kAppleMenu:
				HandleAppleMenu(item);
				break;
		
			case kFileMenu:
				HandleFileMenu(item);
				break;
		}
		
		HiliteMenu(0);
	}
}

/* Apple Menu */
void HandleAppleMenu(short item) {
	Str255 name;
	short daRefNum;
	GrafPtr savePort;
	
	if (item == kAboutItem) {
		ShowAboutDialog();
	} else {
		GetItem(gAppleMenu, item, name);
		GetPort(&savePort);
		daRefNum = OpenDeskAcc(name);
		SetPort(savePort);
	}
}

/* File Menu */
void HandleFileMenu(short item) {
	if (item == kQuitItem) {
		gEndProgram = true;
	}
}
	
/* Show About Dialog */
void ShowAboutDialog(void) {
	Alert(kAboutDialogID, nil);
}
