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
    HMODULE hModule;
    PWSTR pszKeyName;
    PWSTR pszDllName;
    BOOL bEnabled;
    BOOL bAsynchronous;
    BOOL bImpersonate;
    BOOL bSmartCardLogon;
    DWORD dwMaxWait;
    BOOL bSfcNotification;
    PWLX_NOTIFY_HANDLER Handler[LastHandler];
} NOTIFICATION_ITEM, *PNOTIFICATION_ITEM;


static LIST_ENTRY NotificationDllListHead;


/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Retrieves the address of the exported notification handler,
 * specified in the registry entry for the notification DLL.
 **/
static
PWLX_NOTIFY_HANDLER
GetNotificationHandler(
    _In_ HKEY hDllKey,
    _In_ HMODULE hModule,
    _In_ PCSTR pNotification)
{
    LONG lError;
    DWORD dwType, dwSize;
    CHAR szFuncBuffer[128];

    dwSize = sizeof(szFuncBuffer);
    lError = RegQueryValueExA(hDllKey,
                              pNotification,
                              NULL,
                              &dwType,
                              (PBYTE)szFuncBuffer,
                              &dwSize);
    if ((lError != ERROR_SUCCESS) ||
        (dwType != REG_SZ && dwType != REG_EXPAND_SZ) || (dwSize == 0))
    {
        return NULL;
    }

    /* NUL-terminate */
    szFuncBuffer[dwSize / sizeof(CHAR) - 1] = ANSI_NULL;

    return (PWLX_NOTIFY_HANDLER)GetProcAddress(hModule, szFuncBuffer);
}

/**
 * @brief
 * Loads the notification DLL and retrieves its exported notification handlers.
 **/
static
BOOL
LoadNotifyDll(
    _Inout_ PNOTIFICATION_ITEM NotificationDll)
{
    HKEY hNotifyKey, hDllKey;
    HMODULE hModule;
    LONG lError;
    NOTIFICATION_TYPE Type;

    if (NotificationDll->bSfcNotification)
        return TRUE; // Already loaded.

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
                           0,
                           KEY_READ,
                           &hNotifyKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW(Winlogon\\Notify) failed, Error %lu\n", lError);
        return FALSE;
    }

    lError = RegOpenKeyExW(hNotifyKey,
                           NotificationDll->pszKeyName,
                           0,
                           KEY_READ,
                           &hDllKey);
    RegCloseKey(hNotifyKey);

    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW(%S) failed, Error %lu\n", NotificationDll->pszKeyName, lError);
        return FALSE;
    }

    hModule = LoadLibraryW(NotificationDll->pszDllName);
    if (!hModule)
    {
        ERR("LoadLibraryW(%S) failed, Error %lu\n", NotificationDll->pszDllName, GetLastError());
        RegCloseKey(hDllKey);
        return FALSE;
    }
    NotificationDll->hModule = hModule;

    for (Type = LogonHandler; Type < LastHandler; ++Type)
    {
        NotificationDll->Handler[Type] = GetNotificationHandler(hDllKey, hModule, FuncNames[Type]);
        TRACE("%s: %p\n", FuncNames[Type], NotificationDll->Handler[Type]);
    }

    RegCloseKey(hDllKey);
    return TRUE;
}

/**
 * @brief
 * Frees the resources associated to a notification.
 **/
static
VOID
DeleteNotification(
    _In_ PNOTIFICATION_ITEM Notification)
{
    if (Notification->hModule)
        FreeLibrary(Notification->hModule);

    if (Notification->pszKeyName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Notification->pszKeyName);

    if (Notification->pszDllName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Notification->pszDllName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Notification);
}

/**
 * @brief
 * Initializes the internal SFC notifications.
 **/
static
VOID
AddSfcNotification(VOID)
{
    PNOTIFICATION_ITEM NotificationDll;
    HMODULE hModule;
    DWORD dwError;
    WCHAR szSfcPath[MAX_PATH];

    ExpandEnvironmentStringsW(L"%SystemRoot%\\system32\\sfc.dll",
                              szSfcPath,
                              ARRAYSIZE(szSfcPath));

    NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(*NotificationDll));
    if (!NotificationDll)
        return; // If needed: dwError = ERROR_OUTOFMEMORY;

    NotificationDll->pszDllName = WlStrDup(szSfcPath);
    if (NotificationDll->pszDllName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NotificationDll->bEnabled = TRUE;
    NotificationDll->dwMaxWait = 30; /* FIXME: ??? */
    NotificationDll->bSfcNotification = TRUE;

    /* Load sfc.dll, and also sfc_os.dll on systems where it forwards to */
    hModule = LoadLibraryW(szSfcPath);
    if (!hModule)
    {
        dwError = GetLastError();
        ERR("LoadLibraryW(%S) failed, Error %lu\n", szSfcPath, dwError);
        goto done;
    }
    NotificationDll->hModule = hModule;

    NotificationDll->Handler[LogonHandler]  = (PWLX_NOTIFY_HANDLER)GetProcAddress(hModule, "SfcWLEventLogon");
    NotificationDll->Handler[LogoffHandler] = (PWLX_NOTIFY_HANDLER)GetProcAddress(hModule, "SfcWLEventLogoff");
    if (!NotificationDll->Handler[LogonHandler] || !NotificationDll->Handler[LogoffHandler])
    {
        dwError = ERROR_PROC_NOT_FOUND;
        ERR("Couldn't snap SfcWLEventLogon/SfcWLEventLogoff\n");
        goto done;
    }

    InsertHeadList(&NotificationDllListHead,
                   &NotificationDll->ListEntry);

    dwError = ERROR_SUCCESS;

done:
    if (dwError != ERROR_SUCCESS)
        DeleteNotification(NotificationDll);
}

