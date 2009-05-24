/*
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
    DWORD_PTR resume_handle = 0;
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
           lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
           lvi.pszText = pBuffer[i].lgrpi1_name;
           lvi.state = 0;
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
UpdateGroupProperties(HWND hwndDlg)
{
    TCHAR szGroupName[UNLEN];
    INT iItem;
    HWND hwndLV;
    NET_API_STATUS status;
    PLOCALGROUP_INFO_1 pGroupInfo = NULL;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);
    iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (iItem == -1)
        return;

    /* Get the group name */
    ListView_GetItemText(hwndLV,
                         iItem, 0,
                         szGroupName,
                         UNLEN);

    status = NetLocalGroupGetInfo(NULL, szGroupName, 1, (LPBYTE*)&pGroupInfo);

    ListView_SetItemText(hwndLV, iItem, 1,
                         pGroupInfo->lgrpi1_comment);

    NetApiBufferFree(pGroupInfo);
}


INT_PTR CALLBACK
NewGroupDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PLOCALGROUP_INFO_1 groupInfo;
    INT nLength;

    UNREFERENCED_PARAMETER(wParam);

    groupInfo = (PLOCALGROUP_INFO_1)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            groupInfo = (PLOCALGROUP_INFO_1)lParam;
            SendDlgItemMessage(hwndDlg, IDC_GROUP_NEW_NAME, EM_SETLIMITTEXT, 20, 0);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_GROUP_NEW_NAME:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        nLength = SendDlgItemMessage(hwndDlg, IDC_GROUP_NEW_NAME, WM_GETTEXTLENGTH, 0, 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), (nLength > 0));
                    }
                    break;

                case IDOK:
                    if (!CheckAccountName(hwndDlg, IDC_GROUP_NEW_NAME, NULL))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_GROUP_NEW_NAME));
                        SendDlgItemMessage(hwndDlg, IDC_GROUP_NEW_NAME, EM_SETSEL, 0, -1);
                        break;
                    }

                    nLength = SendDlgItemMessage(hwndDlg, IDC_GROUP_NEW_NAME, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        groupInfo->lgrpi1_name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_GROUP_NEW_NAME, groupInfo->lgrpi1_name, nLength + 1);
                    }

                    nLength = SendDlgItemMessage(hwndDlg, IDC_GROUP_NEW_DESCRIPTION, WM_GETTEXTLENGTH, 0, 0);
                    if (nLength > 0)
                    {
                        groupInfo->lgrpi1_comment = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
                        GetDlgItemText(hwndDlg, IDC_GROUP_NEW_DESCRIPTION, groupInfo->lgrpi1_comment, nLength + 1);
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
GroupNew(HWND hwndDlg)
{
    NET_API_STATUS status;
    LOCALGROUP_INFO_1 group;
    LV_ITEM lvi;
    INT iItem;
    HWND hwndLV;

    ZeroMemory(&group, sizeof(LOCALGROUP_INFO_1));

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_GROUP_NEW),
                       hwndDlg,
                       NewGroupDlgProc,
                       (LPARAM)&group) == IDOK)
    {
#if 0
        status = NetLocalGroupAdd(NULL,
                                  1,
                                  (LPBYTE)&group,
                                  NULL);
#else
        status = NERR_Success;
#endif
        if (status != NERR_Success)
        {
            TCHAR szText[256];
            wsprintf(szText, TEXT("Error: %u"), status);
            MessageBox(NULL, szText, TEXT("NetUserAdd"), MB_ICONERROR | MB_OK);
            return;
        }

        hwndLV = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.pszText = group.lgrpi1_name;
        lvi.state = 0;
        lvi.iImage = 0;
        iItem = ListView_InsertItem(hwndLV, &lvi);

        ListView_SetItemText(hwndLV, iItem, 1,
                             group.lgrpi1_comment);
    }

    if (group.lgrpi1_name)
        HeapFree(GetProcessHeap, 0, group.lgrpi1_name);

    if (group.lgrpi1_comment)
        HeapFree(GetProcessHeap, 0, group.lgrpi1_comment);
}


