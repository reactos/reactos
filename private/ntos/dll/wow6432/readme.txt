readme.txt for wow64\386hack directory
---------------------------------------

Date:   May 8, 1998
Author: Barry Bond


This directory contains the files required to build a "prototype" of
32-bit x86 ntdll.dll which the INT 2E instructions have been replaced by
CALL FS:[] instructions.  The CALL instructions will be used to call the
transition function from IA to 64-bit EM code.

To build this new ntdll.dll, make a directory under ntos\dll called sdi386,
copy the contents of this directory to sdi386, cd ntos\dll\sdi386 and do
a build.  The resulting binary will be obj\i386\ntdll32.dll.

Appended: June 30, 1998 by Mike Zoran

Directory has been moved to ntos so no copying is required.  
Just add wow64 to BUILD_OPTIONS.

 