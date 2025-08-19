# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MacTRMNL is a Classic Mac OS application that displays images from the TRMNL e-ink display service on vintage Macintosh computers. The project consists of a Mac client written in C using the Retro68K cross compiler and a Ruby proxy server that bridges between the vintage Mac and the modern TRMNL API.

## Build and Development Commands

### Building the Mac Application
- **Compiler**: Retro68K cross compiler (https://github.com/autc04/Retro68)
- **Host OS**: macOS Sequoia
- **Interfaces**: Apple Universal Interfaces
- **Build Command**: 
  ```bash
  cd build
  make
  ```

### Running the Proxy Server
```bash
cd proxy
ruby trmnappl.rb <ACCESS_TOKEN> [port]
```
Default port is 1337. The ACCESS_TOKEN can also be set via environment variable.

### Testing Commands
```bash
# Test proxy server with netcat
nc localhost 1337 > trmnl.bmp

# Monitor Mac debug log (on vintage Mac)
tail -f MacTrmnl_Log.txt
```

### Distribution (TODO)
```bash
# Use ditto to preserve resource forks when creating distribution
ditto -c -k --sequesterRsrc --keepParent MacTRMNL MacTRMNL.zip
```

## Architecture

### Client-Server Design
- **Mac Client** (`src/MacTRMNL.c`): Handles display, networking via MacTCP, and user interface
- **Proxy Server** (`proxy/trmnappl.rb`): Fetches from TRMNL API and streams BMP data to Mac
- **Protocol**: Raw TCP (no HTTP on Mac side)

### Key Components
1. **src/MacTRMNL.c**: Main application with event loop, window management, and BMP rendering
2. **src/MacTCPHelper.c/h**: Network abstraction layer for MacTCP operations
3. **src/Logging.c/h**: File-based logging system for debugging

### Important Patterns
- Classic Mac OS event-driven architecture with main event loop
- Manual memory management (no modern conveniences)
- Uses Apple Universal Interfaces for Classic Mac OS API compatibility
- Full-screen, borderless window display
- Blocking network operations (no async in Classic Mac OS)

## Development Notes

- The codebase uses Classic Mac OS conventions (Pascal strings, event loops, QuickDraw)
- Error handling is comprehensive with detailed logging
- Exit methods: ESC key, Cmd+Q, or mouse click
- Only supports 1-bit BMP images with custom bit conversion for Mac display
- No external dependencies on Mac side (only system APIs)
- Ruby proxy requires no gems (stdlib only)