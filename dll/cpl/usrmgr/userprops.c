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

typedef struct _PROFILE_USER_DATA
{
    TCHAR szUserName[1];
} PROFILE_USER_DATA, *PPROFILE_USER_DATA;

typedef struct _MEMBERSHIP_USER_DATA
{
    PLOCALGROUP_USERS_INFO_0 pGroupData;
    DWORD dwGroupCount;
    TCHAR szUserName[1];
} MEMBERSHIP_USER_DATA, *PMEMBERSHIP_USER_DATA;


static VOID
GetUserProfileData(HWND hwndDlg,
                   PPROFILE_USER_DATA pUserData)
{
    PUSER_INFO_3 userInfo = NULL;
    NET_API_STATUS status;
    BOOL bLocal;
    TCHAR szDrive[8];
    INT i;
    INT nSel;

    status = NetUserGetInfo(NULL, pUserData->szUserName, 3, (LPBYTE*)&userInfo);
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


static BOOL
SetUserProfileData(HWND hwndDlg,
                   PPROFILE_USER_DATA pUserData)
{
    PUSER_INFO_3 pUserInfo = NULL;
    LPTSTR pszProfilePath = NULL;
    LPTSTR pszScriptPath = NULL;
    LPTSTR pszHomeDir = NULL;
    LPTSTR pszHomeDrive = NULL;
    NET_API_STATUS status;
    DWORD dwIndex;
    INT nLength;
    INT nIndex;

    NetUserGetInfo(NULL, pUserData->szUserName, 3, (LPBYTE*)&pUserInfo);

    /* Get the profile path */
    nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_PROFILE_PATH));
    if (nLength == 0)
    {
        pUserInfo->usri3_profile = NULL;
    }
    else
    {
        pszProfilePath = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
        GetDlgItemText(hwndDlg, IDC_USER_PROFILE_PATH, pszProfilePath, nLength + 1);
        pUserInfo->usri3_profile = pszProfilePath;
    }

    /* Get the script path */
    nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_PROFILE_SCRIPT));
    if (nLength == 0)
    {
        pUserInfo->usri3_script_path = NULL;
    }
    else
    {
        pszScriptPath = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
        GetDlgItemText(hwndDlg, IDC_USER_PROFILE_SCRIPT, pszScriptPath, nLength + 1);
        pUserInfo->usri3_script_path = pszScriptPath;
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_USER_PROFILE_LOCAL) == BST_CHECKED)
    {
        /* Local home directory */
        nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_PROFILE_LOCAL_PATH));
        if (nLength == 0)
        {
            pUserInfo->usri3_home_dir = NULL;
        }
        else
        {
            pszHomeDir = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
            GetDlgItemText(hwndDlg, IDC_USER_PROFILE_LOCAL_PATH, pszHomeDir, nLength + 1);
            pUserInfo->usri3_home_dir = pszHomeDir;
        }
    }
    else
    {
        /* Remote home directory */
        nLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH));
        if (nLength == 0)
        {
            pUserInfo->usri3_home_dir = NULL;
        }
        else
        {
            pszHomeDir = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
            GetDlgItemText(hwndDlg, IDC_USER_PROFILE_REMOTE_PATH, pszHomeDir, nLength + 1);
            pUserInfo->usri3_home_dir = pszHomeDir;
        }

        nIndex = SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), CB_GETCURSEL, 0, 0);
        if (nIndex != CB_ERR)
        {
            nLength = SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), CB_GETLBTEXTLEN, nIndex, 0);
            pszHomeDrive = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
            SendMessage(GetDlgItem(hwndDlg, IDC_USER_PROFILE_DRIVE), CB_GETLBTEXT, nIndex, (LPARAM)pszHomeDrive);
            pUserInfo->usri3_home_dir_drive = pszHomeDrive;
        }
    }

    status = NetUserSetInfo(NULL, pUserData->szUserName, 3, (LPBYTE)pUserInfo, &dwIndex);
    if (status != NERR_Success)
    {
        DebugPrintf(_T("Status: %lu  Index: %lu"), status, dwIndex);
    }

    if (pszProfilePath)
        HeapFree(GetProcessHeap(), 0, pszProfilePath);

    if (pszScriptPath)
        HeapFree(GetProcessHeap(), 0, pszScriptPath);

    if (pszHomeDir)
        HeapFree(GetProcessHeap(), 0, pszHomeDir);

    if (pszHomeDrive)
        HeapFree(GetProcessHeap(), 0, pszHomeDrive);

    NetApiBufferFree(pUserInfo);

    return (status == NERR_Success);
}


