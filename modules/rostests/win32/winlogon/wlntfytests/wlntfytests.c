/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests for Winlogon notifications
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * REFERENCES:
 * - https://learn.microsoft.com/en-us/windows/win32/secauthn/creating-a-winlogon-notification-package
 * - https://learn.microsoft.com/en-us/windows/win32/secauthn/winlogon-notification-events
 * - https://rsdn.org/article/baseserv/winlogon.xml
 */

/* C Headers */
#include <stdlib.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <windowsx.h> // For GetInstanceModule()
#include <winwlx.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h> // For RtlAnsiStringToUnicodeString()

#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(wlnotify_tests);

#define NOTIFY_PKG_NAME L"WLNotifyTests"
#define NOTIFY_REG_PATH L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\" NOTIFY_PKG_NAME

typedef enum _WLNOTIFY_STATE
{
    WLNotify_NonInitialized = -1,
    WLNotify_Startup,
    WLNotify_Shutdown,
    WLNotify_Logon,
    WLNotify_Logoff,
    WLNotify_StartShell,
    WLNotify_PostShell,
    WLNotify_Lock,
    WLNotify_Unlock,
    WLNotify_StartScreenSaver,
    WLNotify_StopScreenSaver,
    WLNotify_Disconnect,
    WLNotify_Reconnect,
    WLNotify_MaxState
} WLNOTIFY_STATE;

static const PCSTR NotifyStateName[] =
{
    "Startup",
    "Shutdown",
    "Logon",
    "Logoff",
    "StartShell",
    "PostShell",
    "Lock",
    "Unlock",
    "StartScreenSaver",
    "StopScreenSaver",
    "Disconnect",
    "Reconnect",
    "NON-INITIALIZED",
};

static WLNOTIFY_STATE g_CurrentState  = WLNotify_NonInitialized;
static WLNOTIFY_STATE g_PreviousState = WLNotify_NonInitialized;
static HMODULE g_hModule = NULL;

#if 0
void DPRINTF_FN(PCSTR FunctionName, PCWSTR Format, ...)
{
    WCHAR Buffer[512];
    WCHAR* Current = Buffer;
    size_t Length = _countof(Buffer);

    StringCchPrintfExW(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, L"[%-20S] ", FunctionName);
    va_list ArgList;
    va_start(ArgList, Format);
    StringCchVPrintfExW(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, Format, ArgList);
    va_end(ArgList);
    OutputDebugStringW(Buffer);
}

#define DPRINTF(fmt, ...)  DPRINTF_FN(__FUNCTION__, fmt, ##__VA_ARGS__ )
#endif

FORCEINLINE
VOID
DbgBreakOnEvent(VOID)
{
    if (IsDebuggerPresent())
        DbgBreakPoint();
}

/**
 * @brief
 * Maps a WLNOTIFY_STATE value to its human-readable name.
 *
 * @param[in]   State
 * The WLNOTIFY_STATE value for which to retrieve its name.
 *
 * @return
 * A pointer to the corresponding ANSI string name.
 **/
static PCSTR
NotifyStateToName(
    _In_ WLNOTIFY_STATE State)
{
    UINT i;

    if (State > WLNotify_NonInitialized && State < WLNotify_MaxState)
        i = State;
    else
        i = _countof(NotifyStateName)-1;

    return NotifyStateName[i];
}

/**
 * @brief
 * Retrieves the name of the specified window station or desktop object.
 *
 * @param[in]   hObj
 * A handle to the window station or desktop object.
 *
 * @param[out]  PreAllocBuffer
 * Optional pointer to an existing caller pre-allocated buffer.
 *
 * @param[in]   cchBufLength
 * Length in number of WCHARs of the caller pre-allocated buffer, if any.
 *
 * @return
 * If the function succeeds, it returns:
 * - either a pointer to the same buffer pointed by @p PreAllocBuffer, if
 *   this user-given buffer was large enough to store the entire object name;
 * - or, a pointer to a new buffer, allocated with LocalAlloc().
 * After usage, the caller will have to distinguish these two cases,
 * and free the buffer with LocalFree() in the second case.
 *
 * If the function fails, the return value is NULL.
 **/
