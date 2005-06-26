/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/psfuncs.h
 * PURPOSE:         Defintions for Process Manager Functions not documented in DDK/IFS.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _PSFUNCS_H
#define _PSFUNCS_H

/* DEPENDENCIES **************************************************************/
#include "pstypes.h"

/* PROTOTYPES ****************************************************************/

PVOID
STDCALL
PsGetProcessWin32Process(PEPROCESS Process);

VOID
STDCALL
PsSetProcessWin32Process(
    PEPROCESS Process,
    PVOID Win32Process
);

VOID
STDCALL
PsSetThreadWin32Thread(
    PETHREAD Thread,
    PVOID Win32Thread
);

PVOID
STDCALL
PsGetThreadWin32Thread(PETHREAD Thread);
                       
VOID
STDCALL
PsRevertThreadToSelf(
    IN struct _ETHREAD* Thread
);

struct _W32THREAD*
STDCALL
PsGetWin32Thread(
    VOID
);

struct _W32PROCESS*
STDCALL
PsGetWin32Process(
    VOID
);

VOID 
STDCALL
PsEstablishWin32Callouts(PW32_CALLOUT_DATA CalloutData);

HANDLE
STDCALL
PsGetProcessId(struct _EPROCESS *Process);

#endif
