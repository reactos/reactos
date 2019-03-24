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

#include <debug.h>

typedef struct _PROFILEDATA
{
    BOOL bMyProfile;
    DWORD dwState;
    PWSTR pszFullName;
} PROFILEDATA, *PPROFILEDATA;


static
BOOL
OnProfileTypeInit(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    PWSTR pszRawBuffer = NULL, pszCookedBuffer = NULL;
    INT nLength;

    nLength = LoadStringW(hApplet, IDS_USERPROFILE_TYPE_TEXT, (PWSTR)&pszRawBuffer, 0);
    pszRawBuffer = NULL;
    if (nLength == 0)
        return FALSE;

    pszRawBuffer = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(WCHAR));
    if (pszRawBuffer == NULL)
        return FALSE;

    LoadStringW(hApplet, IDS_USERPROFILE_TYPE_TEXT, pszRawBuffer, nLength + 1);

    pszCookedBuffer = HeapAlloc(GetProcessHeap(), 0, (nLength + wcslen(pProfileData->pszFullName) + 1) * sizeof(WCHAR));
    if (pszCookedBuffer == NULL)
        goto done;

    swprintf(pszCookedBuffer, pszRawBuffer, pProfileData->pszFullName);

    /* Set the full text */
    SetDlgItemText(hwndDlg, IDC_USERPROFILE_TYPE_TEXT, pszCookedBuffer);

    /* FIXME: Right now, we support local user profiles only! */
    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_TYPE_ROAMING), FALSE);
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_USERPROFILE_TYPE_LOCAL), BST_CHECKED);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

done:
    if (pszCookedBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pszCookedBuffer);

    if (pszRawBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pszRawBuffer);

    return TRUE;
}


static
INT_PTR
CALLBACK
UserProfileTypeDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnProfileTypeInit(hwndDlg, (PPROFILEDATA)lParam);
            return TRUE;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static
BOOL
ChangeUserProfileType(
    _In_ HWND hwndDlg)
{
    HWND hwndListView;
    LVITEM Item;
    INT iSelected;

    DPRINT("ChangeUserProfileType(%p)\n", hwndDlg);

    hwndListView = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);
    if (hwndListView == NULL)
        return FALSE;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected == -1)
        return FALSE;

    ZeroMemory(&Item, sizeof(LVITEM));
    Item.mask = LVIF_PARAM;
    Item.iItem = iSelected;
    Item.iSubItem = 0;
    if (!ListView_GetItem(hwndListView, &Item))
        return FALSE;

    if (Item.lParam == 0)
        return FALSE;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_USERPROFILE_TYPE),
                       hwndDlg,
                       UserProfileTypeDlgProc,
                       (LPARAM)Item.lParam) == IDOK)
    {
        /* FIXME: Update the profile list view */
        return TRUE;
    }

    return FALSE;
}


static
BOOL
DeleteUserProfile(
    _In_ HWND hwndDlg)
{
    WCHAR szTitle[64], szRawText[128], szCookedText[256];
    HWND hwndListView;
    LVITEM Item;
    INT iSelected;
    PPROFILEDATA pProfileData;

    DPRINT("DeleteUserProfile()\n");

    hwndListView = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);
    if (hwndListView == NULL)
        return FALSE;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected == -1)
        return FALSE;

    ZeroMemory(&Item, sizeof(LVITEM));
    Item.mask = LVIF_PARAM;
    Item.iItem = iSelected;
    Item.iSubItem = 0;
    if (!ListView_GetItem(hwndListView, &Item))
        return FALSE;

    if (Item.lParam == 0)
        return FALSE;

    pProfileData = (PPROFILEDATA)Item.lParam;
    if (pProfileData->bMyProfile)
        return FALSE;

    LoadStringW(hApplet, IDS_USERPROFILE_CONFIRM_DELETE_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadStringW(hApplet, IDS_USERPROFILE_CONFIRM_DELETE, szRawText, ARRAYSIZE(szRawText));
    swprintf(szCookedText, szRawText, pProfileData->pszFullName);

    if (MessageBoxW(hwndDlg,
                    szCookedText,
                    szTitle,
                    MB_ICONQUESTION | MB_YESNO) == IDYES)
    {
        /* FIXME: Delete the profile here! */
        return TRUE;
    }

    return FALSE;
}