INT_PTR CALLBACK
UserProfilePageProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PPROFILE_USER_DATA pUserData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    pUserData= (PPROFILE_USER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pUserData = (PPROFILE_USER_DATA)HeapAlloc(GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof(PROFILE_USER_DATA) +
                                                      lstrlen((LPTSTR)((PROPSHEETPAGE *)lParam)->lParam) * sizeof(TCHAR));
            lstrcpy(pUserData->szUserName, (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);

            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pUserData);

            GetUserProfileData(hwndDlg,
                               pUserData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_PROFILE_PATH:
                case IDC_USER_PROFILE_SCRIPT:
                    if (HIWORD(wParam) == EN_CHANGE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

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

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pUserData);
            break;

        case WM_NOTIFY:
            if (((LPPSHNOTIFY)lParam)->hdr.code == PSN_APPLY)
            {
                SetUserProfileData(hwndDlg, pUserData);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


static VOID
GetUserMembershipData(HWND hwndDlg, PMEMBERSHIP_USER_DATA pUserData)
{
    NET_API_STATUS status;
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
    hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 5, 5);
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


    status = NetUserGetLocalGroups(NULL, pUserData->szUserName, 0, 0,
                                   (LPBYTE*)&pUserData->pGroupData,
                                   MAX_PREFERRED_LENGTH,
                                   &pUserData->dwGroupCount,
                                   &dwTotal);
    if (status != NERR_Success)
        return;

    for (i = 0; i < pUserData->dwGroupCount; i++)
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        lvi.pszText = pUserData->pGroupData[i].lgrui0_name;
        lvi.state = 0;
        lvi.iImage = 0;

        (void)ListView_InsertItem(hwndLV, &lvi);
    }
}


static VOID
RemoveGroupFromUser(HWND hwndDlg,
                    PMEMBERSHIP_USER_DATA pUserData)
{
    TCHAR szGroupName[UNLEN + 1];
    TCHAR szText[256];
    LOCALGROUP_MEMBERS_INFO_3 memberInfo;
    HWND hwndLV;
    INT nItem;
    NET_API_STATUS status;

    hwndLV = GetDlgItem(hwndDlg, IDC_USER_MEMBERSHIP_LIST);
    nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (nItem == -1)
        return;

    /* Get the new user name */
    ListView_GetItemText(hwndLV,
                         nItem, 0,
                         szGroupName,
                         UNLEN + 1);

    /* Display a warning message because the remove operation cannot be reverted */
    wsprintf(szText, TEXT("Do you really want to remove the user \"%s\" from the group \"%s\"?"),
             pUserData->szUserName, szGroupName);
    if (MessageBox(NULL, szText, TEXT("User Accounts"), MB_ICONWARNING | MB_YESNO) == IDNO)
        return;

    memberInfo.lgrmi3_domainandname = pUserData->szUserName;

    status = NetLocalGroupDelMembers(NULL, szGroupName,
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
        EnableWindow(GetDlgItem(hwndDlg, IDC_USER_MEMBERSHIP_REMOVE), FALSE);
}


static VOID
InitUserGroupsList(HWND hwndDlg)
{
    HWND hwndLV;
    LV_COLUMN column;
    RECT rect;
    TCHAR szStr[32];

    NET_API_STATUS netStatus;
    PLOCALGROUP_INFO_1 pBuffer;
    DWORD entriesread;
    DWORD totalentries;
    DWORD_PTR resume_handle = 0;
    DWORD i;
    LV_ITEM lvi;
    INT iItem;

    HIMAGELIST hImgList;
    HICON hIcon;

    hwndLV = GetDlgItem(hwndDlg, IDC_USER_ADD_MEMBERSHIP_LIST);
    GetClientRect(hwndLV, &rect);

    hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 5, 5);
    hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_GROUP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    ImageList_AddIcon(hImgList, hIcon);
    DestroyIcon(hIcon);

    (void)ListView_SetImageList(hwndLV, hImgList, LVSIL_SMALL);
    (void)ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);

    memset(&column, 0x00, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.40);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_NAME, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndLV, 0, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.60);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_DESCRIPTION, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndLV, 1, &column);

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
           iItem = ListView_InsertItem(hwndLV, &lvi);

           ListView_SetItemText(hwndLV, iItem, 1,
                                pBuffer[i].lgrpi1_comment);
        }

        NetApiBufferFree(pBuffer);

        /* No more data left */
        if (netStatus != ERROR_MORE_DATA)
            break;
    }
}


