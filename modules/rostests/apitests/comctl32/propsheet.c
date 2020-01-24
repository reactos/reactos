/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for v6 property sheet
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

#ifdef _MSC_VER
#define CHECK_STRUCT_SIZE(x, y) C_ASSERT((x) == (y))
#else
// Can't do this compile time, thanks gcc
// 'error: non-nested function with variably modified type'
#define CHECK_STRUCT_SIZE(x, y) ok((x) == (y), "Wrong size for %s, got %u, expected %u\n", #x, y, x)
#endif

// Validate struct sizes
static void test_StructSizes()
{
#ifdef _M_X64
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V1_SIZE, 72);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V2_SIZE, 88);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V3_SIZE, 96);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V4_SIZE, 104);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERA_V1_SIZE, 72);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERA_V2_SIZE, 96);

    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V1_SIZE, 72);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V2_SIZE, 88);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V3_SIZE, 96);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V4_SIZE, 104);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERW_V1_SIZE, 72);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERW_V2_SIZE, 96);
#else
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V1_SIZE, 40);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V2_SIZE, 48);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V3_SIZE, 52);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEA_V4_SIZE, 56);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERA_V1_SIZE, 40);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERA_V2_SIZE, 52);

    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V1_SIZE, 40);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V2_SIZE, 48);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V3_SIZE, 52);
    CHECK_STRUCT_SIZE(PROPSHEETPAGEW_V4_SIZE, 56);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERW_V1_SIZE, 40);
    CHECK_STRUCT_SIZE(PROPSHEETHEADERW_V2_SIZE, 52);
#endif
}

typedef HPROPSHEETPAGE (WINAPI *FN_CreatePropertySheetPageW)(LPCPROPSHEETPAGEW);
typedef int (WINAPI *FN_PropertySheetW)(LPCPROPSHEETHEADERW);
static FN_CreatePropertySheetPageW pCreatePropertySheetPageW;
static FN_PropertySheetW pPropertySheetW;

// Show that the Apply button is not enabled by default
static void test_ApplyButtonDisabled()
{
    PROPSHEETPAGEW psp = {0};
    PROPSHEETHEADERW header = {0};
    HPROPSHEETPAGE hpsp[1];

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = GetModuleHandleW(NULL);
    psp.pszTemplate = MAKEINTRESOURCEW(1);
    psp.pfnDlgProc = Page1DlgProc;
    hpsp[0] = pCreatePropertySheetPageW(&psp);
    ok(hpsp[0] != NULL, "hpsp[0] was NULL.\n");

    header.dwSize = sizeof(header);
    header.dwFlags = 0;
    header.hInstance = GetModuleHandleW(NULL);
    header.hwndParent = NULL;
    header.nPages = ARRAYSIZE(hpsp);
    header.phpage = hpsp;
    header.pszCaption = L"propsheet";
    ok(pPropertySheetW(&header) > 0, "PropertySheet returned non-positive value.\n");
}


START_TEST(propsheet)
{
    HMODULE hComCtl32;

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

    test_StructSizes();
    test_ApplyButtonDisabled();

    FreeLibrary(hComCtl32);
}
