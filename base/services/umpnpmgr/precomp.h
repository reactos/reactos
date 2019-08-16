/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             base/services/umpnpmgr/install.c
 * PURPOSE:          Device installer
 * PROGRAMMER:       Eric Kohl (eric.kohl@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Colin Finck (colin@reactos.org)
 */

#ifndef _UMPNPMGR_PCH_
#define _UMPNPMGR_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <winuser.h>
#include <dbt.h>
#include <stdio.h>
#include <cmfuncs.h>
#include <rtlfuncs.h>
#include <setypes.h>
#include <umpnpmgr/sysguid.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <userenv.h>
#include <shlwapi.h>
#include <pnp_s.h>


typedef struct
{
    SLIST_ENTRY ListEntry;
    WCHAR DeviceIds[1];
} DeviceInstallParams;

/* install.c */

extern HANDLE hUserToken;
extern HANDLE hInstallEvent;
extern HANDLE hNoPendingInstalls;

extern SLIST_HEADER DeviceInstallListHead;
extern HANDLE hDeviceInstallListNotEmpty;

BOOL
SetupIsActive(VOID);

FORCEINLINE
BOOL
IsUISuppressionAllowed(VOID);

DWORD
WINAPI
DeviceInstallThread(
    LPVOID lpParameter);


/* rpcserver.c */

DWORD
WINAPI
RpcServerThread(
    LPVOID lpParameter);


/* umpnpmgr.c */

extern HKEY hEnumKey;
extern HKEY hClassKey;
extern BOOL g_IsUISuppressed;

BOOL
GetSuppressNewUIValue(VOID);

#endif /* _UMPNPMGR_PCH_ */