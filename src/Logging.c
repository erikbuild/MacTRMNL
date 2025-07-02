/*
 * Logging.c
 * 
 * Logging functions for MacTRMNL
 * Provides file-based logging functionality for Classic Mac OS
 * 
 * Written by Erik Reynolds
 * v20250702-1
 */

#include <Files.h>
#include <OSUtils.h>
#include <string.h>
#include <stdio.h>
#include "Logging.h"

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