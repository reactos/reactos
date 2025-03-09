/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/notify.c
 * PURPOSE:         Logon notifications
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

/* GLOBALS ******************************************************************/


// void Event_Handler_Function_Name(PWLX_NOTIFICATION_INFO pInfo);
typedef VOID (WINAPI *PWLX_NOTIFY_HANDLER)(PWLX_NOTIFICATION_INFO pInfo);

static PSTR FuncNames[LastHandler] =
{
    "Logon",
    "Logoff",
    "Lock",
    "Unlock",
    "Startup",
    "Shutdown",
    "StartScreenSaver",
    "StopScreenSaver",
    "Disconnect",
    "Reconnect",
    "StartShell",
    "PostShell"
};

typedef struct _NOTIFICATION_ITEM
{
    LIST_ENTRY ListEntry;
    PWSTR pszKeyName;
    PWSTR pszDllName;
    BOOL bEnabled;
    BOOL bAsynchronous;
    BOOL bSafe;
    BOOL bImpersonate;
    BOOL bSmartCardLogon;
    DWORD dwMaxWait;
    BOOL bSfcNotification;
} NOTIFICATION_ITEM, *PNOTIFICATION_ITEM;


static LIST_ENTRY NotificationDllListHead;


/* FUNCTIONS *****************************************************************/

static
VOID
AddSfcNotification(VOID)
{
    WCHAR szSfcFileName[MAX_PATH];
    PNOTIFICATION_ITEM NotificationDll;
    DWORD dwError = ERROR_SUCCESS;

    ExpandEnvironmentStringsW(L"%windir%\\system32\\sfc.dll",
                              szSfcFileName,
                              ARRAYSIZE(szSfcFileName));

    NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(NOTIFICATION_ITEM));
    if (NotificationDll == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NotificationDll->pszDllName = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  (wcslen(szSfcFileName) + 1) * sizeof(WCHAR));
    if (NotificationDll->pszDllName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    wcscpy(NotificationDll->pszDllName, szSfcFileName);

    NotificationDll->bEnabled = TRUE;
    NotificationDll->dwMaxWait = 30; /* FIXME: ??? */
    NotificationDll->bSfcNotification = TRUE;

    InsertHeadList(&NotificationDllListHead,
                   &NotificationDll->ListEntry);

done:
    if (dwError != ERROR_SUCCESS)
    {
        if (NotificationDll != NULL)
        {
            if (NotificationDll->pszDllName != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll->pszDllName);

            RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll);
        }
    }
}

static
VOID
AddNotificationDll(
    HKEY hNotifyKey,
    PWSTR pszKeyName)
{
    HKEY hDllKey = NULL;
    PNOTIFICATION_ITEM NotificationDll = NULL;
    DWORD dwSize, dwType;
    DWORD dwError;

    TRACE("AddNotificationDll(%p %S)\n", hNotifyKey, pszKeyName);

    dwError = RegOpenKeyExW(hNotifyKey,
                            pszKeyName,
                            0,
                            KEY_READ,
                            &hDllKey);
    if (dwError != ERROR_SUCCESS)
        return;

    NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(NOTIFICATION_ITEM));
    if (NotificationDll == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NotificationDll->pszKeyName = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  (wcslen(pszKeyName) + 1) * sizeof(WCHAR));
    if (NotificationDll->pszKeyName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    wcscpy(NotificationDll->pszKeyName, pszKeyName);

    dwSize = 0;
    RegQueryValueExW(hDllKey,
                     L"DllName",
                     NULL,
                     &dwType,
                     NULL,
                     &dwSize);
    if (dwSize == 0)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    NotificationDll->pszDllName = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  dwSize);
    if (NotificationDll->pszDllName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    dwError = RegQueryValueExW(hDllKey,
                               L"DllName",
                               NULL,
                               &dwType,
                               (PBYTE)NotificationDll->pszDllName,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        goto done;

    NotificationDll->bEnabled = TRUE;
    NotificationDll->dwMaxWait = 30; /* FIXME: ??? */

    dwSize = sizeof(BOOL);
    RegQueryValueExW(hDllKey,
                     L"Asynchronous",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->bAsynchronous,
                     &dwSize);

    dwSize = sizeof(BOOL);
    RegQueryValueExW(hDllKey,
                     L"Enabled",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->bEnabled,
                     &dwSize);

    dwSize = sizeof(BOOL);
    RegQueryValueExW(hDllKey,
                     L"Impersonate",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->bImpersonate,
                     &dwSize);

    dwSize = sizeof(BOOL);
    RegQueryValueExW(hDllKey,
                     L"Safe",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->bSafe,
                     &dwSize);

    dwSize = sizeof(BOOL);
    RegQueryValueExW(hDllKey,
                     L"SmartCardLogonNotify",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->bSmartCardLogon,
                     &dwSize);

    dwSize = sizeof(DWORD);
    RegQueryValueExW(hDllKey,
                     L"MaxWait",
                     NULL,
                     &dwType,
                     (PBYTE)&NotificationDll->dwMaxWait,
                     &dwSize);

    InsertHeadList(&NotificationDllListHead,
                   &NotificationDll->ListEntry);

done:
    if (dwError != ERROR_SUCCESS)
    {
        if (NotificationDll != NULL)
        {
            if (NotificationDll->pszKeyName != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll->pszKeyName);

            if (NotificationDll->pszDllName != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll->pszDllName);

            RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll);
        }
    }

    RegCloseKey(hDllKey);
}


