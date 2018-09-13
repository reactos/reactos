/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    stkwalk.h

Abstract:

    This module contains memory debug function prototypes and macros.

Author:

    Stolen from dbgmem.h
    Jim Stewart/Ramesh Pabbati    January 8, 1996

    Fixed up for regleaks
    UShaji                        Dev 11th,  1998

Revision History:

--*/

#ifdef LOCAL
#ifdef LEAK_TRACK

//
// define the amount of symbol info to keep per function in the stack trace.
//
#define MAX_FUNCTION_INFO_SIZE  40
typedef struct {

    DWORD   Displacement;                   // displacement into the function
    UCHAR   Buff[MAX_FUNCTION_INFO_SIZE];   // name of function on call stack
    PVOID   Addr;
    

} CALLER_SYM, *PCALLER_SYM;

BOOL
InitDebug(
    );

BOOL 
StopDebug();


VOID
GetCallStack(
    IN PCALLER_SYM   Caller,
    IN int           Skip,
    IN int           cFind,
    IN int           fResolveSymbols
    );

#define MY_DBG_EXCEPTION 3

extern BOOL fDebugInitialised;

#endif // LEAK_TRACK
#endif // LOCAL
