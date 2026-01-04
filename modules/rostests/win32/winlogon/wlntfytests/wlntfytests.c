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

/* HEADERS *******************************************************************/

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

#define STANDALONE
#include "minitest.h" // Includes also <wine/debug.h>

#define WINE_DEFAULT_DEBUG_CHANNEL_EX(ch, flags) \
    static struct __wine_debug_channel __wine_dbch_##ch = { (unsigned char)(flags), #ch }; \
    static struct __wine_debug_channel * const __wine_dbch___default = &__wine_dbch_##ch

// WINE_DEFAULT_DEBUG_CHANNEL(wlnotify_tests);
WINE_DEFAULT_DEBUG_CHANNEL_EX(wlnotify_tests,
    (1 << __WINE_DBCL_TRACE) | (1 << __WINE_DBCL_WARN) |
    (1 << __WINE_DBCL_ERR)   | (1 << __WINE_DBCL_FIXME));


/* GLOBALS *******************************************************************/

#define NOTIFY_PKG_NAME     L"WLNotifyTests"
#define NOTIFY_REG_PATH     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\" NOTIFY_PKG_NAME

#define DEFAULT_WINSTA0     L"WinSta0"
#define DESKTOP_WINLOGON    L"Winlogon"
#define DESKTOP_DEFAULT     L"Default"
#define DESKTOP_SCRSAVE     L"Screen-saver"

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
static ULONG g_fLogoffShutdownFlags = EWX_LOGOFF;
static HMODULE g_hModule = NULL;
static BOOL g_bInitialized = FALSE; ///< TRUE when the flags have been initialized.
static struct {
    UINT8 bAsync : 1;
    UINT8 bImpersonate : 1;
    UINT8 Reserved : 6;
} g_fFlags = {0};

FORCEINLINE
BOOL
IsUserLoggedIn(
    _In_ WLNOTIFY_STATE State)
{
    /*
     * - The user is not logged in when State is either of:
     *   WLNotify_NonInitialized, WLNotify_Startup, WLNotify_Shutdown,
     *   WLNotify_Logoff, and perhaps (TODO TBD.): WLNotify_Disconnect,
     *   WLNotify_Reconnect.
     *
     * - The user is logged in when State is either of:
     *   WLNotify_Logon, WLNotify_StartShell, WLNotify_PostShell,
     *   WLNotify_Lock, WLNotify_Unlock.
     *
     * Note that WLNotify_StartScreenSaver and WLNotify_StopScreenSaver
     * are not reliable for this test, because they can be invoked from
     * either a logged-off or a logged-on state.
     */
    return ((State == WLNotify_Logon) ||
            (State >= WLNotify_StartShell && State <= WLNotify_Unlock));
}


/* DEBUGGING HELPERS *********************************************************/

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


/* SYSTEM INFORMATION HELPERS ************************************************/

/**
 * @brief
 * Retrieves the notification settings from the registry.
 **/
static VOID
GetSettings(VOID)
{
    HKEY hNotifyKey;
    LSTATUS lError;
    DWORD dwValue, dwSize, dwType;

    if (InterlockedCompareExchange((PLONG)&g_bInitialized, TRUE, FALSE))
        return;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           NOTIFY_REG_PATH,
                           0,
                           KEY_QUERY_VALUE,
                           &hNotifyKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed; error %lu\n", lError);
        return; // HRESULT_FROM_WIN32(lError);
    }

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hNotifyKey,
                              L"Asynchronous",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        g_fFlags.bAsync = !!dwValue;

    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hNotifyKey,
                              L"Impersonate",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        g_fFlags.bImpersonate = !!dwValue;

#if 0
    dwSize = sizeof(dwValue);
    lError = RegQueryValueExW(hNotifyKey,
                              L"MaxWait",
                              NULL,
                              &dwType,
                              (PBYTE)&dwValue,
                              &dwSize);
    if ((lError == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(dwValue)))
        g_dwMaxWait = !!dwValue;
#endif

    RegCloseKey(hNotifyKey);

    return; // S_OK;
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

/**
 * @brief
 * Retrieves the user and domain names corresponding to the given token.
 *
 * @param[in]   hToken
 * Handle to the user token, for which to retrieve the user and domain names.
 *
 * @param[out]  UserName
 * Receives in output, a pointer to an allocated string representing
 * the user name. The string is allocated with LocalAlloc(). After usage,
 * free the pointer with LocalFree().
 *
 * @param[out]  DomainName
 * Receives in output, a pointer to an allocated string representing
 * the domain name. The string is allocated with LocalAlloc(). After usage,
 * free the pointer with LocalFree().
 *
 * @return
 * TRUE if the information has been retrieved successfully; FALSE if not.
 *
 * @see
 * Copied and adapted from dll/win32/userenv/environment.c!GetUserAndDomainName
 * See also base/shell/progman/main.c!GetUserAndDomainName
 **/
