/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Registry watcher
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "ctfmon.h"
#include "CRegWatcher.h"

// The event handles to use in watching
HANDLE CRegWatcher::s_ahWatchEvents[WATCHENTRY_MAX] = { NULL };

// The registry entries to watch
WATCHENTRY CRegWatcher::s_WatchEntries[WATCHENTRY_MAX] =
{
    { HKEY_CURRENT_USER,  L"Keyboard Layout\\Toggle"                           }, // EI_TOGGLE
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\CTF\\TIP"                     }, // EI_MACHINE_TIF
    { HKEY_CURRENT_USER,  L"Keyboard Layout\\Preload"                          }, // EI_PRELOAD
    { HKEY_CURRENT_USER,  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run" }, // EI_RUN
    { HKEY_CURRENT_USER,  L"SOFTWARE\\Microsoft\\CTF\\TIP"                     }, // EI_USER_TIF
    { HKEY_CURRENT_USER,  L"SOFTWARE\\Microsoft\\Speech"                       }, // EI_USER_SPEECH
    { HKEY_CURRENT_USER,  L"Control Panel\\Appearance"                         }, // EI_APPEARANCE
    { HKEY_CURRENT_USER,  L"Control Panel\\Colors"                             }, // EI_COLORS
    { HKEY_CURRENT_USER,  L"Control Panel\\Desktop\\WindowMetrics"             }, // EI_WINDOW_METRICS
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Speech"                       }, // EI_MACHINE_SPEECH
    { HKEY_CURRENT_USER,  L"Keyboard Layout"                                   }, // EI_KEYBOARD_LAYOUT
    { HKEY_CURRENT_USER,  L"SOFTWARE\\Microsoft\\CTF\\Assemblies"              }, // EI_ASSEMBLIES
};

// The timer IDs: For delaying ignitions
UINT CRegWatcher::s_nSysColorTimerId    = 0;
UINT CRegWatcher::s_nKbdToggleTimerId   = 0;
UINT CRegWatcher::s_nRegImxTimerId      = 0;

// advapi32!RegNotifyChangeKeyValue
typedef LONG (WINAPI *FN_RegNotifyChangeKeyValue)(HKEY hKey,
                                                  BOOL bWatchSubtree,
                                                  DWORD dwNotifyFilter,
                                                  HANDLE hEvent,
                                                  BOOL fAsynchronous);
FN_RegNotifyChangeKeyValue g_fnRegNotifyChangeKeyValue = NULL;

// %WINDIR%/IME/sptip.dll!TF_CreateLangProfileUtil
typedef HRESULT (WINAPI* FN_TF_CreateLangProfileUtil)(ITfFnLangProfileUtil**);

BOOL
CRegWatcher::Init()
{
    if (!(g_dwOsInfo & OSINFO_NT)) // Non-NT?
        s_WatchEntries[EI_RUN].hRootKey = HKEY_LOCAL_MACHINE;

    // To watch registry, advapi32!RegNotifyChangeKeyValue is required
    HINSTANCE hAdvApi32 = LoadSystemLibrary(L"advapi32.dll", FALSE);
    g_fnRegNotifyChangeKeyValue =
        (FN_RegNotifyChangeKeyValue)::GetProcAddress(hAdvApi32, "RegNotifyChangeKeyValue");
    if (!g_fnRegNotifyChangeKeyValue)
        return FALSE;

    // Create some nameless events and initialize them
    for (SIZE_T iEvent = 0; iEvent < _countof(s_ahWatchEvents); ++iEvent)
    {
        s_ahWatchEvents[iEvent] = ::CreateEventW(NULL, TRUE, FALSE, NULL);
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
    error = ::RegOpenKeyExW(entry.hRootKey, entry.pszSubKey, 0, KEY_READ, &entry.hKey);
    if (error != ERROR_SUCCESS)
    {
        error = ::RegCreateKeyExW(entry.hRootKey, entry.pszSubKey, 0, NULL, 0,
                                  KEY_ALL_ACCESS, NULL, &entry.hKey, NULL);
        if (error != ERROR_SUCCESS)
            return FALSE;
    }

    // Start registry watching
    error = g_fnRegNotifyChangeKeyValue(entry.hKey,
                                        TRUE,
                                        REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                                        s_ahWatchEvents[iEvent],
                                        TRUE);
    return error == ERROR_SUCCESS;
}

VOID
CRegWatcher::UpdateSpTip()
{
    // Post message 0x8002 to "SapiTipWorkerClass" windows
    ::EnumWindows(EnumWndProc, 0);

    // Clear "ProfileInitialized" value
    HKEY hKey;
    LSTATUS error = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\CTF\\Sapilayr",
                                    0, KEY_WRITE, &hKey);
    if (error == ERROR_SUCCESS)
    {
        DWORD dwValue = 0, cbValue = sizeof(dwValue);
        ::RegSetValueExW(hKey, L"ProfileInitialized", NULL, REG_DWORD, (LPBYTE)&dwValue, cbValue);
        ::RegCloseKey(hKey);
    }

    // Get %WINDIR%/IME/sptip.dll!TF_CreateLangProfileUtil function
    HINSTANCE hSPTIP = LoadSystemLibrary(L"IME\\sptip.dll", TRUE);
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
    WATCHENTRY& entry = s_WatchEntries[EI_RUN];

    // Delete internat.exe from registry "Run" key
    LSTATUS error = ::RegOpenKeyExW(entry.hRootKey, entry.pszSubKey, 0, KEY_ALL_ACCESS, &hKey);
    if (error == ERROR_SUCCESS)
    {
        ::RegDeleteValueW(hKey, L"internat.exe");
        ::RegCloseKey(hKey);
    }

    // Kill the "Indicator" window (that internat.exe creates)
    HWND hwndInternat = ::FindWindowW(L"Indicator", NULL);
    if (hwndInternat)
        ::PostMessageW(hwndInternat, WM_CLOSE, 0, 0);
}

// Post message 0x8002 to every "SapiTipWorkerClass" window.
// Called from CRegWatcher::UpdateSpTip
BOOL CALLBACK
CRegWatcher::EnumWndProc(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
    WCHAR ClassName[MAX_PATH];

    UNREFERENCED_PARAMETER(lParam);

    if (::GetClassNameW(hWnd, ClassName, _countof(ClassName)) &&
        _wcsicmp(ClassName, L"SapiTipWorkerClass") == 0)
    {
        PostMessageW(hWnd, 0x8002, 0, 0); // FIXME: Magic number
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
        case EI_TOGGLE:
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
        case EI_MACHINE_TIF:
        case EI_PRELOAD:
        case EI_USER_TIF:
        case EI_MACHINE_SPEECH:
        case EI_KEYBOARD_LAYOUT:
        case EI_ASSEMBLIES:
        {
            if (iEvent == EI_MACHINE_SPEECH)
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
        case EI_RUN: // The "Run" key is changed
        {
            KillInternat(); // Deny internat.exe the right to live
            break;
        }
        case EI_USER_SPEECH:
        case EI_APPEARANCE:
        case EI_COLORS:
        case EI_WINDOW_METRICS:
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
