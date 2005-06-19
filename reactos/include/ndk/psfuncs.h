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

/* PROTOTYPES ****************************************************************/

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

#endif