static BOOL
GetUserAndDomainName(
    _In_ HANDLE hToken,
    _Out_ PWSTR* UserName,
    _Out_ PWSTR* DomainName)
{
    BOOL bRet = FALSE;
    DWORD cbTokenBuffer = 0;
    PTOKEN_USER pUserToken;

    PWSTR pUserName    = NULL;
    PWSTR pDomainName  = NULL;
    DWORD cbUserName   = 0;
    DWORD cbDomainName = 0;

    SID_NAME_USE SidNameUse;

    /* Retrieve token's information */
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;
    }

    pUserToken = LocalAlloc(LMEM_FIXED, cbTokenBuffer);
    if (!pUserToken)
        return FALSE;

    if (!GetTokenInformation(hToken, TokenUser, pUserToken, cbTokenBuffer, &cbTokenBuffer))
    {
        LocalFree(pUserToken);
        return FALSE;
    }

    /* Retrieve the domain and user name */
    if (!LookupAccountSidW(NULL,
                           pUserToken->User.Sid,
                           NULL,
                           &cbUserName,
                           NULL,
                           &cbDomainName,
                           &SidNameUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto done;
    }

    pUserName = LocalAlloc(LPTR, cbUserName * sizeof(WCHAR));
    if (!pUserName)
        goto done;

    pDomainName = LocalAlloc(LPTR, cbDomainName * sizeof(WCHAR));
    if (!pDomainName)
        goto done;

    if (!LookupAccountSidW(NULL,
                           pUserToken->User.Sid,
                           pUserName,
                           &cbUserName,
                           pDomainName,
                           &cbDomainName,
                           &SidNameUse))
    {
        goto done;
    }

    *UserName   = pUserName;
    *DomainName = pDomainName;
    bRet = TRUE;

done:
    if (bRet == FALSE)
    {
        if (pUserName)
            LocalFree(pUserName);
        if (pDomainName)
            LocalFree(pDomainName);
    }
    LocalFree(pUserToken);

    return bRet;
}


typedef struct _SYSTEM_USER_INFO
{
    HANDLE hProcToken, hThrdToken;
    PWSTR pProcUserName, pProcDomainName;
    PWSTR pThrdUserName, pThrdDomainName;
    PWSTR pNotifUserName, pNotifDomainName;
    HWINSTA hWinSta;
    HDESK hThreadDesk, hInputDesk;
    PWSTR pszWinSta, pszThreadDesk, pszInputDesk, pszNotifDesk;
    // BYTE Data[ANYSIZE_ARRAY];
} SYSTEM_USER_INFO, *PSYSTEM_USER_INFO;

