/*
 * MacTRMNL: A simple TRMNL Client for Classic Mac OS.
 * MacTRMNL.c
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
#include <Controls.h>
#include <Menus.h>
#include <Devices.h>
#include <OSUtils.h>
#include <Gestalt.h>
#include <Sound.h>
#include <string.h>

// Local includes
#include "Logging.h"
#include "MacTCPHelper.h"
#include "Preferences.h"

// Constants
#define kMaxBMPSize			    65536L

#define kSleep				    60

#define kOn				        1
#define kOff				    0

#define kStartWindowWidth	    300
#define kStartWindowHeight      200
#define kStartButtonWidth       80
#define kStartButtonHeight      20
#define kStartButtonTop         100

#define kMenuBarID              128
#define kAppleMenu			    128
#define kFileMenu			    129
#define kFileMenuRefreshItem    1  /* Refresh */
#define kFileMenuSettingsItem   3  /* Settings (separator is item 2) */
#define kFileMenuQuitItem       5  /* Quit (separator is item 4) */
#define	kEditMenu			    130
#define kAboutDialogID          129  /* Alert resource ID */
#define kSettingsDialogID       128  /* Dialog resource ID */

#define kAboutItem			    1
#define kQuitItem			    1

#define kDisplayWindow          129
#define kStartButtonID          1   /* OK/Start button */
#define kIPAddressItem          2   /* IP Address edit text */
#define kPortItem               3   /* Port edit text */
#define kRefreshRateItem            4   /* Refresh Rate edit text */
#define kAutoRefreshItem        5   /* Auto Refresh checkbox */
#define kEnableLogFileItem      6   /* Enable Log checkbox */
#define kSaveSettingsItem       7   /* Save Settings checkbox */
#define kExitButtonID           8   /* Exit button */
#define kSaveButtonID           9   /* Save button */

/* Strucs and Enums */
typedef struct {
    char ipAddress[20];
    long port;
    long refreshRate;
    Boolean autoRefresh;
    Boolean enableLogFile;
    Boolean saveSettings;
} AppSettings;

/* Globals */
WindowPtr		gSettingsWindow;
WindowPtr       gMainWindow;
MenuHandle		gAppleMenu;
MenuHandle		gFileMenu;
MenuHandle		gEditMenu;
Boolean		    gEndProgram = false;
Boolean         gReturnToSettings = false;  /* Flag to return to settings dialog */
Boolean         gRefreshImage = false;      /* Flag to refresh the image */
EventRecord	    gTheEvent;
AppSettings     gSavedSettings;
Ptr             gBmpData = NULL;
long            gDataSize = 0;
StreamPtr       gTcpStream = NULL;          /* Global TCP stream for refresh */
ip_addr         gServerIP;

// Logging globals
short gLogFileRefNum = 0;

/* Function Prototypes */
void Draw1BitBMPFromData(WindowPtr win, Ptr bmpData, long dataSize, Boolean centerImage);
void InitializeToolbox(void);
void SetUpMenus(void);
void DrawCenteredText(Str255 text, short y);
void ShowAboutDialog(void);
void HandleMouseDown(void);
void HandleMenuChoice(long menuChoice);
void HandleAppleMenu(short item);
void HandleFileMenu(short item);
Handle DialogItemGet(DialogPtr dialog, int item);
void ControlSetValue(DialogPtr dialog, int item, int value);
int ControlGetValue(DialogPtr dialog, int item);
void ControlFlip(DialogPtr dialog, int item);
void RestoreSettings(DialogPtr settingsDialog);
void SaveSettings(DialogPtr settingsDialog);
void SettingsDialogInit(void);
Boolean HandleSettingsDialog(void);  /* Returns true to connect, false to quit */
void HandleEvent(void);
void RefreshImage(void);  /* Download and display new image */

/* External functions from MacTCPHelper */
extern OSErr DoTCPControl(TCPiopb *pb);
extern short gTCPDriverRefNum;

