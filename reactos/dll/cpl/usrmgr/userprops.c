/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/userprops.c
 * PURPOSE:         User property sheet
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"

typedef struct _GENERAL_USER_DATA
{
    DWORD dwFlags;
    DWORD dwPasswordExpired;
    TCHAR szUserName[1];
} GENERAL_USER_DATA, *PGENERAL_USER_DATA;

#define VALID_GENERAL_FLAGS (UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD | UF_ACCOUNTDISABLE | UF_LOCKOUT)



static VOID
GetProfileData(HWND hwndDlg, LPTSTR lpUserName)
{
    PUSER_INFO_3 userInfo = NULL;
    NET_API_STATUS status;
    BOOL bLocal;
    TCHAR szDrive[8];
    INT i;
    INT nSel;

    status = NetUserGetInfo(NULL, lpUserName, 3, (LPBYTE*)&userInfo);
    if (status != NERR_Success)
        return;

    SetDlgItemText(hwndDlg, IDC_USER_PROFILE_PATH, userInfo->usri3_profile);
    SetDlgItemText(hwndDlg, IDC_USER_PROFILE_SCRIPT, userInfo->usri3_script_path);


    bLocal = (userInfo->usri3_home_dir_drive == NULL) ||
              (_tcslen(userInfo->usri3_home_dir_drive) == 0);
    CheckRadioButton(hwndDlg, IDC_USER_PROFILE_LOCAL, IDC_USER_PROFILE_REMOTE,
                     bLocal ? IDC_USER_PROFILE_LOCAL : IDC_USER_PROFILE_REMOTE);

    for (i = 0; i < 26; i++)
    {
        wsprintf(szDrive, _T("%c:"), (TCHAR)('A' + i));
        nSel = SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE),
                           CB_INSERTSTRING, -1, (LPARAM)szDrive);
    }

    if (bLocal)
    {
        SetDlgItemText(hwndDlg, IDC_USER_PROFILE_LOCAL_PATH, userInfo->usri3_home_dir);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH), FALSE);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH, userInfo->usri3_home_dir);
        nSel = SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE),
                           CB_FINDSTRINGEXACT, -1, (LPARAM)userInfo->usri3_home_dir_drive);
    }

    SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE),
                CB_SETCURSEL, nSel, 0);

    NetApiBufferFree(userInfo);
}


INT_PTR CALLBACK
UserProfilePageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            GetProfileData(hwndDlg,
                           (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_PROFILE_LOCAL:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_LOCAL_PATH), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH), FALSE);
                    break;

                case IDC_USER_PROFILE_REMOTE:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_LOCAL_PATH), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH), TRUE);
                    break;
            }
            break;
    }

    return FALSE;
}


static VOID
GetMembershipData(HWND hwndDlg, LPTSTR lpUserName)
{
    PLOCALGROUP_USERS_INFO_0 usersInfo = NULL;
    NET_API_STATUS status;
    DWORD dwRead;
    DWORD dwTotal;
    DWORD i;
    HIMAGELIST hImgList;
    HICON hIcon;
    LV_ITEM lvi;
    HWND hwndLV;
    LV_COLUMN column;
    RECT rect;


    hwndLV = GetDlgItem(hwndDlg, IDC_USER_MEMBERSHIP_LIST);

    /* Create the image list */
    hImgList = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 5, 5);
    hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_GROUP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList, hIcon);
    DestroyIcon(hIcon);
    (void)ListView_SetImageList(hwndLV, hImgList, LVSIL_SMALL);

    /* Set the list column */
    GetClientRect(hwndLV, &rect);

    memset(&column, 0x00, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)(rect.right - rect.left);
    column.iSubItem = 0;
    (void)ListView_InsertColumn(hwndLV, 0, &column);


    status = NetUserGetLocalGroups(NULL, lpUserName, 0, 0,
                                   (LPBYTE*)&usersInfo,
                                   MAX_PREFERRED_LENGTH,
                                   &dwRead,
                                   &dwTotal);
    if (status != NERR_Success)
        return;

    for (i = 0; i < dwRead; i++)
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.pszText = usersInfo[i].lgrui0_name;
        lvi.state = 0;
        lvi.iImage = 0; 

        (void)ListView_InsertItem(hwndLV, &lvi);
    }


    NetApiBufferFree(usersInfo);

}


INT_PTR CALLBACK
UserMembershipPageProc(HWND hwndDlg,
                       UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam)
{

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            GetMembershipData(hwndDlg,
                              (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);
            break;

    }

    return FALSE;
}


static VOID
UpdateUserOptions(HWND hwndDlg,
                  PGENERAL_USER_DATA pUserData,
                  BOOL bInit)
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_CANNOT_CHANGE),
                 !pUserData->dwPasswordExpired);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_NEVER_EXPIRES),
                 !pUserData->dwPasswordExpired);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_FORCE_CHANGE),
                 (pUserData->dwFlags & (UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD)) == 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_LOCKED),
                 (pUserData->dwFlags & UF_LOCKOUT) != 0);

    if (bInit)
    {
        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_FORCE_CHANGE,
                       pUserData->dwPasswordExpired ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_CANNOT_CHANGE,
                       (pUserData->dwFlags & UF_PASSWD_CANT_CHANGE) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_NEVER_EXPIRES,
                       (pUserData->dwFlags & UF_DONT_EXPIRE_PASSWD) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_DISABLED,
                       (pUserData->dwFlags & UF_ACCOUNTDISABLE) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_LOCKED,
                       (pUserData->dwFlags & UF_LOCKOUT) ? BST_CHECKED : BST_UNCHECKED);
    }
}