static PSYSTEM_USER_INFO
GetSystemUserInfo(
    _In_ HANDLE hNotifToken,
    _In_ HDESK hNotifDesktop)
{
    SYSTEM_USER_INFO LocalInfo = {NULL}, *pSysUserInfo;
    PWSTR pString;
    SIZE_T BufSize;
    WCHAR szBuffer0[MAX_PATH], szBuffer1[MAX_PATH], szBuffer2[MAX_PATH], szBuffer3[MAX_PATH];

    /* Retrieve the process' token and the corresponding user/domain */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &LocalInfo.hProcToken))
    {
        GetUserAndDomainName(LocalInfo.hProcToken, &LocalInfo.pProcUserName, &LocalInfo.pProcDomainName);
        // CloseHandle(LocalInfo.hProcToken);
    }

    /* Retrieve the current thread's impersonation token, if any (otherwise
     * no impersonation occurs), and the corresponding user/domain */
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &LocalInfo.hThrdToken))
    {
        GetUserAndDomainName(LocalInfo.hThrdToken, &LocalInfo.pThrdUserName, &LocalInfo.pThrdDomainName);
        // CloseHandle(LocalInfo.hThrdToken);
    }

    /* Retrieve the notification's corresponding user/domain */
    GetUserAndDomainName(hNotifToken, &LocalInfo.pNotifUserName, &LocalInfo.pNotifDomainName);

    /* Retrieve the process' window station name */
    LocalInfo.hWinSta = GetProcessWindowStation();
    LocalInfo.pszWinSta = GetUserObjectName(LocalInfo.hWinSta, szBuffer0, _countof(szBuffer0));

    /* Retrieve the thread desktop name */
    LocalInfo.hThreadDesk = GetThreadDesktop(GetCurrentThreadId());
    LocalInfo.pszThreadDesk = GetUserObjectName(LocalInfo.hThreadDesk, szBuffer1, _countof(szBuffer1));

    /* Retrieve the input desktop name */
    LocalInfo.hInputDesk = OpenInputDesktop(0, FALSE, STANDARD_RIGHTS_READ);
    LocalInfo.pszInputDesk = GetUserObjectName(LocalInfo.hInputDesk, szBuffer2, _countof(szBuffer2));
    // CloseDesktop(LocalInfo.hInputDesk);

    /* Retrieve the notification desktop name */
    LocalInfo.pszNotifDesk = GetUserObjectName(hNotifDesktop, szBuffer3, _countof(szBuffer3));


    /* Allocate a single buffer to hold all the retrieved information */
    BufSize = sizeof(*pSysUserInfo) +
        ((LocalInfo.pProcUserName   ? wcslen(LocalInfo.pProcUserName)   + 1 : 0) +
         (LocalInfo.pProcDomainName ? wcslen(LocalInfo.pProcDomainName) + 1 : 0) +
         (LocalInfo.pThrdUserName   ? wcslen(LocalInfo.pThrdUserName)   + 1 : 0) +
         (LocalInfo.pThrdDomainName ? wcslen(LocalInfo.pThrdDomainName) + 1 : 0) +
         (LocalInfo.pNotifUserName   ? wcslen(LocalInfo.pNotifUserName)   + 1 : 0) +
         (LocalInfo.pNotifDomainName ? wcslen(LocalInfo.pNotifDomainName) + 1 : 0) +
         (LocalInfo.pszWinSta     ? wcslen(LocalInfo.pszWinSta)     + 1 : 0) +
         (LocalInfo.pszThreadDesk ? wcslen(LocalInfo.pszThreadDesk) + 1 : 0) +
         (LocalInfo.pszInputDesk  ? wcslen(LocalInfo.pszInputDesk)  + 1 : 0) +
         (LocalInfo.pszNotifDesk  ? wcslen(LocalInfo.pszNotifDesk)  + 1 : 0)) * sizeof(WCHAR);
    pSysUserInfo = LocalAlloc(LMEM_FIXED, BufSize);
    if (!pSysUserInfo)
        goto Done;

    /* Copy the fixed part of the structure */
    *pSysUserInfo = LocalInfo;

    /* Copy the strings and adjust the pointers */
    pString = (PWSTR)(pSysUserInfo + 1);
    if (LocalInfo.pProcUserName)
    {
        wcscpy(pString, LocalInfo.pProcUserName);
        pSysUserInfo->pProcUserName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pProcDomainName)
    {
        wcscpy(pString, LocalInfo.pProcDomainName);
        pSysUserInfo->pProcDomainName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pThrdUserName)
    {
        wcscpy(pString, LocalInfo.pThrdUserName);
        pSysUserInfo->pThrdUserName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pThrdDomainName)
    {
        wcscpy(pString, LocalInfo.pThrdDomainName);
        pSysUserInfo->pThrdDomainName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pNotifUserName)
    {
        wcscpy(pString, LocalInfo.pNotifUserName);
        pSysUserInfo->pNotifUserName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pNotifDomainName)
    {
        wcscpy(pString, LocalInfo.pNotifDomainName);
        pSysUserInfo->pNotifDomainName = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pszWinSta)
    {
        wcscpy(pString, LocalInfo.pszWinSta);
        pSysUserInfo->pszWinSta = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pszThreadDesk)
    {
        wcscpy(pString, LocalInfo.pszThreadDesk);
        pSysUserInfo->pszThreadDesk = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pszInputDesk)
    {
        wcscpy(pString, LocalInfo.pszInputDesk);
        pSysUserInfo->pszInputDesk = pString;
        pString += wcslen(pString) + 1;
    }
    if (LocalInfo.pszNotifDesk)
    {
        wcscpy(pString, LocalInfo.pszNotifDesk);
        pSysUserInfo->pszNotifDesk = pString;
    }

Done:
    if (LocalInfo.pszNotifDesk != szBuffer3)
        LocalFree(LocalInfo.pszNotifDesk);
    if (LocalInfo.pszInputDesk != szBuffer2)
        LocalFree(LocalInfo.pszInputDesk);
    if (LocalInfo.pszThreadDesk != szBuffer1)
        LocalFree(LocalInfo.pszThreadDesk);
    if (LocalInfo.pszWinSta != szBuffer0)
        LocalFree(LocalInfo.pszWinSta);

    if (LocalInfo.pNotifUserName)
        LocalFree(LocalInfo.pNotifUserName);
    if (LocalInfo.pNotifDomainName)
        LocalFree(LocalInfo.pNotifDomainName);
    if (LocalInfo.pThrdUserName)
        LocalFree(LocalInfo.pThrdUserName);
    if (LocalInfo.pThrdDomainName)
        LocalFree(LocalInfo.pThrdDomainName);
    if (LocalInfo.pProcUserName)
        LocalFree(LocalInfo.pProcUserName);
    if (LocalInfo.pProcDomainName)
        LocalFree(LocalInfo.pProcDomainName);

    /* If we failed to allocate the buffer, close the opened handles */
    if (!pSysUserInfo)
    {
        CloseDesktop(LocalInfo.hInputDesk);
        CloseHandle(LocalInfo.hThrdToken);
        CloseHandle(LocalInfo.hProcToken);
    }

    return pSysUserInfo;
}

static VOID
FreeSystemUserInfo(
    _Inout_ PSYSTEM_USER_INFO SysUserInfo)
{
    CloseHandle(SysUserInfo->hProcToken);
    CloseHandle(SysUserInfo->hThrdToken);
    CloseDesktop(SysUserInfo->hInputDesk);
    LocalFree(SysUserInfo);
}


