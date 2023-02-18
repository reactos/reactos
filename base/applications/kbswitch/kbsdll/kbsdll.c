/*
 * PROJECT:         ReactOS Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbsdll/kbsdll.c
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 */

#include "../kbswitch.h"

HINSTANCE hInstance = NULL;

#ifdef _MSC_VER
    #define SHARED(name)
#else
    #define SHARED(name) __attribute__((section(name), shared))
#endif

#ifdef _MSC_VER
    #pragma data_seg(".shared")
#endif

/* The following handles are shared throughout the system: */
HHOOK hWinHook SHARED(".shared") = NULL;
HHOOK hShellHook SHARED(".shared") = NULL;
HWND hKbSwitchWnd SHARED(".shared") = NULL;

#ifdef _MSC_VER
    #pragma data_seg()
    #pragma comment(linker, "/section:.shared,rws")
#endif

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
                PostMessage(hKbSwitchWnd, WM_WINDOW_ACTIVATE, wParam, lParam);
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
            PostMessage(hKbSwitchWnd, WM_LANG_CHANGED, wParam, lParam);
            break;

        case HSHELL_WINDOWACTIVATED:
            PostMessageW(hKbSwitchWnd, WM_WINDOW_ACTIVATE, wParam, lParam);
            break;

        case HSHELL_WINDOWCREATED:
            PostMessageW(hKbSwitchWnd, WM_WINDOW_CREATE, wParam, lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            PostMessageW(hKbSwitchWnd, WM_WINDOW_DESTROY, wParam, lParam);
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
