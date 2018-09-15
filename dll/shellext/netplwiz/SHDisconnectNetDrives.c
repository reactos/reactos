/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implements the Disconnct Network Drive dialog
 * COPYRIGHT:   Copyright 2018 Jared Smudde (computerwhiz02@hotmail.com)
 */

#include "netplwiz.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(netplwiz);

HINSTANCE hInstance;

VOID InitializeListView(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_CONNECTEDDRIVELIST);
    LV_COLUMN column;

    if(hListView == NULL)
        return;

    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.pszText = L"Drive Letter";
    column.cx = 75;
    ListView_InsertColumn(hListView, 0, &column);

    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.pszText = L"Network Path";
    column.cx = 150;
    ListView_InsertColumn(hListView, 1, &column);
}

VOID EnumerateConnectedDrives(HWND hDlg)
{
    LV_ITEM item;
    HIMAGELIST hImgList;
    HICON hIconDrive;
    HMODULE hShell32;
    HWND hListView = GetDlgItem(hDlg, IDC_CONNECTEDDRIVELIST);

    DWORD dRet;
    HANDLE hEnum;
    LPNETRESOURCE lpRes;
    DWORD dAllocSize = 0x1000;
    DWORD dCount;
    LPNETRESOURCE lpCur;

    /* List View Icons */
    hShell32 = GetModuleHandleW(L"shell32.dll");

    if (hShell32 == NULL)
        return;

    hIconDrive = LoadImageW(hShell32, MAKEINTRESOURCEW(10), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON),
                            GetSystemMetrics(SM_CYSMICON), 0);

    if (hIconDrive == NULL)
        return;

    hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);

    if (hImgList == NULL)
    {
        DestroyIcon(hIconDrive);
        return;
    }

    ImageList_AddIcon(hImgList, hIconDrive);
    DestroyIcon(hIconDrive);
    ListView_SetImageList(hListView, hImgList, LVSIL_SMALL);

    dRet = WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK, 0, NULL, &hEnum);
    if (dRet != NO_ERROR)
    {
        ERR("WNetOpenEnum failed with error: 0x%08x\n", dRet);
        return;
    }

    lpRes = HeapAlloc(GetProcessHeap(), 0, dAllocSize);

    if (lpRes == NULL)
    {
        ERR("Could not allocate memory.\n", dRet);
        WNetCloseEnum(hEnum);
        return;
    }

    do
    {
        DWORD dSize;
        dCount = -1;
        memset(lpRes, 0, dAllocSize);

        dRet = WNetEnumResource(hEnum, &dCount, lpRes, &dSize);

        if (dRet == WN_MORE_DATA)
        {
            PVOID pBuff = HeapReAlloc(GetProcessHeap(), 0, lpRes, dSize);

            if (pBuff == NULL)
            {
                ERR("Could not reallocate memory.\n", dRet);
                break;
            }

            lpRes = pBuff;
            dAllocSize = dSize;

            continue;
        }
        else if (dRet == WN_SUCCESS)
        {
            lpCur = lpRes;

            while (dCount--)
            {
                memset(&item, 0, sizeof(item));
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iImage = 0;
                item.pszText = lpCur->lpLocalName;
                item.lParam = 0;
                item.iItem = ListView_InsertItem(hListView, &item);
                ListView_SetItemText(hListView, item.iItem, 1, lpCur->lpRemoteName);
                lpCur++;
            }
        }
        else
        {
            TRACE("WNetEnumResource failed with error: 0x%08x\n", dRet);
            break;
        }
    }
    while (dRet != WN_NO_MORE_ENTRIES);

    HeapFree(GetProcessHeap(), 0, lpRes);
    WNetCloseEnum(hEnum);
}

VOID UpdateButtonStatus(WPARAM wParam, LPARAM lParam, HWND hDlg)
{
    LPNMHDR pnmh;
    HWND hListView = GetDlgItem(hDlg, IDC_CONNECTEDDRIVELIST);
    HWND hOkbutton = GetDlgItem(hDlg, ID_OK);

    pnmh = (LPNMHDR)lParam;

    if (pnmh->hwndFrom == hListView)
    {
        switch (pnmh->code)
        {
            case LVN_ITEMCHANGED:
                if (ListView_GetSelectedCount(hListView))
                {
                    EnableWindow(hOkbutton, TRUE);
                }
                else
                {
                    EnableWindow(hOkbutton, FALSE);
                }
                break;
        }
    }
}

DWORD DisconnectDriveExit(HWND hDlg)
{
    TCHAR driveLetter[10];
    HRESULT hr;
    HWND hListView = GetDlgItem(hDlg, IDC_CONNECTEDDRIVELIST);
    INT nItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    if (nItem == -1)
        return 0;

    ListView_GetItemText(hListView, nItem, 0, driveLetter, ARRAYSIZE(driveLetter));
    hr = WNetCancelConnection2(driveLetter, CONNECT_UPDATE_PROFILE, FALSE);

    EndDialog(hDlg, ID_OK);
    return hr;
}

static INT_PTR CALLBACK DisconnectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon, hIconSm;
    HWND hOkbutton = GetDlgItem(hDlg, ID_OK);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_DISCONNECT_NET_DRIVES), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
            hIconSm = (HICON)CopyImage(hIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_COPYFROMRESOURCE);
            SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
            EnableWindow(hOkbutton, FALSE);
            InitializeListView(hDlg);
            EnumerateConnectedDrives(hDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_OK:
                    DisconnectDriveExit(hDlg);
                    break;
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return -1;
                    break;
            }
            break;

        case WM_NOTIFY:
            UpdateButtonStatus(wParam, lParam, hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

HRESULT WINAPI SHDisconnectNetDrives(PVOID Unused)
{
    DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_DISCONNECTDRIVES), NULL, DisconnectDlgProc);
    return S_OK;
}