static VOID
DumpNotificationState(
    _In_ PCSTR FileName,
    _In_ INT LineNum,
    _In_ PCSTR FuncName,
    _In_ PSYSTEM_USER_INFO SysUserInfo,
    _In_ PWLX_NOTIFICATION_INFO pInfo)
{
    /*
     * Dump the information and the WLX_NOTIFICATION_INFO structure.
     */
    // TRACE(
    ros_dbg_log(__WINE_DBCL_ERR, __wine_dbch___default,
                FileName, FuncName, LineNum,
        "\nWLNOTIFY(%lx.%lx) [Async: %s, Impers: %s]:  Entering `%s`\n"
        "\tProcess Token  : 0x%p - User: '%S\\%S'\n"
        "\tThread Token   : 0x%p - User: '%S\\%S'\n"
        "\tNotif Token    : 0x%p - User: '%S\\%S'\n"
        "\tProcess WinSta : 0x%p '%S'\n"
        "\tThread Desktop : 0x%p '%S'\n"
        "\tInput Desktop  : 0x%p '%S'\n"
        "\tNotif Desktop  : 0x%p '%S'\n",
        GetCurrentProcessId(), // NtCurrentTeb()->ClientId.UniqueProcess
        GetCurrentThreadId(),  // NtCurrentTeb()->ClientId.UniqueThread
        g_fFlags.bAsync ? "TRUE" : "FALSE",
        g_fFlags.bImpersonate ? "TRUE" : "FALSE",
        FuncName,
        SysUserInfo->hProcToken, SysUserInfo->pProcDomainName, SysUserInfo->pProcUserName,
        SysUserInfo->hThrdToken, SysUserInfo->pThrdDomainName, SysUserInfo->pThrdUserName,
        pInfo->hToken, SysUserInfo->pNotifDomainName, SysUserInfo->pNotifUserName,
        SysUserInfo->hWinSta, SysUserInfo->pszWinSta,
        SysUserInfo->hThreadDesk, SysUserInfo->pszThreadDesk,
        SysUserInfo->hInputDesk, SysUserInfo->pszInputDesk,
        pInfo->hDesktop, SysUserInfo->pszNotifDesk);
    // if (__WINE_IS_DEBUG_ON(_ERR, __wine_dbch___default))
    DPRINTF(
        "\tInfo.Size           : %lu\n"
        "\tInfo.Flags          : 0x%lx\n"
        "\tInfo.UserName       : '%S'\n"
        "\tInfo.Domain         : '%S'\n"
        "\tInfo.WindowStation  : '%S'\n"
        "\tInfo.hToken         : 0x%p\n"
        "\tInfo.hDesktop       : 0x%p\n"
        "\tInfo.pStatusCallback: 0x%p\n",
        pInfo->Size, pInfo->Flags, pInfo->UserName,
        pInfo->Domain, pInfo->WindowStation, pInfo->hToken,
        pInfo->hDesktop, pInfo->pStatusCallback);
}

#define DUMP_WLX_NOTIFICATION(SysUserInfo, pInfo) \
    DumpNotificationState(__RELFILE__, __LINE__, __FUNCTION__, (SysUserInfo), (pInfo))


