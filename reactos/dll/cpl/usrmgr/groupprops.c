/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/groupprops.c
 * PURPOSE:         Group property sheet
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"

typedef struct _GENERAL_GROUP_DATA
{
    TCHAR szGroupName[1];
} GENERAL_GROUP_DATA, *PGENERAL_GROUP_DATA;


static VOID
GetTextSid(PSID pSid,
           LPTSTR pTextSid)
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev = SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    psia = GetSidIdentifierAuthority(pSid);

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    dwSidSize = wsprintf(pTextSid, TEXT("S-%lu-"), dwSidRev);

    if ((psia->Value[0] != 0) || (psia->Value[1] != 0))
    {
        dwSidSize += wsprintf(pTextSid + lstrlen(pTextSid),
                              TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                              (USHORT)psia->Value[0],
                              (USHORT)psia->Value[1],
                              (USHORT)psia->Value[2],
                              (USHORT)psia->Value[3],
                              (USHORT)psia->Value[4],
                              (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize += wsprintf(pTextSid + lstrlen(pTextSid),
                              TEXT("%lu"),
                              (ULONG)(psia->Value[5]) +
                              (ULONG)(psia->Value[4] <<  8) +
                              (ULONG)(psia->Value[3] << 16) +
                              (ULONG)(psia->Value[2] << 24));
    }

    for (dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize += wsprintf(pTextSid + dwSidSize, TEXT("-%lu"),
                              *GetSidSubAuthority(pSid, dwCounter));
    }
}


static VOID
RemoveUserFromGroup(HWND hwndDlg,
                    PGENERAL_GROUP_DATA pGroupData)
{
    TCHAR szUserName[UNLEN];
    TCHAR szText[256];
    LOCALGROUP_MEMBERS_INFO_3 memberInfo;
    HWND hwndLV;
    INT nItem;
    NET_API_STATUS status;

    hwndLV = GetDlgItem(hwndDlg, IDC_GROUP_GENERAL_MEMBERS);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szUserName,
                         UNLEN);

    /* Display a warning message because the remove operation cannot be reverted */
    wsprintf(szText, TEXT("Do you really want to remove the user \"%s\" from the group \"%s\"?"),
             szUserName, pGroupData->szGroupName);
    if (MessageBox(NULL, szText, TEXT("User Accounts"), MB_ICONWARNING | MB_YESNO) == IDNO)
        return;

    memberInfo.lgrmi3_domainandname = szUserName;

    status = NetLocalGroupDelMembers(NULL, pGroupData->szGroupName,
                                     3, (LPBYTE)&memberInfo, 1);
    if (status != NERR_Success)
    {
        TCHAR szText[256];
        wsprintf(szText, TEXT("Error: %u"), status);
        MessageBox(NULL, szText, TEXT("NetLocalGroupDelMembers"), MB_ICONERROR | MB_OK);
        return;
    }

    (void)ListView_DeleteItem(hwndLV, nItem);

    if (ListView_GetItemCount(hwndLV) == 0)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GROUP_GENERAL_REMOVE), FALSE);
}


static BOOL
OnNotify(HWND hwndDlg,
         PGENERAL_GROUP_DATA pGroupData,
         LPARAM lParam)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;

    switch (((LPNMHDR)lParam)->idFrom)
    {
        case IDC_GROUP_GENERAL_MEMBERS:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GROUP_GENERAL_REMOVE), (lpnmlv->iItem != -1));
                    break;

                case LVN_KEYDOWN:
                    if (((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE)
                    {
                        RemoveUserFromGroup(hwndDlg, pGroupData);
                    }
                    break;

            }
            break;
    }

    return FALSE;
}


static VOID
GetGeneralGroupData(HWND hwndDlg,
                    PGENERAL_GROUP_DATA pGroupData)
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
    TCHAR szGroupName[256];


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
    SetDlgItemText(hwndDlg, IDC_GROUP_GENERAL_NAME, pGroupData->szGroupName);

    /* Set group description */
    NetLocalGroupGetInfo(NULL, pGroupData->szGroupName, 1, (LPBYTE*)&groupInfo);
    SetDlgItemText(hwndDlg, IDC_GROUP_GENERAL_DESCRIPTION, groupInfo->lgrpi1_comment);
    NetApiBufferFree(groupInfo);

    /* Set group members */
    NetLocalGroupGetMembers(NULL, pGroupData->szGroupName, 1, (LPBYTE*)&membersInfo,
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

        if (membersInfo[i].lgrmi1_sidusage == SidTypeWellKnownGroup)
        {
            TCHAR szSid[256];

            GetTextSid(membersInfo[i].lgrmi1_sid, szSid);

            wsprintf(szGroupName,
                     TEXT("%s\\%s (%s)"),
                     membersInfo[i].lgrmi1_name,
                     szSid);

            lvi.pszText = szGroupName;
        }

        (void)ListView_InsertItem(hwndLV, &lvi);
    }

    NetApiBufferFree(membersInfo);
}


static BOOL
SetGeneralGroupData(HWND hwndDlg,
                    PGENERAL_GROUP_DATA pGroupData)
{
    LOCALGROUP_INFO_1 groupInfo;
    LPTSTR pszComment = NULL;
    INT nLength;
    NET_API_STATUS status;
    DWORD dwIndex;

    /* Get the group description */
    nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GROUP_GENERAL_DESCRIPTION));
    if (nLength == 0)
    {
        groupInfo.lgrpi1_comment = NULL;
    }
    else
    {
        pszComment = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
        GetDlgItemText(hwndDlg, IDC_GROUP_GENERAL_DESCRIPTION, pszComment, nLength + 1);
        groupInfo.lgrpi1_comment = pszComment;
    }

    status = NetLocalGroupSetInfo(NULL, pGroupData->szGroupName, 1, (LPBYTE)&groupInfo, &dwIndex);
    if (status != NERR_Success)
    {
        DebugPrintf(_T("Status: %lu  Index: %lu"), status, dwIndex);
    }

    if (pszComment)
        HeapFree(GetProcessHeap(), 0, pszComment);

    return TRUE;
}


INT_PTR CALLBACK
GroupGeneralPageProc(HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    PGENERAL_GROUP_DATA pGroupData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    pGroupData= (PGENERAL_GROUP_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGroupData = (PGENERAL_GROUP_DATA)HeapAlloc(GetProcessHeap(),
                                                        HEAP_ZERO_MEMORY,
                                                        sizeof(GENERAL_GROUP_DATA) + 
                                                        lstrlen((LPTSTR)((PROPSHEETPAGE *)lParam)->lParam) * sizeof(TCHAR));
            lstrcpy(pGroupData->szGroupName, (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);

            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pGroupData);

            GetGeneralGroupData(hwndDlg,
                                pGroupData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_GROUP_GENERAL_DESCRIPTION:
                    if (HIWORD(wParam) == EN_CHANGE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_GROUP_GENERAL_REMOVE:
                    RemoveUserFromGroup(hwndDlg, pGroupData);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPPSHNOTIFY)lParam)->hdr.code == PSN_APPLY)
            {
                SetGeneralGroupData(hwndDlg, pGroupData);
                return TRUE;
            }
            else
            {
                return OnNotify(hwndDlg, pGroupData, lParam);
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGroupData);
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


BOOL
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
        return FALSE;

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

    return (PropertySheet(&psh) == IDOK);
}
