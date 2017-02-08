/*
 * PROJECT:         ReactOS Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbsdll/kbsdll.c
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 */

#include "../kbswitch.h"

HHOOK hKeyboardHook, hLangHook, hWinHook;
HINSTANCE hInstance;
HWND hKbSwitchWnd;

static VOID
SendMessageToMainWnd(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PostMessage(hKbSwitchWnd, Msg, wParam, lParam);
}

/* Not used yet */
LRESULT CALLBACK
KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}

LRESULT CALLBACK
LangHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    PMSG msg;
    msg = (PMSG) lParam;

    switch (msg->message)
    {
        case WM_INPUTLANGCHANGEREQUEST:
        {
            SendMessageToMainWnd(WM_LANG_CHANGED, wParam, msg->lParam);
        }
        break;

        case WM_HOTKEY:
        {
            if (msg->hwnd)
            {
                SendMessageToMainWnd(WM_LOAD_LAYOUT, (WPARAM)msg->hwnd, msg->lParam);
            }
        }
        break;
    }

    return CallNextHookEx(hLangHook, code, wParam, lParam);
}

LRESULT CALLBACK
WinHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    int id = GlobalAddAtom(_T("KBSWITCH"));

    switch (code)
    {
        case HCBT_SETFOCUS:
        {
            if ((HWND)wParam != NULL)
            {
                if ((HWND)wParam != hKbSwitchWnd)
                {
                    SendMessageToMainWnd(WM_WINDOW_ACTIVATE, wParam, lParam);
                }
            }
        }
        break;

        case HCBT_CREATEWND:
        {
            RegisterHotKey((HWND)wParam, id, MOD_ALT, VK_F10);
        }
        break;

        case HCBT_DESTROYWND:
        {
            UnregisterHotKey((HWND)wParam, id);
        }
        break;
    }

    GlobalDeleteAtom(id);

    return CallNextHookEx(hWinHook, code, wParam, lParam);
}

BOOL WINAPI
KbSwitchSetHooks(VOID)
{
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardHookProc, hInstance, 0);
    hLangHook = SetWindowsHookEx(WH_GETMESSAGE, LangHookProc, hInstance, 0);
    hWinHook = SetWindowsHookEx(WH_CBT, WinHookProc, hInstance, 0);

    if ((hKeyboardHook)&&(hLangHook)&&(hWinHook))
        return TRUE;
    else
        return FALSE;
}

VOID WINAPI
KbSwitchDeleteHooks(VOID)
{
    if (hKeyboardHook) UnhookWindowsHookEx(hKeyboardHook);
    if (hLangHook) UnhookWindowsHookEx(hLangHook);
    if (hWinHook) UnhookWindowsHookEx(hWinHook);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            hKbSwitchWnd = FindWindow(szKbSwitcherName, NULL);
            if (!hKbSwitchWnd) return FALSE;
            break;
    }

    return TRUE;
}
