/*
 * PROJECT:     ReactOS Keyboard Layout Switcher
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Switching Keyboard Layouts
 * COPYRIGHT:   Copyright Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright Colin Finck (mail@colinfinck.de)
 *              Copyright 2022-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "../kbswitch.h"

HHOOK hWinHook = NULL;
HHOOK hShellHook = NULL;
HHOOK hKeyboardLLHook = NULL;
HINSTANCE hInstance = NULL;
HWND hKbSwitchWnd = NULL;

static VOID
PostMessageToMainWnd(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PostMessage(hKbSwitchWnd, Msg, wParam, lParam);
}

static LRESULT CALLBACK
WinHookProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(hWinHook, code, wParam, lParam);

    switch (code)
    {
        case HCBT_ACTIVATE:
        case HCBT_SETFOCUS:
        {
            HWND hwndFocus = (HWND)wParam;
            if (hwndFocus && hwndFocus != hKbSwitchWnd)
                PostMessageToMainWnd(WM_WINDOW_ACTIVATE, (WPARAM)hwndFocus, 0);
            break;
        }
    }

    return CallNextHookEx(hWinHook, code, wParam, lParam);
}

static LRESULT CALLBACK
ShellHookProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(hShellHook, code, wParam, lParam);

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

    return CallNextHookEx(hShellHook, code, wParam, lParam);
}

static LRESULT CALLBACK
KeyboardLLHook(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(hKeyboardLLHook, code, wParam, lParam);

    if (code == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKbStruct = (KBDLLHOOKSTRUCT *)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            BOOL bShiftPressed = GetAsyncKeyState(VK_SHIFT) < 0;
            BOOL bAltPressed = GetAsyncKeyState(VK_MENU) < 0;
            BOOL bCtrlPressed = GetAsyncKeyState(VK_CONTROL) < 0;
            // Detect Alt+Shift and Ctrl+Shift
            if ((pKbStruct->vkCode == VK_SHIFT && bAltPressed) ||
                (pKbStruct->vkCode == VK_MENU && bShiftPressed) ||
                (pKbStruct->vkCode == VK_SHIFT && bCtrlPressed) ||
                (pKbStruct->vkCode == VK_CONTROL && bShiftPressed))
            {
                PostMessageToMainWnd(WM_LANG_CHANGED, 0, 0);
            }
        }
    }

    return CallNextHookEx(hKeyboardLLHook, code, wParam, lParam);
}

BOOL APIENTRY
KbSwitchSetHooks(_In_ BOOL bDoHook)
{
    if (bDoHook)
    {
        hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, hInstance, 0);
        hShellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, hInstance, 0);
        hKeyboardLLHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardLLHook, hInstance, 0);

        if (hWinHook && hShellHook && hKeyboardLLHook)
            return TRUE;
    }

    /* Unhook */
    if (hKeyboardLLHook)
    {
        UnhookWindowsHookEx(hKeyboardLLHook);
        hKeyboardLLHook = NULL;
    }
    if (hShellHook)
    {
        UnhookWindowsHookEx(hShellHook);
        hShellHook = NULL;
    }
    if (hWinHook)
    {
        UnhookWindowsHookEx(hWinHook);
        hWinHook = NULL;
    }
    return !bDoHook;
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
            hInstance = hinstDLL;
            hKbSwitchWnd = FindWindow(szKbSwitcherName, NULL);
            if (!hKbSwitchWnd)
                return FALSE;
        }
        break;
    }

    return TRUE;
}