static VOID
ChangeNotificationState(
    _In_ PCSTR FileName,
    _In_ INT LineNum,
    _In_ PCSTR FuncName,
    _In_ BOOL bChange,
    _In_ WLNOTIFY_STATE NewState)
{
    // TRACE(
    ros_dbg_log(__WINE_DBCL_ERR, __wine_dbch___default,
                FileName, FuncName, LineNum,
        "**** %s: %s state %s %s %s\n",
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


/* TESTING SUPPORT HELPERS ***************************************************/

#define BEGIN_TEST \
do { \
    static const struct test winetest_testlist[] = { { __FUNCTION__, NULL } }; \
    init_test( &winetest_testlist[0] ); \
    winetest_push_context("**** %s", __FUNCTION__); \
    {

#define END_TEST \
    } \
    winetest_pop_context(); \
    (void)fini_test(); \
} while(0);

#define ok_state_1(state1) \
    ok(g_CurrentState == (state1), \
       "ERROR: Wrong state %s, expected %s\n", \
       NotifyStateToName(g_CurrentState), \
       NotifyStateToName(state1))

#define ok_state_2(state1, state2) \
    ok(g_CurrentState == (state1) || g_CurrentState == (state2), \
       "ERROR: Wrong state %s, expected %s or %s\n", \
       NotifyStateToName(g_CurrentState), \
       NotifyStateToName(state1), \
       NotifyStateToName(state2))

#define ok_state_3(state1, state2, state3) \
    ok(g_CurrentState == (state1) || g_CurrentState == (state2) || g_CurrentState == (state3), \
       "ERROR: Wrong state %s, expected %s or %s or %s\n", \
       NotifyStateToName(g_CurrentState), \
       NotifyStateToName(state1), \
       NotifyStateToName(state2), \
       NotifyStateToName(state3))

typedef struct _TEST_ENTRY
{
    _Field_size_(NumStates) WLNOTIFY_STATE* AllowedStates;
    ULONG NumStates; ///< Number of elements in the AllowedStates array.
    // BOOL bProcToken;
    // PCWSTR pProcUserName, pProcDomainName;
    BOOL bThrdToken, bUserLoggedIn; ///< When bUserLoggedIn is TRUE we expect a valid notification user token.
    PCWSTR pszWinSta, pszThreadDesk;
    _Maybenull_ PCWSTR pszInputDesk;
    PCWSTR pszNotifDesk;
} TEST_ENTRY, *PTEST_ENTRY;

static void
DoTest(const char* file, int line, const char* funcname,
    _In_ PSYSTEM_USER_INFO SysUserInfo,
    _In_ PWLX_NOTIFICATION_INFO pInfo,
    _In_ PTEST_ENTRY pTest)
{
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(line);
    UNREFERENCED_PARAMETER(funcname); // Displayed via winetest_push_context()

    if (pTest->NumStates == 1)
        ok_state_1(pTest->AllowedStates[0]);
    else if (pTest->NumStates == 2)
        ok_state_2(pTest->AllowedStates[0], pTest->AllowedStates[1]);
    else if (pTest->NumStates == 3)
        ok_state_3(pTest->AllowedStates[0], pTest->AllowedStates[1], pTest->AllowedStates[2]);
    else
        skip("Unsupported number of allowed notify states: %lu\n", pTest->NumStates);

    ok(SysUserInfo->hProcToken != NULL, "Expected hProcToken != NULL\n");
    ok(SysUserInfo->pProcDomainName != NULL, "Expected pProcDomainName != NULL\n");
    ok(SysUserInfo->pProcUserName != NULL, "Expected pProcUserName != NULL\n");

    if (pTest->bThrdToken)
    {
        ok(SysUserInfo->hThrdToken != NULL, "Expected hThrdToken != NULL\n");

        ok(SysUserInfo->pThrdDomainName != NULL, "Expected pThrdDomainName != NULL\n");
        if (!SysUserInfo->pThrdDomainName || !SysUserInfo->pNotifDomainName)
            skip("Skipped pThrdDomainName/pNotifDomainName test\n");
        else
            ok_wstr(SysUserInfo->pThrdDomainName, SysUserInfo->pNotifDomainName);

        ok(SysUserInfo->pThrdUserName != NULL, "Expected pThrdUserName != NULL\n");
        if (!SysUserInfo->pThrdUserName || !SysUserInfo->pNotifUserName)
            skip("Skipped pThrdUserName/pNotifUserName test\n");
        else
            ok_wstr(SysUserInfo->pThrdUserName, SysUserInfo->pNotifUserName);
    }
    else
    {
        ok(SysUserInfo->hThrdToken == NULL, "Expected hThrdToken == NULL\n");
        ok(SysUserInfo->pThrdDomainName == NULL, "Expected pThrdDomainName == NULL\n");
        ok(SysUserInfo->pThrdUserName == NULL, "Expected pThrdUserName == NULL\n");
    }

    if (pTest->bUserLoggedIn)
    {
        /* See also pInfo->UserName and pInfo->Domain below */
        ok(pInfo->hToken != NULL, "Expected pInfo->hToken != NULL\n");
        ok(SysUserInfo->pNotifDomainName != NULL, "Expected pNotifDomainName != NULL\n");
        ok(SysUserInfo->pNotifUserName != NULL, "Expected pNotifUserName != NULL\n");
    }
    else
    {
        ok(pInfo->hToken == NULL, "Expected pInfo->hToken == NULL\n");
        ok(SysUserInfo->pNotifDomainName == NULL, "Expected pNotifDomainName == NULL\n");
        ok(SysUserInfo->pNotifUserName == NULL, "Expected pNotifUserName == NULL\n");
    }

    // if (pTest->pszWinSta) // Expected winsta -- DEFAULT_WINSTA0
    ok(SysUserInfo->hWinSta != NULL, "Expected hProcWinSta != NULL\n");
    ok(SysUserInfo->pszWinSta != NULL, "Expected pszWinSta != NULL\n");
    if (!SysUserInfo->pszWinSta)
        skip("Skipped pszWinSta test\n");
    else
        ok_wstr(SysUserInfo->pszWinSta, /*pTest->pszWinSta*/ DEFAULT_WINSTA0);

    // if (pTest->pszThreadDesk) // Expected thread desk -- DESKTOP_WINLOGON
    ok(SysUserInfo->hThreadDesk != NULL, "Expected hThreadDesk != NULL\n");
    ok(SysUserInfo->pszThreadDesk != NULL, "Expected pszThreadDesk != NULL\n");
    if (!SysUserInfo->pszThreadDesk)
        skip("Skipped pszThreadDesk test\n");
    else
        ok_wstr(SysUserInfo->pszThreadDesk, DESKTOP_WINLOGON);

    if (pTest->pszInputDesk)
    {
        ok(SysUserInfo->hInputDesk != NULL, "Expected hInputDesk != NULL\n");
        ok(SysUserInfo->pszInputDesk != NULL, "Expected pszInputDesk != NULL\n");
        if (!SysUserInfo->pszInputDesk)
            skip("Skipped pszInputDesk test\n");
        else
            ok_wstr(SysUserInfo->pszInputDesk, pTest->pszInputDesk);
    }
    else
    {
        ok(SysUserInfo->hInputDesk == NULL, "Expected hInputDesk == NULL\n");
        ok(SysUserInfo->pszInputDesk == NULL, "Expected pszInputDesk == NULL\n");
    }

    // if (pTest->pszNotifDesk)
    ok(pInfo->hDesktop != NULL, "Expected pInfo->hDesktop != NULL\n");
    ok(SysUserInfo->pszNotifDesk != NULL, "Expected pszNotifDesk != NULL\n");
    if (!SysUserInfo->pszNotifDesk)
        skip("Skipped pszNotifDesk test\n");
    else
        ok_wstr(SysUserInfo->pszNotifDesk, pTest->pszNotifDesk);

    ok_int(pInfo->Size, sizeof(*pInfo)); // == sizeof(WLX_NOTIFICATION_INFO);
    // ok_int(pInfo->Flags, 0); // This is tested separately: more than one flag is possible.

    if (pTest->bUserLoggedIn)
    {
        ok(pInfo->UserName != NULL, "Expected pInfo->UserName != NULL\n");
        ok(pInfo->Domain != NULL, "Expected pInfo->Domain != NULL\n");
        // ok(pInfo->hToken != NULL, "Expected pInfo->hToken != NULL\n");

        if (!pInfo->UserName || !SysUserInfo->pNotifUserName)
            skip("Skipped UserName/pNotifUserName test\n");
        else
            ok_wstr(pInfo->UserName, SysUserInfo->pNotifUserName);

        if (!pInfo->Domain || !SysUserInfo->pNotifDomainName)
            skip("Skipped Domain/pNotifDomainName test\n");
        else
            ok_wstr(pInfo->Domain, SysUserInfo->pNotifDomainName);
    }
    else
    {
        ok(pInfo->UserName == NULL, "Expected pInfo->UserName == NULL\n");
        ok(pInfo->Domain == NULL, "Expected pInfo->Domain == NULL\n");
        // ok(pInfo->hToken == NULL, "Expected pInfo->hToken == NULL\n");
    }

    ok(pInfo->WindowStation != NULL, "Expected pInfo->WindowStation != NULL\n");
    if (!pInfo->WindowStation || !SysUserInfo->pszWinSta)
        skip("Skipped WindowStation/pszWinSta test\n");
    else
        ok_wstr(pInfo->WindowStation, SysUserInfo->pszWinSta);

    if (wcscmp(pTest->pszNotifDesk, /*pTest->pszThreadDesk*/ DESKTOP_WINLOGON) == 0)
        ok(pInfo->hDesktop == SysUserInfo->hThreadDesk, "Expected pInfo->hDesktop == hThreadDesk\n");
    else
        ok(pInfo->hDesktop != SysUserInfo->hThreadDesk, "Expected pInfo->hDesktop != hThreadDesk\n");

    if (g_fFlags.bAsync)
        ok(pInfo->pStatusCallback == NULL, "Expected pStatusCallback == NULL, was: 0x%p\n", pInfo->pStatusCallback);
    else
        ok(pInfo->pStatusCallback != NULL, "Expected pStatusCallback != NULL\n");
}


/* NOTIFICATION HANDLERS *****************************************************/

#define __HANDLER_PARAM(pNotifInfo)(_In_ PWLX_NOTIFICATION_INFO pNotifInfo)
#define WLNOTIFY_HANDLER(name) \
    VOID WINAPI name __HANDLER_PARAM

#define BEGIN_HANDLER /*(SysUserInfo, pInfo)*/ \
    PSYSTEM_USER_INFO SysUserInfo; \
    SysUserInfo = GetSystemUserInfo(pInfo->hToken, pInfo->hDesktop); \
    DUMP_WLX_NOTIFICATION(SysUserInfo, pInfo); \
    DisplayWlxMessageA(pInfo->pStatusCallback, FALSE, __FUNCTION__); \
    DbgBreakOnEvent();

#define END_HANDLER(bChangeState, NewState) \
    FreeSystemUserInfo(SysUserInfo); \
    /* Change (bChangeState == TRUE) or Restore previous (FALSE) state */ \
    CHANGE_STATE(bChangeState, NewState);


/**
 * @brief   Invoked at system startup.
 **/
WLNOTIFY_HANDLER(WLEventStartup)(pInfo)
{
    /* Initially retrieve the notification registry settings */
    GetSettings();

    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from the non-initialized state */
        WLNOTIFY_STATE States[] = {WLNotify_NonInitialized};
        TEST_ENTRY StartupTest =
            { States, _countof(States), FALSE, FALSE, DEFAULT_WINSTA0,
              DESKTOP_WINLOGON, DESKTOP_WINLOGON, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &StartupTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Startup);
}

/**
 * @brief   Invoked at system shutdown.
 **/
WLNOTIFY_HANDLER(WLEventShutdown)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from either Startup or previous Logoff */
        WLNOTIFY_STATE States[] = {WLNotify_Startup, WLNotify_Logoff};
        TEST_ENTRY ShutdownTest =
            { States, _countof(States), FALSE, FALSE, DEFAULT_WINSTA0,
              DESKTOP_WINLOGON, DESKTOP_WINLOGON, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &ShutdownTest);

        /* Compare these flags with those from WLEventLogoff */
        ok_int(pInfo->Flags, g_fLogoffShutdownFlags);
        /*
         * - If the logged-on user chooses to shutdown or reboot the computer,
         *   WLEventLogoff() is first invoked with pInfo->Flags == EWX_SHUTDOWN
         *   or EWX_SHUTDOWN | EWX_REBOOT (stored in g_fLogoffShutdownFlags),
         *   then WLEventShutdown() is invoked with the same values.
         *
         * - If the logged-on user chooses to log off only, WLEventLogoff()
         *   is invoked with pInfo->Flags == 0 (== EWX_LOGOFF).
         *
         * - Independently of whether the user chose to shutdown or reboot
         *   the computer at the SAS dialog before logging in first (thus,
         *   g_CurrentState == WLNotify_Startup), or when coming back at the
         *   SAS dialog after logged off (g_CurrentState == WLNotify_Logoff),
         *   WLEventShutdown() is invoked with pInfo->Flags == 0.
         *   (In both these cases, g_fLogoffShutdownFlags == 0 too.)
         */
        ok(pInfo->Flags == EWX_LOGOFF   ||
           pInfo->Flags == EWX_SHUTDOWN ||
           pInfo->Flags == (EWX_SHUTDOWN | EWX_REBOOT),
           "Expected pInfo->Flags == EWX_LOGOFF (0) or EWX_SHUTDOWN (1) or EWX_SHUTDOWN|EWX_REBOOT (3), got %lu\n",
           pInfo->Flags);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Shutdown);
}