/* Draw the 1-bit BitMap from raw data */
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
    portRect = qd.screenBits.bounds;
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
    OSErr err;
    long dataSize;
    DialogPtr settingsDialog;
    short dialogItemHit;
    Boolean keepTrying = true;
    
    // Initialize logging
	InitLogging();
	LogInfo("MacTRMNL Started");
	
    // Initialize everything
    LogInfo("Initializing Toolbox...");
	InitializeToolbox();
	LogInfo("Toolbox initialized");
	
	LogInfo("Setting up menus...");
	SetUpMenus();
	LogInfo("Menus set up");
	
    // Initialize settings (loads from preferences if available)
    LogInfo("Initializing settings dialog...");
    SettingsDialogInit();
    LogInfo("Settings dialog initialized");
    
    // Initialize MacTCP once at startup
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
    LogInfo("MacTCP initialized successfully");
    
    // Main application loop - allows returning to settings from image display
    while (!gEndProgram) {
        keepTrying = true;
        gReturnToSettings = false;
        
        // Connection loop - keep trying until successful or user quits
        while (keepTrying && !gEndProgram) {
        // Show the Settings Dialog
        LogInfo("Showing settings dialog...");
        if (!HandleSettingsDialog()) {
            LogInfo("User cancelled from settings dialog");
            keepTrying = false;
            gEndProgram = true;
            break;  // Exit the retry loop
        }
        LogInfo("User clicked Start, attempting connection...");
        
        // Initialize the main display window
        if (gMainWindow == NULL) {
            gMainWindow = GetNewWindow(kDisplayWindow, NULL, (WindowPtr)-1);
        }
        SetPort(gMainWindow);
        ShowWindow(gMainWindow);

        // Convert IP address string to ip_addr
        LogInfo("Parsing IP address...");
        err = ParseIPAddress(gSavedSettings.ipAddress, &gServerIP);
        if (err != noErr) {
            LogError("IP parsing failed. Please check the IP address.");
            SysBeep(10);  // Alert user about parsing failure
            HideWindow(gMainWindow);  // Hide the window
            continue;  // Go back to settings dialog
        }
        LogInfo("IP parsed successfully");
        
        // Connect to server and receive BMP
        LogInfo("Connecting to server...");
        err = ConnectToServer(gServerIP, gSavedSettings.port, &gTcpStream);
        if (err == noErr) {
            LogInfo("Connected! Receiving data...");
            err = ReceiveBMPData(gTcpStream, &gBmpData, &gDataSize);
            if (err == noErr && gBmpData != NULL && gDataSize > 0) {
                LogInfo("Data received! Drawing image...");
                Draw1BitBMPFromData(gMainWindow, gBmpData, gDataSize, true);
                // Keep gBmpData for redraws
                keepTrying = false;  // Success! Exit the connection loop
            } else {
                if (err != noErr) {
                    LogError("Receive function failed");
                } else if (gBmpData == NULL) {
                    LogError("No buffer returned");
                } else {
                    LogError("No data in buffer");
                }
                SysBeep(10);
                HideWindow(gMainWindow);  // Hide the window
                // Continue to retry loop
            }
        } else {
            LogError("Connection failed! Please check server address and port.");
            SysBeep(10);
            HideWindow(gMainWindow);  // Hide the window
            // Continue to retry loop
        }
        }  // End of connection loop
        
        // Main Event Loop - only reached after successful connection
        if (!gEndProgram && !keepTrying) {  // Successfully connected
            gEndProgram = false;  // Reset flag
            while (gEndProgram == false) {
                HandleEvent();
                
                // Check if refresh was requested
                if (gRefreshImage) {
                    gRefreshImage = false;  // Reset flag
                    RefreshImage();
                }
            }
            
            // Check if user wants to return to settings
            if (gReturnToSettings) {
                LogInfo("Returning to settings...");
                gEndProgram = false;  // Reset flag to continue main loop
                HideWindow(gMainWindow);  // Hide the display window
                // Free the BMP data
                if (gBmpData != NULL) {
                    DisposePtr(gBmpData);
                    gBmpData = NULL;
                    gDataSize = 0;
                }
            }
        }
    }  // End of main application loop
    
    // Cleanup before exit
    CleanupTCP();
    CloseLog();
}

/* HandleEvent() */
void HandleEvent(void) {
	char key;
	Boolean dummy;

	WaitNextEvent(everyEvent, &gTheEvent, kSleep, NULL);

	switch (gTheEvent.what) {
        case updateEvt:
            BeginUpdate(gMainWindow);
            if (gBmpData != NULL) {
                Draw1BitBMPFromData(gMainWindow, gBmpData, gDataSize, true);
            }
            EndUpdate(gMainWindow);
            break;

		case mouseDown:
			HandleMouseDown();
			break;

		case keyDown: case autoKey:
			key = (char)(gTheEvent.message & charCodeMask);
            if (key == 27 || ((gTheEvent.modifiers & cmdKey) && (key == 'Q' || key == 'q'))) {
                CleanupTCP();
                if (gBmpData != NULL) {
                    DisposePtr(gBmpData);
                }
                CloseLog();
            ExitToShell();
            } else if ((gTheEvent.modifiers & cmdKey) != 0) {
				HandleMenuChoice(MenuKey(key));
			}
			break;

		case nullEvent:
			//HandleCountDown();
			break;
	}
}

