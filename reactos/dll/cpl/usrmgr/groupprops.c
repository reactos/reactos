/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/groupprops.c
 * PURPOSE:         Group property sheet
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"


static VOID
GetGroupData(HWND hwndDlg, LPTSTR lpGroupName)
{
    PLOCALGROUP_INFO_1 groupInfo = NULL;
    PLOCALGROUP_MEMBERS_INFO_1 membersInfo = NULL;
    DWORD dwRead;
    DWORD dwTotal;
    DWORD_PTR resumeHandle = 0;
    DWORD i;
    LV_ITEM lvi;
    HWND hwndLV;
    LV_COLUMN column;
    RECT rect;
    HIMAGELIST hImgList;
    HICON hIcon;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUP_GENERAL_MEMBERS);

    /* Create the image list */
    hImgList = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 5, 5);
    hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_USER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList, hIcon);
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

    /* Set group name */
    SetDlgItemText(hwndDlg, IDC_GROUP_GENERAL_NAME, lpGroupName);

    /* Set group description */
    NetLocalGroupGetInfo(NULL, lpGroupName, 1, (LPBYTE*)&groupInfo);
    SetDlgItemText(hwndDlg, IDC_GROUP_GENERAL_DESCRIPTION, groupInfo->lgrpi1_comment);
    NetApiBufferFree(groupInfo);

    /* Set group members */
    NetLocalGroupGetMembers(NULL, lpGroupName, 1, (LPBYTE*)&membersInfo,
                            MAX_PREFERRED_LENGTH, &dwRead, &dwTotal,
                            &resumeHandle);

    for (i = 0; i < dwRead; i++)
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.pszText = membersInfo[i].lgrmi1_name;
        lvi.state = 0;
        lvi.iImage = (membersInfo[i].lgrmi1_sidusage == SidTypeGroup ||
                      membersInfo[i].lgrmi1_sidusage == SidTypeWellKnownGroup) ? 1 : 0;

        (void)ListView_InsertItem(hwndLV, &lvi);
    }

    NetApiBufferFree(membersInfo);
}


INT_PTR CALLBACK
GroupGeneralPageProc(HWND hwndDlg,
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
            GetGroupData(hwndDlg,
                         (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);
            break;

        case WM_COMMAND:
            break;

        case WM_DESTROY:
            break;
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPTSTR pszGroup)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
    psp->lParam = (LPARAM)pszGroup;
}


VOID
GroupProperties(HWND hwndDlg)
{
    PROPSHEETPAGE psp[1];
    PROPSHEETHEADER psh;
    TCHAR szGroupName[UNLEN];
    INT nItem;
    HWND hwndLV;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUPS_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szGroupName,
                         UNLEN);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.hIcon = NULL;
    psh.pszCaption = szGroupName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_GROUP_GENERAL, (DLGPROC)GroupGeneralPageProc, szGroupName);

    PropertySheet(&psh);
}
