/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/userprofile.c
 * PURPOSE:     Computer settings for networking
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static VOID
SetListViewColumns(HWND hwndListView)
{
    LV_COLUMN column;
    RECT rect;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    memset(&column, 0x00, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.40);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_USERPROFILE_NAME, szStr, 32);
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.fmt = LVCFMT_RIGHT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_USERPROFILE_SIZE, szStr, 32);
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 2;
    LoadString(hApplet, IDS_USERPROFILE_TYPE, szStr, 32);
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 2, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 3;
    LoadString(hApplet, IDS_USERPROFILE_STATUS, szStr, 32);
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 3, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15) - GetSystemMetrics(SM_CYHSCROLL);
    column.iSubItem = 4;
    LoadString(hApplet, IDS_USERPROFILE_MODIFIED, szStr, 32);
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 4, &column);
}


static VOID
AddUserProfile(HWND hwndListView,
               LPTSTR lpProfileSid)
{
    LV_ITEM lvi;

    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.pszText = lpProfileSid;
    lvi.state = 0;
    ListView_InsertItem(hwndListView, &lvi);
}


static VOID
AddUserProfiles(HWND hwndListView)
{
    HKEY hKeyUserProfiles;
    DWORD dwIndex;
    TCHAR szProfileSid[64];
    DWORD dwSidLength;
    FILETIME ftLastWrite;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
                     0,
                     KEY_READ,
                     &hKeyUserProfiles))
        return;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwSidLength = 64;
        if (RegEnumKeyEx(hKeyUserProfiles,
                         dwIndex,
                         szProfileSid,
                         &dwSidLength,
                         NULL,
                         NULL,
                         NULL,
                         &ftLastWrite))
            break;

        AddUserProfile(hwndListView, szProfileSid);
    }

    RegCloseKey(hKeyUserProfiles);
}


static VOID
OnInitDialog(HWND hwndDlg)
{
    /* Initialize the list view control */
    SetListViewColumns(GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));

    AddUserProfiles(GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));

    /* Disable the "Delete" and "Copy To" buttons if the user is not an admin */
    if (!IsUserAnAdmin())
    {
         EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), FALSE);
         EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), FALSE);
    }
}


/* Property page dialog callback */
INT_PTR CALLBACK
UserProfileDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hwndDlg,
                          LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR *nmhdr = (NMHDR *)lParam;

            if (nmhdr->idFrom == IDC_USERACCOUNT_LINK && nmhdr->code == NM_CLICK)
            {
                ShellExecute(hwndDlg,
                             TEXT("open"),
                             TEXT("rundll32.exe"),
                             TEXT("shell32.dll, Control_RunDLL usrmgr.cpl"),
                             NULL,
                             SW_SHOWNORMAL);
            }
            break;
        }
    }

    return FALSE;
}
