/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ImmEnumInputContext
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include <windowsx.h>

static HWND s_hwnd = NULL;
static INT s_nCounter = 0;
static HIMC s_hImc1 = NULL;
static HIMC s_hImc2 = NULL;
static HIMC s_hImc3 = NULL;
static HIMC s_hImc4 = NULL;

static BOOL CALLBACK
ImcEnumProc(HIMC hImc, LPARAM lParam)
{
    trace("s_nCounter %d\n", s_nCounter);
    switch (s_nCounter)
    {
        case 0:
            ok_long((LONG)lParam, 0xDEADBEEF);
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 1:
            ok_long((LONG)lParam, 0xDEADFACE);
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 2:
            ok_long((LONG)lParam, 0xDEADFACE);
            ok(hImc == s_hImc2, "hImc was %p, s_hImc2 was %p\n", hImc, s_hImc1);
            break;
        case 3:
            ok_long((LONG)lParam, 0xBEEFCAFE);
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 4:
            ok_long((LONG)lParam, 0xAC1DFACE);
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 5:
            ok_long((LONG)lParam, 0xDEADBEEF);
            s_hImc3 = hImc;
            ok(hImc != s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 6:
            ok_long((LONG)lParam, 0xBEEFCAFE);
            ok(hImc == s_hImc3, "hImc was %p, s_hImc3 was %p\n", hImc, s_hImc3);
            break;
        case 7:
            ok_long((LONG)lParam, 0xDEADFACE);
            ok(hImc == s_hImc3, "hImc was %p, s_hImc3 was %p\n", hImc, s_hImc3);
            break;
        case 8:
            ok_long((LONG)lParam, 0xDEADFACE);
            ok(hImc == s_hImc4, "hImc was %p, s_hImc4 was %p\n", hImc, s_hImc4);
            break;
        case 9:
            ok_long((LONG)lParam, 0xFEEDF00D);
            ok(hImc == s_hImc3, "hImc was %p, s_hImc3 was %p\n", hImc, s_hImc3);
            break;
        case 10:
            ok_long((LONG)lParam, 0xFEEDF00D);
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        default:
            ok_long(0, 1);
            ok_int(0, 1);
            break;
    }
    ++s_nCounter;
    return TRUE;
}

static DWORD WINAPI AnotherThreadFunc(LPVOID arg)
{
    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0xDEADBEEF), TRUE);
    ok_int(s_nCounter, 6);

    ok_int(ImmEnumInputContext(GetCurrentThreadId(), ImcEnumProc, 0xBEEFCAFE), TRUE);
    ok_int(s_nCounter, 7);

    s_hImc4 = ImmCreateContext();

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0xDEADFACE), TRUE);
    ok_int(s_nCounter, 9);

    ok_int(ImmDestroyContext(s_hImc4), TRUE);
    s_hImc4 = NULL;

    ok_int(ImmEnumInputContext((DWORD)-1, ImcEnumProc, 0xFEEDF00D), TRUE);
    ok_int(s_nCounter, 11);

    PostMessageW(s_hwnd, WM_COMMAND, IDYES, 0);
    return 0;
}

/* WM_INITDIALOG */
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HANDLE hThread;

    s_hwnd = hwnd;
    s_nCounter = 0;

    s_hImc1 = ImmGetContext(hwnd);
    ok(s_hImc1 != NULL, "s_hImc1 was NULL\n");

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0xDEADBEEF), TRUE);
    ok_int(s_nCounter, 1);

    ok_int(ImmEnumInputContext(1, ImcEnumProc, 0xFEEDF00D), FALSE);
    ok_int(s_nCounter, 1);

    s_hImc2 = ImmCreateContext();
    ok(s_hImc2 != NULL, "s_hImc1 was NULL\n");

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0xDEADFACE), TRUE);
    ok_int(s_nCounter, 3);

    ok_int(ImmDestroyContext(s_hImc2), TRUE);
    s_hImc2 = NULL;

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0xBEEFCAFE), TRUE);
    ok_int(s_nCounter, 4);

    ok_int(ImmEnumInputContext(GetCurrentThreadId(), ImcEnumProc, 0xAC1DFACE), TRUE);
    ok_int(s_nCounter, 5);

    hThread = CreateThread(NULL, 0, AnotherThreadFunc, NULL, 0, NULL);
    if (hThread == NULL)
    {
        skip("hThread was NULL\n");
        PostMessageW(hwnd, WM_COMMAND, IDNO, 0);
    }
    CloseHandle(hThread);

    return TRUE;
}

/* WM_COMMAND */
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
        case IDYES:
            EndDialog(hwnd, id);
            break;
    }
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

START_TEST(ImmEnumInputContext)
{
    INT_PTR id;
    id = DialogBoxW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), NULL, DialogProc);
    if (id != IDYES)
    {
        skip("Tests skipped\n");
    }
}
