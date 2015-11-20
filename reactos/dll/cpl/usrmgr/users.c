/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/users.c
 * PURPOSE:         Users property page
 *
 * PROGRAMMERS:     Eric Kohl
 */

/*
 * TODO:
 *  - Add new user to the users group.
 *  - Remove a user from all groups.
 *  - Use localized messages.
 */

#include "usrmgr.h"

typedef struct _USER_DATA
{
    HMENU hPopupMenu;

    INT iCurrentItem;

} USER_DATA, *PUSER_DATA;



static BOOL
CheckPasswords(HWND hwndDlg,
               INT nIdDlgItem1,
               INT nIdDlgItem2)
{
    TCHAR szPassword1[PWLEN];
    TCHAR szPassword2[PWLEN];
    UINT uLen1;
    UINT uLen2;

    uLen1 = GetDlgItemText(hwndDlg, nIdDlgItem1, szPassword1, PWLEN);
    uLen2 = GetDlgItemText(hwndDlg, nIdDlgItem2, szPassword2, PWLEN);

    /* Check the passwords */
    if (uLen1 != uLen2 || _tcscmp(szPassword1, szPassword2) != 0)
    {
        MessageBox(hwndDlg,
                   TEXT("The passwords you entered are not the same!"),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}


INT_PTR CALLBACK
ChangePasswordDlgProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    PUSER_INFO_1003 userInfo;
    INT nLength;

    UNREFERENCED_PARAMETER(wParam);

    userInfo = (PUSER_INFO_1003)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            userInfo = (PUSER_INFO_1003)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (CheckPasswords(hwndDlg, IDC_EDIT_PASSWORD1, IDC_EDIT_PASSWORD2))
                    {

                        /* Store the password */
                        nLength = SendDlgItemMessage(hwndDlg, IDC_EDIT_PASSWORD1, WM_GETTEXTLENGTH, 0, 0);
                        if (nLength > 0)
                        {
                            userInfo->usri1003_password = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                            GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD1, userInfo->usri1003_password, nLength + 1);
                        }

                        EndDialog(hwndDlg, IDOK);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


static VOID
UserChangePassword(HWND hwndDlg)
{
    TCHAR szUserName[UNLEN];
    USER_INFO_1003 user;
    INT nItem;
    HWND hwndLV;
    NET_API_STATUS status;

    ZeroMemory(&user, sizeof(USER_INFO_1003));

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szUserName,
                         UNLEN);

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_CHANGE_PASSWORD),
                       hwndDlg,
                       ChangePasswordDlgProc,
                       (LPARAM)&user) == IDOK)
    {
        status = NetUserSetInfo(NULL,
                                szUserName,
                                1003,
                                (LPBYTE)&user,
                                NULL);
        if (status != NERR_Success)
        {
            TCHAR szText[256];
            wsprintf(szText, TEXT("Error: %u"), status);
            MessageBox(NULL, szText, TEXT("NetUserSetInfo"), MB_ICONERROR | MB_OK);
        }
    }

    if (user.usri1003_password)
        HeapFree(GetProcessHeap(), 0, user.usri1003_password);
}


static VOID
UpdateUserOptions(HWND hwndDlg,
                  PUSER_INFO_3 userInfo,
                  BOOL bInit)
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_NEW_CANNOT_CHANGE),
                 !userInfo->usri3_password_expired);
    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_NEW_NEVER_EXPIRES),
                 !userInfo->usri3_password_expired);

    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_NEW_FORCE_CHANGE),
                 (userInfo->usri3_flags & (UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD)) == 0);

    if (bInit)
    {
        CheckDlgButton(hwndDlg, IDC_USER_NEW_FORCE_CHANGE,
                       userInfo->usri3_password_expired ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_NEW_CANNOT_CHANGE,
                       (userInfo->usri3_flags & UF_PASSWD_CANT_CHANGE) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_NEW_NEVER_EXPIRES,
                       (userInfo->usri3_flags & UF_DONT_EXPIRE_PASSWD) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_USER_NEW_DISABLED,
                       (userInfo->usri3_flags & UF_ACCOUNTDISABLE) ? BST_CHECKED : BST_UNCHECKED);
    }
}