static
VOID
AddNotificationDll(
    _In_ HKEY hNotifyKey,
    _In_ PCWSTR pszKeyName)
{
    HKEY hDllKey = NULL;
    PNOTIFICATION_ITEM NotificationDll;
    PWSTR pszDllName;
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

    /* In safe-boot mode, load the notification DLL only if it is enabled there */
    if (GetSystemMetrics(SM_CLEANBOOT) != 0) // TODO: Cache when Winlogon starts
    {
        BOOL bSafeMode = FALSE; // Default to NOT loading the DLL in SafeMode.

        dwSize = sizeof(dwValue);
        lError = RegQueryValueExW(hDllKey,
                                  L"SafeMode",
                                  NULL,
                                  &dwType,
                                  (PBYTE)&dwValue,
                                  &dwSize);
        if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
            bSafeMode = !!dwValue;

        /* Bail out if the DLL should not be loaded in safe-boot mode.
         * NOTE: On Win2000 and later, the value is always overridden
         * to TRUE, and the DLL is loaded unconditionally. This defeats
         * the whole purpose of this feature... In ReactOS we restore it! */
        if (!bSafeMode)
        {
            RegCloseKey(hDllKey);
            return;
        }
    }

    NotificationDll = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(*NotificationDll));
    if (!NotificationDll)
    {
        lError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NotificationDll->pszKeyName = WlStrDup(pszKeyName);
    if (NotificationDll->pszKeyName == NULL)
    {
        lError = ERROR_OUTOFMEMORY;
        goto done;
    }

    dwSize = 0;
    lError = RegQueryValueExW(hDllKey,
                              L"DllName",
                              NULL,
                              &dwType,
                              NULL,
                              &dwSize);
    if ((lError != ERROR_SUCCESS) ||
        (dwType != REG_SZ && dwType != REG_EXPAND_SZ) ||
        (dwSize == 0) || (dwSize % sizeof(WCHAR)))
    {
        lError = ERROR_FILE_NOT_FOUND;
        goto done;
    }
    /* Ensure the string can be NUL-terminated */
    dwSize += sizeof(WCHAR);

    pszDllName = RtlAllocateHeap(RtlGetProcessHeap(), 0, dwSize);
    if (!pszDllName)
    {
        lError = ERROR_OUTOFMEMORY;
        goto done;
    }

    lError = RegQueryValueExW(hDllKey,
                              L"DllName",
                              NULL,
                              &dwType,
                              (PBYTE)pszDllName,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, pszDllName);
        goto done;
    }
    /* NUL-terminate */
    pszDllName[dwSize / sizeof(WCHAR) - 1] = UNICODE_NULL;

    /* Expand the path if applicable. Note that Windows always does the
     * expansion, independently of the REG_SZ or REG_EXPAND_SZ type. */
    if (wcschr(pszDllName, L'%') != NULL)
    {
        dwSize = ExpandEnvironmentStringsW(pszDllName, NULL, 0);
        if (dwSize)
        {
            PWSTR pszDllPath = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                               dwSize * sizeof(WCHAR));
            if (!pszDllPath)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, pszDllName);
                lError = ERROR_OUTOFMEMORY;
                goto done;
            }
            ExpandEnvironmentStringsW(pszDllName, pszDllPath, dwSize);

            /* Free the old buffer and replace it with the expanded path */
            RtlFreeHeap(RtlGetProcessHeap(), 0, pszDllName);
            pszDllName = pszDllPath;
        }
    }

    NotificationDll->pszDllName = pszDllName;

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
    _In_ PNOTIFICATION_ITEM NotificationDll,
    _In_ NOTIFICATION_TYPE Type,
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    PWLX_NOTIFY_HANDLER pNotifyHandler;
    WLX_NOTIFICATION_INFO Info;
    HANDLE UserToken;

    /* Delay-load the DLL if needed */
    if (!NotificationDll->hModule)
    {
        if (!LoadNotifyDll(NotificationDll))
        {
            /* We failed, disable it */
            NotificationDll->bEnabled = FALSE;
            return;
        }
        ASSERT(NotificationDll->hModule);
    }

    /* Retrieve the notification handler; bail out if none is specified */
    pNotifyHandler = NotificationDll->Handler[Type];
    if (!pNotifyHandler)
        return;

    /* Capture the notification info structure, since
     * the notification handler might mess with it */
    Info = *pInfo;

    /* Impersonate the logged-on user if necessary */
    UserToken = (NotificationDll->bImpersonate ? Info.hToken : NULL);
    if (UserToken && !ImpersonateLoggedOnUser(UserToken))
    {
        ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return;
    }

    /* Call the notification handler in SEH to prevent any Winlogon crashes */
    _SEH2_TRY
    {
        pNotifyHandler(&Info);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("WL: Exception 0x%08lx hit by notification DLL %ws while executing %s notify function\n",
            _SEH2_GetExceptionCode(), NotificationDll->pszDllName, FuncNames[Type]);
    }
    _SEH2_END;

    /* Revert impersonation */
    if (UserToken)
        RevertToSelf();
}


VOID
CallNotificationDlls(
    PWLSESSION pSession,
    NOTIFICATION_TYPE Type)
{
    PLIST_ENTRY ListEntry;
    WLX_NOTIFICATION_INFO Info;

    /* Check for invalid notification type */
    ASSERT(Type < LastHandler);

    TRACE("CallNotificationDlls(%s)\n", FuncNames[Type]);

    /* Set up the notification info structure template */
    Info.Size = sizeof(Info);

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

    Info.UserName = pSession->UserName;
    Info.Domain = pSession->Domain;
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
            CallNotificationDll(Notification, Type, &Info);
    }
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
