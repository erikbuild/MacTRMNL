/*
 * MacTRMNL: A simple TRMNL Client for Classic Mac OS.
 * mactrmnl.c
 * 
 * A Classic Mac OS application that connects to a server, receives a 1-bit BMP image,
 * and displays it in a fullscreen window. This code is designed to run on System 6.0.8 and later with MacTCP support.
 * It uses the MacTCP API to establish a TCP connection, receive BMP data, and draw it on the screen.
 * 
 * Written by Erik Reynolds
 * v20250701-1
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

// THINK C 5.0 Compatibility definitions for MacTCP
#define __TYPES__
#define __QUICKDRAW__
#define __CONDITIONALMACROS__
#define __MACTYPES__
#define __MIXEDMODE__
#undef GENERATINGCFM

// Define modern types for THINK C 5.0
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef short SInt16;
typedef unsigned long UInt32;
typedef long SInt32;

// Define missing constants for Universal Interfaces
#define kCStackBased 0
#define kPascalStackBased 0
#define STACK_ROUTINE_PARAMETER(n, s) 0
#define SIZE_CODE(s) 0
#define RESULT_SIZE(s) 0
#define CALLBACK_API_C(ret, name) ret (*name)

// MacTCP headers
#include <MacTCP.h>

// Constants
#define kRcvBufferSize 		8192
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

#define kAboutItem			1
#define kQuitItem			1


/* Globals */
WindowPtr		gWindow;
ControlHandle	gStartButton;
MenuHandle		gAppleMenu;
MenuHandle		gFileMenu;
MenuHandle		gEditMenu;
Boolean			gDone = false;

// TCP globals
StreamPtr tcpStream = 0;
WindowPtr mainWindow;
Boolean gHaveMacTCP = false;
short gTCPDriverRefNum = 0;  // MacTCP driver reference number

// Dialog item constants
#define kIPAddressItem 3
#define kPortItem 5
#define kStartButton 6
#define kCancelButton 7

// Logging globals
short gLogFileRefNum = 0;

/* Function Prototypes */
OSErr InitMacTCP(void);
OSErr ParseIPAddress(const char *ipString, ip_addr *ipAddr);
OSErr ConnectToServer(ip_addr serverIP, unsigned short serverPort, StreamPtr *stream);
OSErr ReceiveBMPData(StreamPtr stream, Ptr *bmpData, long *dataSize);
void Draw1BitBMPFromData(WindowPtr win, Ptr bmpData, long dataSize, Boolean centerImage);
void CleanupTCP(void);
Boolean ShowSettingsDialog(char *ipString, int *port);
void InitializeToolbox(void);
void SetUpMenus(void);
void CreateStartWindow(void);
void DrawStartWindow(void);
void DrawCenteredText(Str255 text, short y);
void DoAboutDialog(void);
void HandleMouseDown(EventRecord *event);
void HandleMenuChoice(long menuChoice);
void HandleAppleMenu(short item);
void HandleFileMenu(short item);

// Logging function prototypes
OSErr InitLogging(void);
void LogMessage(const char *prefix, const char *message);
void LogError(const char *message);
void LogInfo(const char *message);
void CloseLog(void);

// Helper function for MacTCP control calls
OSErr DoTCPControl(TCPiopb *pb) {
    OSErr result;
    if (gTCPDriverRefNum == 0) {
        return -1;  // Driver not opened
    }
    pb->ioCRefNum = gTCPDriverRefNum;
    pb->ioCompletion = NULL;  // Ensure synchronous call
    
    // THINK C 5.0 - call Device Manager directly
    result = PBControl((ParmBlkPtr)pb, false);
    return result;
}

OSErr InitMacTCP(void) {
    OSErr err;
    long response;
    
    // Check if MacTCP is installed using Gestalt
    err = Gestalt('mtcp', &response);
    if (err == noErr) {
        // MacTCP is installed
        // The response contains version information
        // Open the MacTCP driver - try different names
        // In THINK C 5.0, we might need to use a parameter block
        {
            ParamBlockRec pb;
            
            // Try .IPP first
            pb.ioParam.ioNamePtr = "\p.IPP";
            pb.ioParam.ioPermssn = 0;
            err = PBOpenSync(&pb);
            
            if (err != noErr) {
                // Try .MacTCP
                pb.ioParam.ioNamePtr = "\p.MacTCP";
                pb.ioParam.ioPermssn = 0;
                err = PBOpenSync(&pb);
            }
            
            if (err == noErr) {
                gTCPDriverRefNum = pb.ioParam.ioRefNum;
            }
        }
        if (err == noErr) {
            gHaveMacTCP = true;
        }
        return err;
    }
    
    // MacTCP not installed
    return err;
}

