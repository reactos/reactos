/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/usersrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __USERSRV_H__
#define __USERSRV_H__

/* Main header */
#include "../winsrv.h"

/* PSDK/NDK Headers */
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

// #define NTOS_MODE_USER

/* BASE Header */
#include <win/base.h>

/* USER Headers */
#include <win/winmsg.h>

/* Globals */
extern HINSTANCE UserServerDllInstance;
extern HANDLE UserServerHeap;
extern ULONG_PTR ServicesProcessId;
extern ULONG_PTR LogonProcessId;

#endif /* __USERSRV_H__ */
