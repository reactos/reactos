/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    psfuncs.h

Abstract:

    Function definitions for the Process Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _PSFUNCS_H
#define _PSFUNCS_H

//
// Dependencies
//
#include "pstypes.h"

//
// Win32K Process/Thread Functions
//
struct _W32THREAD*
NTAPI
PsGetWin32Thread(
    VOID
);

struct _W32PROCESS*
NTAPI
PsGetWin32Process(
    VOID
);

PVOID
NTAPI
PsGetProcessWin32Process(
    PEPROCESS Process
);

VOID
NTAPI
PsSetProcessWin32Process(
    PEPROCESS Process,
    PVOID Win32Process
);

VOID
NTAPI
PsSetThreadWin32Thread(
    PETHREAD Thread,
    PVOID Win32Thread
);

PVOID
NTAPI
PsGetThreadWin32Thread(
    PETHREAD Thread
);

VOID 
NTAPI
PsEstablishWin32Callouts(
    PW32_CALLOUT_DATA CalloutData
);

//
// Process Impersonation Functions
//
VOID
NTAPI
PsRevertThreadToSelf(
    IN PETHREAD Thread
);

//
// Misc. Functions
//
HANDLE
NTAPI
PsGetProcessId(PEPROCESS Process);

#endif
