/*
 * Logging.h
 * 
 * Logging functions for MacTRMNL
 * Provides file-based logging functionality for Classic Mac OS
 * 
 * Written by Erik Reynolds
 * v20250702-1
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <Files.h>
#include <OSUtils.h>

// External reference to log file handle (defined in mactrmnl.c)
extern short gLogFileRefNum;

/* Function Prototypes */
OSErr InitLogging(void);
void LogMessage(const char *prefix, const char *message);
void LogError(const char *message);
void LogInfo(const char *message);
void CloseLog(void);

#endif /* __LOGGING_H__ */