/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/users.c
 * PURPOSE:         User property sheet
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"

static VOID
UpdateUserOptions(HWND hwndDlg,
                  PUSER_INFO_3 userInfo,
                  BOOL bInit)
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_CANNOT_CHANGE),
                 !userInfo->usri3_password_expired);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_NEVER_EXPIRES),
                 !userInfo->usri3_password_expired);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_GENERAL_FORCE_CHANGE),
                 (userInfo->usri3_flags & (UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD)) == 0);

    if (bInit)
    {
        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_FORCE_CHANGE,
                       userInfo->usri3_password_expired ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_CANNOT_CHANGE,
                       (userInfo->usri3_flags & UF_PASSWD_CANT_CHANGE) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_NEVER_EXPIRES,
                       (userInfo->usri3_flags & UF_DONT_EXPIRE_PASSWD) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_GENERAL_DISABLED,
                       (userInfo->usri3_flags & UF_ACCOUNTDISABLE) ? BST_CHECKED : BST_UNCHECKED);
    }
}


static VOID
GetUserData(HWND hwndDlg, LPTSTR lpUserName, PUSER_INFO_3 *usrInfo)
{
    PUSER_INFO_3 userInfo = NULL;

    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_NAME, lpUserName);

    NetUserGetInfo(NULL, lpUserName, 3, (LPBYTE*)&userInfo);

    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_FULL_NAME, userInfo->usri3_full_name);
    SetDlgItemText(hwndDlg, IDC_USER_GENERAL_DESCRIPTION, userInfo->usri3_comment);

    UpdateUserOptions(hwndDlg, userInfo, TRUE);

    *usrInfo = userInfo;
}


INT_PTR CALLBACK
UserGeneralPageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PUSER_INFO_3 userInfo;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    userInfo = (PUSER_INFO_3)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            GetUserData(hwndDlg,
                        (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam,
                        &userInfo);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)userInfo);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_GENERAL_FORCE_CHANGE:
                    userInfo->usri3_password_expired = !userInfo->usri3_password_expired;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_GENERAL_CANNOT_CHANGE:
                    userInfo->usri3_flags ^= UF_PASSWD_CANT_CHANGE;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_GENERAL_NEVER_EXPIRES:
                    userInfo->usri3_flags ^= UF_DONT_EXPIRE_PASSWD;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_GENERAL_DISABLED:
                    userInfo->usri3_flags ^= UF_ACCOUNTDISABLE;
                    break;

                case IDC_USER_GENERAL_LOCKED:
                    break;
            }
            break;

        case WM_DESTROY:
            NetApiBufferFree(userInfo);
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


VOID
UserProperties(HWND hwndDlg)
{
    PROPSHEETPAGE psp[1];
    PROPSHEETHEADER psh;
    TCHAR szUserName[UNLEN];
    INT nItem;
    HWND hwndLV;

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return;

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
//    InitPropSheetPage(&psp[1], IDD_USER_MEMBERSHIP, (DLGPROC)UserMembershipPageProc);
//    InitPropSheetPage(&psp[2], IDD_USER_PROFILE, (DLGPROC)UserProfilePageProc);

    PropertySheet(&psh);
}
