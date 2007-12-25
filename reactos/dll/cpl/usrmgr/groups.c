/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/groups.c
 * PURPOSE:         Groups property page
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"

typedef struct _GROUP_DATA
{
    HMENU hPopupMenu;

    INT iCurrentItem;

} GROUP_DATA, *PGROUP_DATA;


static VOID
SetGroupsListColumns(HWND hwndListView)
{
    LV_COLUMN column;
    RECT rect;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    memset(&column, 0x00, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.40);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_NAME, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.60);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_DESCRIPTION, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);
}


static VOID
UpdateGroupsList(HWND hwndListView)
{
    NET_API_STATUS netStatus;
    PLOCALGROUP_INFO_1 pBuffer;
    DWORD entriesread;
    DWORD totalentries;
    DWORD resume_handle = 0;
    DWORD i;

    LV_ITEM lvi;
    INT iItem;


    for (;;)
    {
        netStatus = NetLocalGroupEnum(NULL, 1, (LPBYTE*)&pBuffer,
                                      1024, &entriesread,
                                      &totalentries, &resume_handle);
        if (netStatus != NERR_Success && netStatus != ERROR_MORE_DATA)
            break;

        for (i = 0; i < entriesread; i++)
        {
           memset(&lvi, 0x00, sizeof(lvi));
           lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE; // | LVIF_PARAM;
//           lvi.lParam = (LPARAM)VarData;
           lvi.pszText = pBuffer[i].lgrpi1_name;
           lvi.state = 0; //(i == 0) ? LVIS_SELECTED : 0;
           lvi.iImage = 0;
           iItem = ListView_InsertItem(hwndListView, &lvi);

           ListView_SetItemText(hwndListView, iItem, 1,
                                pBuffer[i].lgrpi1_comment);
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
    hImgList = ImageList_Create(16,16,ILC_COLOR8 | ILC_MASK,5,5);
    hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_GROUP),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList,hIcon);
    DestroyIcon(hIcon);

    hwndListView = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);

    (VOID)ListView_SetImageList(hwndListView, hImgList, LVSIL_SMALL);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetGroupsListColumns(hwndListView);

    UpdateGroupsList(hwndListView);

//    (void)ListView_SetColumnWidth(hwndListView, 3, LVSCW_AUTOSIZE_USEHEADER);
//    (void)ListView_Update(hwndListView, 0);
}


static VOID
OnNotify(HWND hwndDlg, PGROUP_DATA pGroupData, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_GROUPS_LIST:
            switch(phdr->code)
            {
                case NM_CLICK:
                    pGroupData->iCurrentItem = lpnmlv->iItem;
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
                    ClientToScreen(GetDlgItem(hwndDlg, IDC_GROUPS_LIST), &lpnmlv->ptAction);
                    TrackPopupMenu(GetSubMenu(pGroupData->hPopupMenu, (lpnmlv->iItem == -1) ? 0 : 1),
                                   TPM_LEFTALIGN, lpnmlv->ptAction.x, lpnmlv->ptAction.y, 0, hwndDlg, NULL);
                    break;
            }
            break;
    }
}


INT_PTR CALLBACK
GroupsPageProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PGROUP_DATA pGroupData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);


    pGroupData = (PGROUP_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGroupData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GROUP_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGroupData);

            pGroupData->hPopupMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDM_POPUP_GROUP));

            OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            OnNotify(hwndDlg, pGroupData, (NMHDR *)lParam);
            break;

        case WM_DESTROY:
            DestroyMenu(pGroupData->hPopupMenu);
            HeapFree(GetProcessHeap(), 0, pGroupData);
            break;
    }

    return FALSE;
}
