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

struct _W32THREAD* NTAPI
PsGetWin32Thread(VOID);

struct _W32PROCESS* NTAPI
PsGetWin32Process(VOID);

PVOID
NTAPI
PsGetProcessWin32Process(PEPROCESS Process);

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
PsGetThreadWin32Thread(PETHREAD Thread);
                       
VOID
NTAPI
PsRevertThreadToSelf(
    IN struct _ETHREAD* Thread
);

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

VOID 
NTAPI
PsEstablishWin32Callouts(PW32_CALLOUT_DATA CalloutData);

HANDLE
NTAPI
PsGetProcessId(struct _EPROCESS *Process);

#endif
