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

typedef enum _NOTIFICATION_TYPE
{
    LogonHandler,
    LogoffHandler,
    LockHandler,
    UnlockHandler,
    StartupHandler,
    ShutdownHandler,
    StartScreenSaverHandler,
    StopScreenSaverHandler,
    DisconnectHandler,
    ReconnectHandler,
    StartShellHandler,
    PostShellHandler,
    LastHandler
} NOTIFICATION_TYPE, *PNOTIFICATION_TYPE;

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
            return;

        NotificationDll->hInstance = hInstance;

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



        }

        ListEntry = ListEntry->Flink;

        RemoveEntryList(&NotificationDll->ListEntry);
    }
}

/* EOF */
