/*
 * PROJECT:         ReactOS Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbsdll/kbsdll.c
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
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

LRESULT CALLBACK
WinHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(hWinHook, code, wParam, lParam);
    }

    switch (code)
    {
        case HCBT_SETFOCUS:
        {
            HWND hwndFocus = (HWND)wParam;
            if (hwndFocus && hwndFocus != hKbSwitchWnd)
                PostMessageToMainWnd(WM_WINDOW_ACTIVATE, WINDOW_ACTIVATE_FROM_FOCUS, 0);
            break;
        }
    }

    return CallNextHookEx(hWinHook, code, wParam, lParam);
}

LRESULT CALLBACK
ShellHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(hShellHook, code, wParam, lParam);
    }

    switch (code)
    {
        case HSHELL_WINDOWACTIVATED:
        {
            PostMessageToMainWnd(WM_WINDOW_ACTIVATE, WINDOW_ACTIVATE_FROM_SHELL, 0);
            break;
        }
        case HSHELL_LANGUAGE:
        {
            PostMessageToMainWnd(WM_LANG_CHANGED, LANG_CHANGED_FROM_SHELL, 0);
            break;
        }
    }

    return CallNextHookEx(hShellHook, code, wParam, lParam);
}

LRESULT CALLBACK
KeyboardLLHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(hKeyboardLLHook, code, wParam, lParam);

    if (code == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKbStruct = (KBDLLHOOKSTRUCT *)lParam;
        BOOL bAltPressed = GetAsyncKeyState(VK_MENU) < 0;
        BOOL bShiftPressed = GetAsyncKeyState(VK_SHIFT) < 0;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            if ((bAltPressed && bShiftPressed) ||
                (pKbStruct->vkCode == VK_SHIFT && bAltPressed) ||
                (pKbStruct->vkCode == VK_MENU && bShiftPressed))
            {
                PostMessageToMainWnd(WM_LANG_CHANGED, LANG_CHANGED_FROM_KBD_LL, 0);
            }
        }
    }

    return CallNextHookEx(hKeyboardLLHook, code, wParam, lParam);
}

BOOL WINAPI
KbSwitchSetHooks(VOID)
{
    hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, hInstance, 0);
    hShellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, hInstance, 0);
    hKeyboardLLHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardLLHook, hInstance, 0);

    if (!hWinHook || !hShellHook)
        return FALSE;

    return TRUE;
}

VOID WINAPI
KbSwitchDeleteHooks(VOID)
{
    if (hKeyboardLLHook)
    {
        UnhookWindowsHookEx(hKeyboardLLHook);
        hKeyboardLLHook = NULL;
    }
    if (hWinHook)
    {
        UnhookWindowsHookEx(hWinHook);
        hWinHook = NULL;
    }
    if (hShellHook)
    {
        UnhookWindowsHookEx(hShellHook);
        hShellHook = NULL;
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
            hInstance = hinstDLL;
            hKbSwitchWnd = FindWindow(szKbSwitcherName, NULL);
            if (!hKbSwitchWnd)
            {
                return FALSE;
            }
        }
        break;
    }

    return TRUE;
}