OSErr ParseIPAddress(const char *ipString, ip_addr *ipAddr) {
    unsigned char octets[4];
    int octetIndex = 0;
    int value = 0;
    const char *p = ipString;
    Boolean hasDigits = false;
    int i;
    
    if (ipString == NULL || ipAddr == NULL) {
        return paramErr;
    }
    
    // Initialize
    for (i = 0; i < 4; i++) {
        octets[i] = 0;
    }
    
    // Parse each character
    while (*p && octetIndex < 4) {
        if (*p >= '0' && *p <= '9') {
            // Digit found
            value = value * 10 + (*p - '0');
            hasDigits = true;
            
            // Check for overflow (values > 255)
            if (value > 255) {
                return paramErr;
            }
        } else if (*p == '.') {
            // End of octet
            if (!hasDigits) {
                return paramErr;  // Empty octet (e.g., "10..1.1")
            }
            octets[octetIndex] = (unsigned char)value;
            octetIndex++;
            value = 0;
            hasDigits = false;
        } else {
            // Invalid character
            return paramErr;
        }
        p++;
    }
    
    // Handle the last octet (no trailing dot)
    if (hasDigits && octetIndex == 3) {
        octets[3] = (unsigned char)value;
        octetIndex++;
    }
    
    // Verify we have exactly 4 octets
    if (octetIndex != 4 || !hasDigits) {
        return paramErr;
    }
    
    // Convert to network byte order (big endian)
    *ipAddr = ((unsigned long)octets[0] << 24) | 
              ((unsigned long)octets[1] << 16) | 
              ((unsigned long)octets[2] << 8) | 
              (unsigned long)octets[3];
    
    return noErr;
}

OSErr ConnectToServer(ip_addr serverIP, unsigned short serverPort, StreamPtr *stream) {
    OSErr err;
    TCPiopb pb;
    tcp_port localPort = 0;  // Let MacTCP assign
    
    // Create stream
    pb.ioCompletion = NULL;
    pb.ioCRefNum = gTCPDriverRefNum;
    pb.csCode = TCPCreate;
    pb.csParam.create.rcvBuff = (Ptr)NewPtr(kRcvBufferSize);
    pb.csParam.create.rcvBuffLen = kRcvBufferSize;
    pb.csParam.create.notifyProc = NULL;
    pb.csParam.create.userDataPtr = NULL;
    
    err = DoTCPControl(&pb);
    if (err != noErr) {
        return err;
    }
    
    *stream = pb.tcpStream;
    
    // Debug: Show stream value
    if (*stream != 0) {
        LogInfo("TCP stream created");
    } else {
        LogError("Failed to create stream");
        return -1;
    }
    
    // Open connection
    pb.ioCompletion = NULL;
    pb.ioCRefNum = gTCPDriverRefNum;
    pb.csCode = TCPActiveOpen;
    pb.tcpStream = *stream;
    pb.csParam.open.ulpTimeoutValue = 30;  // 30 second timeout
    pb.csParam.open.ulpTimeoutAction = 1;  // Abort on timeout
    pb.csParam.open.validityFlags = 0;
    pb.csParam.open.commandTimeoutValue = 30;
    pb.csParam.open.remoteHost = serverIP;
    pb.csParam.open.remotePort = serverPort;
    pb.csParam.open.localPort = localPort;
    pb.csParam.open.tosFlags = 0;
    pb.csParam.open.precedence = 0;
    pb.csParam.open.dontFrag = 0;
    pb.csParam.open.timeToLive = 0;
    pb.csParam.open.security = 0;
    pb.csParam.open.optionCnt = 0;
    
    err = DoTCPControl(&pb);
    
    if (err == noErr) {
        // Connection initiated, but may not be established yet
        // Add a small delay to let connection establish
        long finalTicks = TickCount() + 60;  // 1 second delay
        while (TickCount() < finalTicks) {
            // Do nothing, just wait
        }
    }
    
    return err;
}

