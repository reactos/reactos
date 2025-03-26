/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig/toolspage.c
 * PURPOSE:     Tools page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

#include <shlwapi.h>

HWND hToolsPage;
HWND hToolsListCtrl;
HWND hToolsDialog;

void AddItem ( DWORD, DWORD, DWORD, DWORD, int );
void FillListView ( void );

DWORD ListItems_Cmds[20];
DWORD ListItems_Params[20];
DWORD ListItems_Locations[20];

void AddItem ( DWORD name_id, DWORD descr_id, DWORD cmd_id , DWORD param_id, int csidl ) {
    TCHAR szTemp[256];
    LV_ITEM item;

    LoadString(hInst, name_id, szTemp, 256);
    memset(&item, 0, sizeof(LV_ITEM));
    item.mask = LVIF_TEXT;
    item.iImage = 0;
    item.pszText = szTemp;
    item.iItem = ListView_GetItemCount(hToolsListCtrl);
    item.lParam = 0;
    (void)ListView_InsertItem(hToolsListCtrl, &item);

    ListItems_Cmds[item.iItem] = cmd_id;
    ListItems_Params[item.iItem] = param_id;
    ListItems_Locations[item.iItem] = csidl;

    LoadString(hInst, descr_id, szTemp, 256);
    item.pszText = szTemp;
    item.iSubItem = 1;
    SendMessage(hToolsListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
}

void FillListView ( void ) {
    AddItem(IDS_TOOLS_CMD_NAME, IDS_TOOLS_CMD_DESCR, IDS_TOOLS_CMD_CMD, IDS_TOOLS_CMD_PARAM, CSIDL_SYSTEM);
    AddItem(IDS_TOOLS_REGEDIT_NAME, IDS_TOOLS_REGEDIT_DESCR, IDS_TOOLS_REGEDIT_CMD,IDS_TOOLS_REGEDIT_PARAM, CSIDL_WINDOWS);
    AddItem(IDS_TOOLS_SYSDM_NAME, IDS_TOOLS_SYSDM_DESCR, IDS_TOOLS_SYSDM_CMD, IDS_TOOLS_SYSDM_PARAM, CSIDL_SYSTEM);
    AddItem(IDS_TOOLS_INFO_NAME, IDS_TOOLS_INFO_DESCR, IDS_TOOLS_INFO_CMD, IDS_TOOLS_INFO_PARAM, CSIDL_SYSTEM);
}

INT_PTR CALLBACK
ToolsPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LV_COLUMN   column;
    TCHAR       szTemp[MAX_PATH*2];
    TCHAR       szTemp2[256];
    TCHAR * Ptr = NULL;
    LPNMITEMACTIVATE lpnmitem;
    LPNMHDR nmh;
    DWORD dwStyle;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hToolsListCtrl = GetDlgItem(hDlg, IDC_TOOLS_LIST);
            hToolsDialog = hDlg;

            dwStyle = (DWORD) SendMessage(hToolsListCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
            SendMessage(hToolsListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

            // Initialize the application page's controls
            column.mask = LVCF_TEXT | LVCF_WIDTH;

            LoadString(hInst, IDS_TOOLS_COLUMN_NAME, szTemp, 256);
            column.pszText = szTemp;
            column.cx = 150;
            (void)ListView_InsertColumn(hToolsListCtrl, 0, &column);

            column.mask = LVCF_TEXT | LVCF_WIDTH;
            LoadString(hInst, IDS_TOOLS_COLUMN_DESCR, szTemp, 256);
            column.pszText = szTemp;
            column.cx = 500;
            (void)ListView_InsertColumn(hToolsListCtrl, 1, &column);

            FillListView();
            return TRUE;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTN_RUN:
                {
                    if (ListView_GetSelectionMark(hToolsListCtrl) != -1)
                    {
                        if (SHGetSpecialFolderPath(NULL, szTemp, ListItems_Locations[ListView_GetSelectionMark(hToolsListCtrl)], FALSE))
                            Ptr = PathAddBackslash(szTemp);

                        if (!Ptr)
                            Ptr = szTemp;

                        szTemp2[0] = _T('\0');
                        LoadString(hInst, ListItems_Cmds[ListView_GetSelectionMark(hToolsListCtrl)], Ptr, 256);
                        LoadString(hInst, ListItems_Params[ListView_GetSelectionMark(hToolsListCtrl)], szTemp2, 256);
                        if (_tcslen(szTemp2))
                        {
                           _tcscat(szTemp, _T(" "));
                           _tcscat(Ptr, szTemp2);
                        }
                        ZeroMemory(&si, sizeof(STARTUPINFO));
                        si.cb = sizeof(STARTUPINFO);
                        if (CreateProcess(NULL, szTemp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        {
                            CloseHandle(pi.hThread);
                            CloseHandle(pi.hProcess);
                        }
                    }
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            nmh = (LPNMHDR) lParam;
            if (nmh->hwndFrom == hToolsListCtrl)
            {
                switch (nmh->code)
                {
                    case LVN_ITEMCHANGED:
                    {
                        lpnmitem = (LPNMITEMACTIVATE) lParam;
                        if (lpnmitem->iItem > -1)
                        {
                            LoadString(hInst, ListItems_Cmds[lpnmitem->iItem], szTemp, 256);
                            szTemp2[0] = _T('\0');
                            LoadString(hInst, ListItems_Params[lpnmitem->iItem], szTemp2, 256);
                            _tcscat(szTemp, _T(" "));
                            _tcscat(szTemp, szTemp2);
                            SendDlgItemMessage(hToolsDialog, IDC_TOOLS_CMDLINE, WM_SETTEXT, 0, (LPARAM) szTemp);
                        }
                        break;
                    }
                    case NM_DBLCLK:
                    {
                        lpnmitem = (LPNMITEMACTIVATE) lParam;
                        if (lpnmitem->iItem > -1)
                        {
                            if (SHGetSpecialFolderPath(NULL, szTemp, ListItems_Locations[ListView_GetSelectionMark(hToolsListCtrl)], FALSE))
                                Ptr = PathAddBackslash(szTemp);

                            if (!Ptr)
                                Ptr = szTemp;

                            szTemp2[0] = _T('\0');
                            LoadString(hInst, ListItems_Cmds[lpnmitem->iItem], Ptr, 256);
                            LoadString(hInst, ListItems_Params[lpnmitem->iItem], szTemp2, 256);
                            if (_tcslen(szTemp2))
                            {
                                _tcscat(szTemp, _T(" "));
                                _tcscat(Ptr, szTemp2);
                            }
                            ZeroMemory(&si, sizeof(STARTUPINFO));
                            si.cb = sizeof(STARTUPINFO);
                            if (CreateProcess(NULL, szTemp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                            {
                                CloseHandle(pi.hThread);
                                CloseHandle(pi.hProcess);
                            }
                        }
                        break;
                    }
                }
        }
        }
        break;
    }
    return 0;
}

