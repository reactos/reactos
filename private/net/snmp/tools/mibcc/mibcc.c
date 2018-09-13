/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibcc.c

Abstract:

    MibCC.c contains driver that calls the main program for the MIB compiler.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----
#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int SnmpMgrMibCC(
    int  argc,
    char *argv[]);


//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// the actual compiler is in the mgmtapi.dll.  this routine is necessary
// due to the structure of the nt build environment.

int __cdecl main(
   int  argc,
   char *argv[])
{
    return SnmpMgrMibCC(argc, argv);
}


//-------------------------------- END --------------------------------------


