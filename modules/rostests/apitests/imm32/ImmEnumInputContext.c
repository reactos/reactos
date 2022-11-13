/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ImmEnumInputContext
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include <windowsx.h>

static INT s_nCounter = 0;
static HIMC s_hImc1 = NULL;
static HIMC s_hImc2 = NULL;

static BOOL CALLBACK
ImcEnumProc(HIMC hImc, LPARAM lParam)
{
    trace("s_nCounter %d\n", s_nCounter);
    switch (s_nCounter)
    {
        case 0:
        case 1:
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        case 2:
            ok(hImc == s_hImc2, "hImc was %p, s_hImc2 was %p\n", hImc, s_hImc1);
            break;
        case 3:
        case 4:
            ok(hImc == s_hImc1, "hImc was %p, s_hImc1 was %p\n", hImc, s_hImc1);
            break;
        default:
            ok_int(0, 1);
            break;
    }
    ++s_nCounter;
    return TRUE;
}

/* WM_INITDIALOG */
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    s_nCounter = 0;

    s_hImc1 = ImmGetContext(hwnd);
    ok(s_hImc1 != NULL, "s_hImc1 was NULL\n");

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0), TRUE);
    ok_int(s_nCounter, 1);

    ok_int(ImmEnumInputContext(1, ImcEnumProc, 0), FALSE);
    ok_int(s_nCounter, 1);

    s_hImc2 = ImmCreateContext();
    ok(s_hImc2 != NULL, "s_hImc1 was NULL\n");

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0), TRUE);
    ok_int(s_nCounter, 3);

    ok_int(ImmDestroyContext(s_hImc2), TRUE);
    s_hImc2 = NULL;

    ok_int(ImmEnumInputContext(0, ImcEnumProc, 0), TRUE);
    ok_int(s_nCounter, 4);

    ok_int(ImmEnumInputContext(GetCurrentThreadId(), ImcEnumProc, 0), TRUE);
    ok_int(s_nCounter, 5);

    PostMessageW(hwnd, WM_COMMAND, IDOK, 0);

    return TRUE;
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
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
    if (id != IDOK)
    {
        skip("Tests skipped\n");
    }
}