INT_PTR CALLBACK
NewUserDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PUSER_INFO_3 userInfo;
    INT nLength;

    UNREFERENCED_PARAMETER(wParam);

    userInfo = (PUSER_INFO_3)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            userInfo = (PUSER_INFO_3)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            SendDlgItemMessage(hwndDlg, IDC_USER_NEW_NAME, EM_SETLIMITTEXT, 20, 0);
            UpdateUserOptions(hwndDlg, userInfo, TRUE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_NEW_NAME:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        nLength = SendDlgItemMessage(hwndDlg, IDC_USER_NEW_NAME, WM_GETTEXTLENGTH, 0, 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), (nLength > 0));
                    }
                    break;

                case IDC_USER_NEW_FORCE_CHANGE:
                    userInfo->usri3_password_expired = !userInfo->usri3_password_expired;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_NEW_CANNOT_CHANGE:
                    userInfo->usri3_flags ^= UF_PASSWD_CANT_CHANGE;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_NEW_NEVER_EXPIRES:
                    userInfo->usri3_flags ^= UF_DONT_EXPIRE_PASSWD;
                    UpdateUserOptions(hwndDlg, userInfo, FALSE);
                    break;

                case IDC_USER_NEW_DISABLED:
                    userInfo->usri3_flags ^= UF_ACCOUNTDISABLE;
                    break;

                case IDOK:
                    if (!CheckAccountName(hwndDlg, IDC_USER_NEW_NAME, NULL))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_USER_NEW_NAME));
                        SendDlgItemMessage(hwndDlg, IDC_USER_NEW_NAME, EM_SETSEL, 0, -1);
                        break;
                    }

                    if (!CheckPasswords(hwndDlg, IDC_USER_NEW_PASSWORD1, IDC_USER_NEW_PASSWORD2))
                    {
                        SetDlgItemText(hwndDlg, IDC_USER_NEW_PASSWORD1, TEXT(""));
                        SetDlgItemText(hwndDlg, IDC_USER_NEW_PASSWORD2, TEXT(""));
                        break;
                    }

                    /* Store the user name */
                    nLength = SendDlgItemMessage(hwndDlg, IDC_USER_NEW_NAME, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        userInfo->usri3_name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_USER_NEW_NAME, userInfo->usri3_name, nLength + 1);
                    }

                    /* Store the full user name */
                    nLength = SendDlgItemMessage(hwndDlg, IDC_USER_NEW_FULL_NAME, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        userInfo->usri3_full_name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_USER_NEW_FULL_NAME, userInfo->usri3_full_name, nLength + 1);
                    }

                    /* Store the description */
                    nLength = SendDlgItemMessage(hwndDlg, IDC_USER_NEW_DESCRIPTION, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        userInfo->usri3_comment = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_USER_NEW_DESCRIPTION, userInfo->usri3_comment, nLength + 1);
                    }

                    /* Store the password */
                    nLength = SendDlgItemMessage(hwndDlg, IDC_USER_NEW_PASSWORD1, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        userInfo->usri3_password = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_USER_NEW_PASSWORD1, userInfo->usri3_password, nLength + 1);
                    }

                    EndDialog(hwndDlg, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


static VOID
UserNew(HWND hwndDlg)
{
    USER_INFO_3 user;
    NET_API_STATUS status;
    LV_ITEM lvi;
    INT iItem;
    HWND hwndLV;

    ZeroMemory(&user, sizeof(USER_INFO_3));

    user.usri3_priv = USER_PRIV_USER;
    user.usri3_flags = UF_SCRIPT;
    user.usri3_acct_expires = TIMEQ_FOREVER;
    user.usri3_max_storage = USER_MAXSTORAGE_UNLIMITED;
    user.usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;

    user.usri3_password_expired = TRUE;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_USER_NEW),
                       hwndDlg,
                       NewUserDlgProc,
                       (LPARAM)&user) == IDOK)
    {
        status = NetUserAdd(NULL,
                            3,
                            (LPBYTE)&user,
                            NULL);
        if (status != NERR_Success)
        {
            TCHAR szText[256];
            wsprintf(szText, TEXT("Error: %u"), status);
            MessageBox(NULL, szText, TEXT("NetUserAdd"), MB_ICONERROR | MB_OK);
            return;
        }

        hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.pszText = user.usri3_name;
        lvi.state = 0;
        lvi.iImage = (user.usri3_flags & UF_ACCOUNTDISABLE) ? 1 : 0;
        iItem = ListView_InsertItem(hwndLV, &lvi);

        ListView_SetItemText(hwndLV, iItem, 1,
                             user.usri3_full_name);

        ListView_SetItemText(hwndLV, iItem, 2,
                             user.usri3_comment);
    }

    if (user.usri3_name)
        HeapFree(GetProcessHeap(), 0, user.usri3_name);

    if (user.usri3_full_name)
        HeapFree(GetProcessHeap(), 0, user.usri3_full_name);

    if (user.usri3_comment)
        HeapFree(GetProcessHeap(), 0, user.usri3_comment);

    if (user.usri3_password)
        HeapFree(GetProcessHeap(), 0, user.usri3_password);
}


