/*
 * Preferences.c
 * 
 * Preferences management implementation for MacTRMNL
 * Stores preferences in System Folder:Preferences following Classic Mac OS conventions
 * 
 * Written by Erik Reynolds
 * v20250108-1
 */

#include <Files.h>
#include <Folders.h>
#include <Resources.h>
#include <Memory.h>
#include <Errors.h>
#include <string.h>
#include "Preferences.h"

// Get the System Folder's Preferences folder
OSErr GetPreferencesFolder(short *vRefNum, long *dirID) {
    OSErr err;
    
    // Find the Preferences folder in the System Folder
    err = FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, 
                     vRefNum, dirID);
    
    return err;
}

// Create the preferences file if it doesn't exist
OSErr CreatePreferencesFile(short vRefNum, long dirID) {
    OSErr err;
    short refNum;
    
    // Create the file with our type and creator
    err = HCreate(vRefNum, dirID, kPrefsFileName, kPrefsFileCreator, kPrefsFileType);
    
    if (err == dupFNErr) {
        // File already exists, which is fine
        return noErr;
    }
    
    if (err != noErr) {
        return err;
    }
    
    // Create the resource fork
    HCreateResFile(vRefNum, dirID, kPrefsFileName);
    err = ResError();
    
    if (err != noErr) {
        // Delete the file if we couldn't create the resource fork
        HDelete(vRefNum, dirID, kPrefsFileName);
        return err;
    }
    
    // Open the resource file
    refNum = HOpenResFile(vRefNum, dirID, kPrefsFileName, fsRdWrPerm);
    if (refNum == -1) {
        err = ResError();
        return err;
    }
    
    // Create an empty PREF resource
    Handle prefHandle = NewHandle(sizeof(PrefsData));
    if (prefHandle == NULL) {
        CloseResFile(refNum);
        return MemError();
    }
    
    // Initialize with default values
    PrefsData *prefsPtr = (PrefsData *)*prefHandle;
    strcpy(prefsPtr->ipAddress, "10.0.1.26");
    prefsPtr->port = 1337;
    prefsPtr->enableLogFile = true;
    prefsPtr->saveSettings = true;
    
    // Add the resource
    AddResource(prefHandle, kPrefsResType, kPrefsResID, "\pMacTRMNL Preferences");
    err = ResError();
    
    if (err == noErr) {
        WriteResource(prefHandle);
        err = ResError();
    }
    
    CloseResFile(refNum);
    
    return err;
}

// Load preferences from file
OSErr LoadPreferences(PrefsData *prefs) {
    OSErr err;
    short vRefNum;
    long dirID;
    short refNum;
    Handle prefHandle;
    
    if (prefs == NULL) {
        return paramErr;
    }
    
    // Get the Preferences folder
    err = GetPreferencesFolder(&vRefNum, &dirID);
    if (err != noErr) {
        return err;
    }
    
    // Open the preferences file
    refNum = HOpenResFile(vRefNum, dirID, kPrefsFileName, fsRdPerm);
    if (refNum == -1) {
        err = ResError();
        if (err == fnfErr) {
            // File doesn't exist, create it with defaults
            err = CreatePreferencesFile(vRefNum, dirID);
            if (err != noErr) {
                return err;
            }
            
            // Now try to open it again
            refNum = HOpenResFile(vRefNum, dirID, kPrefsFileName, fsRdPerm);
            if (refNum == -1) {
                return ResError();
            }
        } else {
            return err;
        }
    }
    
    // Get the PREF resource
    prefHandle = GetResource(kPrefsResType, kPrefsResID);
    if (prefHandle == NULL) {
        CloseResFile(refNum);
        return resNotFound;
    }
    
    // Copy the data
    HLock(prefHandle);
    BlockMove(*prefHandle, prefs, sizeof(PrefsData));
    HUnlock(prefHandle);
    
    ReleaseResource(prefHandle);
    CloseResFile(refNum);
    
    return noErr;
}

// Save preferences to file
OSErr SavePreferences(const PrefsData *prefs) {
    OSErr err;
    short vRefNum;
    long dirID;
    short refNum;
    Handle prefHandle;
    
    if (prefs == NULL) {
        return paramErr;
    }
    
    // Get the Preferences folder
    err = GetPreferencesFolder(&vRefNum, &dirID);
    if (err != noErr) {
        return err;
    }
    
    // Make sure the file exists
    err = CreatePreferencesFile(vRefNum, dirID);
    if (err != noErr && err != dupFNErr) {
        return err;
    }
    
    // Open the preferences file
    refNum = HOpenResFile(vRefNum, dirID, kPrefsFileName, fsRdWrPerm);
    if (refNum == -1) {
        return ResError();
    }
    
    // Get or create the PREF resource
    prefHandle = GetResource(kPrefsResType, kPrefsResID);
    if (prefHandle == NULL) {
        // Create new resource
        prefHandle = NewHandle(sizeof(PrefsData));
        if (prefHandle == NULL) {
            CloseResFile(refNum);
            return MemError();
        }
        
        AddResource(prefHandle, kPrefsResType, kPrefsResID, "\pMacTRMNL Preferences");
        err = ResError();
        if (err != noErr) {
            DisposeHandle(prefHandle);
            CloseResFile(refNum);
            return err;
        }
    }
    
    // Update the data
    HLock(prefHandle);
    BlockMove(prefs, *prefHandle, sizeof(PrefsData));
    HUnlock(prefHandle);
    
    // Write the resource
    ChangedResource(prefHandle);
    WriteResource(prefHandle);
    err = ResError();
    
    ReleaseResource(prefHandle);
    CloseResFile(refNum);
    
    return err;
}