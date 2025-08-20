# Classic Mac OS Coordinate System

  The coordinates are in the format: {top, left, bottom, right}

  - Origin (0,0): Top-left corner of the screen
  - Positive Y: Goes DOWN from top
  - Positive X: Goes RIGHT from left

  For Rectangles (Windows, Dialogs, Buttons, etc.):

  {top, left, bottom, right}

  Examples from your file:

  Settings Dialog Window:

  {40, 40, 200, 400}
  - Top: 40 pixels from top of screen
  - Left: 40 pixels from left of screen
  - Bottom: 200 pixels from top (height = 200-40 = 160 pixels)
  - Right: 400 pixels from left (width = 400-40 = 360 pixels)

  Start Button:

  {130, 280, 150, 340}
  - Top: 130 pixels from dialog top
  - Left: 280 pixels from dialog left
  - Bottom: 150 pixels (height = 150-130 = 20 pixels)
  - Right: 340 pixels (width = 340-280 = 60 pixels)

  Exit Button:

  {130, 190, 150, 250}
  - Same vertical position as Start (130-150)
  - Positioned to the left (190-250)
  - Both buttons are 20 pixels tall, 60 pixels wide

  IP Address Edit Field:

  {30, 120, 46, 340}
  - Top: 30 pixels from dialog top
  - Height: 16 pixels (46-30)
  - Width: 220 pixels (340-120)

  Visual Layout of Settings Dialog:

  Dialog Box (360x160 pixels)
  ┌─────────────────────────────────────┐
  │                                     │ 30px
  │ Server IP: [___________________]   │ ← IP field (30,120,46,340)
  │                                     │ 60px
  │ Port:      [_____]                  │ ← Port field (60,120,76,200)
  │                                     │ 90px
  │ ☐ Enable Log File                   │ ← Checkbox (90,20,106,200)
  │                                     │ 110px
  │ ☐ Save Settings                     │ ← Checkbox (110,20,126,200)
  │                                     │ 130px
  │            [Exit]  [Start]          │ ← Buttons at y=130-150
  └─────────────────────────────────────┘

  Special Notes:

  1. Dialog items: Coordinates are relative to the dialog window, not the screen
  2. Static text: Often has the same top/bottom as its associated control for alignment
  3. Standard button height: Usually 20 pixels in Classic Mac OS
  4. Standard spacing: 10-20 pixels between elements is typical

  The centerMainScreen flag can override the window position to center it on screen, but the size is still determined by the rectangle coordinates.