static VOID
UserRename(HWND hwndDlg)
{
    HWND hwndLV;
    INT nItem;

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    if (hwndLV == NULL)
        return;

    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem != -1)
    {
        (void)ListView_EditLabel(hwndLV, nItem);
    }
}


static BOOL
UserDelete(HWND hwndDlg)
{
    TCHAR szUserName[UNLEN];
    TCHAR szText[256];
    INT nItem;
    HWND hwndLV;
    NET_API_STATUS status;

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return FALSE;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szUserName,
                         UNLEN);

    /* Display a warning message because the delete operation cannot be reverted */
    wsprintf(szText, TEXT("Do you really want to delete the user \"%s\"?"), szUserName);
    if (MessageBox(NULL, szText, TEXT("User Accounts"), MB_ICONWARNING | MB_YESNO) == IDNO)
        return FALSE;

    /* Delete the user */
    status = NetUserDel(NULL, szUserName);
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, TEXT("Error: %u"), status);
        MessageBox(NULL, szText, TEXT("NetUserDel"), MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* Delete the user from the list */
    (void)ListView_DeleteItem(hwndLV, nItem);

    return TRUE;
}


static VOID
SetUsersListColumns(HWND hwndListView)
{
    LV_COLUMN column;
    RECT rect;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    memset(&column, 0x00, sizeof(column));
    column.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt=LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.25);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_NAME, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.50);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_FULLNAME, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.25);
    column.iSubItem = 2;
    LoadString(hApplet, IDS_DESCRIPTION, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 2, &column);
}


static VOID
UpdateUsersList(HWND hwndListView)
{
    NET_API_STATUS netStatus;
    PUSER_INFO_20 pBuffer;
    DWORD entriesread;
    DWORD totalentries;
    DWORD resume_handle = 0;
    DWORD i;
    LV_ITEM lvi;
    INT iItem;

    for (;;)
    {
        netStatus = NetUserEnum(NULL, 20, FILTER_NORMAL_ACCOUNT,
                                (LPBYTE*)&pBuffer,
                                1024, &entriesread,
                                &totalentries, &resume_handle);
        if (netStatus != NERR_Success && netStatus != ERROR_MORE_DATA)
            break;

        for (i = 0; i < entriesread; i++)
        {
           memset(&lvi, 0x00, sizeof(lvi));
           lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
           lvi.pszText = pBuffer[i].usri20_name;
           lvi.state = 0;
           lvi.iImage = (pBuffer[i].usri20_flags & UF_ACCOUNTDISABLE) ? 1 : 0;
           iItem = ListView_InsertItem(hwndListView, &lvi);

           if (pBuffer[i].usri20_full_name != NULL)
               ListView_SetItemText(hwndListView, iItem, 1,
                                    pBuffer[i].usri20_full_name);

           if (pBuffer[i].usri20_comment != NULL)
               ListView_SetItemText(hwndListView, iItem, 2,
                                    pBuffer[i].usri20_comment);
        }

        NetApiBufferFree(pBuffer);

        /* No more data left */
        if (netStatus != ERROR_MORE_DATA)
            break;
    }
}


static VOID
OnInitDialog(HWND hwndDlg)
{
    HWND hwndListView;
    HIMAGELIST hImgList;
    HICON hIcon;

    /* Create the image list */
    hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 5, 5);
    hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_USER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList, hIcon);
    hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_LOCKED_USER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList, hIcon);
    DestroyIcon(hIcon);

    hwndListView = GetDlgItem(hwndDlg, IDC_USERS_LIST);

    (VOID)ListView_SetImageList(hwndListView, hImgList, LVSIL_SMALL);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetUsersListColumns(hwndListView);

    UpdateUsersList(hwndListView);
}


static BOOL
OnBeginLabelEdit(LPNMLVDISPINFO pnmv)
{
    HWND hwndEdit;

    hwndEdit = ListView_GetEditControl(pnmv->hdr.hwndFrom);
    if (hwndEdit == NULL)
        return TRUE;

    SendMessage(hwndEdit, EM_SETLIMITTEXT, 20, 0);

    return FALSE;
}


