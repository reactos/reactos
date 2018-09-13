/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmpdos.h

Abstract:

    Definitions that only need to be present in the DOS testing environment.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef snmpdos_h
#define snmpdos_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define TRUE  1
#define FALSE 0

#define SetLastError( X ) snmpErrno = X
#define GetLastError() snmpErrno

//--------------------------- PUBLIC STRUCTS --------------------------------

// stuff really in windows.h...
typedef unsigned long ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef char far *LPSTR;
typedef void VOID;

// Define error support
extern int snmpErrno;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//------------------------------- END ---------------------------------------

#endif /* snmpdos_h */

