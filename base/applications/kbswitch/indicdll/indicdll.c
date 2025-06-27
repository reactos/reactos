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

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(internat);

typedef struct tagSHARED_DATA
{
    HHOOK hWinHook;
    HHOOK hShellHook;
    HHOOK hKeyboardHook;
    HWND hKbSwitchWnd;
    UINT nHotID;
    DWORD_PTR dwHotMenuItemData;
} SHARED_DATA, *PSHARED_DATA;

HINSTANCE g_hInstance = NULL;
HANDLE g_hShared = NULL;
PSHARED_DATA g_pShared = NULL;
HANDLE g_hMutex = NULL;

#define MUTEX_TIMEOUT_MS (4 * 1000)

static inline BOOL EnterProtectedSection(VOID)
{
    DWORD dwWaitResult = WaitForSingleObject(g_hMutex, MUTEX_TIMEOUT_MS);
    if (dwWaitResult == WAIT_OBJECT_0)
        return TRUE;

    if (dwWaitResult == WAIT_TIMEOUT)
        WARN("Timeout while waiting for mutex.\n");
    else
        WARN("Failed to acquire mutex. Error code: %lu\n", GetLastError());

    return FALSE;
}

static inline VOID LeaveProtectedSection(VOID)
{
    ReleaseMutex(g_hMutex);
}

static inline VOID
PostMessageToMainWnd(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (g_pShared->hKbSwitchWnd)
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
CheckVirtualKey(UINT vKey, UINT vKey0, UINT vKey1, UINT vKey2)
{
    return vKey == vKey0 || vKey == vKey1 || vKey == vKey2;
}

static LRESULT CALLBACK
KeyboardProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_pShared->hKeyboardHook, code, wParam, lParam);

    if (code == HC_ACTION)
    {
        UINT vKey = (UINT)wParam;
        LONG keyFlags = HIWORD(lParam);
        if (!(keyFlags & KF_UP) && !(keyFlags & KF_REPEAT))
        {
            BOOL bShiftPressed = (GetKeyState(VK_SHIFT) < 0);
            BOOL bAltPressed = (keyFlags & KF_ALTDOWN) || (GetKeyState(VK_MENU) < 0);
            BOOL bCtrlPressed = (GetKeyState(VK_CONTROL) < 0);
            // Detect Alt+Shift and Ctrl+Shift
            if ((bAltPressed && CheckVirtualKey(vKey, VK_SHIFT, VK_LSHIFT, VK_RSHIFT)) ||
                (bShiftPressed && CheckVirtualKey(vKey, VK_MENU, VK_LMENU, VK_RMENU)) ||
                (bCtrlPressed && CheckVirtualKey(vKey, VK_SHIFT, VK_LSHIFT, VK_RSHIFT)) ||
                (bShiftPressed && CheckVirtualKey(vKey, VK_CONTROL, VK_LCONTROL, VK_RCONTROL)))
            {
                PostMessageToMainWnd(WM_LANG_CHANGED, 0, 0);
            }
        }
    }

    return CallNextHookEx(g_pShared->hKeyboardHook, code, wParam, lParam);
}

BOOL APIENTRY
KbSwitchSetHooks(_In_ BOOL bDoHook)
{
    TRACE("bDoHook: %d\n", bDoHook);

    if (!EnterProtectedSection())
        return FALSE;

    if (bDoHook)
    {
        g_pShared->hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, g_hInstance, 0);
        g_pShared->hShellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, g_hInstance, 0);
        g_pShared->hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, g_hInstance, 0);

        if (g_pShared->hWinHook &&
            g_pShared->hShellHook &&
            g_pShared->hKeyboardHook)
        {
            // Find kbswitch window if necessary
            if (!g_pShared->hKbSwitchWnd || !IsWindow(g_pShared->hKbSwitchWnd))
            {
                g_pShared->hKbSwitchWnd = FindWindow(INDICATOR_CLASS, NULL);
                TRACE("hKbSwitchWnd: %p\n", g_pShared->hKbSwitchWnd);
            }

            LeaveProtectedSection();
            return TRUE;
        }
    }

    /* Unhook */
    if (g_pShared->hKeyboardHook)
    {
        UnhookWindowsHookEx(g_pShared->hKeyboardHook);
        g_pShared->hKeyboardHook = NULL;
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

    LeaveProtectedSection();
    return !bDoHook;
}

// indicdll!12
VOID APIENTRY
GetPenMenuData(PUINT pnID, PDWORD_PTR pdwItemData)
{
    if (EnterProtectedSection())
    {
        *pnID = g_pShared->nHotID;
        *pdwItemData = g_pShared->dwHotMenuItemData;
        LeaveProtectedSection();
    }
    else
    {
        WARN("EnterProtectedSection failed\n");
        *pnID = 0;
        *pdwItemData = 0;
    }
}

// indicdll!14
VOID APIENTRY
SetPenMenuData(_In_ UINT nID, _In_ DWORD_PTR dwItemData)
{
    if (EnterProtectedSection())
    {
        g_pShared->nHotID = nID;
        g_pShared->dwHotMenuItemData = dwItemData;
        LeaveProtectedSection();
    }
    else
    {
        WARN("EnterProtectedSection failed\n");
    }
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
            TRACE("DLL_PROCESS_ATTACH\n");
            g_hInstance = hinstDLL;

            g_hShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                          0, sizeof(SHARED_DATA), TEXT("InternatSHData"));
            if (!g_hShared)
            {
                ERR("!g_hShared\n");
                return FALSE;
            }

            BOOL bAlreadyExists = GetLastError() == ERROR_ALREADY_EXISTS;
            TRACE("bAlreadyExists: %d\n", bAlreadyExists);

            g_pShared = (PSHARED_DATA)MapViewOfFile(g_hShared, FILE_MAP_WRITE, 0, 0, 0);
            if (!g_pShared)
            {
                ERR("!g_pShared\n");
                return FALSE;
            }

            if (!bAlreadyExists)
                ZeroMemory(g_pShared, sizeof(*g_pShared));

            g_hMutex = CreateMutex(NULL, FALSE, TEXT("INDICDLL_PROTECTED"));
            if (!g_hMutex)
            {
                ERR("Failed to create mutex\n");
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            TRACE("DLL_PROCESS_DETACH\n");
            if (g_hMutex)
                CloseHandle(g_hMutex);
            UnmapViewOfFile(g_pShared);
            CloseHandle(g_hShared);
            break;
        }
    }

    return TRUE;
}
