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

/* Public Win32K Headers */
#include <ntuser.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* External Winlogon Header */
#include <winlogon.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* USER Headers */
#include <win/winmsg.h>

/* Public Win32 Headers */
#include <commctrl.h>

#include "resource.h"


/* Globals */
extern HINSTANCE UserServerDllInstance;
extern HANDLE UserServerHeap;
extern ULONG_PTR LogonProcessId;

#endif // __WINSRV_H__

/* EOF */