static BOOL
AddSelectedGroupsToUser(HWND hwndDlg,
                        PMEMBERSHIP_USER_DATA pUserData)
{
    HWND hwndLV;
    INT nItem;
    TCHAR szGroupName[UNLEN + 1];
    BOOL bResult = FALSE;
    BOOL bFound;
    DWORD i;
    LOCALGROUP_MEMBERS_INFO_3 memberInfo;
    NET_API_STATUS status;

    hwndLV = GetDlgItem(hwndDlg, IDC_USER_ADD_MEMBERSHIP_LIST);

    if (ListView_GetSelectedCount(hwndLV) > 0)
    {
        nItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
        while (nItem != -1)
        {
            /* Get the new user name */
            ListView_GetItemText(hwndLV,
                                 nItem, 0,
                                 szGroupName,
                                 UNLEN + 1);

            bFound = FALSE;
            for (i = 0; i < pUserData->dwGroupCount; i++)
            {
                if (_tcscmp(pUserData->pGroupData[i].lgrui0_name, szGroupName) == 0)
                    bFound = TRUE;
            }

            if (!bFound)
            {
                memberInfo.lgrmi3_domainandname = pUserData->szUserName;

                status = NetLocalGroupAddMembers(NULL, szGroupName, 3,
                                                 (LPBYTE)&memberInfo, 1);
                if (status == NERR_Success)
                {
                    DebugPrintf(_TEXT("Selected group: %s"), szGroupName);
                    bResult = TRUE;
                }
                else
                {
                    TCHAR szText[256];
                    wsprintf(szText, TEXT("Error: %u"), status);
                    MessageBox(NULL, szText, TEXT("NetLocalGroupAddMembers"), MB_ICONERROR | MB_OK);
                }
            }

            nItem = ListView_GetNextItem(hwndLV, nItem, LVNI_SELECTED);
        }
    }

    return bResult;
}


INT_PTR CALLBACK
AddGroupToUserDlgProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    PMEMBERSHIP_USER_DATA pUserData;

    UNREFERENCED_PARAMETER(wParam);

    pUserData= (PMEMBERSHIP_USER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pUserData= (PMEMBERSHIP_USER_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pUserData);
            InitUserGroupsList(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (AddSelectedGroupsToUser(hwndDlg, pUserData))
                        EndDialog(hwndDlg, IDOK);
                    else
                        EndDialog(hwndDlg, IDCANCEL);
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
AddGroupToUser(HWND hwndDlg,
               PMEMBERSHIP_USER_DATA pUserData)
{
    HWND hwndLV;
    NET_API_STATUS status;
    DWORD i;
    DWORD dwTotal;
    LV_ITEM lvi;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_USER_ADD_MEMBERSHIP),
                       hwndDlg,
                       AddGroupToUserDlgProc,
                       (LPARAM)pUserData) == IDOK)
    {
        // TODO: Update Membership list!
        hwndLV = GetDlgItem(hwndDlg, IDC_USER_MEMBERSHIP_LIST);

        if (pUserData->pGroupData)
            NetApiBufferFree(pUserData->pGroupData);

        (void)ListView_DeleteAllItems(hwndLV);

        status = NetUserGetLocalGroups(NULL, pUserData->szUserName, 0, 0,
                                       (LPBYTE*)&pUserData->pGroupData,
                                       MAX_PREFERRED_LENGTH,
                                       &pUserData->dwGroupCount,
                                       &dwTotal);
        if (status != NERR_Success)
            return;

        for (i = 0; i < pUserData->dwGroupCount; i++)
        {
            ZeroMemory(&lvi, sizeof(lvi));
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
            lvi.pszText = pUserData->pGroupData[i].lgrui0_name;
            lvi.state = 0;
            lvi.iImage = 0;

            (void)ListView_InsertItem(hwndLV, &lvi);
        }
    }
}


