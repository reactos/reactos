/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/winsrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __WINSRV_H__
#define __WINSRV_H__

#include <stdarg.h>

/* PSDK/NDK Headers */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winuser.h>
#include <imm.h>
#include <immdev.h>
#include <imm32_undoc.h>

/* Undocumented user definitions */
#include <undocuser.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Public Win32K Headers */
#include <ntuser.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

typedef struct tagSHUTDOWN_SETTINGS
{
    BOOL  AutoEndTasks;
    ULONG HungAppTimeout;
    ULONG WaitToKillAppTimeout;
    ULONG WaitToKillServiceTimeout;
    ULONG ProcessTerminateTimeout;
} SHUTDOWN_SETTINGS, *PSHUTDOWN_SETTINGS;

extern SHUTDOWN_SETTINGS ShutdownSettings;

VOID FASTCALL
GetTimeouts(IN PSHUTDOWN_SETTINGS ShutdownSettings);

#endif /* __WINSRV_H__ */
