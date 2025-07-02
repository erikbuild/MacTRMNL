MacTRMNL
====================================

MacTRMNL is a Classic Mac OS TRMNL "eInk" client for System 6.0.8 and up! 

This project is a minimal Classic Mac application (System 6.0.8+) that displays a 1-bit monochrome BMP image from the same directory as the application. It is designed for THINK C 5.0.

## Important Notes
Classic Mac OS wants line endings to be CR (Macintosh style in bbedit 4).  Modern editors usually only allow you to set Linux (LF) or Windows (CRLF) line endings.  I like to open the source fine in bbedit on System 7 to Save-As --> options --> Macintosh line endings, prior to opening in THINK C.

This requires:
MacTCP.h
AddressXlation.h

## TODO
- Pass the logging instance to the auxillary functions (if using)!
- Continue with refactor / clean up.
- Continue with ResEdit Dialog box?
- USE DITTO to zip up resource fork stuff ala https://github.com/hotdang-ca/MacToolboxBoilerplate/tree/master
