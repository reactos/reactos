/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/usersrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __USERSRV_H__
#define __USERSRV_H__

/* PSDK/NDK Headers */
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>

#define NTOS_MODE_USER
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

/* Public Win32K Headers */
#include <ntuser.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* USER Headers */
#include <win/winmsg.h>

/* Globals */
extern HINSTANCE UserServerDllInstance;
extern HANDLE UserServerHeap;
extern ULONG_PTR ServicesProcessId;
extern ULONG_PTR LogonProcessId;

#endif /* __USERSRV_H__ */