static VOID
GroupRename(HWND hwndDlg)
{
    INT nItem;
    HWND hwndLV;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem != -1)
    {
        (void)ListView_EditLabel(hwndLV, nItem);
    }
}


static BOOL
GroupDelete(HWND hwndDlg)
{
    TCHAR szGroupName[UNLEN];
    TCHAR szText[256];
    INT nItem;
    HWND hwndLV;
    NET_API_STATUS status;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return FALSE;

    /* Get the new group name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szGroupName,
                         UNLEN);

    /* Display a warning message because the delete operation cannot be reverted */
    wsprintf(szText, TEXT("Dou you really want to delete the user group \"%s\"?"), szGroupName);
    if (MessageBox(NULL, szText, TEXT("User Groups"), MB_ICONWARNING | MB_YESNO) == IDNO)
        return FALSE;

    /* Delete the group */
#if 0
    status = NetLocalGroupDel(NULL, szGroupName);
#else
    status = NERR_Success;
#endif
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, TEXT("Error: %u"), status);
        MessageBox(NULL, szText, TEXT("NetLocalGroupDel"), MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* Delete the group from the list */
    (void)ListView_DeleteItem(hwndLV, nItem);

    return TRUE;
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
    TCHAR szOldGroupName[UNLEN];
    TCHAR szNewGroupName[UNLEN];
    LOCALGROUP_INFO_0 lgrpi0;
    NET_API_STATUS status;

    /* Leave, if there is no valid listview item */
    if (pnmv->item.iItem == -1)
        return FALSE;

    /* Get the new user name */
    ListView_GetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szOldGroupName,
                         UNLEN);

    /* Leave, if the user canceled the edit action */
    if (pnmv->item.pszText == NULL)
        return FALSE;

    /* Get the new user name */
    lstrcpy(szNewGroupName, pnmv->item.pszText);

    /* Leave, if the user name was not changed */
    if (lstrcmp(szOldGroupName, szNewGroupName) == 0)
        return FALSE;

    /* Check the group name for illegal characters */
    if (!CheckAccountName(NULL, 0, szNewGroupName))
        return FALSE;

    /* Change the user name */
    lgrpi0.lgrpi0_name = szNewGroupName;

#if 0
    status = NetLocalGroupSetInfo(NULL, szOldGroupName, 0, (LPBYTE)&lgrpi0, NULL);
#else
    status = NERR_Success;
#endif
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, TEXT("Error: %u"), status);
        MessageBox(NULL, szText, TEXT("NetLocalGroupSetInfo"), MB_ICONERROR | MB_OK);
        return FALSE;
    }

    /* Update the listview item */
    ListView_SetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szNewGroupName);

    return TRUE;
}


static BOOL
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
                    break;

                case NM_DBLCLK:
                    if (lpnmlv->iItem != -1)
                    {
                        UINT uItem;

                        uItem =  GetMenuDefaultItem(GetSubMenu(pGroupData->hPopupMenu, 1),
                                                    FALSE, 0);
                        if (uItem != (UINT)-1)
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(uItem, 0), 0);
                    }
                    break;

                case NM_RCLICK:
                    ClientToScreen(GetDlgItem(hwndDlg, IDC_GROUPS_LIST), &lpnmlv->ptAction);
                    TrackPopupMenu(GetSubMenu(pGroupData->hPopupMenu, (lpnmlv->iItem == -1) ? 0 : 1),
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
            SetMenuDefaultItem(GetSubMenu(pGroupData->hPopupMenu, 1),
                               IDM_GROUP_PROPERTIES,
                               FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_GROUP_NEW:
                    GroupNew(hwndDlg);
                    break;

                case IDM_GROUP_RENAME:
                    GroupRename(hwndDlg);
                    break;

                case IDM_GROUP_DELETE:
                    GroupDelete(hwndDlg);
                    break;

                case IDM_GROUP_PROPERTIES:
                    if (GroupProperties(hwndDlg) == IDOK)
                        UpdateGroupProperties(hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            return OnNotify(hwndDlg, pGroupData, (NMHDR *)lParam);

        case WM_DESTROY:
            DestroyMenu(pGroupData->hPopupMenu);
            HeapFree(GetProcessHeap(), 0, pGroupData);
            break;
    }

    return FALSE;
}
