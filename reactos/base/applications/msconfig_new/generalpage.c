/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/generalpage.c
 * PURPOSE:     General page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

static LPCWSTR lpszRestoreProgPath1 = L"%SystemRoot%\\System32\\rstrui.exe";
static LPCWSTR lpszRestoreProgPath2 = L"%SystemRoot%\\System32\\restore\\rstrui.exe";

HWND hGeneralPage;

#if 0 // TODO: Will be used later on...
static VOID EnableSelectiveStartupControls(BOOL bEnable)
{
    assert(hGeneralPage);

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_SYSTEM_SERVICES), bEnable);
    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_STARTUP_ITEMS)  , bEnable);

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), bEnable);

    // EnableWindow(GetDlgItem(hGeneralPage, IDC_RB_USE_ORIGINAL_BOOTCAT), bEnable);
    // EnableWindow(GetDlgItem(hGeneralPage, IDC_RB_USE_MODIFIED_BOOTCAT), (bEnable ? !bIsOriginalBootIni : FALSE));

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_SYSTEM_INI), bEnable);
    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_WIN_INI)   , bEnable);

    return;
}

static VOID CheckSelectiveStartupControls(BOOL bCheck)
{
    assert(hGeneralPage);

    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_SYSTEM_SERVICES), (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_STARTUP_ITEMS)  , (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_SYSTEM_INI)          , (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_WIN_INI)             , (bCheck ? BST_CHECKED : BST_UNCHECKED));

    return;
}
#endif

INT_PTR CALLBACK
GeneralPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hGeneralPage = hDlg;
            PropSheet_UnChanged(hMainWnd, hGeneralPage);

#if 0
            /* FIXME */
            SendDlgItemMessage(hDlg, IDC_RB_NORMAL_STARTUP, BM_SETCHECK, BST_CHECKED, 0);
            EnableCheckboxControls(hDlg, FALSE);
#endif

            return TRUE;
        }
    }

    return FALSE;
}
