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
#include <winnls.h>

#include <debug.h>

typedef struct _PROFILEDATA
{
    DWORD dwRefCount;
    DWORD dwState;
    BOOL bUnknownProfile;
    PWSTR pszFullName;
    PWSTR pszProfilePath;
} PROFILEDATA, *PPROFILEDATA;


static
BOOL
OnProfileTypeInit(
    _In_ HWND hwndDlg,
    _In_ PPROFILEDATA pProfileData)
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
    if (!ListView_GetItem(hwndListView, &Item))
        return FALSE;

    if (Item.lParam == 0)
        return FALSE;

    pProfileData = (PPROFILEDATA)Item.lParam;
    if (pProfileData->dwRefCount != 0)
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


static
BOOL
GetProfileSize(
    _In_ PWSTR pszProfilePath,
    _Inout_ PULONGLONG pullProfileSize)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    DWORD dwProfilePathLength;
    ULARGE_INTEGER Size;
    BOOL bResult = TRUE;

    dwProfilePathLength = wcslen(pszProfilePath);

    wcscat(pszProfilePath, L"\\*.*");

    hFile = FindFirstFileW(pszProfilePath, &FindData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        if ((GetLastError() != ERROR_FILE_NOT_FOUND) &&
            (GetLastError() != ERROR_PATH_NOT_FOUND))
            bResult = FALSE;

        goto done;
    }

    do
    {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if ((_wcsicmp(FindData.cFileName, L".") == 0) ||
                (_wcsicmp(FindData.cFileName, L"..") == 0))
                continue;

            pszProfilePath[dwProfilePathLength + 1] = UNICODE_NULL;
            wcscat(pszProfilePath, FindData.cFileName);

            if (!GetProfileSize(pszProfilePath, pullProfileSize))
            {
                bResult = FALSE;
                goto done;
            }
        }
        else
        {
            Size.u.LowPart = FindData.nFileSizeLow;
            Size.u.HighPart = FindData.nFileSizeHigh;
            *pullProfileSize += Size.QuadPart;
        }
    }
    while (FindNextFile(hFile, &FindData));

done:
    pszProfilePath[dwProfilePathLength] = UNICODE_NULL;

    if (hFile != INVALID_HANDLE_VALUE)
        FindClose(hFile);

    return bResult;
}


static
BOOL
GetProfileName(
    _In_ PSID pProfileSid,
    _In_ DWORD dwNameBufferSize,
    _Out_ PWSTR pszNameBuffer,
    _Out_ PBOOL pbUnknownProfile)
{
    WCHAR szAccountName[128], szDomainName[128];
    DWORD dwAccountNameSize, dwDomainNameSize;
    SID_NAME_USE Use;

    dwAccountNameSize = ARRAYSIZE(szAccountName);
    dwDomainNameSize = ARRAYSIZE(szDomainName);
    if (!LookupAccountSidW(NULL,
                           pProfileSid,
                           szAccountName,
                           &dwAccountNameSize,
                           szDomainName,
                           &dwDomainNameSize,
                           &Use))
    {
        /* Unknown account */
        LoadStringW(hApplet, IDS_USERPROFILE_ACCOUNT_UNKNOWN, pszNameBuffer, dwNameBufferSize);
        *pbUnknownProfile = TRUE;
    }
    else
    {
        /* Show only the user accounts */
        if (Use != SidTypeUser)
            return FALSE;

        if (szAccountName[0] == UNICODE_NULL)
        {
            /* Deleted account */
            LoadStringW(hApplet, IDS_USERPROFILE_ACCOUNT_DELETED, pszNameBuffer, dwNameBufferSize);
        }
        else
        {
            /* Normal account */
            wsprintf(pszNameBuffer, L"%s\\%s", szDomainName, szAccountName);
        }
        *pbUnknownProfile = FALSE;
    }

    return TRUE;
}


