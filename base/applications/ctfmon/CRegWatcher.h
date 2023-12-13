/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Registry watcher
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

struct WATCHENTRY
{
    HKEY hRootKey;
    LPCWSTR pszSubKey;
    HKEY hKey;
};

#define WATCHENTRY_MAX 12

struct CRegWatcher
{
    static HANDLE s_ahWatchEvents[WATCHENTRY_MAX];
    static WATCHENTRY s_WatchEntries[WATCHENTRY_MAX];
    static UINT s_nSysColorTimerId, s_nKbdToggleTimerId, s_nRegImxTimerId;

    static BOOL Init();
    static VOID Uninit();
    static BOOL InitEvent(_In_ SIZE_T iEvent, _In_ BOOL bResetEvent);
    static VOID UpdateSpTip();
    static VOID KillInternat();
    static VOID StartSysColorChangeTimer();
    static VOID OnEvent(_In_ SIZE_T iEvent);

protected:
    static BOOL CALLBACK
    EnumWndProc(_In_ HWND hWnd, _In_ LPARAM lParam);

    static VOID CALLBACK
    SysColorTimerProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime);

    static VOID CALLBACK
    KbdToggleTimerProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime);

    static VOID CALLBACK
    RegImxTimerProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime);
};
