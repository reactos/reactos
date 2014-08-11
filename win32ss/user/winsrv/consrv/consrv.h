/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/consrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __CONSRV_H__
#define __CONSRV_H__

/* PSDK/NDK Headers */
#include <stdarg.h>
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

/* Public Win32K Headers */
#include <ntuser.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

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
    HANDLE InputWaitHandle;

    HANDLE ConsoleHandle;
    HANDLE ParentConsoleHandle;

    BOOLEAN ConsoleApp;    // TRUE if it is a CUI app, FALSE otherwise.

    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    struct _CONSOLE_IO_HANDLE* /* PCONSOLE_IO_HANDLE */ HandleTable; // Length-varying table

    LPTHREAD_START_ROUTINE CtrlDispatcher;
    LPTHREAD_START_ROUTINE PropDispatcher; // We hold the property dialog handler there, till all the GUI thingie moves out from CSRSS.
} CONSOLE_PROCESS_DATA, *PCONSOLE_PROCESS_DATA;

#include "include/conio.h"
// #include "include/conio_winsrv.h"
#include "include/console.h"
#include "include/settings.h"
#include "include/term.h"
#include "console.h"
#include "conoutput.h"
#include "handle.h"
#include "lineinput.h"

#endif /* __CONSRV_H__ */