static VOID
AddUserProfile(
    _In_ HWND hwndListView,
    _In_ PSID pProfileSid,
    _In_ HKEY hProfileKey)
{
    WCHAR szTempProfilePath[MAX_PATH], szProfilePath[MAX_PATH];
    WCHAR szNameBuffer[256];
    PPROFILEDATA pProfileData = NULL;
    DWORD dwProfileData, dwSize, dwType, dwState = 0, dwRefCount = 0;
    DWORD dwProfilePathLength;
    PWSTR ptr;
    INT nId, iItem;
    LV_ITEM lvi;
    WIN32_FIND_DATA FindData;
    HANDLE hFile;
    SYSTEMTIME SystemTime;
    ULONGLONG ullProfileSize;
    BOOL bUnknownProfile;
    DWORD dwError;

    /* Get the profile path */
    dwSize = MAX_PATH * sizeof(WCHAR);
    dwError = RegQueryValueExW(hProfileKey,
                               L"ProfileImagePath",
                               NULL,
                               &dwType,
                               (LPBYTE)szTempProfilePath,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return;

    /* Expand it */
    ExpandEnvironmentStringsW(szTempProfilePath,
                              szProfilePath,
                              MAX_PATH);

    /* Check if the profile path exists */
    hFile = FindFirstFileW(szProfilePath, &FindData);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    FindClose(hFile);

    /* Get the length of the profile path */
    dwProfilePathLength = wcslen(szProfilePath);

    /* Check for the ntuser.dat file */
    wcscat(szProfilePath, L"\\ntuser.dat");
    hFile = FindFirstFileW(szProfilePath, &FindData);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    FindClose(hFile);
    szProfilePath[dwProfilePathLength] = UNICODE_NULL;

    /* Get the profile size */
    ullProfileSize = 0ULL;
    GetProfileSize(szProfilePath, &ullProfileSize);

    /* Get the profile name */
    if (!GetProfileName(pProfileSid,
                        ARRAYSIZE(szNameBuffer),
                        szNameBuffer,
                        &bUnknownProfile))
        return;

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

    /* Get the profile reference counter */
    dwSize = sizeof(dwRefCount);
    if (RegQueryValueExW(hProfileKey,
                         L"RefCount",
                         NULL,
                         &dwType,
                         (LPBYTE)&dwRefCount,
                         &dwSize) != ERROR_SUCCESS)
    {
        dwRefCount = 0;
    }

    /* Create and fill the profile data entry */
    dwProfileData = sizeof(PROFILEDATA) +
                    ((wcslen(szNameBuffer) + 1) * sizeof(WCHAR)) +
                    ((wcslen(szProfilePath) + 1) * sizeof(WCHAR));
    pProfileData = HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             dwProfileData);
    if (pProfileData == NULL)
        return;

    pProfileData->dwRefCount = dwRefCount;
    pProfileData->dwState = dwState;
    pProfileData->bUnknownProfile = bUnknownProfile;

    ptr = (PWSTR)((ULONG_PTR)pProfileData + sizeof(PROFILEDATA));
    pProfileData->pszFullName = ptr;

    wcscpy(pProfileData->pszFullName, szNameBuffer);

    ptr = (PWSTR)((ULONG_PTR)ptr + ((wcslen(pProfileData->pszFullName) + 1) * sizeof(WCHAR)));
    pProfileData->pszProfilePath = ptr;
    wcscpy(pProfileData->pszProfilePath, szProfilePath);

    /* Add the profile and set its name */
    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvi.pszText = pProfileData->pszFullName;
    lvi.state = 0;
    lvi.lParam = (LPARAM)pProfileData;
    iItem = ListView_InsertItem(hwndListView, &lvi);

    /* Set the profile size */
    StrFormatByteSizeW(ullProfileSize, szNameBuffer, ARRAYSIZE(szNameBuffer) - 1);
    ListView_SetItemText(hwndListView, iItem, 1, szNameBuffer);

    /* Set the profile type */
    if (dwState & 0x0010) // PROFILE_UPDATE_CENTRAL
        nId = IDS_USERPROFILE_ROAMING;
    else
        nId = IDS_USERPROFILE_LOCAL;

    LoadStringW(hApplet, nId, szNameBuffer, ARRAYSIZE(szNameBuffer));

    ListView_SetItemText(hwndListView, iItem, 2, szNameBuffer);

    /* FIXME: Set the profile status */
    if (dwState & 0x0001) // PROFILE_MANDATORY
        nId = IDS_USERPROFILE_MANDATORY;
    else if (dwState & 0x0010) // PROFILE_UPDATE_CENTRAL
        nId = IDS_USERPROFILE_ROAMING;
    else
        nId = IDS_USERPROFILE_LOCAL;

    LoadStringW(hApplet, nId, szNameBuffer, ARRAYSIZE(szNameBuffer));

    ListView_SetItemText(hwndListView, iItem, 3, szNameBuffer);

    /* Set the profile modified time */
    FileTimeToSystemTime(&FindData.ftLastWriteTime,
                         &SystemTime);

    GetDateFormatW(LOCALE_USER_DEFAULT,
                   DATE_SHORTDATE,
                   &SystemTime,
                   NULL,
                   szNameBuffer,
                   ARRAYSIZE(szNameBuffer));

    ListView_SetItemText(hwndListView, iItem, 4, szNameBuffer);
}


