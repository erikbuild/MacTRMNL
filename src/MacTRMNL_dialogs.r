// ABOUTME: Dialog and menu resources for MacTRMNL
// ABOUTME: Defines the settings dialog, menus, and alerts

#include "Types.r"
#include "SysTypes.r"

/* Menu Bar */
resource 'MBAR' (128) {
    { 128, 129, 130 }  /* Apple, File, Edit menus */
};

/* Apple Menu */
resource 'MENU' (128) {
    128,
    textMenuProc,
    0x7FFFFFFD,  /* Enable all items except separator */
    enabled,
    apple,
    {
        "About MacTRMNL...", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain;
    }
};

/* File Menu */
resource 'MENU' (129) {
    129,
    textMenuProc,
    allEnabled,
    enabled,
    "File",
    {
        "Refresh", noIcon, "R", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Settings...", noIcon, "-", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

/* Edit Menu */
resource 'MENU' (130) {
    130,
    textMenuProc,
    0x0,  /* All items disabled */
    enabled,
    "Edit",
    {
        "Undo", noIcon, "Z", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Cut", noIcon, "X", noMark, plain;
        "Copy", noIcon, "C", noMark, plain;
        "Paste", noIcon, "V", noMark, plain;
        "Clear", noIcon, noKey, noMark, plain;
    }
};

/* Settings Dialog */
resource 'DLOG' (128) {
    {40, 40, 280, 440},
    documentProc,
    visible,
    goAway,
    0x0,
    128,  /* DITL resource ID */
    "MacTRMNL Settings",
    centerMainScreen
};

/* Dialog Item List for Settings */
resource 'DITL' (128) {
    {
        /* Item 1: OK button (Start) */
        {200, 280, 220, 340},
        Button {
            enabled,
            "Start"
        };
        
        /* Item 2: IP Address edit text */
        {20, 120, 36, 280},
        EditText {
            enabled,
            ""
        };
        
        /* Item 3: Port edit text */
        {50, 120, 66, 200},
        EditText {
            enabled,
            ""
        };

        /* Item 4: Refresh rate edit text */
        {80, 120, 96, 200},
        EditText {
            enabled,
            ""
        };

        /* Item 5: Enable Auto Refresh checkbox */
        {120, 20, 140, 200},
        CheckBox {
            enabled,
            "Enable Auto Refresh"
        };
        
        /* Item 6: Enable Log checkbox */
        {140, 20, 160, 200},
        CheckBox {
            enabled,
            "Enable Log File"
        };
        
        /* Item 7: Save Settings checkbox */
        {160, 20, 180, 200},
        CheckBox {
            enabled,
            "Save Settings"
        };
        
        /* Item 8: Exit button */
        {200, 20, 220, 80},
        Button {
            enabled,
            "Exit"
        };
        
        /* Item 9: Save button */
        {200, 200, 220, 260},
        Button {
            enabled,
            "Save"
        };
        
        /* Static text: IP Address label */
        {20, 20, 46, 110},
        StaticText {
            disabled,
            "Server IP:"
        };
        
        /* Static text: Port label */
        {50, 20, 76, 110},
        StaticText {
            disabled,
            "Port:"
        };

        /* Static text: Refresh Rate label */
        {80, 20, 96, 110},
        StaticText {
            disabled,
            "Refresh Rate:"
        };

        /* Static text: Refresh Rate minutes label */
        {80, 210, 96, 280},
        StaticText {
            disabled,
            "minutes"
        };

        /* Static text: Instructions 1 */
        {120, 200, 140, 380},
        StaticText {
            disabled,
            "CMD+F to toggle fullscreen"
        };

        /* Static text: Instructions 2 */
        {140, 200, 160, 380},
        StaticText {
            disabled,
            "CMD+R to force refresh"
        };

        /* Static text: Instructions 3 */
        {160, 200, 180, 380},
        StaticText {
            disabled,
            "CMD+P to print screen"
        };
    }
};

/* About Dialog (Alert) */
resource 'ALRT' (129) {
    {40, 40, 200, 400},
    129,  /* DITL resource ID */
    {
        OK, visible, sound1,
        OK, visible, sound1,
        OK, visible, sound1,
        OK, visible, sound1
    },
    centerMainScreen
};

/* Alert Dialog Item List */
resource 'DITL' (129) {
    {
        /* OK button */
        {130, 290, 150, 350},
        Button {
            enabled,
            "OK"
        };
        
        /* Text */
        {10, 60, 70, 350},
        StaticText {
            disabled,
            "MacTRMNL 1.0"
        };

        {35, 60, 70, 350},
        StaticText {
            disabled,
            "TRMNL Display Client for Classic Mac OS"
        };

        {55, 60, 70, 350},
        StaticText {
            disabled,
            "(c) 2025 Erik Reynolds"
        };
                
        {75, 60, 70, 350},
        StaticText {
            disabled,
            "https://github.com/erikbuild/mactrmnl"
        };

        /* Icon placeholder */
        {10, 10, 42, 42},
        Icon {
            disabled,
            1000  /* Application icon resource ID */
        };
    }
};

/* Display Window */
resource 'WIND' (129) {
    {40, 40, 400, 600},
    plainDBox,
    visible,
    goAway,
    0x0,
    "MacTRMNL Display",
    centerMainScreen
};