/* Initialize the Mac Toolbox */
void InitializeToolbox(void) {
#ifdef __GNUC__
	// For Retro68, use qd global structure
	InitGraf(&qd.thePort);
#else
	// For THINK C, use thePort directly
	InitGraf(&thePort);
#endif
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
	
	menuBar = GetNewMBar(kMenuBarID);
	
	if (menuBar == nil)
		return;
		
	SetMenuBar(menuBar);
	
	gAppleMenu = GetMenuHandle(kAppleMenu);
	gFileMenu = GetMenuHandle(kFileMenu);
	gEditMenu = GetMenuHandle(kEditMenu);
	
	if (gAppleMenu != nil)
		AppendResMenu(gAppleMenu, 'DRVR');
		
	DrawMenuBar();
}

/* Handle MouseDown Events */
void HandleMouseDown(void) {
	WindowPtr whichWindow;
	short part;
	long menuChoice;
	ControlHandle control;
	
	part = FindWindow(gTheEvent.where, &whichWindow);
	
	switch (part) {
        case inSysWindow:
            SystemClick(&gTheEvent, whichWindow);
            break;
        
		case inMenuBar:
			menuChoice = MenuSelect(gTheEvent.where);
			HandleMenuChoice(menuChoice);
			break;
			
		case inContent:
			if (whichWindow == gSettingsWindow) {
				SetPort(gSettingsWindow);
				GlobalToLocal(&gTheEvent.where);
				part = FindControl(gTheEvent.where, gSettingsWindow, &control);
				if (part && TrackControl(control, gTheEvent.where, nil)) {
                    // Control was clicked - handle if needed
				}
			}
			break;
			
		case inDrag:
			if (whichWindow == gSettingsWindow) {
				DragWindow(whichWindow, gTheEvent.where, &qd.screenBits.bounds);
			} else if (whichWindow == gMainWindow) {
                DragWindow(whichWindow, gTheEvent.where, &qd.screenBits.bounds);
            }
			break;
			
		case inGoAway:
			if (whichWindow == gSettingsWindow && TrackGoAway(whichWindow, gTheEvent.where)) {
			    gEndProgram = true;
			} else if (whichWindow == gMainWindow && TrackGoAway(whichWindow, gTheEvent.where)) {
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
		GetMenuItemText(gAppleMenu, item, name);
		GetPort(&savePort);
		daRefNum = OpenDeskAcc(name);
		SetPort(savePort);
	}
}

/* File Menu */
void HandleFileMenu(short item) {
    switch (item) {
        case kFileMenuRefreshItem:
            // Refresh the image
            gRefreshImage = true;
            break;
        case kFileMenuSettingsItem:
            // Return to settings dialog
            gReturnToSettings = true;
            gEndProgram = true;  // Exit the current event loop
            break;
        case kFileMenuQuitItem:
            gEndProgram = true;
            break;
    }
}
	
/* Show About Dialog */
void ShowAboutDialog(void) {
	Alert(kAboutDialogID, nil);
}

/* Get Dialog Item Handle */
Handle DialogItemGet(DialogPtr dialog, int item) {
	DialogItemType itemType;
	Rect itemRect;
	Handle itemHandle;
	GetDialogItem(dialog, item, &itemType, &itemHandle, &itemRect);
	return itemHandle;
}

/* Set Control Value */
void ControlSetValue(DialogPtr dialog, int item, int value) {
	DialogItemType itemType;
	Rect itemRect;
	Handle itemHandle;
	
	GetDialogItem(dialog, item, &itemType, &itemHandle, &itemRect);
	
	/* Check if it's actually a control (checkbox, radio button, etc.) */
	if ((itemType & ctrlItem) && itemHandle != NULL) {
		SetControlValue((ControlHandle)itemHandle, value);
	}
}

/* Get Control Value */
int ControlGetValue(DialogPtr dialog, int item) {
	DialogItemType itemType;
	Rect itemRect;
	Handle itemHandle;
	
	GetDialogItem(dialog, item, &itemType, &itemHandle, &itemRect);
	
	/* Check if it's actually a control */
	if ((itemType & ctrlItem) && itemHandle != NULL) {
		return GetControlValue((ControlHandle)itemHandle);
	}
	return 0;
}

/* Toggle control value between kOn and kOff */
void ControlFlip(DialogPtr dialog, int item) {
	int currentValue = ControlGetValue(dialog, item);
	ControlSetValue(dialog, item, (currentValue == kOn) ? kOff : kOn);
}

/* Restore Settings */
void RestoreSettings(DialogPtr settingsDialog) {
	Handle itemHandle;
    Str255 portString;
    Str255 ipPascalString;
    Str255 refreshRateString;

    LogInfo("Restoring IP address...");
    // Restore IP Address
    itemHandle = DialogItemGet(settingsDialog, kIPAddressItem);
    if (itemHandle != NULL) {
        // Convert C string to Pascal string
        ipPascalString[0] = strlen(gSavedSettings.ipAddress);
        BlockMove(gSavedSettings.ipAddress, &ipPascalString[1], ipPascalString[0]);
        SetDialogItemText(itemHandle, ipPascalString);
    }

    LogInfo("Restoring port...");
    // Restore Port
    itemHandle = DialogItemGet(settingsDialog, kPortItem);
    if (itemHandle != NULL) {
        NumToString(gSavedSettings.port, portString);
        SetDialogItemText(itemHandle, portString);
    }

    LogInfo("Restoring refresh rate...");
    // Restore Refresh Rate
    itemHandle = DialogItemGet(settingsDialog, kRefreshRateItem);
    if (itemHandle != NULL) {
        NumToString(gSavedSettings.refreshRate, refreshRateString);
        SetDialogItemText(itemHandle, refreshRateString);
    }

    LogInfo("Restoring checkboxes...");
    // Restore "Auto Refresh" Option
    ControlSetValue(settingsDialog, kAutoRefreshItem, gSavedSettings.autoRefresh);

    // Restore "Enable Log File" Option
    ControlSetValue(settingsDialog, kEnableLogFileItem, gSavedSettings.enableLogFile);

    // Restore "Save Settings" Option
    ControlSetValue(settingsDialog, kSaveSettingsItem, gSavedSettings.saveSettings);
}

/* Save Settings */
void SaveSettings(DialogPtr settingsDialog) {
	Handle itemHandle;
	Str255 ipAddressString;
    Str255 portString;
    Str255 refreshRateString;

    // Save IP Address
    itemHandle = DialogItemGet(settingsDialog, kIPAddressItem);
	GetDialogItemText(itemHandle, ipAddressString);
	// Convert Pascal string to C string
	BlockMove(&ipAddressString[1], gSavedSettings.ipAddress, ipAddressString[0]);
	gSavedSettings.ipAddress[ipAddressString[0]] = '\0';

    // Save Port
    itemHandle = DialogItemGet(settingsDialog, kPortItem);
	GetDialogItemText(itemHandle, portString);
	StringToNum(portString, &gSavedSettings.port);

    // Save Refresh Rate
    itemHandle = DialogItemGet(settingsDialog, kRefreshRateItem);
	GetDialogItemText(itemHandle, refreshRateString);
	StringToNum(refreshRateString, &gSavedSettings.refreshRate);

    // Save "Auto Refresh" Option
    gSavedSettings.autoRefresh = ControlGetValue(settingsDialog, kAutoRefreshItem);

    // Save "Enable Log File" Option
    gSavedSettings.enableLogFile = ControlGetValue(settingsDialog, kEnableLogFileItem);

    // Save "Save Settings" Option
    gSavedSettings.saveSettings = ControlGetValue(settingsDialog, kSaveSettingsItem);
    
    // Save preferences to disk if requested
    if (gSavedSettings.saveSettings) {
        PrefsData prefs;
        strcpy(prefs.ipAddress, gSavedSettings.ipAddress);
        prefs.port = gSavedSettings.port;
        prefs.refreshRate = gSavedSettings.refreshRate;
        prefs.autoRefresh = gSavedSettings.autoRefresh;
        prefs.enableLogFile = gSavedSettings.enableLogFile;
        prefs.saveSettings = gSavedSettings.saveSettings;
        
        SavePreferences(&prefs);
        // We don't check the error here - preferences saving is best-effort
    }
}

/* Initialize Settings Defaults */
void SettingsDialogInit(void) {
    OSErr err;
    PrefsData prefs;
    
    // Try to load preferences first
    err = LoadPreferences(&prefs);
    if (err == noErr) {
        // Successfully loaded preferences
        strcpy(gSavedSettings.ipAddress, prefs.ipAddress);
        gSavedSettings.port = prefs.port;
        gSavedSettings.refreshRate = prefs.refreshRate;
        gSavedSettings.autoRefresh = prefs.autoRefresh;
        gSavedSettings.enableLogFile = prefs.enableLogFile;
        gSavedSettings.saveSettings = prefs.saveSettings;
    } else {
        // Use defaults if preferences couldn't be loaded
        strcpy(gSavedSettings.ipAddress, "10.0.1.26"); // Default IP
        gSavedSettings.port = 1337; // Default port
        gSavedSettings.refreshRate = 10; // Default refresh rate
        gSavedSettings.autoRefresh = kOn; // Enable auto refresh by default
        gSavedSettings.enableLogFile = kOn; // Enable log file by default
        gSavedSettings.saveSettings = kOn; // Save settings by default
    }
}

/* Handle Settings Dialog */
Boolean HandleSettingsDialog(void) {
    Boolean dialogDone;
    Boolean userWantsToConnect = false;
    DialogItemIndex itemHit;
    Handle itemHandle;
    DialogPtr settingsDialog;

    LogInfo("Getting new dialog...");
    settingsDialog = GetNewDialog(kSettingsDialogID, NULL, (WindowPtr)-1);
    if (settingsDialog == NULL) {
        LogError("Failed to get settings dialog");
        return false;  // Can't continue without dialog
    }
    LogInfo("Got dialog, showing window...");
    ShowWindow(settingsDialog);
    LogInfo("Window shown, restoring settings...");
    RestoreSettings(settingsDialog);
    LogInfo("Settings restored");

    dialogDone = false;

    while (dialogDone == false) {
        ModalDialog(NULL, &itemHit);

        switch (itemHit) {
            case kStartButtonID:
                SaveSettings(settingsDialog); // update them.
                userWantsToConnect = true;
                dialogDone = true;
                break;
            case kExitButtonID:
                userWantsToConnect = false;
                dialogDone = true;
                break;
            case kSaveButtonID:
                SaveSettings(settingsDialog); // save without starting
                LogInfo("Settings saved");
                // Flash the button or beep to indicate save
                SysBeep(1);
                break;
            case kEnableLogFileItem:
            case kSaveSettingsItem:
                ControlFlip(settingsDialog, itemHit);
                break;
        }
    }
    DisposeDialog(settingsDialog);
    return userWantsToConnect;
}

/* Refresh Image - download and display new image */
void RefreshImage(void) {
    OSErr err;
    Ptr newBmpData = NULL;
    long newDataSize = 0;
    TCPiopb pb;
    
    LogInfo("Refreshing image...");
    
    // Check if we have a valid connection
    if (gTcpStream == NULL) {
        LogError("No active connection for refresh");
        return;
    }
    
    // Close the existing connection
    // The server expects one image per connection
    LogInfo("Closing existing connection...");
    pb.ioCompletion = NULL;
    pb.ioCRefNum = gTCPDriverRefNum;
    pb.csCode = TCPClose;
    pb.tcpStream = gTcpStream;
    pb.csParam.close.validityFlags = 0;
    pb.csParam.close.ulpTimeoutValue = 30;
    pb.csParam.close.ulpTimeoutAction = 1;
    
    DoTCPControl(&pb);
    
    // Release the stream
    pb.ioCompletion = NULL;
    pb.ioCRefNum = gTCPDriverRefNum;
    pb.csCode = TCPRelease;
    pb.tcpStream = gTcpStream;
    
    DoTCPControl(&pb);
    gTcpStream = NULL;
    
    // Reconnect to server
    LogInfo("Reconnecting to server...");
    err = ConnectToServer(gServerIP, gSavedSettings.port, &gTcpStream);
    if (err != noErr) {
        LogError("Refresh failed - couldn't reconnect");
        SysBeep(10);
        gTcpStream = NULL;
        return;
    }
    
    // Receive new BMP data
    LogInfo("Downloading new image...");
    err = ReceiveBMPData(gTcpStream, &newBmpData, &newDataSize);
    if (err == noErr && newBmpData != NULL && newDataSize > 0) {
        // Free old image data
        if (gBmpData != NULL) {
            DisposePtr(gBmpData);
        }
        
        // Update with new data
        gBmpData = newBmpData;
        gDataSize = newDataSize;
        
        // Redraw the window
        LogInfo("Drawing new image...");
        SetPort(gMainWindow);
        InvalRect(&gMainWindow->portRect);  // Force window update
        
        LogInfo("Image refreshed successfully");
    } else {
        LogError("Failed to receive new image data");
        SysBeep(10);
    }
}