static PWSTR
GetUserObjectName(
    _In_ HANDLE hObj,
    _Out_writes_opt_z_(cchBufLength) PWSTR PreAllocBuffer, // _Inout_updates_opt_z_
    _In_range_(0, MAXDWORD / sizeof(WCHAR)) DWORD cchBufLength)
{
    PWSTR pszBuffer = (cchBufLength ? PreAllocBuffer : NULL);
    DWORD dwBufSize = (pszBuffer ? cchBufLength * sizeof(WCHAR) : 0);
    DWORD dwReqSize = 0;

    if (GetUserObjectInformationW(hObj,
                                  UOI_NAME,
                                  pszBuffer,
                                  dwBufSize,
                                  &dwReqSize))
    {
        return pszBuffer;
    }

    if (dwReqSize > dwBufSize)
    {
        /* The call failed because the buffer was too small: reallocate it */
        dwBufSize = dwReqSize;
        pszBuffer = LocalAlloc(LMEM_FIXED, dwBufSize);
    }
    else
    {
        /* The call failed but the buffer was large enough: something else
         * failed and we won't retrieve the object name */
        pszBuffer = NULL;
    }
    if (!pszBuffer ||
        !GetUserObjectInformationW(hObj,
                                   UOI_NAME,
                                   pszBuffer,
                                   dwBufSize,
                                   &dwReqSize))
    {
        if (pszBuffer)
            LocalFree(pszBuffer);
        pszBuffer = NULL;
    }
    return pszBuffer;
}

static VOID
DumpNotificationState(
    _In_ PCSTR FileName,
    _In_ INT LineNum,
    _In_ PCSTR FuncName,
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    HWINSTA hWinSta;
    HDESK hThreadDesk, hInputDesk;
    PWSTR pszWinSta, pszNotifDesk, pszThreadDesk, pszInputDesk;
    WCHAR szBuffer0[MAX_PATH], szBuffer1[MAX_PATH], szBuffer2[MAX_PATH], szBuffer3[MAX_PATH];

    /* Retrieve the process' window station name */
    hWinSta = GetProcessWindowStation();
    pszWinSta = GetUserObjectName(hWinSta, szBuffer0, _countof(szBuffer0));

    /* Retrieve the notification desktop name */
    pszNotifDesk = GetUserObjectName(pInfo->hDesktop, szBuffer1, _countof(szBuffer1));

    /* Retrieve the thread desktop name */
    hThreadDesk = GetThreadDesktop(GetCurrentThreadId());
    pszThreadDesk = GetUserObjectName(hThreadDesk, szBuffer2, _countof(szBuffer2));

    /* Retrieve the input desktop name */
    hInputDesk = OpenInputDesktop(0, FALSE, STANDARD_RIGHTS_READ);
    pszInputDesk = GetUserObjectName(hInputDesk, szBuffer3, _countof(szBuffer3));
    CloseDesktop(hInputDesk);

    /*
     * Dump the information and the WLX_NOTIFICATION_INFO structure.
     */
    ros_dbg_log(__WINE_DBCL_ERR, __wine_dbch___default,
                FileName, FuncName, LineNum,
    /*TRACE(*/"\nWLNOTIFY(%lx.%lx):  Entering `%s`\n"
          "\tProcess WinSta : 0x%p '%S'\n"
          "\tNotif Desktop  : 0x%p '%S'\n"
          "\tThread Desktop : 0x%p '%S'\n"
          "\tInput Desktop  : 0x%p '%S'\n"
          "\tInfo.Size           : %lu\n"
          "\tInfo.Flags          : 0x%lx\n"
          "\tInfo.UserName       : '%S'\n"
          "\tInfo.Domain         : '%S'\n"
          "\tInfo.WindowStation  : '%S'\n"
          "\tInfo.hToken         : 0x%p\n"
          "\tInfo.hDesktop       : 0x%p\n"
          "\tInfo.pStatusCallback: 0x%p\n",
          GetCurrentProcessId(), // NtCurrentTeb()->ClientId.UniqueProcess
          GetCurrentThreadId(),  // NtCurrentTeb()->ClientId.UniqueThread
          FuncName,
          hWinSta, pszWinSta,
          pInfo->hDesktop, pszNotifDesk,
          hThreadDesk, pszThreadDesk,
          hInputDesk, pszInputDesk,
          pInfo->Size, pInfo->Flags, pInfo->UserName,
          pInfo->Domain, pInfo->WindowStation, pInfo->hToken,
          pInfo->hDesktop, pInfo->pStatusCallback);

    if (pszInputDesk != szBuffer3)
        LocalFree(pszInputDesk);
    if (pszThreadDesk != szBuffer2)
        LocalFree(pszThreadDesk);
    if (pszNotifDesk != szBuffer1)
        LocalFree(pszNotifDesk);
    if (pszWinSta != szBuffer0)
        LocalFree(pszWinSta);
}

