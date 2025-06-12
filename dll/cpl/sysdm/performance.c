/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * FILE:        dll/cpl/sysdm/performance.c
 * PURPOSE:     Performance settings property sheets
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 *
 */

#include "precomp.h"
#include <shlguid_undoc.h> // CLSID_CRegTreeOptions
#include <shlobj_undoc.h> // IRegTreeOptions

static VOID
RegTreeOpt_OnDestroy(IRegTreeOptions *pRTO)
{
    if (pRTO)
    {
        IRegTreeOptions_WalkTree(pRTO, WALK_TREE_DESTROY);
        IRegTreeOptions_Release(pRTO);
    }
}

static BOOL
RegTreeOpt_ToggleCheckItem(IRegTreeOptions *pRTO, HTREEITEM hItem)
{
    return pRTO && SUCCEEDED(IRegTreeOptions_ToggleItem(pRTO, hItem));
}

static BOOL
RegTreeOpt_OnTreeViewClick(IRegTreeOptions *pRTO, HWND hWndTree)
{
    TV_HITTESTINFO HitTest;
    DWORD dwPos = GetMessagePos();
    HitTest.pt.x = GET_X_LPARAM(dwPos);
    HitTest.pt.y = GET_Y_LPARAM(dwPos);
    ScreenToClient(hWndTree, &HitTest.pt);
    HTREEITEM hItem = TreeView_HitTest(hWndTree, &HitTest);
    return RegTreeOpt_ToggleCheckItem(pRTO, hItem);
}

static BOOL
RegTreeOpt_OnTreeViewKeyDown(IRegTreeOptions *pRTO, HWND hWndTree, TV_KEYDOWN *pKey)
{
    if (pKey->wVKey == VK_SPACE)
        return RegTreeOpt_ToggleCheckItem(pRTO, TreeView_GetSelection(hWndTree));
    else if (pKey->wVKey == VK_F5 && pRTO)
        IRegTreeOptions_WalkTree(pRTO, WALK_TREE_REFRESH);
    return FALSE;
}

static INT_PTR CALLBACK
VisualEffectsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IRegTreeOptions *pRTO = (IRegTreeOptions*)GetWindowLongPtrW(hDlg, DWLP_USER);
    HWND hWndTree = GetDlgItem(hDlg, IDC_TREE);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            IRegTreeOptions *pRTO = NULL;
            if (SUCCEEDED(CoCreateInstance(&CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER,
                                           &IID_IRegTreeOptions, (void**)&pRTO)))
            {
                LPCSTR pszPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects";
                SetWindowLongPtrW(hDlg, DWLP_USER, (LPARAM)pRTO);
                IRegTreeOptions_InitTree(pRTO, hWndTree, HKEY_LOCAL_MACHINE, pszPath, NULL);
            }
            else
            {
                // Without IRegTreeOptions, our page is pointless so we remove it.
                // MSDN says we have to post the remove message from inside WM_INITDIALOG.
                HWND hWndPS = GetParent(hDlg);
                int iPage = PropSheet_HwndToIndex(hWndPS, hDlg);
                PostMessage(hWndPS, PSM_REMOVEPAGE, 0, SendMessageW(hWndPS, PSM_INDEXTOPAGE, iPage, 0));
            }
            return TRUE;
        }

        case WM_DESTROY:
            RegTreeOpt_OnDestroy(pRTO);
            SetWindowLongPtrW(hDlg, DWLP_USER, (LPARAM)NULL);
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                    if (RegTreeOpt_OnTreeViewClick(pRTO, hWndTree))
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;

                case TVN_KEYDOWN:
                    if (RegTreeOpt_OnTreeViewKeyDown(pRTO, hWndTree, (TV_KEYDOWN*)lParam))
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;

                case PSN_APPLY:
                {
                    if (pRTO)
                        IRegTreeOptions_WalkTree(pRTO, WALK_TREE_SAVE);

                    SHSendMessageBroadcastW(WM_SETTINGCHANGE, 0, 0); // For the ListviewShadow setting
                    return TRUE;
                }
            }
            break;
    }
    return FALSE;
}

static INT_PTR CALLBACK
AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // TODO: Not implemented yet
            EnableWindow(GetDlgItem(hDlg, IDC_CPUCLIENT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CPUSERVER), FALSE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CHANGESWAP:
                    DialogBoxW(hApplet, MAKEINTRESOURCEW(IDD_VIRTMEM), hDlg, VirtMemDlgProc);
                    break;
            }
            break;
    }
    return FALSE;
}

VOID
ShowPerformanceOptions(HWND hDlg)
{
    HRESULT hrInit = CoInitialize(NULL); // For IRegTreeOptions
    PROPSHEETHEADERW psh = { sizeof(psh), PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP };
    PROPSHEETPAGEW pages[2] = { 0 };

    pages[0].dwSize = sizeof(*pages);
    pages[0].hInstance = hApplet;
    pages[0].pszTemplate = MAKEINTRESOURCEW(IDD_VISUALEFFECTS);
    pages[0].pfnDlgProc = VisualEffectsDlgProc;

    pages[1].dwSize = sizeof(*pages);
    pages[1].hInstance = hApplet;
    pages[1].pszTemplate = MAKEINTRESOURCEW(IDD_ADVANCEDPERF);
    pages[1].pfnDlgProc = AdvancedDlgProc;

    psh.hwndParent = hDlg;
    psh.hInstance = hApplet;
    psh.pszCaption = MAKEINTRESOURCEW(IDS_PERFORMANCEOPTIONS);
    psh.nPages = _countof(pages);
    psh.ppsp = pages;

    PropertySheetW(&psh);

    if (SUCCEEDED(hrInit))
        CoUninitialize();
}