OSErr ReceiveBMPData(StreamPtr stream, Ptr *bmpData, long *dataSize) {
    OSErr err;
    TCPiopb pb;
    long totalReceived = 0;
    long bufferSize = kMaxBMPSize;
    Ptr buffer;
    long finalTicks;
    unsigned short timeout;
    
    // Allocate buffer for BMP data
    buffer = NewPtr(bufferSize);
    if (buffer == NULL) {
        LogError("Memory allocation failed");
        return memFullErr;
    }
    
    LogInfo("Buffer allocated, starting receive...");
    
    // Check connection status first
    {
        TCPiopb statusPB;
        statusPB.ioCompletion = NULL;
        statusPB.ioCRefNum = gTCPDriverRefNum;
        statusPB.csCode = TCPStatus;
        statusPB.tcpStream = stream;
        
        err = DoTCPControl(&statusPB);
        if (err != noErr) {
            LogError("Connection status check failed");
            DisposePtr(buffer);
            return err;
        }
        
        LogInfo("Connection status OK");
    }
    
    // Receive data in chunks
    timeout = 60;  // 60 second timeout for data
    
    // Try a simple single receive first
    // Clear the entire parameter block first
    {
        char *pbPtr = (char *)&pb;
        int i;
        for (i = 0; i < sizeof(TCPiopb); i++) {
            pbPtr[i] = 0;
        }
    }
    
    pb.ioCompletion = NULL;
    pb.ioCRefNum = gTCPDriverRefNum;
    pb.csCode = TCPRcv;  // Use standard receive
    pb.tcpStream = stream;
    pb.csParam.receive.commandTimeoutValue = 30;  // Use a reasonable timeout
    pb.csParam.receive.rcvBuff = buffer;
    pb.csParam.receive.rcvBuffLen = 512;  // Even smaller buffer
    pb.csParam.receive.userDataPtr = NULL;
    
    // Check if there's data available first
    {
        TCPiopb statusPB;
        // Clear status parameter block
        char *statusPtr = (char *)&statusPB;
        int i;
        for (i = 0; i < sizeof(TCPiopb); i++) {
            statusPtr[i] = 0;
        }
        
        statusPB.ioCompletion = NULL;
        statusPB.ioCRefNum = gTCPDriverRefNum;
        statusPB.csCode = TCPStatus;
        statusPB.tcpStream = stream;
        
        err = DoTCPControl(&statusPB);
        if (err == noErr) {
            if (statusPB.csParam.status.amtUnreadData > 0) {
                LogInfo("Data available for reading");
            } else {
                LogInfo("No data available yet, waiting...");
                // Wait a moment for data to arrive
                finalTicks = TickCount() + 180;  // 3 second wait
                while (TickCount() < finalTicks) {
                    // Do nothing, just wait
                }
                LogInfo("Waited for data");
            }
        }
    }
    
    LogInfo("Attempting first receive...");
    
    err = DoTCPControl(&pb);
    
    if (err == noErr) {
        LogInfo("First receive succeeded!");
        totalReceived = pb.csParam.receive.rcvBuffLen;
    } else {
        LogError("First receive failed");
        if (err == -1) {
            LogError("Driver not opened");
        } else if (err == invalidStreamPtr) {
            LogError("Invalid stream pointer");
        } else if (err == connectionDoesntExist) {
            LogError("Connection doesn't exist");
        } else if (err == connectionClosing) {
            LogError("Connection is closing");
        } else if (err == connectionTerminated) {
            LogError("Connection terminated");
        } else if (err == commandTimeout) {
            LogError("Command timeout");
        } else {
            LogError("Unknown TCP error");
        }
        DisposePtr(buffer);
        return err;
    }
    
    // If we got here, the basic receive works, so continue with the loop
    while (totalReceived < bufferSize) {
        pb.ioCompletion = NULL;
        pb.ioCRefNum = gTCPDriverRefNum;
        pb.csCode = TCPRcv;
        pb.tcpStream = stream;
        pb.csParam.receive.commandTimeoutValue = timeout;
        pb.csParam.receive.rcvBuff = buffer + totalReceived;
        pb.csParam.receive.rcvBuffLen = bufferSize - totalReceived;
        pb.csParam.receive.userDataPtr = NULL;
        
        err = DoTCPControl(&pb);
        
        if (err == noErr) {
            totalReceived += pb.csParam.receive.rcvBuffLen;
            
            LogInfo("Received some data...");
            
            // Check if we have received the BMP header to know the file size
            if (totalReceived >= 14) {
                unsigned char *headerBytes = (unsigned char *)buffer;
                long fileSize = headerBytes[2] | (headerBytes[3] << 8) | 
                               (headerBytes[4] << 16) | (headerBytes[5] << 24);
                
                LogInfo("BMP header received");
                
                if (fileSize > 0 && fileSize <= bufferSize && totalReceived >= fileSize) {
                    // We have the complete file
                    LogInfo("Complete file received!");
                    break;
                }
            }
        } else if (err == connectionClosing || err == connectionTerminated) {
            // Connection closed by server, assume we have all data
            LogInfo("Connection closed by server");
            break;
        } else {
            // Error receiving data
            LogError("Error receiving data");
            // Show error code for debugging
            if (err == -1) {
                LogError("Driver not opened");
            } else if (err == invalidStreamPtr) {
                LogError("Invalid stream pointer");
            } else if (err == connectionDoesntExist) {
                LogError("Connection doesn't exist");
            } else if (err == connectionClosing) {
                LogError("Connection is closing");
            } else if (err == connectionTerminated) {
                LogError("Connection terminated");
            } else if (err == commandTimeout) {
                LogError("Command timeout");
            } else {
                LogError("TCP receive error");
            }
            DisposePtr(buffer);
            return err;
        }
    }
    
    *bmpData = buffer;
    *dataSize = totalReceived;
    
    if (totalReceived > 0) {
        LogInfo("Data received successfully!");
    } else {
        LogError("No data received");
    }
    
    return noErr;
}

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

