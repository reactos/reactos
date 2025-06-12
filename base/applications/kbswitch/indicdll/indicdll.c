/*
 * PROJECT:     ReactOS Keyboard Layout Switcher
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Switching Keyboard Layouts
 * COPYRIGHT:   Copyright Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright Colin Finck (mail@colinfinck.de)
 *              Copyright 2022-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "../kbswitch.h"
#include "resource.h"

typedef struct tagSHARED_DATA
{
    HHOOK hWinHook;
    HHOOK hShellHook;
    HHOOK hKeyboardLLHook;
    HWND hKbSwitchWnd;
    UINT nHotID;
    DWORD_PTR dwHotMenuItemData;
    CRITICAL_SECTION csLock;
} SHARED_DATA, *PSHARED_DATA;

HINSTANCE g_hInstance = NULL;
HANDLE g_hShared = NULL;
PSHARED_DATA g_pShared = NULL;
BOOL g_bCriticalSectionInitialized = 0;

static VOID
PostMessageToMainWnd(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PostMessage(g_pShared->hKbSwitchWnd, Msg, wParam, lParam);
}

static LRESULT CALLBACK
WinHookProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_pShared->hWinHook, code, wParam, lParam);

    switch (code)
    {
        case HCBT_ACTIVATE:
        case HCBT_SETFOCUS:
        {
            HWND hwndFocus = (HWND)wParam;
            if (hwndFocus && hwndFocus != g_pShared->hKbSwitchWnd)
                PostMessageToMainWnd(WM_WINDOW_ACTIVATE, (WPARAM)hwndFocus, 0);
            break;
        }
    }

    return CallNextHookEx(g_pShared->hWinHook, code, wParam, lParam);
}

static LRESULT CALLBACK
ShellHookProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_pShared->hShellHook, code, wParam, lParam);

    switch (code)
    {
        case HSHELL_WINDOWACTIVATED:
        {
            PostMessageToMainWnd(WM_WINDOW_ACTIVATE, wParam, 0);
            break;
        }
        case HSHELL_LANGUAGE:
        {
            PostMessageToMainWnd(WM_LANG_CHANGED, wParam, lParam);
            break;
        }
    }

    return CallNextHookEx(g_pShared->hShellHook, code, wParam, lParam);
}

static inline BOOL
CheckVirtualKey(UINT vKey, UINT vKey1, UINT vKey2)
{
    return vKey == vKey1 || vKey == vKey2;
}

static LRESULT CALLBACK
KeyboardLLHook(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_pShared->hKeyboardLLHook, code, wParam, lParam);

    if (code == HC_ACTION)
    {
        PKBDLLHOOKSTRUCT pKbStruct = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            BOOL bShiftPressed = GetAsyncKeyState(VK_SHIFT) < 0;
            BOOL bAltPressed = GetAsyncKeyState(VK_MENU) < 0;
            BOOL bCtrlPressed = GetAsyncKeyState(VK_CONTROL) < 0;
            // Detect Alt+Shift and Ctrl+Shift
            UINT vkCode = pKbStruct->vkCode;
            if ((bAltPressed && CheckVirtualKey(vkCode, VK_LSHIFT, VK_RSHIFT)) ||
                (bShiftPressed && CheckVirtualKey(vkCode, VK_LMENU, VK_RMENU)) ||
                (bCtrlPressed && CheckVirtualKey(vkCode, VK_LSHIFT, VK_RSHIFT)) ||
                (bShiftPressed && CheckVirtualKey(vkCode, VK_LCONTROL, VK_RCONTROL)))
            {
                PostMessageToMainWnd(WM_LANG_CHANGED, 0, 0);
            }
        }
    }

    return CallNextHookEx(g_pShared->hKeyboardLLHook, code, wParam, lParam);
}

BOOL APIENTRY
KbSwitchSetHooks(_In_ BOOL bDoHook)
{
    EnterCriticalSection(&g_pShared->csLock);
    if (bDoHook)
    {
        g_pShared->hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, g_hInstance, 0);
        g_pShared->hShellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, g_hInstance, 0);
        g_pShared->hKeyboardLLHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardLLHook, g_hInstance, 0);

        if (g_pShared->hWinHook &&
            g_pShared->hShellHook &&
            g_pShared->hKeyboardLLHook)
        {
            LeaveCriticalSection(&g_pShared->csLock);
            return TRUE;
        }
    }

    /* Unhook */
    if (g_pShared->hKeyboardLLHook)
    {
        UnhookWindowsHookEx(g_pShared->hKeyboardLLHook);
        g_pShared->hKeyboardLLHook = NULL;
    }
    if (g_pShared->hShellHook)
    {
        UnhookWindowsHookEx(g_pShared->hShellHook);
        g_pShared->hShellHook = NULL;
    }
    if (g_pShared->hWinHook)
    {
        UnhookWindowsHookEx(g_pShared->hWinHook);
        g_pShared->hWinHook = NULL;
    }

    LeaveCriticalSection(&g_pShared->csLock);
    return !bDoHook;
}

// indicdll!12
VOID APIENTRY
GetPenMenuData(PUINT pnID, PDWORD_PTR pdwItemData)
{
    EnterCriticalSection(&g_pShared->csLock);
    *pnID = g_pShared->nHotID;
    *pdwItemData = g_pShared->dwHotMenuItemData;
    LeaveCriticalSection(&g_pShared->csLock);
}

// indicdll!14
VOID APIENTRY
SetPenMenuData(_In_ UINT nID, _In_ DWORD_PTR dwItemData)
{
    EnterCriticalSection(&g_pShared->csLock);
    g_pShared->nHotID = nID;
    g_pShared->dwHotMenuItemData = dwItemData;
    LeaveCriticalSection(&g_pShared->csLock);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hInstance = hinstDLL;
            g_hShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                          0, sizeof(SHARED_DATA), TEXT("InternatSHData"));
            if (!g_hShared)
                return FALSE;

            BOOL bAlreadyExists = GetLastError() == ERROR_ALREADY_EXISTS;

            g_pShared = (PSHARED_DATA)MapViewOfFile(g_hShared, FILE_MAP_WRITE, 0, 0, 0);
            if (!g_pShared)
                return FALSE;

            if (!bAlreadyExists)
            {
                ZeroMemory(g_pShared, sizeof(*g_pShared));
                g_pShared->hKbSwitchWnd = FindWindow(INDICATOR_CLASS, NULL);
                InitializeCriticalSection(&g_pShared->csLock);
                g_bCriticalSectionInitialized = TRUE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            if (g_bCriticalSectionInitialized)
            {
                DeleteCriticalSection(&g_pShared->csLock);
            }
            UnmapViewOfFile(g_pShared);
            CloseHandle(g_hShared);
            break;
        }
    }

    return TRUE;
}