static BOOL
OnEndLabelEdit(LPNMLVDISPINFO pnmv)
{
    TCHAR szOldUserName[UNLEN];
    TCHAR szNewUserName[UNLEN];
    USER_INFO_0 useri0;
    NET_API_STATUS status;

    /* Leave, if there is no valid listview item */
    if (pnmv->item.iItem == -1)
        return FALSE;

    /* Get the new user name */
    ListView_GetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szOldUserName,
                         UNLEN);

    /* Leave, if the user canceled the edit action */
    if (pnmv->item.pszText == NULL)
        return FALSE;

    /* Get the new user name */
    lstrcpy(szNewUserName, pnmv->item.pszText);

    /* Leave, if the user name was not changed */
    if (lstrcmp(szOldUserName, szNewUserName) == 0)
        return FALSE;

    /* Check the user name for illegal characters */
    if (!CheckAccountName(NULL, 0, szNewUserName))
        return FALSE;

    /* Change the user name */
    useri0.usri0_name = szNewUserName;

    status = NetUserSetInfo(NULL, szOldUserName, 0, (LPBYTE)&useri0, NULL);
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, TEXT("Error: %u"), status);
        MessageBox(NULL, szText, TEXT("NetUserSetInfo"), MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* Update the listview item */
    ListView_SetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szNewUserName);

    return TRUE;
}


static BOOL
OnNotify(HWND hwndDlg, PUSER_DATA pUserData, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_USERS_LIST:
            switch(phdr->code)
            {
                case NM_CLICK:
                    pUserData->iCurrentItem = lpnmlv->iItem;
                    break;

                case NM_DBLCLK:
                    if (lpnmlv->iItem != -1)
                    {
                        UINT uItem;

                        uItem =  GetMenuDefaultItem(GetSubMenu(pUserData->hPopupMenu, 1),
                                                    FALSE, 0);
                        if (uItem != (UINT)-1)
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(uItem, 0), 0);
                    }
                    break;

                case NM_RCLICK:
                    ClientToScreen(GetDlgItem(hwndDlg, IDC_USERS_LIST), &lpnmlv->ptAction);
                    TrackPopupMenu(GetSubMenu(pUserData->hPopupMenu, (lpnmlv->iItem == -1) ? 0 : 1),
                                   TPM_LEFTALIGN, lpnmlv->ptAction.x, lpnmlv->ptAction.y, 0, hwndDlg, NULL);
                    break;

                case LVN_BEGINLABELEDIT:
                    return OnBeginLabelEdit((LPNMLVDISPINFO)phdr);

                case LVN_ENDLABELEDIT:
                    return OnEndLabelEdit((LPNMLVDISPINFO)phdr);
            }
            break;
    }

    return FALSE;
}


static VOID
UpdateUserProperties(HWND hwndDlg)
{
    TCHAR szUserName[UNLEN];
    INT iItem;
    HWND hwndLV;
    PUSER_INFO_2 pUserInfo = NULL;
    LV_ITEM lvi;

    hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
    iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (iItem == -1)
        return;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         iItem, 0,
                         szUserName,
                         UNLEN);

    NetUserGetInfo(NULL, szUserName, 2, (LPBYTE*)&pUserInfo);

    memset(&lvi, 0x00, sizeof(lvi));
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_IMAGE;
    lvi.iImage = (pUserInfo->usri2_flags & UF_ACCOUNTDISABLE) ? 1 : 0;
    (void)ListView_SetItem(hwndLV, &lvi);

    ListView_SetItemText(hwndLV, iItem, 1,
                         pUserInfo->usri2_full_name);

    ListView_SetItemText(hwndLV, iItem, 2,
                         pUserInfo->usri2_comment);

    NetApiBufferFree(pUserInfo);
}


INT_PTR CALLBACK
UsersPageProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    PUSER_DATA pUserData;

    UNREFERENCED_PARAMETER(wParam);

    pUserData = (PUSER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pUserData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USER_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pUserData);

            pUserData->hPopupMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDM_POPUP_USER));

            OnInitDialog(hwndDlg);
            SetMenuDefaultItem(GetSubMenu(pUserData->hPopupMenu, 1),
                               IDM_USER_PROPERTIES,
                               FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_USER_CHANGE_PASSWORD:
                    UserChangePassword(hwndDlg);
                    break;

                case IDM_USER_RENAME:
                    UserRename(hwndDlg);
                    break;

                case IDM_USER_NEW:
                case IDC_USERS_ADD:
                    UserNew(hwndDlg);
                    break;

                case IDM_USER_DELETE:
                case IDC_USERS_REMOVE:
                    UserDelete(hwndDlg);
                    break;

                case IDM_USER_PROPERTIES:
                case IDC_USERS_PROPERTIES:
                    if (UserProperties(hwndDlg))
                    {
                        UpdateUserProperties(hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            return OnNotify(hwndDlg, pUserData, (NMHDR *)lParam);

        case WM_DESTROY:
            DestroyMenu(pUserData->hPopupMenu);
            HeapFree(GetProcessHeap(), 0, pUserData);
            break;
    }

    return FALSE;
}
