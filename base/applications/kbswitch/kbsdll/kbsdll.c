/*
 * PROJECT:         ReactOS Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbsdll/kbsdll.c
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 */

#include "../kbswitch.h"

HHOOK hWinHook = NULL;
HHOOK hShellHook = NULL;
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
            {
                PostMessageToMainWnd(WM_WINDOW_ACTIVATE, wParam, lParam);
            }
        }
        break;
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
        case HSHELL_LANGUAGE:
        {
            PostMessageToMainWnd(WM_LANG_CHANGED, wParam, lParam);
        }
        break;
    }

    return CallNextHookEx(hShellHook, code, wParam, lParam);
}

BOOL WINAPI
KbSwitchSetHooks(VOID)
{
    hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, hInstance, 0);
    hShellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, hInstance, 0);

    if (!hWinHook || !hShellHook)
    {
        return FALSE;
    }

    return TRUE;
}

VOID WINAPI
KbSwitchDeleteHooks(VOID)
{
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
