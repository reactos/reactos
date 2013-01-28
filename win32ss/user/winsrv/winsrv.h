/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/winsrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __WINSRV_H__
#define __WINSRV_H__

#pragma once

/* PSDK/NDK Headers */
#include <stdarg.h>
#include <stdlib.h>
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winuser.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

/* External Winlogon Header */
#include <winlogon.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* USER Headers */
#include <win/winmsg.h>
// #include <win/base.h>
// #include <win/windows.h>

/* Public Win32 Headers */
#include <commctrl.h>

#include "resource.h"


extern HINSTANCE UserSrvDllInstance;
extern HANDLE UserSrvHeap;
// extern HANDLE BaseSrvSharedHeap;
// extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

/* init.c */
BOOL WINAPI _UserSoundSentry(VOID);

/* harderror.c */
VOID
WINAPI
Win32CsrHardError(IN PCSR_THREAD ThreadData,
                  IN PHARDERROR_MSG Message);


/* shutdown.c */
CSR_API(SrvExitWindowsEx);
// CSR_API(CsrRegisterSystemClasses);
CSR_API(SrvRegisterServicesProcess);
CSR_API(SrvRegisterLogonProcess);

/// HACK: ReactOS-specific
CSR_API(RosSetLogonNotifyWindow);


/*****************************

/\*
typedef VOID (WINAPI *CSR_CLEANUP_OBJECT_PROC)(Object_t *Object);

typedef struct tagCSRSS_OBJECT_DEFINITION
{
  LONG Type;
  CSR_CLEANUP_OBJECT_PROC CsrCleanupObjectProc;
} CSRSS_OBJECT_DEFINITION, *PCSRSS_OBJECT_DEFINITION;
*\/



*****************************/

#endif // __WINSRV_H__

/* EOF */
