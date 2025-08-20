/*
 * MacTRMNL.r
 * 
 * Resource definitions for MacTRMNL
 * Includes custom PREF resource type for preferences storage
 * 
 * Written by Erik Reynolds
 * v20250108-1
 */

#include "Types.r"
#include "SysTypes.r"

/* Define the PREF resource type for preferences */
type 'PREF' {
    pstring[20];        /* IP Address */
    integer;            /* Port */
    integer;            /* Refresh Rate */
    boolean;            /* Enable Auto Refresh */
    boolean;            /* Enable Log File */
    boolean;            /* Save Settings */
    align word;         /* Align to word boundary */
};

/* Application signature resource */
resource 'STR ' (0, "Application Name") {
    "MacTRMNL 1.0"
};

/* Version resource */
resource 'vers' (1) {
    0x01, 0x00, release, 0x00,
    verUS,
    "1.0",
    "1.0, Copyright © 2025 Erik Reynolds"
};

/* Creator resource */
resource 'STR ' (-16396) {
    "MacTRMNL - TRMNL Display Client"
};