#define DUMP_WLX_NOTIFICATION(pInfo) \
    DumpNotificationState(__RELFILE__, __LINE__, __FUNCTION__, pInfo)

static VOID
ChangeNotificationState(
    _In_ PCSTR FileName,
    _In_ INT LineNum,
    _In_ PCSTR FuncName,
    _In_ BOOL bChange,
    _In_ WLNOTIFY_STATE NewState)
{
    ros_dbg_log(__WINE_DBCL_ERR, __wine_dbch___default,
                FileName, FuncName, LineNum,
    /*TRACE(*/"**** %s: %s state %s %s %s\n",
        FuncName,
        bChange ? "Changing" : "Restoring",
        NotifyStateToName(g_CurrentState),
        bChange ? "to" : "back to",
        NotifyStateToName(NewState));

    g_PreviousState = InterlockedExchange((PLONG)&g_CurrentState, NewState);
}

#define CHANGE_STATE(bChange, NewState) \
    ChangeNotificationState(__RELFILE__, __LINE__, __FUNCTION__, (bChange), (NewState))

static DWORD
DisplayWlxMessageW(
    _In_opt_ PFNMSGECALLBACK pStatusCallback,
    _In_ BOOL bVerbose,
    _In_ PCWSTR pMessage)
{
    DWORD dwRet;

    if (!pStatusCallback)
        return (DWORD)(-1);

    dwRet = pStatusCallback(bVerbose, (LPWSTR)pMessage);

    /* Do a 1-second delay so that the message can be seen while testing */
    Sleep(1000);

    return dwRet;
}

static DWORD
DisplayWlxMessageA(
    _In_opt_ PFNMSGECALLBACK pStatusCallback,
    _In_ BOOL bVerbose,
    _In_ PCSTR pMessage)
{
    NTSTATUS Status;
    DWORD dwRet;
    ANSI_STRING MessageA;
    UNICODE_STRING MessageU;

    if (!pStatusCallback)
        return (DWORD)(-1);

    RtlInitAnsiString(&MessageA, pMessage);
    Status = RtlAnsiStringToUnicodeString(&MessageU, &MessageA, TRUE);
    if (!NT_SUCCESS(Status))
        return (DWORD)(-1);

    dwRet = DisplayWlxMessageW(pStatusCallback, bVerbose, MessageU.Buffer);

    RtlFreeUnicodeString(&MessageU);

    return dwRet;
}


/* NOTIFICATION CALLBACKS ****************************************************/

/**
 * @brief   Invoked on system startup.
 **/
VOID
WINAPI
WLEventStartup(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from the non-initialized state */
    if (g_CurrentState != WLNotify_NonInitialized)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_NonInitialized));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Startup);
}

/**
 * @brief   Invoked on system shutdown.
 **/
VOID
WINAPI
WLEventShutdown(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from either Startup or previous Logoff */
    if (g_CurrentState != WLNotify_Startup &&
        g_CurrentState != WLNotify_Logoff)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s or %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Startup),
            NotifyStateToName(WLNotify_Logoff));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Shutdown);
}


/**
 * @brief   Invoked on user logon.
 **/
VOID
WINAPI
WLEventLogon(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from either Startup or previous Logoff */
    if (g_CurrentState != WLNotify_Startup &&
        g_CurrentState != WLNotify_Logoff)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s or %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Startup),
            NotifyStateToName(WLNotify_Logoff));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Logon);
}

/**
 * @brief   Invoked on user logoff.
 **/
VOID
WINAPI
WLEventLogoff(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from PostShell, or StartShell if we failed to start
     * the shell (the PostShell notification isn't emitted in this case) */
    if (g_CurrentState != WLNotify_PostShell &&
        g_CurrentState != WLNotify_StartShell)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s or %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_PostShell),
            NotifyStateToName(WLNotify_StartShell));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Logoff);
}