// Simple atoi implementation for THINK C compatibility
int SimpleAtoi(const char *str) {
    int result = 0;
    int i = 0;
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result;
}

// Logging functions
OSErr InitLogging(void) {
    OSErr err;
    Str255 fileName = "\pMacTrmnl_Log.txt";
    long count;
    
    // Create and open log file
    err = Create(fileName, 0, 'TEXT', 'TEXT');
    // Ignore error if file already exists
    
    // Open the file
    err = FSOpen(fileName, 0, &gLogFileRefNum);
    if (err != noErr) {
        return err;
    }
    
    // Move to end of file for appending
    err = SetFPos(gLogFileRefNum, fsFromLEOF, 0);
    
    // Log startup message
    LogInfo("MacTRMNL Started");
    
    return noErr;
}

void LogMessage(const char *prefix, const char *message) {
    char buffer[256];
    long count;
    unsigned long ticks;
    int hours, minutes, seconds;
    int len;
    int i;
    
    if (gLogFileRefNum == 0) return;
    
    // Get current time
    ticks = TickCount();
    seconds = (ticks / 60) % 60;
    minutes = (ticks / 3600) % 60;
    hours = (ticks / 216000) % 24;
    
    // Format message with timestamp
    sprintf(buffer, "[%02d:%02d:%02d] %s: %s\r", hours, minutes, seconds, prefix, message);
    
    // Write to file
    count = strlen(buffer);
    FSWrite(gLogFileRefNum, &count, buffer);
}

void LogError(const char *message) {
    LogMessage("ERROR", message);
}

void LogInfo(const char *message) {
    LogMessage("INFO", message);
}

void CloseLog(void) {
    if (gLogFileRefNum != 0) {
        LogInfo("MacTRMNL Closing");
        FSClose(gLogFileRefNum);
        FlushVol(NULL, 0);
        gLogFileRefNum = 0;
    }
}