/**
 * @brief   Invoked at user logon.
 **/
WLNOTIFY_HANDLER(WLEventLogon)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from either Startup or previous Logoff */
        WLNOTIFY_STATE States[] = {WLNotify_Startup, WLNotify_Logoff};
        PWSTR pszInputDesk = (g_fFlags.bImpersonate ? NULL : DESKTOP_WINLOGON);
        TEST_ENTRY LogonTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_DEFAULT };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &LogonTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Logon);
}

/**
 * @brief   Invoked at user logoff.
 **/
WLNOTIFY_HANDLER(WLEventLogoff)(pInfo)
{
    BEGIN_HANDLER;

    /* Save the flags for comparison with those in WLEventShutdown */
    InterlockedExchange((PLONG)&g_fLogoffShutdownFlags, pInfo->Flags);

    BEGIN_TEST
    {
        /* We must be called from PostShell, or StartShell if we failed to start
         * the shell (the PostShell notification isn't emitted in this case) */
        WLNOTIFY_STATE States[] = {WLNotify_PostShell, WLNotify_StartShell};
        TEST_ENTRY LogoffTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, DESKTOP_DEFAULT, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &LogoffTest);

        ok(pInfo->Flags == EWX_LOGOFF   ||
           pInfo->Flags == EWX_SHUTDOWN ||
           pInfo->Flags == (EWX_SHUTDOWN | EWX_REBOOT),
           "Expected pInfo->Flags == EWX_LOGOFF (0) or EWX_SHUTDOWN (1) or EWX_SHUTDOWN|EWX_REBOOT (3), got %lu\n",
           pInfo->Flags);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Logoff);
}


