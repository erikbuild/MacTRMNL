MacTRMNL
====================================

MacTRMNL is a Classic Mac OS TRMNL "eInk" client for System 7 and up! 

Goal is to get it running on System 6.0.8 and up, but need to tweak some EventHandling stuff first...

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
