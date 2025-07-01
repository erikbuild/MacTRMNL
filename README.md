MacTRMNL
====================================

MacTRMNL is a Classic Mac OS TRMNL "eInk" client for System 6.0.8 and up! 

This project is a minimal Classic Mac application (System 6.0.8+) that displays a 1-bit monochrome BMP image from the same directory as the application. It is designed for THINK C 5.0.

## NOTE
Classic Mac OS wants line endings to be CR (Macintosh style in bbedit 4).  Modern editors usually only allow you to set Linux (LF) or Windows (CRLF) line endings.  I like to open the source fine in bbedit on System 7 to Save-As --> options --> Macintosh line endings, prior to opening in THINK C.

## TODO
- Current Status: Working on reading a BMP from a TCP socket from [schrockwell/mactrmnl-proxy](https://github.com/schrockwell/mactrmnl-proxy)

## Files
- mactrmnl.c      : Main source file for the application.
- test1.bmp   : Place your 1-bit BMP file here (this is a test file)

## Instructions
1. Open THINK C 5.0 and create a new Application project.
2. Add `mactrmnl.c` to your project.
3. Build the project.
4. Place a 1-bit (monochrome, uncompressed) BMP file named `test1.bmp` in the same folder as the compiled application.
5. Run the application. The window will display the BMP image.

## BMP Requirements
- Must be 1-bit (monochrome), uncompressed BMP format.
- Any size supported.
- The BMP must be named `test1.bmp` and reside in the same directory as the app.

## Usage

- The app opens and displays the BMP image in a fullscreen mode.
- To quit, press Command-Q.
