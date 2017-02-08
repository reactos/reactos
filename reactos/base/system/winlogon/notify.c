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

    HINSTANCE hInstance;
    BOOL bEnabled;
    BOOL bAsynchronous;
    BOOL bSafe;
    BOOL bImpersonate;
    BOOL bSmartCardLogon;
    DWORD dwMaxWait;
    PWLX_NOTIFY_HANDLER Handler[LastHandler];
} NOTIFICATION_ITEM, *PNOTIFICATION_ITEM;


static LIST_ENTRY NotificationDllListHead;


/* FUNCTIONS *****************************************************************/

PWLX_NOTIFY_HANDLER
GetNotificationHandler(
    HKEY hDllKey,
    HINSTANCE hInstance,
    PSTR pNotification)
{
    CHAR szFuncBuffer[128];
    DWORD dwSize;
    DWORD dwType;
    LONG lError;

    dwSize = 128;
    lError = RegQueryValueExA(hDllKey,
                              pNotification,
                              NULL,
                              &dwType,
                              (PBYTE)szFuncBuffer,
                              &dwSize);
    if (lError == ERROR_SUCCESS)
    {
        return (PWLX_NOTIFY_HANDLER)GetProcAddress(hInstance, szFuncBuffer);
    }

    return NULL;
}


static
VOID
LoadNotificationDll(
    HKEY hNotifyKey,
    PWSTR pKeyName)
{
    HKEY hDllKey = NULL;
    PNOTIFICATION_ITEM NotificationDll = NULL;
    LONG lError;
    WCHAR szBuffer[80];
    DWORD dwSize;
    DWORD dwType;
    HINSTANCE hInstance;
    NOTIFICATION_TYPE i;

    TRACE("LoadNotificationDll(%p %S)\n", hNotifyKey, pKeyName);

    lError = RegOpenKeyExW(hNotifyKey,
                           pKeyName,
                           0,
                           KEY_READ,
                           &hDllKey);
    if (lError != ERROR_SUCCESS)
        return;

    dwSize = 80 * sizeof(WCHAR);
    lError = RegQueryValueExW(hDllKey,
                              L"DllName",
                              NULL,
                              &dwType,
                              (PBYTE)szBuffer,
                              &dwSize);
    if (lError == ERROR_SUCCESS)
    {
        hInstance = LoadLibraryW(szBuffer);
        if (hInstance == NULL)
            return;

        NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          sizeof(NOTIFICATION_ITEM));
        if (NotificationDll == NULL)
        {
            FreeLibrary(hInstance);
            return;
        }

        NotificationDll->bEnabled = TRUE;
        NotificationDll->dwMaxWait = 30; /* FIXME: ??? */
        NotificationDll->hInstance = hInstance;

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

        for (i = LogonHandler; i < LastHandler; i++)
        {
            NotificationDll->Handler[i] = GetNotificationHandler(hDllKey, hInstance, FuncNames[i]);
            TRACE("%s: %p\n", FuncNames[i], NotificationDll->Handler[i]);
        }

        InsertHeadList(&NotificationDllListHead,
                       &NotificationDll->ListEntry);
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
        dwKeyName = 80 * sizeof(WCHAR);
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
        LoadNotificationDll(hNotifyKey, szKeyName);

        dwIndex++;
    }

    RegCloseKey(hNotifyKey);

    TRACE("InitNotifications() done\n");

    return TRUE;
}


VOID
CallNotificationDlls(
    PWLSESSION pSession,
    NOTIFICATION_TYPE Type)
{
    PLIST_ENTRY ListEntry;
    PNOTIFICATION_ITEM NotificationDll;
    WLX_NOTIFICATION_INFO Info;

    TRACE("CallNotificationDlls()\n");

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
        {
TRACE("NotificationDll->Handler: %p\n", NotificationDll->Handler[Type]);
            if (NotificationDll->Handler[Type] != NULL)
                NotificationDll->Handler[Type](&Info);
        }

        ListEntry = ListEntry->Flink;
    }
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
            FreeLibrary(NotificationDll->hInstance);
        }

        ListEntry = ListEntry->Flink;

        RemoveEntryList(&NotificationDll->ListEntry);

        RtlFreeHeap(RtlGetProcessHeap(), 0, NotificationDll);
    }
}

/* EOF */
