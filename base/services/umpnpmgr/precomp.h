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
#include <wdmguid.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <userenv.h>
#include <shlwapi.h>
#include <winsvc_undoc.h>
#include <pnp_s.h>


#define LOGCONF_NAME_BUFFER_SIZE 32

typedef struct
{
    LIST_ENTRY ListEntry;
    WCHAR DeviceIds[ANYSIZE_ARRAY];
} DeviceInstallParams;

typedef enum
{
    CLASS_NOTIFICATION = 1,
    TARGET_NOTIFICATION
} NOTIFICATION_TYPE;


typedef struct
{
    LIST_ENTRY ListEntry;
    NOTIFICATION_TYPE dwType;
    PWSTR pszName;
    DWORD_PTR hRecipient;
    DWORD ulFlags;
    GUID ClassGuid;
} NOTIFY_ENTRY, *PNOTIFY_ENTRY;

/* event.c */

DWORD
WINAPI
PnpEventThread(
    LPVOID lpParameter);


/* install.c */

extern HANDLE hUserToken;
extern HANDLE hInstallEvent;
extern HANDLE hNoPendingInstalls;

/* Device-install event list */
extern HANDLE hDeviceInstallListMutex;
extern LIST_ENTRY DeviceInstallListHead;
extern HANDLE hDeviceInstallListNotEmpty;

BOOL
SetupIsActive(VOID);

DWORD
WINAPI
DeviceInstallThread(
    LPVOID lpParameter);


/* rpcserver.c */

extern LIST_ENTRY NotificationListHead;
extern RTL_RESOURCE NotificationListLock;

DWORD
WINAPI
RpcServerThread(
    LPVOID lpParameter);


/* umpnpmgr.c */

extern HKEY hEnumKey;
extern HKEY hClassKey;
extern BOOL g_IsUISuppressed;
extern BOOL g_ShuttingDown;

BOOL
GetSuppressNewUIValue(VOID);

#endif /* _UMPNPMGR_PCH_ */
