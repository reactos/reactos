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
#include <sddl.h>


typedef struct _PROFILEDATA
{
    PWSTR pszFullName;
} PROFILEDATA, *PPROFILEDATA;


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
    PPROFILEDATA pProfileData = NULL;
    PWSTR pszAccountName = NULL;
    PWSTR pszDomainName = NULL;
    SID_NAME_USE Use;
    DWORD dwAccountNameSize, dwDomainNameSize;
    DWORD dwProfileData;
    PWSTR ptr;
    PSID pSid = NULL;
    LV_ITEM lvi;

    if (!ConvertStringSidToSid(lpProfileSid,
                               &pSid))
        return;

    dwAccountNameSize = 0;
    dwDomainNameSize = 0;
    LookupAccountSidW(NULL,
                      pSid,
                      NULL,
                      &dwAccountNameSize,
                      NULL,
                      &dwDomainNameSize,
                      &Use);

    pszDomainName = HeapAlloc(GetProcessHeap(),
                              0,
                              dwDomainNameSize * sizeof(WCHAR));
    if (pszDomainName == NULL)
        goto done;

    pszAccountName = HeapAlloc(GetProcessHeap(),
                               0,
                               dwAccountNameSize * sizeof(WCHAR));
    if (pszAccountName == NULL)
        goto done;

    if (!LookupAccountSidW(NULL,
                           pSid,
                           pszAccountName,
                           &dwAccountNameSize,
                           pszDomainName,
                           &dwDomainNameSize,
                           &Use))
        goto done;

    /* Show only the user accounts */
    if (Use != SidTypeUser)
        goto done;

    dwProfileData = sizeof(PROFILEDATA) +
                    ((wcslen(pszDomainName) + wcslen(pszAccountName) + 2) * sizeof(WCHAR));
    pProfileData = HeapAlloc(GetProcessHeap(),
                             0,
                             dwProfileData);
    if (pProfileData == NULL)
        goto done;

    ptr = (PWSTR)((ULONG_PTR)pProfileData + sizeof(PROFILEDATA));
    pProfileData->pszFullName = ptr;

    wsprintf(pProfileData->pszFullName, L"%s\\%s", pszDomainName, pszAccountName);

    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.pszText = pProfileData->pszFullName;
    lvi.state = 0;
    ListView_InsertItem(hwndListView, &lvi);

done:
    if (pProfileData != NULL)
        HeapFree(GetProcessHeap(), 0, pProfileData);

    if (pszAccountName != NULL)
        HeapFree(GetProcessHeap(), 0, pszAccountName);

    if (pszDomainName != NULL)
        HeapFree(GetProcessHeap(), 0, pszDomainName);

    if (pSid != NULL)
        LocalFree(pSid);
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
OnInitUserProfileDialog(HWND hwndDlg)
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
            OnInitUserProfileDialog(hwndDlg);
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
                ShellExecuteW(hwndDlg, NULL, L"usrmgr.cpl", NULL, NULL, 0);
            }
            break;
        }
    }

    return FALSE;
}
