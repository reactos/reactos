/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    args.h

Abstract:

    Contains definitions for processing command line arguments.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _ARGS_H_
#define _ARGS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _CMD_LINE_ARGUMENTS {

    UINT nLogType;
    UINT nLogLevel;
    BOOL fBypassCtrlDispatcher;

} CMD_LINE_ARGUMENTS, *PCMD_LINE_ARGUMENTS;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessArguments(
    DWORD  NumberOfArgs,
    LPSTR ArgumentPtrs[]
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEFINE_ARGUMENT(x) \
    (x ## _ARGUMENT)

#define IS_ARGUMENT(x,y) \
    (!_strnicmp(((LPSTR)(x)),DEFINE_ARGUMENT(y),strlen(DEFINE_ARGUMENT(y))))

#define DWORD_ARGUMENT(x,y) \
    (atoi(&((LPSTR)(x))[strlen(DEFINE_ARGUMENT(y))]))

#define DEBUG_ARGUMENT      "/debug"
#define LOGTYPE_ARGUMENT    "/logtype:"
#define LOGLEVEL_ARGUMENT   "/loglevel:"

#define INVALID_ARGUMENT    0xffffffff

#endif // _ARGS_H_