/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Registry watcher
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "CRegWatcher.h"
#include <ime/indicml.h>

// The event handles to use in watching
HANDLE CRegWatcher::s_ahWatchEvents[WATCHENTRY_MAX] = { NULL };

// The registry entries to watch
WATCHENTRY CRegWatcher::s_WatchEntries[WATCHENTRY_MAX] =
{
    { HKEY_CURRENT_USER,  TEXT("Keyboard Layout\\Toggle")                           }, // WI_TOGGLE
    { HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\CTF\\TIP")                     }, // WI_MACHINE_TIF
    { HKEY_CURRENT_USER,  TEXT("Keyboard Layout\\Preload")                          }, // WI_PRELOAD
    { HKEY_CURRENT_USER,  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run") }, // WI_RUN
    { HKEY_CURRENT_USER,  TEXT("SOFTWARE\\Microsoft\\CTF\\TIP")                     }, // WI_USER_TIF
    { HKEY_CURRENT_USER,  TEXT("SOFTWARE\\Microsoft\\Speech")                       }, // WI_USER_SPEECH
    { HKEY_CURRENT_USER,  TEXT("Control Panel\\Appearance")                         }, // WI_APPEARANCE
    { HKEY_CURRENT_USER,  TEXT("Control Panel\\Colors")                             }, // WI_COLORS
    { HKEY_CURRENT_USER,  TEXT("Control Panel\\Desktop\\WindowMetrics")             }, // WI_WINDOW_METRICS
    { HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Speech")                       }, // WI_MACHINE_SPEECH
    { HKEY_CURRENT_USER,  TEXT("Keyboard Layout")                                   }, // WI_KEYBOARD_LAYOUT
    { HKEY_CURRENT_USER,  TEXT("SOFTWARE\\Microsoft\\CTF\\Assemblies")              }, // WI_ASSEMBLIES
};

// The timer IDs: For delaying ignitions
UINT CRegWatcher::s_nSysColorTimerId    = 0;
UINT CRegWatcher::s_nKbdToggleTimerId   = 0;
UINT CRegWatcher::s_nRegImxTimerId      = 0;

// %WINDIR%/IME/sptip.dll!TF_CreateLangProfileUtil
typedef HRESULT (WINAPI* FN_TF_CreateLangProfileUtil)(ITfFnLangProfileUtil**);

BOOL
CRegWatcher::Init()
{
    // NOTE: We don't support Win95/98/Me
#ifdef SUPPORT_WIN9X
    if (!(g_dwOsInfo & CIC_OSINFO_NT))
        s_WatchEntries[WI_RUN].hRootKey = HKEY_LOCAL_MACHINE;
#endif

    // Create some nameless events and initialize them
    for (SIZE_T iEvent = 0; iEvent < _countof(s_ahWatchEvents); ++iEvent)
    {
        s_ahWatchEvents[iEvent] = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        InitEvent(iEvent, FALSE);
    }

    // Internat.exe is an enemy of ctfmon.exe
    KillInternat();

    UpdateSpTip();

    return TRUE;
}

VOID
CRegWatcher::Uninit()
{
    for (SIZE_T iEvent = 0; iEvent < _countof(s_ahWatchEvents); ++iEvent)
    {
        // Close the key
        WATCHENTRY& entry = s_WatchEntries[iEvent];
        if (entry.hKey)
        {
            ::RegCloseKey(entry.hKey);
            entry.hKey = NULL;
        }

        // Close the event handle
        HANDLE& hEvent = s_ahWatchEvents[iEvent];
        if (hEvent)
        {
            ::CloseHandle(hEvent);
            hEvent = NULL;
        }
    }
}

// advapi32!RegNotifyChangeKeyValue
typedef LONG (WINAPI *FN_RegNotifyChangeKeyValue)(HKEY, BOOL, DWORD, HANDLE, BOOL);

LONG WINAPI
DelayedRegNotifyChangeKeyValue(
    HKEY hKey,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    HANDLE hEvent,
    BOOL fAsynchronous)
{
    static FN_RegNotifyChangeKeyValue s_fnRegNotifyChangeKeyValue = NULL;

    if (!s_fnRegNotifyChangeKeyValue)
    {
        HINSTANCE hAdvApi32 = cicGetSystemModuleHandle(TEXT("advapi32.dll"), FALSE);
        s_fnRegNotifyChangeKeyValue =
            (FN_RegNotifyChangeKeyValue)GetProcAddress(hAdvApi32, "RegNotifyChangeKeyValue");
        if (!s_fnRegNotifyChangeKeyValue)
            return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return s_fnRegNotifyChangeKeyValue(hKey, bWatchSubtree, dwNotifyFilter, hEvent, fAsynchronous);
}

BOOL
CRegWatcher::InitEvent(
    _In_ SIZE_T iEvent,
    _In_ BOOL bResetEvent)
{
    // Reset the signal status
    if (bResetEvent)
        ::ResetEvent(s_ahWatchEvents[iEvent]);

    // Close once to re-open
    WATCHENTRY& entry = s_WatchEntries[iEvent];
    if (entry.hKey)
    {
        ::RegCloseKey(entry.hKey);
        entry.hKey = NULL;
    }

    // Open or create a registry key to watch registry key
    LSTATUS error;
    error = ::RegOpenKeyEx(entry.hRootKey, entry.pszSubKey, 0, KEY_READ, &entry.hKey);
    if (error != ERROR_SUCCESS)
    {
        error = ::RegCreateKeyEx(entry.hRootKey, entry.pszSubKey, 0, NULL, 0,
                                 KEY_ALL_ACCESS, NULL, &entry.hKey, NULL);
        if (error != ERROR_SUCCESS)
            return FALSE;
    }

    // Start registry watching
    error = DelayedRegNotifyChangeKeyValue(entry.hKey,
                                           TRUE,
                                           REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                                           s_ahWatchEvents[iEvent],
                                           TRUE);
#ifndef NDEBUG
    if (error != ERROR_SUCCESS)
        OutputDebugStringA("RegNotifyChangeKeyValue failed\n");
#endif
    return error == ERROR_SUCCESS;
}

VOID
CRegWatcher::UpdateSpTip()
{
    // Post message 0x8002 to "SapiTipWorkerClass" windows
    ::EnumWindows(EnumWndProc, 0);

    // Clear "ProfileInitialized" value
    HKEY hKey;
    LSTATUS error = ::RegOpenKeyEx(HKEY_CURRENT_USER,
                                   TEXT("SOFTWARE\\Microsoft\\CTF\\Sapilayr"),
                                   0, KEY_WRITE, &hKey);
    if (error == ERROR_SUCCESS)
    {
        DWORD dwValue = 0, cbValue = sizeof(dwValue);
        ::RegSetValueEx(hKey, TEXT("ProfileInitialized"), NULL, REG_DWORD, (LPBYTE)&dwValue, cbValue);
        ::RegCloseKey(hKey);
    }

    // Get %WINDIR%/IME/sptip.dll!TF_CreateLangProfileUtil function
    HINSTANCE hSPTIP = cicLoadSystemLibrary(TEXT("IME\\sptip.dll"), TRUE);
    FN_TF_CreateLangProfileUtil fnTF_CreateLangProfileUtil =
        (FN_TF_CreateLangProfileUtil)::GetProcAddress(hSPTIP, "TF_CreateLangProfileUtil");
    if (fnTF_CreateLangProfileUtil)
    {
        // Call it
        ITfFnLangProfileUtil *pProfileUtil = NULL;
        HRESULT hr = fnTF_CreateLangProfileUtil(&pProfileUtil);
        if ((hr == S_OK) && pProfileUtil) // Success!
        {
            // Register profile
            hr = pProfileUtil->RegisterActiveProfiles();
            if (hr == S_OK)
                TF_InvalidAssemblyListCacheIfExist(); // Invalidate the assembly list cache

            pProfileUtil->Release();
        }
    }

    if (hSPTIP)
        ::FreeLibrary(hSPTIP);
}

VOID
CRegWatcher::KillInternat()
{
    HKEY hKey;
    WATCHENTRY& entry = s_WatchEntries[WI_RUN];

    // Delete internat.exe from registry "Run" key
    LSTATUS error = ::RegOpenKeyEx(entry.hRootKey, entry.pszSubKey, 0, KEY_ALL_ACCESS, &hKey);
    if (error == ERROR_SUCCESS)
    {
        ::RegDeleteValue(hKey, TEXT("internat.exe"));
        ::RegCloseKey(hKey);
    }

    // Kill the "Indicator" window (that internat.exe creates)
    HWND hwndInternat = ::FindWindow(INDICATOR_CLASS, NULL);
    if (hwndInternat)
        ::PostMessage(hwndInternat, WM_CLOSE, 0, 0);
}

// Post message 0x8002 to every "SapiTipWorkerClass" window.
// Called from CRegWatcher::UpdateSpTip
BOOL CALLBACK
CRegWatcher::EnumWndProc(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
    TCHAR ClassName[MAX_PATH];

    UNREFERENCED_PARAMETER(lParam);

    if (::GetClassName(hWnd, ClassName, _countof(ClassName)) &&
        _tcsicmp(ClassName, TEXT("SapiTipWorkerClass")) == 0)
    {
        PostMessage(hWnd, 0x8002, 0, 0); // FIXME: Magic number
    }

    return TRUE;
}

VOID CALLBACK
CRegWatcher::SysColorTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ DWORD dwTime)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(idEvent);
    UNREFERENCED_PARAMETER(dwTime);

    // Cancel the timer
    if (s_nSysColorTimerId)
    {
        ::KillTimer(NULL, s_nSysColorTimerId);
        s_nSysColorTimerId = 0;
    }

    TF_PostAllThreadMsg(15, 16);
}

VOID
CRegWatcher::StartSysColorChangeTimer()
{
    // Call SysColorTimerProc 0.5 seconds later (Delayed)
    if (s_nSysColorTimerId)
    {
        ::KillTimer(NULL, s_nSysColorTimerId);
        s_nSysColorTimerId = 0;
    }
    s_nSysColorTimerId = ::SetTimer(NULL, 0, 500, SysColorTimerProc);
}

VOID CALLBACK
CRegWatcher::RegImxTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ DWORD dwTime)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(idEvent);
    UNREFERENCED_PARAMETER(dwTime);

    // Cancel the timer
    if (s_nRegImxTimerId)
    {
        ::KillTimer(NULL, s_nRegImxTimerId);
        s_nRegImxTimerId = 0;
    }

    TF_InvalidAssemblyListCache();
    TF_PostAllThreadMsg(12, 16);
}

