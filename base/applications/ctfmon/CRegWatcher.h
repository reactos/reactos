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
    static BOOL InitEvent(SIZE_T iEvent, BOOL bReset);
    static VOID UpdateSpTip();
    static VOID KillInternat();
    static VOID StartSysColorChangeTimer();
    static VOID OnEvent(INT iEvent);

protected:
    static BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam);

    static VOID CALLBACK KbdToggleTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static VOID CALLBACK SysColorTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static VOID CALLBACK RegImxTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};
