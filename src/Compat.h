// ABOUTME: Compatibility layer for System 7.0 support
// ABOUTME: Maps newer API names to older ones for System 7.0

#ifndef COMPAT_H
#define COMPAT_H

// Force use of old routine names for System 7.0 compatibility
// These are the actual trap names that exist in System 7.0
#ifdef __GNUC__

// Menu Manager - GetMenuItemText doesn't exist in System 7.0, use GetItem
#ifdef GetItem
#undef GetItem
#endif
#define GetMenuItemText GetItem
pascal void GetItem(MenuHandle menu, short item, Str255 itemString);

// Dialog Manager - newer names don't exist in System 7.0
#ifdef GetDItem
#undef GetDItem
#endif
#define GetDialogItem GetDItem
pascal void GetDItem(DialogPtr dialog, short itemNo, short *itemType, Handle *item, Rect *box);

#ifdef SetIText
#undef SetIText
#endif
#define SetDialogItemText SetIText
pascal void SetIText(Handle item, ConstStr255Param text);

#ifdef GetIText
#undef GetIText
#endif
#define GetDialogItemText GetIText
pascal void GetIText(Handle item, Str255 text);

// Control Manager - newer names don't exist in System 7.0
#ifdef SetCtlValue
#undef SetCtlValue
#endif
#define SetControlValue SetCtlValue
pascal void SetCtlValue(ControlHandle theControl, short theValue);

#ifdef GetCtlValue
#undef GetCtlValue
#endif
#define GetControlValue GetCtlValue
pascal short GetCtlValue(ControlHandle theControl);

#endif // __GNUC__

#endif // COMPAT_H