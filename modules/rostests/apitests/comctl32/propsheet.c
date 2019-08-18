/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for property sheet
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "wine/test.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>

#define IDC_APPLY_BUTTON 12321

static BOOL s_bNotified;

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    s_bNotified = FALSE;
    SetDlgItemTextW(hwnd, edt1, L"text");
    SetTimer(hwnd, 999, 300, NULL);
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
        case edt1:
            if (codeNotify == EN_CHANGE)
            {
                s_bNotified = TRUE;
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
    }
}

static void OnTimer(HWND hwnd, UINT id)
{
    HWND hwndParent, hwndApply;

    KillTimer(hwnd, id);

    ok_int(s_bNotified, TRUE);

    hwndParent = GetParent(hwnd);
    hwndApply = GetDlgItem(hwndParent, IDC_APPLY_BUTTON);
    ok_int(IsWindowEnabled(hwndApply), FALSE);

    PropSheet_Changed(hwndParent, hwnd);
    ok_int(IsWindowEnabled(hwndApply), TRUE);

    PropSheet_UnChanged(hwndParent, hwnd);
    ok_int(IsWindowEnabled(hwndApply), FALSE);

    PropSheet_PressButton(hwndParent, PSBTN_OK);
}

static INT_PTR CALLBACK
Page1DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
    }
    return 0;
}

typedef HPROPSHEETPAGE (WINAPI *FN_CreatePropertySheetPageW)(LPCPROPSHEETPAGEW);
typedef int (WINAPI *FN_PropertySheetW)(LPCPROPSHEETHEADERW);

START_TEST(propsheet)
{
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW header;
    HPROPSHEETPAGE hpsp[1];
    HMODULE hComCtl32;
    FN_CreatePropertySheetPageW pCreatePropertySheetPageW;
    FN_PropertySheetW pPropertySheetW;

    hComCtl32 = LoadLibraryW(L"comctl32.dll");
    pCreatePropertySheetPageW = (FN_CreatePropertySheetPageW)GetProcAddress(hComCtl32, "CreatePropertySheetPageW");
    pPropertySheetW = (FN_PropertySheetW)GetProcAddress(hComCtl32, "PropertySheetW");

    ok(pCreatePropertySheetPageW != NULL, "pCreatePropertySheetPageW was NULL.\n");
    ok(pPropertySheetW != NULL, "pPropertySheetW was NULL.\n");

    if (!pCreatePropertySheetPageW || !pPropertySheetW)
    {
        skip("!pCreatePropertySheetPageW || !pPropertySheetW\n");
        return;
    }

    ZeroMemory(&psp, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = GetModuleHandleW(NULL);
    psp.pszTemplate = MAKEINTRESOURCEW(1);
    psp.pfnDlgProc = Page1DlgProc;
    hpsp[0] = (*pCreatePropertySheetPageW)(&psp);
    ok(hpsp[0] != NULL, "hpsp[0] was NULL.\n");

    ZeroMemory(&header, sizeof(header));
    header.dwSize = sizeof(header);
    header.dwFlags = 0;
    header.hInstance = GetModuleHandleW(NULL);
    header.hwndParent = NULL;
    header.nPages = ARRAYSIZE(hpsp);
    header.phpage = hpsp;
    header.pszCaption = L"propsheet";
    ok((*pPropertySheetW)(&header) > 0, "PropertySheet returned non-positive value.\n");

    FreeLibrary(hComCtl32);
}
