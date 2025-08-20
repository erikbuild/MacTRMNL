MacTRMNL
====================================

## TODO
- USE DITTO to zip up resource fork stuff ala https://github.com/hotdang-ca/MacToolboxBoilerplate/tree/master

## Project Overview

MacTRMNL is a Classic Mac OS application that displays images from the TRMNL e-ink display service on vintage Macintosh computers. The project consists of a Mac client written in C for THINK C 5.0 and a Ruby proxy server that bridges between the vintage Mac and the modern TRMNL API.

## Build and Development Commands

### Building the Mac Application
- **Compiler**: THINK C 5.0 (must be run on vintage Mac or emulator like Basilisk II)
- **Critical Setup**: MacTCP headers must be manually installed in `THINK C 5.0 Folder/Mac #includes/Apple #includes/`
- **Line Endings**: Source files must use CR line endings (Classic Mac format). Use the helper script to convert to classic mac line endings (CR):
    `./convert_line_endings.sh --to-mac src/*.c src/*.h src/*.r`

### Running the Proxy Server

You will need a device with Developer Edition. Get the device API key from [device settings](https://usetrmnl.com/devices/current/edit).

```bash
export ACCESS_TOKEN="ur-device-api-key-here"
./bin/proxy [port]
```

### Testing the Application
1. Start the proxy server with your TRMNL access token
2. Run MacTRMNL on the vintage Mac
3. Enter the proxy server's IP address and port (default: 1337)
4. Monitor `MacTrmnl_Log.txt` on the Mac for debugging

## Architecture

### Client-Server Design
- **Mac Client** (`MacTRMNL.c`): Handles display, networking via MacTCP, and user interface
- **Proxy Server** (`proxy/trmnappl.rb`): Fetches from TRMNL API and streams BMP data to Mac
- **Protocol**: Raw TCP (no HTTP on Mac side)

### Key Components
1. **MacTRMNL.c**: Main application with event loop, window management, and BMP rendering
2. **MacTCPHelper.c/h**: Network abstraction layer for MacTCP operations
3. **Logging.c/h**: File-based logging system for debugging

### Important Patterns
- Classic Mac OS event-driven architecture with main event loop
- Manual memory management (no modern conveniences)
- Compatibility shims for THINK C 5.0 vs modern Universal Interfaces
- Full-screen, borderless window display

## Development Notes

- The codebase uses Classic Mac OS conventions (Pascal strings, event loops, QuickDraw)
- Error handling is comprehensive with detailed logging
- Network operations are blocking (no async in Classic Mac OS)
- Exit methods: ESC key, Cmd+Q, or mouse click
- Only supports 1-bit BMP images with custom bit conversion for Mac display