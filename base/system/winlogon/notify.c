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

/**
 * @brief
 * Frees the resources associated to a notification.
 **/
static
VOID
DeleteNotification(
    _In_ PNOTIFICATION_ITEM Notification)
{
    if (Notification->pszKeyName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Notification->pszKeyName);

    if (Notification->pszDllName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Notification->pszDllName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Notification);
}

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
    _In_ HKEY hNotifyKey,
    _In_ PCWSTR pszKeyName)
{
    HKEY hDllKey = NULL;
    PNOTIFICATION_ITEM NotificationDll;
    DWORD dwValue, dwSize, dwType;
    LONG lError;

    TRACE("AddNotificationDll(0x%p, %S)\n", hNotifyKey, pszKeyName);

    lError = RegOpenKeyExW(hNotifyKey,
                           pszKeyName,
                           0,
                           KEY_READ,
                           &hDllKey);
    if (lError != ERROR_SUCCESS)
        return;

    NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(*NotificationDll));
    if (NotificationDll == NULL)
    {
        lError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NotificationDll->pszKeyName = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  (wcslen(pszKeyName) + 1) * sizeof(WCHAR));
    if (NotificationDll->pszKeyName == NULL)
    {
        lError = ERROR_OUTOFMEMORY;
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
        lError = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    NotificationDll->pszDllName = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  dwSize);
    if (NotificationDll->pszDllName == NULL)
    {
        lError = ERROR_OUTOFMEMORY;
        goto done;
    }

    lError = RegQueryValueExW(hDllKey,
                              L"DllName",
                              NULL,
                              &dwType,
                              (PBYTE)NotificationDll->pszDllName,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    NotificationDll->bEnabled = TRUE;
    NotificationDll->dwMaxWait = 30; /* FIXME: ??? */

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hDllKey,
                              L"Asynchronous",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        NotificationDll->bAsynchronous = !!dwValue;

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hDllKey,
                              L"Impersonate",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        NotificationDll->bImpersonate = !!dwValue;

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hDllKey,
                              L"Safe",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        NotificationDll->bSafe = !!dwValue;

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hDllKey,
                              L"SmartCardLogonNotify",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        NotificationDll->bSmartCardLogon = !!dwValue;

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hDllKey,
                              L"MaxWait",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        NotificationDll->dwMaxWait = dwValue;

    InsertHeadList(&NotificationDllListHead,
                   &NotificationDll->ListEntry);

    lError = ERROR_SUCCESS;

done:
    if (lError != ERROR_SUCCESS)
    {
        if (NotificationDll)
            DeleteNotification(NotificationDll);
    }

    RegCloseKey(hDllKey);
}


BOOL
InitNotifications(VOID)
{
    HKEY hNotifyKey = NULL;
    LONG lError;
    DWORD dwIndex;
    DWORD dwKeyName;
    WCHAR szKeyName[80];

    TRACE("InitNotifications()\n");

    InitializeListHead(&NotificationDllListHead);

    AddSfcNotification();

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
                           0,
                           KEY_READ,
                           &hNotifyKey);
    if (lError != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyExW()\n");
        return TRUE;
    }

    for (dwIndex = 0; ; ++dwIndex)
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
    HMODULE hModule;
    CHAR szFuncBuffer[128];
    DWORD dwError = ERROR_SUCCESS;
    PWLX_NOTIFY_HANDLER pNotifyHandler;

    if (NotificationDll->bSfcNotification)
    {
        switch (Type)
        {
            case LogonHandler:
                strcpy(szFuncBuffer, "SfcWLEventLogon");
                break;

            case LogoffHandler:
                strcpy(szFuncBuffer, "SfcWLEventLogoff");
                break;

            default:
                return;
        }
    }
    else
    {
        HKEY hDllKey;
        DWORD dwSize;
        DWORD dwType;

        dwError = RegOpenKeyExW(hNotifyKey,
                                NotificationDll->pszKeyName,
                                0,
                                KEY_READ,
                                &hDllKey);
        if (dwError != ERROR_SUCCESS)
        {
            TRACE("RegOpenKeyExW()\n");
            return;
        }

        dwSize = sizeof(szFuncBuffer);
        dwError = RegQueryValueExA(hDllKey,
                                   FuncNames[Type],
                                   NULL,
                                   &dwType,
                                   (PBYTE)szFuncBuffer,
                                   &dwSize);

        RegCloseKey(hDllKey);
    }

    if (dwError != ERROR_SUCCESS)
        return;

    hModule = LoadLibraryW(NotificationDll->pszDllName);
    if (!hModule)
        return;

    pNotifyHandler = (PWLX_NOTIFY_HANDLER)GetProcAddress(hModule, szFuncBuffer);

    _SEH2_TRY
    {
        if (pNotifyHandler)
            pNotifyHandler(pInfo);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("WL: Exception while running notification %S!%s, Status 0x%08lx\n",
            NotificationDll->pszDllName, szFuncBuffer, _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    FreeLibrary(hModule);
}


VOID
CallNotificationDlls(
    PWLSESSION pSession,
    NOTIFICATION_TYPE Type)
{
    PLIST_ENTRY ListEntry;
    WLX_NOTIFICATION_INFO Info;
    HKEY hNotifyKey = NULL;
    DWORD dwError;

    TRACE("CallNotificationDlls()\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
                            0,
                            KEY_READ,
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

    for (ListEntry = NotificationDllListHead.Flink;
         ListEntry != &NotificationDllListHead;
         ListEntry = ListEntry->Flink)
    {
        PNOTIFICATION_ITEM Notification =
            CONTAINING_RECORD(ListEntry, NOTIFICATION_ITEM, ListEntry);
        if (Notification->bEnabled)
            CallNotificationDll(hNotifyKey, Notification, Type, &Info);
    }

    RegCloseKey(hNotifyKey);
}


VOID
CleanupNotifications(VOID)
{
    while (!IsListEmpty(&NotificationDllListHead))
    {
        PLIST_ENTRY ListEntry = RemoveHeadList(&NotificationDllListHead);
        PNOTIFICATION_ITEM Notification =
            CONTAINING_RECORD(ListEntry, NOTIFICATION_ITEM, ListEntry);
        DeleteNotification(Notification);
    }
}

/* EOF */