/**
 * @brief   Invoked just before starting the user shell.
 **/
WLNOTIFY_HANDLER(WLEventStartShell)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from Logon */
        WLNOTIFY_STATE States[] = {WLNotify_Logon};
        PWSTR pszInputDesk = (g_fFlags.bImpersonate ? NULL : DESKTOP_WINLOGON);
        TEST_ENTRY StartShellTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_DEFAULT };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &StartShellTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_StartShell);
}

/**
 * @brief   Invoked just after starting the user shell.
 **/
WLNOTIFY_HANDLER(WLEventPostShell)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from StartShell */
        WLNOTIFY_STATE States[] = {WLNotify_StartShell};
        PWSTR pszInputDesk =
            (g_fFlags.bAsync ? DESKTOP_DEFAULT
                             : (g_fFlags.bImpersonate ? NULL : DESKTOP_WINLOGON));
        TEST_ENTRY PostShellTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &PostShellTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_PostShell);
}


/**
 * @brief   Invoked at workstation locking.
 **/
WLNOTIFY_HANDLER(WLEventLock)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from PostShell */
        WLNOTIFY_STATE States[] = {WLNotify_PostShell};
        PWSTR pszInputDesk = (g_fFlags.bImpersonate ? NULL : DESKTOP_WINLOGON);
        TEST_ENTRY LockTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &LockTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Lock);
}

/**
 * @brief   Invoked at workstation unlocking.
 **/
WLNOTIFY_HANDLER(WLEventUnlock)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from Lock */
        WLNOTIFY_STATE States[] = {WLNotify_Lock};
        PWSTR pszInputDesk =
            (g_fFlags.bAsync ? DESKTOP_DEFAULT
                             : (g_fFlags.bImpersonate ? NULL : DESKTOP_WINLOGON));
        TEST_ENTRY UnlockTest =
            { States, _countof(States), g_fFlags.bImpersonate, TRUE,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &UnlockTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Restore previous state */
    END_HANDLER(FALSE, g_PreviousState);
}


