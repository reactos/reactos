/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            subsystems/win32/csrsrv/srv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  ReactOS Portable Systems Group
 */

#ifndef _SRV_H
#define _SRV_H

/* PSDK/NDK Headers */
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>    // For EWX_FORCE
#define NTOS_MODE_USER
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/kefuncs.h>
//#include <ndk/dbgktypes.h>
#include <ndk/mmfuncs.h>
#include <ndk/umfuncs.h>

/* CSR Header */
#include <csr/csrsrv.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Subsystem Manager Header */
#include <sm/helper.h>
#include <sm/smmsg.h>

/* Internal CSRSS Header */
#include "api.h"

/* Defines */
#define SM_REG_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"

#define SESSION_ROOT        L"\\Sessions"
#define GLOBAL_ROOT         L"\\GLOBAL??"
#define SYMLINK_NAME        L"SymbolicLink"
#define SB_PORT_NAME        L"SbAbiPort"
#define CSR_PORT_NAME       L"ApiPort"
#define UNICODE_PATH_SEP    L"\\"

#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))
#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))

#endif // _SRV_H

/* EOF */
