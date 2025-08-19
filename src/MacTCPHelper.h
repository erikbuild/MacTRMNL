/*
 * mactcphelper.h
 * 
 * MacTCP helper functions for MacTRMNL
 * Provides TCP networking functionality for Classic Mac OS
 * 
 * Written by Erik Reynolds
 * v20250702-1
 */

#ifndef __MACTCPHELPER_H__
#define __MACTCPHELPER_H__

/* COMPATIBILITY HACKS FOR MACTCP AND THINK C 5.0 */
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

// Define missing constants for Universal Interfaces (only if not already defined)
#ifndef kCStackBased
#define kCStackBased 0
#endif
#ifndef kPascalStackBased
#define kPascalStackBased 0
#endif
#ifndef STACK_ROUTINE_PARAMETER
#define STACK_ROUTINE_PARAMETER(n, s) 0
#endif
#ifndef SIZE_CODE
#define SIZE_CODE(s) 0
#endif
#ifndef RESULT_SIZE
#define RESULT_SIZE(s) 0
#endif
#ifndef CALLBACK_API_C
#define CALLBACK_API_C(ret, name) ret (*name)
#endif
/* END COMPATIBILITY HACKS FOR MACTCP AND THINK C 5.0 */

#include <MacTCP.h>
#include <OSUtils.h>
#include <Memory.h>

// External references to TCP globals (defined in mactcphelper.c)
extern StreamPtr tcpStream;
extern Boolean gHaveMacTCP;
extern short gTCPDriverRefNum;

/* Function Prototypes */
OSErr DoTCPControl(TCPiopb *pb);
OSErr InitMacTCP(void);
OSErr ParseIPAddress(const char *ipString, ip_addr *ipAddr);
OSErr ConnectToServer(ip_addr serverIP, unsigned short serverPort, StreamPtr *stream);
OSErr ReceiveBMPData(StreamPtr stream, Ptr *bmpData, long *dataSize);
void CleanupTCP(void);

#endif /* __MACTCPHELPER_H__ */