Boolean ShowSettingsDialog(char *ipString, int *port) {
    WindowPtr settingsWindow;
    Rect windowRect = {100, 100, 350, 500};
    EventRecord event;
    Boolean done = false;
    Boolean result = false;
    Rect buttonRect, ipRect, portRect;
    char ipBuffer[20];
    char portBuffer[10];
    int ipLength = 0;
    int portLength = 0;
    Boolean ipActive = true;  // Which field is active
    
    // Copy current values to buffers
    strcpy(ipBuffer, ipString);
    ipLength = strlen(ipBuffer);
    sprintf(portBuffer, "%d", *port);
    portLength = strlen(portBuffer);
    
    // Create settings window
    settingsWindow = NewWindow(NULL, &windowRect, "\pNetwork Settings", true, 
                              documentProc, (WindowPtr)-1, true, 0);
    
    if (settingsWindow == NULL) return false;
    
    SetPort(settingsWindow);
    ShowWindow(settingsWindow);
    
    // Define UI element rectangles
    SetRect(&ipRect, 120, 50, 350, 70);
    SetRect(&portRect, 120, 90, 200, 110);
    SetRect(&buttonRect, 150, 130, 200, 150);
    
    while (!done) {
        if (WaitNextEvent(everyEvent, &event, 60, NULL)) {
            switch (event.what) {
                case updateEvt:
                    if ((WindowPtr)event.message == settingsWindow) {
                        BeginUpdate(settingsWindow);
                        
                        // Draw labels
                        MoveTo(20, 65);
                        DrawString("\pIP Address:");
                        MoveTo(20, 105);
                        DrawString("\pPort:");
                        
                        // Draw input fields
                        FrameRect(&ipRect);
                        FrameRect(&portRect);
                        
                        // Draw field contents
                        {
                            Str255 tempStr;
                            strcpy((char*)tempStr+1, ipBuffer);
                            tempStr[0] = strlen(ipBuffer);
                            MoveTo(125, 65);
                            DrawString(tempStr);
                            
                            strcpy((char*)tempStr+1, portBuffer);
                            tempStr[0] = strlen(portBuffer);
                            MoveTo(125, 105);
                            DrawString(tempStr);
                        }
                        
                        // Draw Start button
                        FrameRect(&buttonRect);
                        MoveTo(165, 145);
                        DrawString("\pStart");
                        
                        // Show which field is active
                        if (ipActive) {
                            InvertRect(&ipRect);
                        } else {
                            InvertRect(&portRect);
                        }
                        
                        EndUpdate(settingsWindow);
                    }
                    break;
                    
                case mouseDown:
                    {
                        Point mousePoint = event.where;
                        GlobalToLocal(&mousePoint);
                        
                        if (PtInRect(mousePoint, &buttonRect)) {
                            // Start button clicked
                            strcpy(ipString, ipBuffer);
                            *port = SimpleAtoi(portBuffer);
                            result = true;
                            done = true;
                        } else if (PtInRect(mousePoint, &ipRect)) {
                            ipActive = true;
                            InvalRect(&windowRect);
                        } else if (PtInRect(mousePoint, &portRect)) {
                            ipActive = false;
                            InvalRect(&windowRect);
                        }
                    }
                    break;
                    
                case keyDown:
                    {
                        char key = event.message & charCodeMask;
                        
                        if (key == 13 || key == 3) {  // Return or Enter
                            strcpy(ipString, ipBuffer);
                            *port = SimpleAtoi(portBuffer);
                            result = true;
                            done = true;
                        } else if (key == 27) {  // Escape
                            done = true;
                        } else if (key == 9) {  // Tab
                            ipActive = !ipActive;
                            InvalRect(&windowRect);
                        } else if (key == 8) {  // Backspace
                            if (ipActive && ipLength > 0) {
                                ipBuffer[--ipLength] = '\0';
                                InvalRect(&windowRect);
                            } else if (!ipActive && portLength > 0) {
                                portBuffer[--portLength] = '\0';
                                InvalRect(&windowRect);
                            }
                        } else if (key >= 32 && key <= 126) {  // Printable characters
                            if (ipActive && ipLength < 15) {
                                ipBuffer[ipLength++] = key;
                                ipBuffer[ipLength] = '\0';
                                InvalRect(&windowRect);
                            } else if (!ipActive && portLength < 5 && key >= '0' && key <= '9') {
                                portBuffer[portLength++] = key;
                                portBuffer[portLength] = '\0';
                                InvalRect(&windowRect);
                            }
                        }
                    }
                    break;
            }
        }
    }
    
    DisposeWindow(settingsWindow);
    return result;
}

void CleanupTCP(void) {
    TCPiopb pb;
    
    if (tcpStream != 0) {
        // Close connection
        pb.ioCompletion = NULL;
        pb.ioCRefNum = gTCPDriverRefNum;
        pb.csCode = TCPClose;
        pb.tcpStream = tcpStream;
        pb.csParam.close.validityFlags = 0;
        pb.csParam.close.ulpTimeoutValue = 30;
        pb.csParam.close.ulpTimeoutAction = 1;
        
        DoTCPControl(&pb);
        
        // Release stream
        pb.ioCompletion = NULL;
        pb.ioCRefNum = gTCPDriverRefNum;
        pb.csCode = TCPRelease;
        pb.tcpStream = tcpStream;
        
        DoTCPControl(&pb);
    }
}

