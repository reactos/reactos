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
#include <winuser.h>
#include <imm.h>

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

#endif /* __WINSRV_H__ */
