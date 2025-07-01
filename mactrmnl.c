
/*
 * mactrmnl.c
 * 
 * A Classic Mac OS application that connects to a server, receives a 1-bit BMP image,
 * and displays it in a fullscreen window. This code is designed to run on System 6.0.8 and later with MacTCP support.
 * It uses the MacTCP API to establish a TCP connection, receive BMP data, and draw it on the screen.
 * 
 * Written by Erik Reynolds & Rockwell Schrock
 * 2025-07-01
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

// MacTCP headers
#include <MacTCP.h>
#include <AddressXlation.h>

#define kRcvBufferSize 8192
#define kMaxBMPSize 65536L

// TCP globals
StreamPtr tcpStream = nil;
WindowPtr mainWindow;
Boolean gHaveMacTCP = false;

// Function prototypes
OSErr InitMacTCP(void);
OSErr ConnectToServer(ip_addr serverIP, unsigned short serverPort, StreamPtr *stream);
OSErr ReceiveBMPData(StreamPtr stream, Ptr *bmpData, long *dataSize);
void Draw1BitBMPFromData(WindowPtr win, Ptr bmpData, long dataSize, Boolean centerImage);
void CleanupTCP(void);

OSErr InitMacTCP(void) {
    OSErr err;
    TCPiopb pb;
    
    // Try to open MacTCP driver directly
    // If MacTCP is not installed, this will fail
    pb.ioCompletion = nil;
    pb.ioCRefNum = -1;  // MacTCP driver refnum
    pb.csCode = TCPCreate;
    pb.csParam.create.rcvBuff = nil;
    pb.csParam.create.rcvBuffLen = 0;
    pb.csParam.create.notifyProc = nil;
    pb.csParam.create.userDataPtr = nil;
    
    err = PBControl((ParmBlkPtr)&pb, false);
    if (err == noErr) {
        // MacTCP is available, release the test stream
        pb.ioCompletion = nil;
        pb.ioCRefNum = -1;
        pb.csCode = TCPRelease;
        pb.tcpStream = pb.tcpStream;
        PBControl((ParmBlkPtr)&pb, false);
        
        gHaveMacTCP = true;
        return noErr;
    }
    
    return err;
}

OSErr ConnectToServer(ip_addr serverIP, unsigned short serverPort, StreamPtr *stream) {
    OSErr err;
    TCPiopb pb;
    tcp_port localPort = 0;  // Let MacTCP assign
    
    // Create stream
    pb.ioCompletion = nil;
    pb.ioCRefNum = -1;  // MacTCP driver refnum
    pb.csCode = TCPCreate;
    pb.csParam.create.rcvBuff = (Ptr)NewPtr(kRcvBufferSize);
    pb.csParam.create.rcvBuffLen = kRcvBufferSize;
    pb.csParam.create.notifyProc = nil;
    pb.csParam.create.userDataPtr = nil;
    
    err = PBControl((ParmBlkPtr)&pb, false);
    if (err != noErr) {
        return err;
    }
    
    *stream = pb.tcpStream;
    
    // Open connection
    pb.ioCompletion = nil;
    pb.ioCRefNum = -1;
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
    
    err = PBControl((ParmBlkPtr)&pb, false);
    
    return err;
}

OSErr ReceiveBMPData(StreamPtr stream, Ptr *bmpData, long *dataSize) {
    OSErr err;
    TCPiopb pb;
    long totalReceived = 0;
    long bufferSize = kMaxBMPSize;
    Ptr buffer;
    unsigned short timeout;
    
    // Allocate buffer for BMP data
    buffer = NewPtr(bufferSize);
    if (buffer == nil) {
        return memFullErr;
    }
    
    // Receive data in chunks
    timeout = 60;  // 60 second timeout for data
    
    while (totalReceived < bufferSize) {
        pb.ioCompletion = nil;
        pb.ioCRefNum = -1;
        pb.csCode = TCPRcv;
        pb.tcpStream = stream;
        pb.csParam.receive.commandTimeoutValue = timeout;
        pb.csParam.receive.rcvBuff = buffer + totalReceived;
        pb.csParam.receive.rcvBuffLen = bufferSize - totalReceived;
        pb.csParam.receive.userDataPtr = nil;
        
        err = PBControl((ParmBlkPtr)&pb, false);
        
        if (err == noErr) {
            totalReceived += pb.csParam.receive.rcvBuffLen;
            
            // Check if we have received the BMP header to know the file size
            if (totalReceived >= 14) {
                unsigned char *headerBytes = (unsigned char *)buffer;
                long fileSize = headerBytes[2] | (headerBytes[3] << 8) | 
                               (headerBytes[4] << 16) | (headerBytes[5] << 24);
                
                if (fileSize > 0 && fileSize <= bufferSize && totalReceived >= fileSize) {
                    // We have the complete file
                    break;
                }
            }
        } else if (err == connectionClosing || err == connectionTerminated) {
            // Connection closed by server, assume we have all data
            break;
        } else {
            // Error receiving data
            DisposePtr(buffer);
            return err;
        }
    }
    
    *bmpData = buffer;
    *dataSize = totalReceived;
    
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
        MoveTo(10, 20);
        DrawString("\pERROR: Invalid BMP data");
        return;
    }
    
    headerBytes = (unsigned char *)bmpData;
    
    // Check signature
    if (headerBytes[0] != 0x42 || headerBytes[1] != 0x4D) { 
        MoveTo(10, 30);
        DrawString("\pERROR: Not a valid BMP file");
        return; 
    }
    
    infoBytes = (unsigned char *)bmpData + 14;
    
    if (infoBytes[14] != 1) { 
        MoveTo(10, 40);
        DrawString("\pERROR: Not a 1-bit BMP file");
        return; 
    }
    
    // Read dimensions
    width = infoBytes[4] | (infoBytes[5] << 8) | (infoBytes[6] << 16) | (infoBytes[7] << 24);
    height = infoBytes[8] | (infoBytes[9] << 8) | (infoBytes[10] << 16) | (infoBytes[11] << 24);
    rowSize = ((width + 31) / 32) * 4;
    
    // Get pixel data offset
    pixelOffset = headerBytes[10] | (headerBytes[11] << 8) | (headerBytes[12] << 16) | (headerBytes[13] << 24);
    
    if (pixelOffset + (rowSize * height) > dataSize) {
        MoveTo(10, 50);
        DrawString("\pERROR: Incomplete BMP data");
        return;
    }
    
    pixelData = (unsigned char *)bmpData + pixelOffset;
    
    // Create offscreen bitmap
    GetPort(&oldPort);
    
    // Calculate row bytes for Mac bitmap (must be even)
    offRowBytes = ((width + 15) / 16) * 2;
    
    // Allocate memory for offscreen bitmap
    offBaseAddr = NewPtr(offRowBytes * height);
    if (offBaseAddr == nil) {
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
    CopyBits(&offBitMap, &win->portBits, &srcRect, &destRect, srcCopy, nil);
    
    // Clean up
    DisposePtr(offBaseAddr);
    SetPort(oldPort);
}

void CleanupTCP(void) {
    TCPiopb pb;
    
    if (tcpStream != nil) {
        // Close connection
        pb.ioCompletion = nil;
        pb.ioCRefNum = -1;
        pb.csCode = TCPClose;
        pb.tcpStream = tcpStream;
        pb.csParam.close.validityFlags = 0;
        pb.csParam.close.ulpTimeoutValue = 30;
        pb.csParam.close.ulpTimeoutAction = 1;
        
        PBControl((ParmBlkPtr)&pb, false);
        
        // Release stream
        pb.ioCompletion = nil;
        pb.ioCRefNum = -1;
        pb.csCode = TCPRelease;
        pb.tcpStream = tcpStream;
        
        PBControl((ParmBlkPtr)&pb, false);
    }
}

// Main entry point
void main(void) {
    Rect winRect;
    EventRecord event;
    Pattern black;
    char key;
    OSErr err;
    ip_addr serverIP;
    Ptr bmpData = nil;
    long dataSize;
    struct hostInfo hostInfoRec;
    
    int serverPort; // I want to get this from a settings splash screen in the future
    char *ipString = "10.0.1.26"; // Replace with your server IP or hostname
    // I want to get this from a settings splash screen in the future
    
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(0);
    InitCursor();
    
    // Initialize MacTCP
    err = InitMacTCP();
    if (err != noErr) {
        // Show error dialog
        SysBeep(10);
        ExitToShell();
    }
    
    // Hide cursor and menu bar for full screen
    HideCursor();
    
    // Get screen bounds and create full screen window
    winRect = qd.screenBits.bounds;
    
    // Use plainDBox window type for borderless window
    mainWindow = NewWindow(nil, &winRect, "\p", true, plainDBox, (WindowPtr)-1, false, 0);
    SetPort(mainWindow);
    
    // Fill screen with white background
    EraseRect(&winRect);
    
    ShowWindow(mainWindow);
    
    // Convert IP address string to ip_addr
    err = StrToAddr(ipString, &hostInfoRec, nil, nil);
    if (err == noErr) {
        serverIP = hostInfoRec.addr[0];
    } else {
        // Fall back to hardcoded value if DNS lookup fails
        serverIP = 0xC0A8000A;  // 192.168.0.10 in hex
    }
    serverPort = 1337;
    
    // Connect to server and receive BMP
    err = ConnectToServer(serverIP, serverPort, &tcpStream);
    if (err == noErr) {
        err = ReceiveBMPData(tcpStream, &bmpData, &dataSize);
        if (err == noErr && bmpData != nil) {
            Draw1BitBMPFromData(mainWindow, bmpData, dataSize, true);
            // Keep bmpData for redraws
        }
    }
    
    // Basic event loop
    while (true) {
        if (WaitNextEvent(everyEvent, &event, 60, nil)) {
            if (event.what == updateEvt) {
                BeginUpdate(mainWindow);
                if (bmpData != nil) {
                    Draw1BitBMPFromData(mainWindow, bmpData, dataSize, true);
                }
                EndUpdate(mainWindow);
            } else if (event.what == keyDown) {
                key = (char)(event.message & charCodeMask);
                // Escape key or Cmd+Q quits
                if (key == 27 || ((event.modifiers & cmdKey) && (key == 'Q' || key == 'q'))) {
                    ShowCursor();
                    CleanupTCP();
                    if (bmpData != nil) {
                        DisposePtr(bmpData);
                    }
                    ExitToShell();
                }
            } else if (event.what == mouseDown) {
                // Click anywhere to exit full screen
                ShowCursor();
                CleanupTCP();
                if (bmpData != nil) {
                    DisposePtr(bmpData);
                }
                ExitToShell();
            }
        }
    }
}