static
INT_PTR
CALLBACK
CopyUserProfileDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static
BOOL
CopyUserProfile(
    _In_ HWND hwndDlg)
{
    HWND hwndListView;
    LVITEM Item;
    INT iSelected;

    DPRINT("CopyUserProfile()\n");

    hwndListView = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);
    if (hwndListView == NULL)
        return FALSE;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected == -1)
        return FALSE;

    ZeroMemory(&Item, sizeof(LVITEM));
    Item.mask = LVIF_PARAM;
    Item.iItem = iSelected;
    Item.iSubItem = 0;
    if (!ListView_GetItem(hwndListView, &Item))
        return FALSE;

    if (Item.lParam == 0)
        return FALSE;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_USERPROFILE_COPY),
                       hwndDlg,
                       CopyUserProfileDlgProc,
                       (LPARAM)Item.lParam) == IDOK)
    {
        /* FIXME: Update the profile list view */
        return TRUE;
    }

    return FALSE;
}


static VOID
SetListViewColumns(
    _In_ HWND hwndListView)
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
    LoadString(hApplet, IDS_USERPROFILE_NAME, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.fmt = LVCFMT_RIGHT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_USERPROFILE_SIZE, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 2;
    LoadString(hApplet, IDS_USERPROFILE_TYPE, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 2, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 3;
    LoadString(hApplet, IDS_USERPROFILE_STATUS, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 3, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15) - GetSystemMetrics(SM_CYHSCROLL);
    column.iSubItem = 4;
    LoadString(hApplet, IDS_USERPROFILE_MODIFIED, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 4, &column);
}


static VOID
AddUserProfile(
    _In_ HWND hwndListView,
    _In_ LPTSTR lpProfileSid,
    _In_ PSID pMySid,
    _In_ HKEY hProfileKey)
{
    PPROFILEDATA pProfileData = NULL;
    WCHAR szAccountName[128], szDomainName[128];
    WCHAR szNameBuffer[256];
    SID_NAME_USE Use;
    DWORD dwAccountNameSize, dwDomainNameSize;
    DWORD dwProfileData, dwSize, dwType, dwState = 0;
    PWSTR ptr;
    PSID pSid = NULL;
    INT nId, iItem;
    LV_ITEM lvi;

    if (!ConvertStringSidToSid(lpProfileSid,
                               &pSid))
        return;

    dwAccountNameSize = ARRAYSIZE(szAccountName);
    dwDomainNameSize = ARRAYSIZE(szDomainName);
    if (!LookupAccountSidW(NULL,
                           pSid,
                           szAccountName,
                           &dwAccountNameSize,
                           szDomainName,
                           &dwDomainNameSize,
                           &Use))
    {
        /* Unknown account */
        LoadStringW(hApplet, IDS_USERPROFILE_ACCOUNT_UNKNOWN, szNameBuffer, ARRAYSIZE(szNameBuffer));
    }
    else
    {
        /* Show only the user accounts */
        if (Use != SidTypeUser)
            goto done;

        if (szAccountName[0] == UNICODE_NULL)
        {
            /* Deleted account */
            LoadStringW(hApplet, IDS_USERPROFILE_ACCOUNT_DELETED, szNameBuffer, ARRAYSIZE(szNameBuffer));
        }
        else
        {
            /* Normal account */
            wsprintf(szNameBuffer, L"%s\\%s", szDomainName, szAccountName);
        }
    }

    /* Get the profile state value */
    dwSize = sizeof(dwState);
    if (RegQueryValueExW(hProfileKey,
                         L"State",
                         NULL,
                         &dwType,
                         (LPBYTE)&dwState,
                         &dwSize) != ERROR_SUCCESS)
    {
        dwState = 0;
    }

    /* Create and fill the profile data entry */
    dwProfileData = sizeof(PROFILEDATA) +
                    ((wcslen(szNameBuffer) + 1) * sizeof(WCHAR));
    pProfileData = HeapAlloc(GetProcessHeap(),
                             0,
                             dwProfileData);
    if (pProfileData == NULL)
        goto done;

    pProfileData->bMyProfile = EqualSid(pMySid, pSid);
    pProfileData->dwState = dwState;

    ptr = (PWSTR)((ULONG_PTR)pProfileData + sizeof(PROFILEDATA));
    pProfileData->pszFullName = ptr;

    wcscpy(pProfileData->pszFullName, szNameBuffer);

    /* Add the profile and set its name */
    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvi.pszText = pProfileData->pszFullName;
    lvi.state = 0;
    lvi.lParam = (LPARAM)pProfileData;
    iItem = ListView_InsertItem(hwndListView, &lvi);

    /* Set the profile type */
    if (dwState & 0x0001) // PROFILE_MANDATORY
        nId = IDS_USERPROFILE_MANDATORY;
    else if (dwState & 0x0010) // PROFILE_UPDATE_CENTRAL
        nId = IDS_USERPROFILE_ROAMING;
    else
        nId = IDS_USERPROFILE_LOCAL;

    LoadStringW(hApplet, nId, szAccountName, ARRAYSIZE(szAccountName));

    ListView_SetItemText(hwndListView, iItem, 2, szAccountName);

done:
    if (pSid != NULL)
        LocalFree(pSid);
}


static VOID
UpdateButtonState(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView)
{
    LVITEM Item;
    INT iSelected;
    BOOL bMyProfile;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected != -1)
    {
        Item.mask = LVIF_PARAM;
        Item.iItem = iSelected;
        Item.iSubItem = 0;
        if (ListView_GetItem(hwndListView, &Item))
        {
            if (Item.lParam != 0)
            {
                bMyProfile = ((PPROFILEDATA)Item.lParam)->bMyProfile;
                if (/*IsUserAnAdmin() &&*/ !bMyProfile)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), TRUE);
                }
            }
        }
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_CHANGE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_CHANGE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), FALSE);
    }
}