VOID CALLBACK
CRegWatcher::KbdToggleTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ DWORD dwTime)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(idEvent);
    UNREFERENCED_PARAMETER(dwTime);

    // Cancel the timer
    if (s_nKbdToggleTimerId)
    {
        ::KillTimer(NULL, s_nKbdToggleTimerId);
        s_nKbdToggleTimerId = 0;
    }

    TF_PostAllThreadMsg(11, 16);
}

VOID
CRegWatcher::OnEvent(
    _In_ SIZE_T iEvent)
{
    InitEvent(iEvent, TRUE);

    switch (iEvent)
    {
        case WI_TOGGLE:
        {
            // Call KbdToggleTimerProc 0.5 seconds later (Delayed)
            if (s_nKbdToggleTimerId)
            {
                ::KillTimer(NULL, s_nKbdToggleTimerId);
                s_nKbdToggleTimerId = 0;
            }
            s_nKbdToggleTimerId = ::SetTimer(NULL, 0, 500, KbdToggleTimerProc);
            break;
        }
        case WI_MACHINE_TIF:
        case WI_PRELOAD:
        case WI_USER_TIF:
        case WI_MACHINE_SPEECH:
        case WI_KEYBOARD_LAYOUT:
        case WI_ASSEMBLIES:
        {
            if (iEvent == WI_MACHINE_SPEECH)
                UpdateSpTip();

            // Call RegImxTimerProc 0.2 seconds later (Delayed)
            if (s_nRegImxTimerId)
            {
                ::KillTimer(NULL, s_nRegImxTimerId);
                s_nRegImxTimerId = 0;
            }
            s_nRegImxTimerId = ::SetTimer(NULL, 0, 200, RegImxTimerProc);
            break;
        }
        case WI_RUN: // The "Run" key is changed
        {
            KillInternat(); // Deny internat.exe the right to live
            break;
        }
        case WI_USER_SPEECH:
        case WI_APPEARANCE:
        case WI_COLORS:
        case WI_WINDOW_METRICS:
        {
            StartSysColorChangeTimer();
            break;
        }
        default:
        {
            break;
        }
    }
}
