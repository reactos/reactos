/*
 * ReactOS Explorer
 *
 * Copyright 2015 Jared Smudde <computerwhiz02@hotmail.com>
 * Copyright 2018 Denis Malikov <filedem@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

VOID ShowBehaviorCombo(HWND hwnd, LPNMITEMACTIVATE itemdata)
{
    HWND hListView = GetDlgItem(hwnd, IDC_NOTIFICATION_LIST);
    HWND hCombo = GetDlgItem(hwnd, IDC_NOTIFICATION_BEHAVIOUR);

    if (ComboBox_GetCount(hCombo) == 0)
    {
        WCHAR sShow[100];
        WCHAR sHide[100];
        WCHAR sInactive[100];
        LoadStringW(NULL, IDS_NOTIF_BEH_SHOW, sShow, _countof(sShow));
        LoadStringW(NULL, IDS_NOTIF_BEH_HIDE, sHide, _countof(sHide));
        LoadStringW(NULL, IDS_NOTIF_BEH_HIDE_INACTIVE, sInactive, _countof(sInactive));
        ComboBox_AddString(hCombo, sShow);
        ComboBox_AddString(hCombo, sHide);
        ComboBox_AddString(hCombo, sInactive);
        ComboBox_SetCurSel(hCombo, BEH_ALWAYS_SHOW);
    }

    int iItem = itemdata->iItem;
    if (iItem > -1)
    {
        POINT pt;
        RECT rc;
        ListView_GetItemPosition(hListView, iItem, &pt);
        GetWindowRect(hListView, &rc);
        POINT lv = { rc.left, rc.top };
        ScreenToClient(hwnd, &lv);

        LVITEM lvItem;
        lvItem.iItem = iItem;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;
        ListView_GetItem(hListView, &lvItem);

        ComboBox_SetCurSel(hCombo, lvItem.lParam);

        int iNameWidth = ListView_GetColumnWidth(hListView, 0);
        int iBehaviorWidth = ListView_GetColumnWidth(hListView, 1);

        ComboBox_SetItemData(hCombo, 0, iItem);
        SetWindowPos(hCombo, HWND_TOP, iNameWidth + lv.x, pt.y + lv.y, iBehaviorWidth, 20, SWP_SHOWWINDOW);
    }
    else
    {
        ShowWindow(hCombo, SW_HIDE);
    }
}

VOID SetItemBehavior(HWND hwnd, HWND hCombo)
{
    int iBehavior = ComboBox_GetCurSel(hCombo);
    int iItem = (int)ComboBox_GetItemData(hCombo, 0);
    HWND hListView = GetDlgItem(hwnd, IDC_NOTIFICATION_LIST);
    CNotifyToolbar* toolbar = (CNotifyToolbar*)GetWindowLongPtrW(hListView, GWLP_USERDATA);
    WCHAR szBehavior[100];
    int resId = iBehavior == BEH_ALWAYS_SHOW 
                    ? IDS_NOTIF_BEH_SHOW 
                    : (iBehavior == BEH_ALWAYS_HIDE ? IDS_NOTIF_BEH_HIDE : IDS_NOTIF_BEH_HIDE_INACTIVE);
    LoadStringW(NULL, resId, szBehavior, _countof(szBehavior));

    toolbar->SetBehavior(iItem, iBehavior);
    
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 1;
    lvItem.cchTextMax = 100;
    lvItem.pszText = szBehavior;
    ListView_SetItem(hListView, &lvItem);

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.lParam = iBehavior;
    lvItem.iItem = iItem;
    ListView_SetItem(hListView, &lvItem);

    ShowWindow(hCombo, SW_HIDE);
}

VOID ResetBehaviors(HWND hwnd)
{
    HWND hListView = GetDlgItem(hwnd, IDC_NOTIFICATION_LIST);
    CNotifyToolbar* toolbar = (CNotifyToolbar*)GetWindowLongPtrW(hListView, GWLP_USERDATA);
    int count = ListView_GetItemCount(hListView);
    WCHAR szBehavior[100];
    LoadStringW(NULL, IDS_NOTIF_BEH_SHOW, szBehavior, _countof(szBehavior));

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.cchTextMax = _countof(szBehavior);
    lvi.pszText = szBehavior;
    lvi.lParam = BEH_ALWAYS_SHOW;

    TBBUTTONINFO tbbi = { sizeof(tbbi) };
    tbbi.dwMask = TBIF_STATE | TBIF_LPARAM;

    for (int i = 0; i < count; i++)
    {
        toolbar->GetButtonInfo(i, &tbbi);
        
        if (((InternalIconData*)tbbi.lParam)->Locked == FALSE)
        {
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            ListView_SetItem(hListView, &lvi);

            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 1;
            ListView_SetItem(hListView, &lvi);
        
            toolbar->SetBehavior(i, BEH_ALWAYS_SHOW);
        }
    }
    
    toolbar->Toogle(FALSE);
}

VOID InitializeListView(HWND hwnd)
{
    HWND hListView = GetDlgItem(hwnd, IDC_NOTIFICATION_LIST);

    LVCOLUMN lvColumn;
    WCHAR sName[256];
                
    SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                
    LoadStringW(NULL, IDS_NOTIF_BEH_NAME, sName, _countof(sName));
    lvColumn.pszText = sName;
    lvColumn.cx = 190;
    ListView_InsertColumn(hListView, 0, &lvColumn);

    LoadStringW(NULL, IDS_NOTIF_BEH_BEHAVIOR, sName, _countof(sName));
    lvColumn.pszText = sName;
    lvColumn.cx = 130;
    ListView_InsertColumn(hListView, 1, &lvColumn);

    ListView_EnableGroupView(hListView, TRUE);

    LVGROUP lvGroup;
    lvGroup.cbSize    = sizeof(LVGROUP);
    lvGroup.mask      = LVGF_HEADER | LVGF_ALIGN | LVGF_GROUPID | LVGF_STATE;
    lvGroup.uAlign    = LVGA_FOOTER_LEFT;
    lvGroup.state     = LVGS_NORMAL;

    LoadStringW(NULL, IDS_NOTIF_BEH_CURRENT, sName, _countof(sName));
    lvGroup.pszHeader = sName;
    lvGroup.iGroupId  = 0;
    ListView_InsertGroup(hListView, -1, &lvGroup);

    LoadStringW(NULL, IDS_NOTIF_BEH_PAST, sName, _countof(sName));
    lvGroup.pszHeader = sName;
    lvGroup.iGroupId  = 1;
    ListView_InsertGroup(hListView, -1, &lvGroup);
}

INT_PTR CALLBACK CustomizeNotifyIconsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG: 
            InitializeListView(hwnd);
            break;
        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_NOTIFICATION_LIST && ((LPNMHDR)lParam)->code == NM_CLICK) 
            {
                ShowBehaviorCombo(hwnd, (LPNMITEMACTIVATE)lParam);
            }
            else return FALSE;
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
                case IDC_NOTIFICATION_BEHAVIOUR:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SetItemBehavior(hwnd, (HWND)lParam);
                    }
                    break;
                case IDC_TASKBARPROP_NOTIREST:
                    ResetBehaviors(hwnd);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

VOID SetNotifyIcons(HWND hDialog, IUnknown *TrayNotify)
{
    HWND hListView = GetDlgItem(hDialog, IDC_NOTIFICATION_LIST);    
    CNotifyToolbar* toolbar = CTrayNotifyWnd_GetTrayToolbar(TrayNotify);
    
    if (toolbar == NULL) return;
    
    SetWindowLongPtrW(hListView, GWLP_USERDATA, (LONG)toolbar);
    
    HIMAGELIST tbImageList = (HIMAGELIST)toolbar->GetImageList();
    HIMAGELIST lvImageList = ImageList_Duplicate(tbImageList);
    ListView_SetImageList(hListView, lvImageList, LVSIL_SMALL);
    
    WCHAR sShow[100];
    WCHAR sHide[100];
    WCHAR sInactive[100];
    LoadStringW(NULL, IDS_NOTIF_BEH_SHOW, sShow, _countof(sShow));
    LoadStringW(NULL, IDS_NOTIF_BEH_HIDE, sHide, _countof(sHide));
    LoadStringW(NULL, IDS_NOTIF_BEH_HIDE_INACTIVE, sInactive, _countof(sInactive));
    
    LVITEM lvItem;
    TBBUTTON tbtn;
    
    DWORD nButtons = toolbar->GetButtonCount();
    for (UINT i = 0; i < nButtons; i++)
    {
        if (toolbar->GetButton(i, &tbtn))
        {
            int bhv = ((InternalIconData*)tbtn.dwData)->uBehaviour;
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_GROUPID | LVIF_PARAM;
            lvItem.cchTextMax = 256;
            lvItem.iImage = tbtn.iBitmap;
            lvItem.iItem = i;
            lvItem.pszText = ((InternalIconData*)tbtn.dwData)->szTip;
            lvItem.lParam = bhv;
            ListView_InsertItem(hListView, &lvItem);
   
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = i;
            lvItem.iSubItem = 1;
            lvItem.cchTextMax = 100;
            lvItem.pszText = bhv == BEH_ALWAYS_SHOW ? sShow : (bhv == BEH_ALWAYS_HIDE ? sHide : sInactive);
            ListView_SetItem(hListView, &lvItem);
        }
    }
}

VOID ShowCustomizeNotifyIcons(HINSTANCE hInst, HWND hExplorer, IUnknown *TrayNotify)
{
    HWND hDlg = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_NOTIFICATIONS_CUSTOMIZE), hExplorer, CustomizeNotifyIconsProc);

    SetNotifyIcons(hDlg, TrayNotify);

    ShowWindow(hDlg, SW_SHOW);
}