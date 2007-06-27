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
           lvi.mask = LVIF_TEXT | LVIF_STATE; // | LVIF_PARAM;
//           lvi.lParam = (LPARAM)VarData;
           lvi.pszText = pBuffer[i].usri20_name;
           lvi.state = 0; //(i == 0) ? LVIS_SELECTED : 0;
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

    /* Set user environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_USERS_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetUsersListColumns(hwndListView);

    UpdateUsersList(hwndListView);

//    (void)ListView_SetColumnWidth(hwndListView, 3, LVSCW_AUTOSIZE_USEHEADER);
//    (void)ListView_Update(hwndListView, 0);
}


static VOID
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

                case NM_RCLICK:
                    ClientToScreen(GetDlgItem(hwndDlg, IDC_USERS_LIST), &lpnmlv->ptAction);
                    TrackPopupMenu(GetSubMenu(pUserData->hPopupMenu, (lpnmlv->iItem == -1) ? 0 : 1),
                                   TPM_LEFTALIGN, lpnmlv->ptAction.x, lpnmlv->ptAction.y, 0, hwndDlg, NULL);
                    break;
            }
            break;
    }
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
                case IDM_USER_PROPERTIES:
                    MessageBeep(-1);
                    break;
            }
            break;

        case WM_NOTIFY:
            OnNotify(hwndDlg, pUserData, (NMHDR *)lParam);
            break;

        case WM_DESTROY:
            DestroyMenu(pUserData->hPopupMenu);
            HeapFree(GetProcessHeap(), 0, pUserData);
            break;
    }

    return FALSE;
}