BOOL
InitNotifications(VOID)
{
    HKEY hNotifyKey = NULL;
    LONG lError;
    DWORD dwIndex;
    WCHAR szKeyName[80];
    DWORD dwKeyName;

    TRACE("InitNotifications()\n");

    InitializeListHead(&NotificationDllListHead);

    AddSfcNotification();

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
                           0,
                           KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                           &hNotifyKey);
    if (lError != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyExW()\n");
        return TRUE;
    }

    dwIndex = 0;
    for(;;)
    {
        dwKeyName = ARRAYSIZE(szKeyName);
        lError = RegEnumKeyExW(hNotifyKey,
                               dwIndex,
                               szKeyName,
                               &dwKeyName,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS)
            break;

        TRACE("Notification DLL: %S\n", szKeyName);
        AddNotificationDll(hNotifyKey, szKeyName);

        dwIndex++;
    }

    RegCloseKey(hNotifyKey);

    TRACE("InitNotifications() done\n");

    return TRUE;
}


static
VOID
CallNotificationDll(
    HKEY hNotifyKey,
    PNOTIFICATION_ITEM NotificationDll,
    NOTIFICATION_TYPE Type,
    PWLX_NOTIFICATION_INFO pInfo)
{
    UNREFERENCED_PARAMETER(FuncNames);
}


VOID
CallNotificationDlls(
    PWLSESSION pSession,
    NOTIFICATION_TYPE Type)
{
    PLIST_ENTRY ListEntry;
    PNOTIFICATION_ITEM NotificationDll;
    WLX_NOTIFICATION_INFO Info;
    HKEY hNotifyKey = NULL;
    DWORD dwError;

    TRACE("CallNotificationDlls()\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
                            0,
                            KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                            &hNotifyKey);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyExW()\n");
        return;
    }

    Info.Size = sizeof(WLX_NOTIFICATION_INFO);

    switch (Type)
    {
        case LogoffHandler:
        case ShutdownHandler:
            Info.Flags = 3;
            break;

        default:
            Info.Flags = 0;
            break;
    }

    Info.UserName = NULL; //UserName;
    Info.Domain = NULL; //Domain;
    Info.WindowStation = pSession->InteractiveWindowStationName;
    Info.hToken = pSession->UserToken;

    switch (Type)
    {
        case LogonHandler:
        case StartShellHandler:
            Info.hDesktop = pSession->ApplicationDesktop;
            break;

        case StartScreenSaverHandler:
            Info.hDesktop = pSession->ApplicationDesktop;
            break;

        default:
            Info.hDesktop = pSession->WinlogonDesktop;
            break;
    }

    Info.pStatusCallback = NULL;

    ListEntry = NotificationDllListHead.Flink;
    while (ListEntry != &NotificationDllListHead)
    {
TRACE("ListEntry %p\n", ListEntry);

        NotificationDll = CONTAINING_RECORD(ListEntry,
                                            NOTIFICATION_ITEM,
                                            ListEntry);
TRACE("NotificationDll: %p\n", NotificationDll);
        if (NotificationDll != NULL && NotificationDll->bEnabled)
            CallNotificationDll(hNotifyKey, NotificationDll, Type, &Info);

        ListEntry = ListEntry->Flink;
    }

    RegCloseKey(hNotifyKey);
}


VOID
CleanupNotifications(VOID)
{
    PLIST_ENTRY ListEntry;
    PNOTIFICATION_ITEM NotificationDll;

    ListEntry = NotificationDllListHead.Flink;
    while (ListEntry != &NotificationDllListHead)
    {
        NotificationDll = CONTAINING_RECORD(ListEntry,
                                            NOTIFICATION_ITEM,
                                            ListEntry);
        if (NotificationDll != NULL)
        {
            if (NotificationDll->pszKeyName != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll->pszKeyName);

            if (NotificationDll->pszDllName != NULL)
                RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll->pszDllName);
        }

        ListEntry = ListEntry->Flink;

        RemoveEntryList(&NotificationDll->ListEntry);

        RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll);
    }
}

/* EOF */
