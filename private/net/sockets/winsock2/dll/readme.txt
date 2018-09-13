
This file contains the instructions for building winsock2.dll.

Prerequisites:

Microsoft Visual C++ 2.1 or later. 
    The PATH, INCLUDE and LIB environment variables set to the BIN,
	INCLUDE and LIB directories of the VC++ installation directory.
 
MKS Toolkit 4.0 or later 
    The makefile uses CP and RM commands.  


How to build winsock2.dll:

1. Add the path to the include and common directories to the INCLUDE
   environment variable.

2. CD to the winsock2 directory and type nmake. The generated .DLL and
   .LIB file will be copied to a release directory at the same level as
   the winsock2 directory.  


Compilation errata:
The compilation generates no errors. Several instances of the warning
"warning C4705: statement has no effect" will be generated.