/**
 * @brief   Invoked just before starting the user shell.
 **/
VOID
WINAPI
WLEventStartShell(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from Logon */
    if (g_CurrentState != WLNotify_Logon)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Logon));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_StartShell);
}

/**
 * @brief   Invoked just after starting the user shell.
 **/
VOID
WINAPI
WLEventPostShell(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from StartShell */
    if (g_CurrentState != WLNotify_StartShell)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_StartShell));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_PostShell);
}


/**
 * @brief   Invoked on workstation locking.
 **/
VOID
WINAPI
WLEventLock(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from PostShell */
    if (g_CurrentState != WLNotify_PostShell)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_PostShell));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Lock);
}

/**
 * @brief   Invoked on workstation unlocking.
 **/
VOID
WINAPI
WLEventUnlock(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from Lock */
    if (g_CurrentState != WLNotify_Lock)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Lock));
    }
    DbgBreakOnEvent();

    /* Restore previous state */
    CHANGE_STATE(FALSE, g_PreviousState);
}


/**
 * @brief   Invoked on screensaver start.
 **/
VOID
WINAPI
WLEventStartScreenSaver(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from Startup, Logoff, PostShell */
    if (g_CurrentState != WLNotify_Startup &&
        g_CurrentState != WLNotify_Logoff  &&
        g_CurrentState != WLNotify_PostShell)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s or %s or %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Startup),
            NotifyStateToName(WLNotify_Logoff),
            NotifyStateToName(WLNotify_PostShell));
    }
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_StartScreenSaver);
}

/**
 * @brief   Invoked on screensaver stop.
 **/
VOID
WINAPI
WLEventStopScreenSaver(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from StartScreenSaver */
    if (g_CurrentState != WLNotify_StartScreenSaver)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_StartScreenSaver));
    }
    DbgBreakOnEvent();

    /* Restore previous state */
    CHANGE_STATE(FALSE, g_PreviousState);
}


/**
 * @brief   Invoked on workstation disconnect (Terminal Services).
 **/
VOID
WINAPI
WLEventDisconnect(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    ERR("**** %s: Previous state %s\n",
        __FUNCTION__, NotifyStateToName(g_CurrentState));
    DbgBreakOnEvent();

    /* Change state */
    CHANGE_STATE(TRUE, WLNotify_Disconnect);
}

/**
 * @brief   Invoked on workstation reconnect (Terminal Services).
 **/
VOID
WINAPI
WLEventReconnect(
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    DUMP_WLX_NOTIFICATION(pInfo);

    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__);

    /* We must be called from Disconnect */
    if (g_CurrentState != WLNotify_Disconnect)
    {
        ERR("**** %s: ERROR: Wrong state %s, expected %s\n",
            __FUNCTION__,
            NotifyStateToName(g_CurrentState),
            NotifyStateToName(WLNotify_Disconnect));
    }
    DbgBreakOnEvent();

    /* Restore previous state */
    CHANGE_STATE(FALSE, g_PreviousState);
}


/* ENTRY-POINT; (UN-)INSTALLATION ROUTINES ***********************************/

BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hInstance,
    _In_ DWORD dwReason,
    _In_ PVOID pReserved)
{
    // MESSAGE // DPRINTF
    TRACE("\nWLNOTIFY(%lx.%lx):  Entering `%s`(hInst: 0x%p, dwReason: 0x%x, pReserved: 0x%p)\n",
         GetCurrentProcessId(), // NtCurrentTeb()->ClientId.UniqueProcess
         GetCurrentThreadId(),  // NtCurrentTeb()->ClientId.UniqueThread
         __FUNCTION__, hInstance, dwReason, pReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hModule = GetInstanceModule(hInstance);
            // DisableThreadLibraryCalls(g_hModule);
            __fallthrough;

        case DLL_PROCESS_DETACH:
        {
            // MESSAGE // DPRINTF
            ERR("\nWLNOTIFY(%lx.%lx):  Entering `%s`(hInst: 0x%p, dwReason: 0x%x, pReserved: 0x%p)\n",
                 GetCurrentProcessId(), // NtCurrentTeb()->ClientId.UniqueProcess
                 GetCurrentThreadId(),  // NtCurrentTeb()->ClientId.UniqueThread
                 __FUNCTION__, hInstance, dwReason, pReserved);

            /* Reset to detect any unwanted reloads */
            g_PreviousState = g_CurrentState = WLNotify_NonInitialized;
            break;
        }
    }

    return TRUE;
}