static BOOL
OnUserPropSheetNotify(HWND hwndDlg,
                      PMEMBERSHIP_USER_DATA pUserData,
                      LPARAM lParam)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;

    switch (((LPNMHDR)lParam)->idFrom)
    {
        case IDC_USER_MEMBERSHIP_LIST:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USER_MEMBERSHIP_REMOVE), (lpnmlv->iItem != -1));
                    break;

                case LVN_KEYDOWN:
                    if (((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE)
                    {
                        RemoveGroupFromUser(hwndDlg, pUserData);
                    }
                    break;

            }
            break;
    }

    return FALSE;
}


INT_PTR CALLBACK
UserMembershipPageProc(HWND hwndDlg,
                       UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam)
{
    PMEMBERSHIP_USER_DATA pUserData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    pUserData= (PMEMBERSHIP_USER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pUserData = (PMEMBERSHIP_USER_DATA)HeapAlloc(GetProcessHeap(),
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof(MEMBERSHIP_USER_DATA) +
                                                         lstrlen((LPTSTR)((PROPSHEETPAGE *)lParam)->lParam) * sizeof(TCHAR));
            lstrcpy(pUserData->szUserName, (LPTSTR)((PROPSHEETPAGE *)lParam)->lParam);

            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pUserData);

            GetUserMembershipData(hwndDlg, pUserData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_MEMBERSHIP_ADD:
                    AddGroupToUser(hwndDlg, pUserData);
                    break;

                case IDC_USER_MEMBERSHIP_REMOVE:
                    RemoveGroupFromUser(hwndDlg, pUserData);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPPSHNOTIFY)lParam)->hdr.code == PSN_APPLY)
            {
                return TRUE;
            }
            else
            {
                return OnUserPropSheetNotify(hwndDlg, pUserData, lParam);
            }
            break;


        case WM_DESTROY:
            if (pUserData->pGroupData)
                NetApiBufferFree(pUserData->pGroupData);

            HeapFree(GetProcessHeap(), 0, pUserData);
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
GetUserGeneralData(HWND hwndDlg,
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
SetUserGeneralData(HWND hwndDlg,
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

    if (pszComment)
        HeapFree(GetProcessHeap(), 0, pszComment);

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

            SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pUserData);

            GetUserGeneralData(hwndDlg,
                               pUserData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_GENERAL_FULL_NAME:
                case IDC_USER_GENERAL_DESCRIPTION:
                    if (HIWORD(wParam) == EN_CHANGE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

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
                SetUserGeneralData(hwndDlg, pUserData);
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
InitUserPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPTSTR pszUser)
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
    TCHAR szUserName[UNLEN + 1];
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
                         UNLEN + 1);

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

    InitUserPropSheetPage(&psp[0], IDD_USER_GENERAL, UserGeneralPageProc, szUserName);
    InitUserPropSheetPage(&psp[1], IDD_USER_MEMBERSHIP, UserMembershipPageProc, szUserName);
    InitUserPropSheetPage(&psp[2], IDD_USER_PROFILE, UserProfilePageProc, szUserName);

    return (PropertySheet(&psh) == IDOK);
}
