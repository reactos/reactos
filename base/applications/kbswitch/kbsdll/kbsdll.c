/*
 * PROJECT:         ReactOS Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbsdll/kbsdll.c
 * PROGRAMMERS:     Dmitry Chapyshev <dmitry@reactos.org>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "../kbswitch.h"

HINSTANCE g_hinstDLL = NULL;
HWND g_hwnd = NULL;

#ifdef _MSC_VER
    #define SHARED(name)
#else
    #define SHARED(name) __attribute__((section(name), shared))
#endif

#ifdef _MSC_VER
    #pragma data_seg(".shared")
#endif
HHOOK g_hCbtHook SHARED(".shared") = NULL;
HHOOK g_hShellHook SHARED(".shared") = NULL;
#ifdef _MSC_VER
    #pragma data_seg()
    #pragma comment(linker, "/section:.shared,rws")
#endif

static LRESULT CALLBACK
CbtProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_hCbtHook, code, wParam, lParam);

    switch (code)
    {
        case HCBT_SETFOCUS:
            PostMessageW(g_hwnd, WM_WINDOWSETFOCUS, wParam, lParam);
            break;
    }

    return CallNextHookEx(g_hCbtHook, code, wParam, lParam);
}

static LRESULT CALLBACK
ShellProc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0)
        return CallNextHookEx(g_hShellHook, code, wParam, lParam);

    switch (code)
    {
        case HSHELL_LANGUAGE:
            PostMessageW(g_hwnd, WM_LANGUAGE, wParam, lParam);
            break;

        case HSHELL_WINDOWACTIVATED:
            PostMessageW(g_hwnd, WM_WINDOWACTIVATED, wParam, lParam);
            break;

        case HSHELL_WINDOWCREATED:
            PostMessageW(g_hwnd, WM_WINDOWCREATED, wParam, lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            PostMessageW(g_hwnd, WM_WINDOWDESTROYED, wParam, lParam);
            break;
    }

    return CallNextHookEx(g_hShellHook, code, wParam, lParam);
}

BOOL APIENTRY KbsStartHook(VOID)
{
    g_hCbtHook = SetWindowsHookEx(WH_CBT, CbtProc, g_hinstDLL, 0);
    g_hShellHook = SetWindowsHookEx(WH_SHELL, ShellProc, g_hinstDLL, 0);
    if (!g_hCbtHook || !g_hShellHook)
    {
        UnhookWindowsHookEx(g_hCbtHook);
        UnhookWindowsHookEx(g_hShellHook);
        g_hCbtHook = g_hShellHook = NULL;
        return FALSE;
    }

    return TRUE;
}

VOID APIENTRY KbsEndHook(VOID)
{
    UnhookWindowsHookEx(g_hCbtHook);
    UnhookWindowsHookEx(g_hShellHook);
    g_hCbtHook = g_hShellHook = NULL;
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hwnd = FindWindowW(KBSWITCH_CLASS, NULL);
            if (g_hwnd == NULL)
                return FALSE;

            g_hinstDLL = hinstDLL;
            break;
    }

    return TRUE;
}