/**
 * @see
 * https://learn.microsoft.com/en-us/windows/win32/secauthn/registering-a-winlogon-notification-package
 * https://learn.microsoft.com/en-us/windows/win32/secauthn/registry-entries
 **/
static const struct
{
    PCWSTR ValueName;
    PCWSTR Value;
} NotifyEvents[] =
{
    {L"Startup" , L"WLEventStartup"},
    {L"Shutdown", L"WLEventShutdown"},
    {L"Logon" , L"WLEventLogon"},
    {L"Logoff", L"WLEventLogoff"},
    {L"StartShell", L"WLEventStartShell"},
    {L"PostShell" , L"WLEventPostShell"},
    {L"Lock"  , L"WLEventLock"},
    {L"Unlock", L"WLEventUnlock"},
    {L"StartScreenSaver", L"WLEventStartScreenSaver"},
    {L"StopScreenSaver" , L"WLEventStopScreenSaver"},
    {L"Disconnect", L"WLEventDisconnect"},
    {L"Reconnect" , L"WLEventReconnect"},
};

HRESULT WINAPI DllRegisterServer(VOID)
{
    HKEY hNotifyKey;
    LSTATUS lError;
    UINT i;
    DWORD dwValue;
    DWORD dwPathSize;
    WCHAR szModule[MAX_PATH];

    dwPathSize = GetModuleFileNameW(g_hModule, szModule, _countof(szModule));
    szModule[_countof(szModule) - 1] = UNICODE_NULL; // Ensure NULL-termination (see WinXP bug)

    if ( !((dwPathSize != 0) && (dwPathSize < _countof(szModule)) &&
           (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) )
    {
        /* Failed to retrieve the module path */
        return E_FAIL; // Cannot register.
    }

    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             NOTIFY_REG_PATH,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_SET_VALUE,
                             NULL,
                             &hNotifyKey,
                             NULL);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW() failed; error %lu\n", lError);
        return HRESULT_FROM_WIN32(lError); // Cannot register.
    }

    RegSetValueExW(hNotifyKey, L"DllName", 0, REG_EXPAND_SZ,
                   (PBYTE)szModule, (wcslen(szModule) + 1) * sizeof(WCHAR));

    /* Make the notifications synchronous by default */
    dwValue = 0;
    RegSetValueExW(hNotifyKey, L"Asynchronous", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    /* Don't impersonate the user when being invoked */
    dwValue = 0;
    RegSetValueExW(hNotifyKey, L"Impersonate", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    // dwValue = 1;
    // RegSetValueExW(hNotifyKey, L"SafeMode", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    // dwValue = 600;
    // RegSetValueExW(hNotifyKey, L"MaxWait", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    // TODO: Purpose TBD.
    // dwValue = 1;
    // RegSetValueExW(hNotifyKey, L"Enabled", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    // TODO: Purpose TBD.
    // dwValue = 1;
    // RegSetValueExW(hNotifyKey, L"Safe", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    for (i = 0; i < _countof(NotifyEvents); ++i)
    {
        RegSetValueExW(hNotifyKey, NotifyEvents[i].ValueName,
                       0, REG_SZ,
                       (PBYTE)NotifyEvents[i].Value,
                       (wcslen(NotifyEvents[i].Value) + 1) * sizeof(WCHAR));
    }

    RegCloseKey(hNotifyKey);

    return S_OK; // Registration successful.
}

HRESULT WINAPI DllUnregisterServer(VOID)
{
    LSTATUS lError;

    lError = RegDeleteKeyW(HKEY_LOCAL_MACHINE, NOTIFY_REG_PATH);
    if (lError == ERROR_FILE_NOT_FOUND)
        return S_FALSE; // Nothing to unregister, not a failure per se.
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegDeleteKeyW() failed; error %lu\n", lError);
        return HRESULT_FROM_WIN32(lError);
    }
    return S_OK; // Unregistration successful.
}

/* EOF */
