/*
 * MacTCPHelper.c
 * 
 * MacTCP helper functions for MacTRMNL
 * Provides TCP networking functionality for Classic Mac OS
 * 
 * Written by Erik Reynolds
 * v20250702-1
 */

#include <OSUtils.h>
#include <Memory.h>
#include <Devices.h>
#include <Gestalt.h>
#include "MacTCPHelper.h"
#include "logging.h"

// Constants from main application
#define kRcvBufferSize 		8192
#define kMaxBMPSize			65536L

// TCP globals
StreamPtr tcpStream = 0;
Boolean gHaveMacTCP = false;
short gTCPDriverRefNum = 0;  // MacTCP driver reference number

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