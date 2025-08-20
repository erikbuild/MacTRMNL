/*
 * Preferences.h
 * 
 * Preferences management for MacTRMNL
 * Handles loading and saving application preferences following Classic Mac OS conventions
 * 
 * Written by Erik Reynolds
 * v20250108-1
 */

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <Types.h>
#include <Files.h>

// Resource constants
#define kPrefsResType       'PREF'
#define kPrefsResID         128
#define kPrefsFileName      "\pMacTRMNL Prefs"
#define kPrefsFileType      'pref'
#define kPrefsFileCreator   'MTRM'

// Preferences structure (must match AppSettings in MacTRMNL.c)
typedef struct {
    char ipAddress[20];
    short port;
    short refreshRate;     // Refresh rate in minutes
    Boolean autoRefresh; 
    Boolean enableLogFile;
    Boolean saveSettings;
} PrefsData;

// Function prototypes
OSErr LoadPreferences(PrefsData *prefs);
OSErr SavePreferences(const PrefsData *prefs);
OSErr GetPreferencesFolder(short *vRefNum, long *dirID);
OSErr CreatePreferencesFile(short vRefNum, long dirID);

#endif /* __PREFERENCES_H__ */