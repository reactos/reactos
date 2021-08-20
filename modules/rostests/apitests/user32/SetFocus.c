/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for SetFocus/GetFocus/GetGUIThreadInfo
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <dlgs.h> // psh1, ...
#include <process.h>

#define INTERVAL 80

static DWORD s_dwMainThreadID;

static HWND GetMainThreadFocus(void)
{
    GUITHREADINFO gui = { sizeof(gui) };
    GetGUIThreadInfo(s_dwMainThreadID, &gui);
    return gui.hwndFocus;
}

static unsigned __stdcall thread_proc_0(void *arg)
{
    HWND hwnd = arg;

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDOK), TRUE);
    SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
    SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, psh1), TRUE);
    SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDOK), TRUE);
    SendMessageA(hwnd, WM_NEXTDLGCTL, TRUE, FALSE);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, psh1), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDOK), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, psh1), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDOK), TRUE);

    PostMessageA(hwnd, WM_COMMAND, psh3, 0);
    return 0;
}

static unsigned __stdcall thread_proc_1(void *arg)
{
    HWND hwnd = arg;

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == NULL, TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == NULL, TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == NULL, TRUE);

    PostMessageA(hwnd, WM_COMMAND, psh3, 0);
    return 0;
}

static unsigned __stdcall thread_proc_2(void *arg)
{
    HWND hwnd = arg;

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, psh1), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    ok_int(GetFocus() == NULL, TRUE);
    ok_int(GetMainThreadFocus() == GetDlgItem(hwnd, IDOK), TRUE);
    keybd_event(VK_TAB, KEYEVENTF_KEYUP, 0, 0);
    Sleep(INTERVAL);

    PostMessageA(hwnd, WM_COMMAND, psh4, 0);
    return 0;
}

static INT_PTR CALLBACK
DialogProc_0(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hThread;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            ok_int(GetFocus() == NULL, TRUE);
            SetFocus(GetDlgItem(hwnd, IDOK));
            ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
            SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

            ok_int(GetFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
            SendMessageA(hwnd, WM_NEXTDLGCTL, TRUE, FALSE);

            ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
            SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

            ok_int(GetFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
            SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

            ok_int(GetFocus() == GetDlgItem(hwnd, psh1), TRUE);
            SendMessageA(hwnd, WM_NEXTDLGCTL, FALSE, FALSE);

            ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
            SetFocus(GetDlgItem(hwnd, IDCANCEL));
            PostMessageA(hwnd, WM_COMMAND, psh2, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case psh2:
                    ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
                    hThread = (HANDLE)_beginthreadex(NULL, 0, thread_proc_0, hwnd, 0, NULL);
                    CloseHandle(hThread);
                    break;

                case psh3:
                    ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
                    EndDialog(hwnd, IDCLOSE);
                    break;
            }
            break;
    }
    return 0;
}

static INT_PTR CALLBACK
DialogProc_1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hThread;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hwnd, IDCANCEL));
            PostMessageA(hwnd, WM_COMMAND, psh2, 0);
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case psh2:
                    ok_int(GetFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
                    ok_int(GetFocus() == NULL, TRUE);

                    hThread = (HANDLE)_beginthreadex(NULL, 0, thread_proc_1, hwnd, 0, NULL);
                    CloseHandle(hThread);
                    break;

                case psh3:
                    ok_int(GetFocus() == NULL, TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);

                    ok_int(GetFocus() == NULL, TRUE);
                    SetFocus(GetDlgItem(hwnd, IDCANCEL));
                    ok_int(GetFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);

                    hThread = (HANDLE)_beginthreadex(NULL, 0, thread_proc_2, hwnd, 0, NULL);
                    CloseHandle(hThread);
                    break;

                case psh4:
                    ok_int(GetFocus() == GetDlgItem(hwnd, IDCANCEL), TRUE);
                    ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);

                    ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
                    ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_SHOW);

                    ok_int(GetFocus() == GetDlgItem(hwnd, IDOK), TRUE);
                    EndDialog(hwnd, IDCLOSE);
                    break;
            }
            break;
    }
    return 0;
}

START_TEST(SetFocus)
{
    s_dwMainThreadID = GetCurrentThreadId();
    Sleep(INTERVAL);
    ok_int((INT)DialogBoxA(GetModuleHandleA(NULL), "SETFOCUS", NULL, DialogProc_0), IDCLOSE);
    ok_int((INT)DialogBoxA(GetModuleHandleA(NULL), "SETFOCUS", NULL, DialogProc_1), IDCLOSE);
}