/* Create the Start Settings Window */
void CreateStartWindow(void) {
	Rect windowRect;
	Rect startButtonRect;
	
	// Center window on screen
	windowRect.left = (screenBits.bounds.right - kStartWindowWidth) / 2;
	windowRect.top = (screenBits.bounds.bottom - kStartWindowHeight) / 2;
	windowRect.right = windowRect.left + kStartWindowWidth;
	windowRect.bottom = windowRect.top + kStartWindowHeight;
	
	gWindow = NewWindow(nil, &windowRect, "\pMacTRMNL", true,
					documentProc, (WindowPtr)-1, true, 0);
					
	SetPort(gWindow);
	
	// Set Controls
	startButtonRect.left = 20 + (kStartButtonWidth + 5);
	startButtonRect.top = kStartButtonTop + kStartButtonHeight;
	startButtonRect.right = startButtonRect.left = kStartButtonWidth;
	startButtonRect.bottom = startButtonRect.top + kStartButtonHeight;
	gStartButton = NewControl(gWindow, &startButtonRect, "\pStart TRMNL",
							true, 0, 0, 1, pushButProc, 0);

	// Initial window draw
	DrawStartWindow();
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
    
	// Initialize everything
	InitializeToolbox();
	SetUpMenus();
	CreateStartWindow();
	
	// Initialize logging
	InitLogging();
	    
    // Show settings dialog
    if (!ShowSettingsDialog(ipString, &serverPort)) {
        // User cancelled
        CloseLog();
        ExitToShell();
    }
    
    // Initialize MacTCP
    err = InitMacTCP();
    if (err != noErr) {
        // Log specific error
        if (err == gestaltUnknownErr) {
            LogError("MacTCP not installed - need MacTCP");
        } else {
            LogError("MacTCP driver failed to open");
        }
        // Show brief visual feedback
        MoveTo(10, 50);
        DrawString("\pMacTCP Error - see log");
        SysBeep(10);
        while (!Button()) { }  // Wait for mouse click
        CloseLog();
        ExitToShell();
    }
    
    // Log success
    LogInfo("MacTCP initialized successfully");
    
    // Hide cursor and menu bar for full screen
    HideCursor();
    
    // Get screen bounds and create full screen window
    winRect = screenBits.bounds;
    
    // Use plainDBox window type for borderless window
    mainWindow = NewWindow(NULL, &winRect, "\p", true, plainDBox, (WindowPtr)-1, false, 0);
    SetPort(mainWindow);
    
    // Fill screen with white background
    EraseRect(&winRect);
    
    ShowWindow(mainWindow);
    
    // Convert IP address string to ip_addr
    MoveTo(10, 50);
    DrawString("\pConnecting...");
    LogInfo("Parsing IP address...");
    err = ParseIPAddress(ipString, &serverIP);
    if (err != noErr) {
        // Fall back to hardcoded value if IP parsing fails
        serverIP = 0x0A00011A;  // 10.0.1.26 in hex
        LogError("IP parsing failed, using fallback");
        SysBeep(10);  // Alert user about parsing failure
    } else {
        LogInfo("IP parsed successfully");
    }
    serverPort = 1337;
    
    // Connect to server and receive BMP
    LogInfo("Connecting to server...");
    err = ConnectToServer(serverIP, serverPort, &tcpStream);
    if (err == noErr) {
        MoveTo(10, 50);
        DrawString("\pReceiving...");
        LogInfo("Connected! Receiving data...");
        err = ReceiveBMPData(tcpStream, &bmpData, &dataSize);
        if (err == noErr && bmpData != NULL && dataSize > 0) {
            MoveTo(10, 50);
            DrawString("\pComplete!");
            LogInfo("Data received! Drawing image...");
            Draw1BitBMPFromData(mainWindow, bmpData, dataSize, true);
            // Keep bmpData for redraws
        } else {
            MoveTo(10, 50);
            DrawString("\pError - see log");
            if (err != noErr) {
                LogError("Receive function failed");
            } else if (bmpData == NULL) {
                LogError("No buffer returned");
            } else {
                LogError("No data in buffer");
            }
            SysBeep(10);
        }
    } else {
        MoveTo(10, 50);
        DrawString("\pConnection failed - see log");
        LogError("Connection failed!");
        SysBeep(10);
        while (!Button()) { }  // Wait for mouse click before continuing
    }
    
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
                    ShowCursor();
                    CleanupTCP();
                    if (bmpData != NULL) {
                        DisposePtr(bmpData);
                    }
                    CloseLog();
                    ExitToShell();
                }
            } else if (event.what == mouseDown) {
                // Click anywhere to exit full screen
                ShowCursor();
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

/* Draw the Startup Window Contents */
void DrawStartWindow(void) {
	SetPort(gWindow);

	// Draw the UI Controls	
	DrawControls(gWindow);
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
				gDone = true;
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
		DoAboutDialog();
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
		gDone = true;
	}
}
	
/* About Dialog */
void DoAboutDialog(void) {
	Alert(128, nil);
}