/**
 * @brief   Invoked at screensaver start.
 **/
WLNOTIFY_HANDLER(WLEventStartScreenSaver)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from Startup, Logoff, PostShell.
         * In case of Startup or Logoff, no user is logged in,
         * therefore no notification user token is present. */
        WLNOTIFY_STATE States[] = {WLNotify_Startup, WLNotify_Logoff, WLNotify_PostShell};
        BOOL bUserLoggedIn = IsUserLoggedIn(g_CurrentState);
        BOOL bImpersonate = (g_fFlags.bImpersonate && bUserLoggedIn);
        TEST_ENTRY StartScreenSaverTest =
            { States, _countof(States), bImpersonate, bUserLoggedIn,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, DESKTOP_SCRSAVE, DESKTOP_SCRSAVE };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &StartScreenSaverTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_StartScreenSaver);
}

/**
 * @brief   Invoked at screensaver stop.
 **/
WLNOTIFY_HANDLER(WLEventStopScreenSaver)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from StartScreenSaver.
         * In case StartScreenSaver was called originally from Startup or Logoff,
         * no user is logged in, therefore no notification user token is present. */
        WLNOTIFY_STATE States[] = {WLNotify_StartScreenSaver};
        BOOL bUserLoggedIn = IsUserLoggedIn(g_PreviousState);
        BOOL bImpersonate = (g_fFlags.bImpersonate && bUserLoggedIn);
        PWSTR pszInputDesk =
            (g_fFlags.bAsync ? (bImpersonate ? NULL : DESKTOP_WINLOGON)
                             : DESKTOP_SCRSAVE);
        TEST_ENTRY StopScreenSaverTest =
            { States, _countof(States), bImpersonate, bUserLoggedIn,
              DEFAULT_WINSTA0, DESKTOP_WINLOGON, pszInputDesk, DESKTOP_WINLOGON };

        DoTest(__RELFILE__, __LINE__, __FUNCTION__, SysUserInfo, pInfo, &StopScreenSaverTest);

        ok_int(pInfo->Flags, 0);
    }
    END_TEST

    /* Restore previous state */
    END_HANDLER(FALSE, g_PreviousState);
}


/**
 * @brief   Invoked at workstation disconnect (Terminal Services).
 **/
WLNOTIFY_HANDLER(WLEventDisconnect)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        ERR("**** %s: Previous state %s\n",
            __FUNCTION__, NotifyStateToName(g_CurrentState));
    }
    END_TEST

    /* Change state */
    END_HANDLER(TRUE, WLNotify_Disconnect);
}

/**
 * @brief   Invoked at workstation reconnect (Terminal Services).
 **/
WLNOTIFY_HANDLER(WLEventReconnect)(pInfo)
{
    BEGIN_HANDLER;

    BEGIN_TEST
    {
        /* We must be called from Disconnect */
        ok_state_1(WLNotify_Disconnect);
    }
    END_TEST

    /* Restore previous state */
    END_HANDLER(FALSE, g_PreviousState);
}


/* ENTRY-POINT; (UN-)INSTALLATION ROUTINES ***********************************/

BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hInstance,
    _In_ DWORD dwReason,
    _In_ PVOID pReserved)
{
    // DPRINTF
    TRACE("\nWLNOTIFY(%lx.%lx):  Entering `%s`(hInst: 0x%p, dwReason: 0x%x, pReserved: 0x%p)\n",
         GetCurrentProcessId(), // NtCurrentTeb()->ClientId.UniqueProcess
         GetCurrentThreadId(),  // NtCurrentTeb()->ClientId.UniqueThread
         __FUNCTION__, hInstance, dwReason, pReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hModule = GetInstanceModule(hInstance);
            DisableThreadLibraryCalls(g_hModule);
            __fallthrough;

        case DLL_PROCESS_DETACH:
        {
            // DPRINTF
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
                   (PBYTE)szModule, (DWORD)(wcslen(szModule) + 1) * sizeof(WCHAR));

    /* Make the notifications synchronous by default */
    dwValue = 0;
    RegSetValueExW(hNotifyKey, L"Asynchronous", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    /* Don't impersonate the user when being invoked */
    dwValue = 0;
    RegSetValueExW(hNotifyKey, L"Impersonate", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    /* Can be invoked also in SafeMode */
    dwValue = 1;
    RegSetValueExW(hNotifyKey, L"SafeMode", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    // dwValue = 600;
    // RegSetValueExW(hNotifyKey, L"MaxWait", 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));

    for (i = 0; i < _countof(NotifyEvents); ++i)
    {
        RegSetValueExW(hNotifyKey, NotifyEvents[i].ValueName,
                       0, REG_SZ,
                       (PBYTE)NotifyEvents[i].Value,
                       (DWORD)(wcslen(NotifyEvents[i].Value) + 1) * sizeof(WCHAR));
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
