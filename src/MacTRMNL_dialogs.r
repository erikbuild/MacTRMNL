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
    {40, 40, 200, 400},
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
        {130, 280, 150, 340},
        Button {
            enabled,
            "Start"
        };
        
        /* Item 2: IP Address edit text */
        {30, 120, 46, 340},
        EditText {
            enabled,
            ""
        };
        
        /* Item 3: Port edit text */
        {60, 120, 76, 200},
        EditText {
            enabled,
            ""
        };
        
        /* Item 4: Enable Log checkbox */
        {90, 20, 106, 200},
        CheckBox {
            enabled,
            "Enable Log File"
        };
        
        /* Item 5: Save Settings checkbox */
        {110, 20, 126, 200},
        CheckBox {
            enabled,
            "Save Settings"
        };
        
        /* Item 6: Exit button */
        {130, 190, 150, 250},
        Button {
            enabled,
            "Exit"
        };
        
        /* Static text: IP Address label */
        {30, 20, 46, 110},
        StaticText {
            disabled,
            "Server IP:"
        };
        
        /* Static text: Port label */
        {60, 20, 76, 110},
        StaticText {
            disabled,
            "Port:"
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