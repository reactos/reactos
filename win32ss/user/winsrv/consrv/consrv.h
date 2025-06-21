/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/consrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __CONSRV_H__
#define __CONSRV_H__

/* Main header */
#include "../winsrv.h"

#include <strsafe.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <winnls.h>
#include <wincon.h>
#include <wincon_undoc.h>

#define NTOS_MODE_USER
#include <ndk/mmfuncs.h>

/* CONSOLE Headers */
#include <win/conmsg.h>

/* Heap Helpers */
#include "heap.h"

/* Globals */
extern HINSTANCE ConSrvDllInstance;

#define ConsoleGetPerProcessData(Process)   \
    ((PCONSOLE_PROCESS_DATA)((Process)->ServerData[CONSRV_SERVERDLL_INDEX]))

typedef struct _CONSOLE_PROCESS_DATA
{
    LIST_ENTRY ConsoleLink;
    PCSR_PROCESS Process;   // Process owning this structure.

    HANDLE ConsoleHandle;
    BOOLEAN ConsoleApp;     // TRUE if it is a CUI app, FALSE otherwise.

    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    struct _CONSOLE_IO_HANDLE* /* PCONSOLE_IO_HANDLE */ HandleTable; // Length-varying table

    LPTHREAD_START_ROUTINE CtrlRoutine;
    LPTHREAD_START_ROUTINE PropRoutine; // We hold the property dialog handler there, till all the GUI thingie moves out from CSRSS.
    // LPTHREAD_START_ROUTINE ImeRoutine;
} CONSOLE_PROCESS_DATA, *PCONSOLE_PROCESS_DATA;

#include "include/conio.h"
#include "include/conio_winsrv.h"

#include "api.h" // For the CON_API_* macros.
#include "include/console.h"
#include "include/settings.h"
#include "include/term.h"
#include "console.h"
#include "coninput.h"
#include "conoutput.h"
#include "handle.h"
#include "lineinput.h"

/* shutdown.c */
ULONG
NTAPI
ConsoleClientShutdown(IN PCSR_PROCESS CsrProcess,
                      IN ULONG Flags,
                      IN BOOLEAN FirstPhase);

#endif /* __CONSRV_H__ */