static VOID
AddUserProfiles(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView)
{
    HKEY hKeyUserProfiles = INVALID_HANDLE_VALUE;
    HKEY hProfileKey;
    DWORD dwIndex;
    WCHAR szProfileSid[64];
    DWORD dwSidLength;
    FILETIME ftLastWrite;
    DWORD dwSize;
    HANDLE hToken = NULL;
    PTOKEN_USER pTokenUser = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize == 0)
        goto done;

    pTokenUser = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (pTokenUser == NULL)
        goto done;

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
        goto done;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                      0,
                      KEY_READ,
                      &hKeyUserProfiles))
        goto done;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwSidLength = ARRAYSIZE(szProfileSid);
        if (RegEnumKeyExW(hKeyUserProfiles,
                          dwIndex,
                          szProfileSid,
                          &dwSidLength,
                          NULL,
                          NULL,
                          NULL,
                          &ftLastWrite))
            break;

        if (RegOpenKeyExW(hKeyUserProfiles,
                          szProfileSid,
                          0,
                          KEY_READ,
                          &hProfileKey) == ERROR_SUCCESS)
        {
            AddUserProfile(hwndListView, szProfileSid, pTokenUser->User.Sid, hProfileKey);
            RegCloseKey(hProfileKey);
        }
    }

    if (ListView_GetItemCount(hwndListView) != 0)
        ListView_SetItemState(hwndListView, 0, LVIS_SELECTED, LVIS_SELECTED);

    UpdateButtonState(hwndDlg, hwndListView);

done:
    if (hKeyUserProfiles != INVALID_HANDLE_VALUE)
        RegCloseKey(hKeyUserProfiles);

    if (pTokenUser != NULL)
        HeapFree(GetProcessHeap(), 0, pTokenUser);

    if (hToken != NULL)
        CloseHandle(hToken);
}


static VOID
OnInitUserProfileDialog(HWND hwndDlg)
{
    /* Initialize the list view control */
    SetListViewColumns(GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));

    AddUserProfiles(hwndDlg, GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));
}


static
VOID
OnDestroy(
    _In_ HWND hwndDlg)
{
    HWND hwndList;
    INT nItems, i;
    LVITEM Item;

    hwndList = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);

    nItems = ListView_GetItemCount(hwndList);
    for (i = 0; i < nItems; i++)
    {
        Item.iItem = i;
        Item.iSubItem = 0;
        if (ListView_GetItem(hwndList, &Item))
        {
            if (Item.lParam != 0)
                HeapFree(GetProcessHeap(), 0, (PVOID)Item.lParam);
        }
    }
}


static
VOID
OnNotify(
    _In_ HWND hwndDlg,
    _In_ NMHDR *nmhdr)
{
    if (nmhdr->idFrom == IDC_USERACCOUNT_LINK && nmhdr->code == NM_CLICK)
    {
        ShellExecuteW(hwndDlg, NULL, L"usrmgr.cpl", NULL, NULL, 0);
    }
    else if (nmhdr->idFrom == IDC_USERPROFILE_LIST && nmhdr->code == LVN_ITEMCHANGED)
    {
        UpdateButtonState(hwndDlg, nmhdr->hwndFrom);
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
            return TRUE;

        case WM_DESTROY:
            OnDestroy(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;

                case IDC_USERPROFILE_CHANGE:
                    ChangeUserProfileType(hwndDlg);
                    break;

                case IDC_USERPROFILE_DELETE:
                    DeleteUserProfile(hwndDlg);
                    break;

                case IDC_USERPROFILE_COPY:
                    CopyUserProfile(hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            OnNotify(hwndDlg, (NMHDR *)lParam);
            break;
    }

    return FALSE;
}