static VOID
GetGeneralUserData(HWND hwndDlg,
                   PGENERAL_USER_DATA pUserData)
{
    PUSER_INFO_3 pUserInfo = NULL;

    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_NAME, pUserData->szUserName);

    NetUserGetInfo(NULL, pUserData->szUserName, 3, (LPBYTE*)&pUserInfo);

    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_FULL_NAME, pUserInfo->usri3_full_name);
    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_DESCRIPTION, pUserInfo->usri3_comment);

    pUserData->dwFlags = pUserInfo->usri3_flags;
    pUserData->dwPasswordExpired = pUserInfo->usri3_password_expired;

    NetApiBufferFree(pUserInfo);

    UpdateUserOptions(hwndDlg, pUserData, TRUE);
}


static BOOL
SetGeneralUserData(HWND hwndDlg,
                   PGENERAL_USER_DATA pUserData)
{
    PUSER_INFO_3 pUserInfo = NULL;
    LPTSTR pszFullName = NULL;
    LPTSTR pszComment = NULL;
    NET_API_STATUS status;
    DWORD dwIndex;
    INT nLength;

    NetUserGetInfo(NULL, pUserData->szUserName, 3, (LPBYTE*)&pUserInfo);

    pUserInfo->usri3_flags =
        (pUserData->dwFlags & VALID_GENERAL_FLAGS) |
        (pUserInfo->usri3_flags & ~VALID_GENERAL_FLAGS);

    pUserInfo->usri3_password_expired = pUserData->dwPasswordExpired;

    nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_GENERAL_FULL_NAME));
    if (nLength == 0)
    {
        pUserInfo->usri3_full_name = NULL;
    }
    else
    {
        pszFullName = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
        GetDlgItemText(hwndDlg, IDC_USER_GENERAL_FULL_NAME, pszFullName, nLength + 1);
        pUserInfo->usri3_full_name = pszFullName;
    }

    nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_GENERAL_DESCRIPTION));
    if (nLength == 0)
    {
        pUserInfo->usri3_full_name = NULL;
    }
    else
    {
        pszComment = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
        GetDlgItemText(hwndDlg, IDC_USER_GENERAL_DESCRIPTION, pszComment, nLength + 1);
        pUserInfo->usri3_comment = pszComment;
    }

    status = NetUserSetInfo(NULL, pUserData->szUserName, 3, (LPBYTE)pUserInfo, &dwIndex);
    if (status != NERR_Success)
    {
        DebugPrintf(_T("Status: %lu  Index: %lu"), status, dwIndex);
    }

    if (pszFullName)
        HeapFree(GetProcessHeap(), 0, pszFullName);

    NetApiBufferFree(pUserInfo);

    return (status == NERR_Success);
}


INT_PTR CALLBACK
UserGeneralPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PGENERAL_USER_DATA pUserData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    pUserData= (PGENERAL_USER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pUserData = (PGENERAL_USER_DATA)HeapAlloc(GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof(GENERAL_USER_DATA) + 
                                                      lstrlen((LPTSTR)((PROPSHEETPAGE *)lParam)->lParam) * sizeof(TCHAR));
            lstrcpy(pUserData->szUserName, (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);

            GetGeneralUserData(hwndDlg,
                               pUserData);

            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pUserData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_GENERAL_FORCE_CHANGE:
                    pUserData->dwPasswordExpired = !pUserData->dwPasswordExpired;
                    UpdateUserOptions(hwndDlg, pUserData, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_USER_GENERAL_CANNOT_CHANGE:
                    pUserData->dwFlags ^= UF_PASSWD_CANT_CHANGE;
                    UpdateUserOptions(hwndDlg, pUserData, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_USER_GENERAL_NEVER_EXPIRES:
                    pUserData->dwFlags ^= UF_DONT_EXPIRE_PASSWD;
                    UpdateUserOptions(hwndDlg, pUserData, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_USER_GENERAL_DISABLED:
                    pUserData->dwFlags ^= UF_ACCOUNTDISABLE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_USER_GENERAL_LOCKED:
                    pUserData->dwFlags ^= UF_LOCKOUT;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPPSHNOTIFY)lParam)->hdr.code == PSN_APPLY)
            {
                SetGeneralUserData(hwndDlg, pUserData);
                return TRUE;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pUserData);
            break;
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPTSTR pszUser)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
    psp->lParam = (LPARAM)pszUser;
}


BOOL
UserProperties(HWND hwndDlg)
{
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;
    TCHAR szUserName[UNLEN];
    INT nItem;
    HWND hwndLV;

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return FALSE;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szUserName,
                         UNLEN);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.hIcon = NULL;
    psh.pszCaption = szUserName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_USER_GENERAL, (DLGPROC)UserGeneralPageProc, szUserName);
    InitPropSheetPage(&psp[1], IDD_USER_MEMBERSHIP, (DLGPROC)UserMembershipPageProc, szUserName);
    InitPropSheetPage(&psp[2], IDD_USER_PROFILE, (DLGPROC)UserProfilePageProc, szUserName);

    return (PropertySheet(&psh) == IDOK);
}
