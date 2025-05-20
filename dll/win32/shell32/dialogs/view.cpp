/*
 *     'View' tab property sheet of Folder Options
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2016-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

static inline CFolderOptions*
GetFolderOptions(HWND hwndDlg)
{
    return (CFolderOptions*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
}

static inline IRegTreeOptions*
GetRegTreeOptions(HWND hwndDlg)
{
    return (IRegTreeOptions*)GetWindowLongPtr(hwndDlg, DWLP_USER);
}

static void
ViewDlg_OnDestroy(HWND hwndDlg)
{
    if (IRegTreeOptions *pRTO = GetRegTreeOptions(hwndDlg))
    {
        pRTO->WalkTree(WALK_TREE_DESTROY);
        pRTO->Release();
        SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
    }
}

static BOOL
ViewDlg_OnInitDialog(HWND hwndDlg, LPPROPSHEETPAGE psp)
{
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, psp->lParam);
    CFolderOptions *pFO = (CFolderOptions*)psp->lParam;

    if (!pFO || !pFO->CanSetDefFolderSettings())
    {
        // The global options (started from rundll32 or control panel) 
        // has no browser to copy the current settings from.
        EnableWindow(GetDlgItem(hwndDlg, IDC_VIEW_APPLY_TO_ALL), FALSE);
    }

    IRegTreeOptions *pRTO = NULL;
    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_CRegTreeOptions, NULL, IID_PPV_ARG(IRegTreeOptions, &pRTO))))
    {
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM)pRTO);

        CHAR apath[MAX_PATH];
        SHUnicodeToAnsi(REGSTR_PATH_EXPLORER L"\\Advanced", apath, _countof(apath));
        HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);
        pRTO->InitTree(hwndTreeView, HKEY_LOCAL_MACHINE, apath, NULL);
    }

    return TRUE; // Set focus
}

static BOOL
ViewDlg_ToggleCheckItem(HWND hwndDlg, HTREEITEM hItem)
{
    IRegTreeOptions *pRTO = GetRegTreeOptions(hwndDlg);
    return pRTO && SUCCEEDED(pRTO->ToggleItem(hItem));
}

static VOID
ViewDlg_OnTreeViewClick(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

    // do hit test to get the clicked item
    TV_HITTESTINFO HitTest;
    ZeroMemory(&HitTest, sizeof(HitTest));
    DWORD dwPos = GetMessagePos();
    HitTest.pt.x = GET_X_LPARAM(dwPos);
    HitTest.pt.y = GET_Y_LPARAM(dwPos);
    ScreenToClient(hwndTreeView, &HitTest.pt);
    HTREEITEM hItem = TreeView_HitTest(hwndTreeView, &HitTest);

    // toggle the check mark if possible
    if (ViewDlg_ToggleCheckItem(hwndDlg, hItem))
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
}

static void
ViewDlg_OnTreeViewKeyDown(HWND hwndDlg, TV_KEYDOWN *KeyDown)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

    if (KeyDown->wVKey == VK_SPACE)
    {
        HTREEITEM hItem = TreeView_GetSelection(hwndTreeView);
        if (ViewDlg_ToggleCheckItem(hwndDlg, hItem))
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
    }
    else if (KeyDown->wVKey == VK_F5)
    {
        IRegTreeOptions *pRTO = GetRegTreeOptions(hwndDlg);
        if (pRTO)
            pRTO->WalkTree(WALK_TREE_REFRESH);
    }
}

static INT_PTR
ViewDlg_OnTreeCustomDraw(HWND hwndDlg, NMTVCUSTOMDRAW *Draw)
{
    NMCUSTOMDRAW& nmcd = Draw->nmcd;
    switch (nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;     // for CDDS_ITEMPREPAINT

        case CDDS_ITEMPREPAINT:
            if (!(nmcd.uItemState & CDIS_SELECTED)) // not selected
            {
                LPARAM lParam = nmcd.lItemlParam;
                if ((HKEY)lParam == HKEY_REGTREEOPTION_GRAYED) // disabled
                {
                    // draw as grayed
                    Draw->clrText = GetSysColor(COLOR_GRAYTEXT);
                    Draw->clrTextBk = GetSysColor(COLOR_WINDOW);
                    return CDRF_NEWFONT;
                }
            }
            break;

        default:
            break;
    }
    return CDRF_DODEFAULT;
}

static VOID
ViewDlg_RestoreDefaults(HWND hwndDlg)
{
    if (IRegTreeOptions *pRTO = GetRegTreeOptions(hwndDlg))
    {
        pRTO->WalkTree(WALK_TREE_DEFAULT);
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
    }
}

static BOOL CALLBACK
PostCabinetMessageCallback(HWND hWnd, LPARAM param)
{
    MSG &data = *(MSG*)param;
    WCHAR ClassName[100];
    if (GetClassNameW(hWnd, ClassName, _countof(ClassName)))
    {
        if (!wcscmp(ClassName, L"Progman") ||
            !wcscmp(ClassName, L"CabinetWClass") ||
            !wcscmp(ClassName, L"ExploreWClass"))
        {
            PostMessage(hWnd, data.message, data.wParam, data.lParam);
        }
    }
    return TRUE;
}

void
PostCabinetMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    MSG data;
    data.message = Msg;
    data.wParam = wParam;
    data.lParam = lParam;
    EnumWindows(PostCabinetMessageCallback, (LPARAM)&data);
}

static VOID
ViewDlg_Apply(HWND hwndDlg)
{
    IRegTreeOptions *pRTO = GetRegTreeOptions(hwndDlg);
    if (!pRTO || FAILED(pRTO->WalkTree(WALK_TREE_SAVE)))
        return;

    SHSettingsChanged(0, 0); // Invalidate restrictions
    SHGetSetSettings(NULL, 0, TRUE); // Invalidate SHELLSTATE

    CABINETSTATE cs;
    cs.cLength = sizeof(cs);
    if (ReadCabinetState(&cs, sizeof(cs)))
        WriteCabinetState(&cs); // Write the settings and invalidate global counter

    SHSendMessageBroadcastW(WM_SETTINGCHANGE, 0, 0);
    PostCabinetMessage(WM_COMMAND, FCIDM_DESKBROWSER_REFRESH, 0);
}

// IDD_FOLDER_OPTIONS_VIEW
INT_PTR CALLBACK
FolderOptionsViewDlg(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    INT_PTR Result;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ViewDlg_OnInitDialog(hwndDlg, (LPPROPSHEETPAGE)lParam);

        case WM_DESTROY:
            ViewDlg_OnDestroy(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_VIEW_RESTORE_DEFAULTS: // Restore Defaults
                    ViewDlg_RestoreDefaults(hwndDlg);
                    break;

                case IDC_VIEW_APPLY_TO_ALL:
                case IDC_VIEW_RESET_ALL:
                {
                    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                    bool ResetToDefault = LOWORD(wParam) == IDC_VIEW_RESET_ALL;
                    CFolderOptions *pFO = GetFolderOptions(hwndDlg);
                    if (pFO)
                        hr = pFO->ApplyDefFolderSettings(ResetToDefault); // Use IBrowserService2
                    if (ResetToDefault && hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
                        hr = CFolderOptions::ResetGlobalAndDefViewFolderSettings(); // No browser
                    if (FAILED(hr))
                        SHELL_ErrorBox(hwndDlg, hr);
                    break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (LPNMHDR(lParam)->code)
            {
                case NM_CLICK:  // clicked on treeview
                    ViewDlg_OnTreeViewClick(hwndDlg);
                    break;

                case NM_CUSTOMDRAW:     // custom draw (for graying)
                    Result = ViewDlg_OnTreeCustomDraw(hwndDlg, (NMTVCUSTOMDRAW*)lParam);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, Result);
                    return Result;

                case TVN_KEYDOWN:       // key is down
                    ViewDlg_OnTreeViewKeyDown(hwndDlg, (TV_KEYDOWN *)lParam);
                    break;

                case PSN_APPLY:         // [Apply] is clicked
                    ViewDlg_Apply(hwndDlg);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}
