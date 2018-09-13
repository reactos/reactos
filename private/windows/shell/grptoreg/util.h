/****************************** Module Header ******************************\
* Module Name: winutil.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define windows utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//


PVOID
Alloc(
    ULONG
    );

ULONG
GetAllocSize(
    PVOID
    );

BOOL
Free(
    PVOID
    );
//
// Define a print routine that only prints on a debug system
//
#if DBG
#define DbgOnlyPrint    DbgPrint
#else
#define DbgOnlyPrint
#endif

