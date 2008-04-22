/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/users.c
 * PURPOSE:         Users property page
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"


typedef struct _USER_DATA
{
    HMENU hPopupMenu;

    INT iCurrentItem;

} USER_DATA, *PUSER_DATA;



static BOOL
SetPassword(HWND hwndDlg)
{
    TCHAR szPassword1[256];
    TCHAR szPassword2[256];
    UINT uLen1;
    UINT uLen2;

    uLen1 = GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD1, szPassword1, 256);
    uLen2 = GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD2, szPassword2, 256);

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
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (SetPassword(hwndDlg))
                        EndDialog(hwndDlg, 0);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
            }
            break;

        default:
            return FALSE;
    }

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
           lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE; // | LVIF_PARAM;
//           lvi.lParam = (LPARAM)VarData;
           lvi.pszText = pBuffer[i].usri20_name;
           lvi.state = 0; //(i == 0) ? LVIS_SELECTED : 0;
           lvi.iImage = (pBuffer[i].usri20_flags & UF_ACCOUNTDISABLE) ? 1 : 0;
           iItem = ListView_InsertItem(hwndListView, &lvi);

           ListView_SetItemText(hwndListView, iItem, 1,
                                pBuffer[i].usri20_full_name);

           ListView_SetItemText(hwndListView, iItem, 2,
                                pBuffer[i].usri20_comment);
        }

        NetApiBufferFree(&pBuffer);

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
    hImgList = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 5, 5);
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

//    (void)ListView_SetColumnWidth(hwndListView, 3, LVSCW_AUTOSIZE_USEHEADER);
//    (void)ListView_Update(hwndListView, 0);
}


static BOOL
OnEndLabelEdit(LPNMLVDISPINFO pnmv)
{
    TCHAR szOldUserName[UNLEN];
    TCHAR szNewUserName[UNLEN];
    USER_INFO_0 useri0;
    NET_API_STATUS status;

    if (pnmv->item.iItem == -1)
        return FALSE;

    ListView_GetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szOldUserName,
                         UNLEN);
    lstrcpy(szNewUserName, pnmv->item.pszText);

    if (lstrcmp(szOldUserName, szNewUserName) == 0)
        return FALSE;

    useri0.usri0_name = szNewUserName;

#if 0
    status = NetUserSetInfo(NULL, szOldUserName, 0, (LPBYTE)&useri0, NULL);
#else
    status = NERR_Success;
#endif
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, _T("Error: %u"), status);
        MessageBox(NULL, szText, _T("NetUserSetInfo"), MB_ICONERROR | MB_OK);
        return FALSE;
    }

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
                    if (lpnmlv->iItem == -1)
                    {
                    }
                    else
                    {
                    }
                    break;

                case NM_DBLCLK:
                    break;

                case LVN_ENDLABELEDIT:
                    return OnEndLabelEdit((LPNMLVDISPINFO)phdr);

                case NM_RCLICK:
                    ClientToScreen(GetDlgItem(hwndDlg, IDC_USERS_LIST), &lpnmlv->ptAction);
                    TrackPopupMenu(GetSubMenu(pUserData->hPopupMenu, (lpnmlv->iItem == -1) ? 0 : 1),
                                   TPM_LEFTALIGN, lpnmlv->ptAction.x, lpnmlv->ptAction.y, 0, hwndDlg, NULL);
                    break;
            }
            break;
    }

    return FALSE;
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
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_USER_CHANGE_PASSWORD:
                    DialogBoxParam(hApplet,
                                   MAKEINTRESOURCE(IDD_CHANGE_PASSWORD),
                                   hwndDlg,
                                   ChangePasswordDlgProc,
                                   (LPARAM)NULL);
                    break;

                case IDM_USER_RENAME:
                    {
                        INT nItem;
                        HWND hwndLV;

                        hwndLV = GetDlgItem(hwndDlg, IDC_USERS_LIST);
                        nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
                        if (nItem != -1)
                        {
                            (void)ListView_EditLabel(hwndLV, nItem);
                        }
                    }
                    break;

                case IDM_USER_PROPERTIES:
                    MessageBeep(-1);
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