static VOID
UpdateButtonState(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView)
{
    LVITEM Item;
    INT iSelected;
    BOOL bChange = FALSE;
    BOOL bCopy = FALSE;
    BOOL bDelete = FALSE;
    PPROFILEDATA pProfileData;

    if (ListView_GetSelectedCount(hwndListView) != 0)
    {
        iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
        if (iSelected != -1)
        {
            ZeroMemory(&Item, sizeof(LVITEM));
            Item.mask = LVIF_PARAM;
            Item.iItem = iSelected;
            if (ListView_GetItem(hwndListView, &Item))
            {
                if (Item.lParam != 0)
                {
                    pProfileData = (PPROFILEDATA)Item.lParam;

                    if (pProfileData->bUnknownProfile)
                    {
                        bDelete = TRUE;
                        bCopy = FALSE;
                    }
                    else
                    {
                        bDelete = (pProfileData->dwRefCount == 0);
                        bCopy = (pProfileData->dwRefCount == 0);
                    }
                }
            }

            bChange = TRUE;
        }
    }

    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_CHANGE), bChange);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), bDelete);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), bCopy);
}


static VOID
AddUserProfiles(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView,
    _In_ BOOL bAdmin)
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
    PSID pProfileSid;
    PWSTR pszProfileSid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return;

    if (GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto done;
    }

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

    if (bAdmin)
    {
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
                if (ConvertStringSidToSid(szProfileSid, &pProfileSid))
                {
                    AddUserProfile(hwndListView,
                                   pProfileSid,
                                   hProfileKey);
                    LocalFree(pProfileSid);
                }

                RegCloseKey(hProfileKey);
            }
        }
    }
    else
    {
        if (ConvertSidToStringSidW(pTokenUser->User.Sid, &pszProfileSid))
        {
            if (RegOpenKeyExW(hKeyUserProfiles,
                              pszProfileSid,
                              0,
                              KEY_READ,
                              &hProfileKey) == ERROR_SUCCESS)
            {
                AddUserProfile(hwndListView,
                               pTokenUser->User.Sid,
                               hProfileKey);
                RegCloseKey(hProfileKey);
            }

            LocalFree(pszProfileSid);
        }
    }

    if (ListView_GetItemCount(hwndListView) != 0)
        ListView_SetItemState(hwndListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

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
    BOOL bAdmin;

    bAdmin = IsUserAdmin();

    /* Initialize the list view control */
    SetListViewColumns(GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));

    /* Hide the delete and copy buttons for non-admins */
    ShowWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), bAdmin ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), bAdmin ? SW_SHOW : SW_HIDE);

    /* Add the profiles to the list view */
    AddUserProfiles(hwndDlg, GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST), bAdmin);
}

static
VOID
OnNotify(
    _In_ HWND hwndDlg,
    _In_ NMHDR *nmhdr)
{
    LPNMLISTVIEW pNMLV;

    if (nmhdr->idFrom == IDC_USERACCOUNT_LINK && nmhdr->code == NM_CLICK)
    {
        ShellExecuteW(hwndDlg, NULL, L"usrmgr.cpl", NULL, NULL, 0);
    }
    else if (nmhdr->idFrom == IDC_USERPROFILE_LIST)
    {
        switch(nmhdr->code)
        {
            case LVN_ITEMCHANGED:
                UpdateButtonState(hwndDlg, nmhdr->hwndFrom);
                break;

            case NM_DBLCLK:
                ChangeUserProfileType(hwndDlg);
                break;

            case LVN_DELETEITEM:
                pNMLV = (LPNMLISTVIEW)nmhdr;
                if (pNMLV->lParam != 0)
                    HeapFree(GetProcessHeap(), 0, (LPVOID)pNMLV->lParam);
                break;
        }
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
