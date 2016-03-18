/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright (C) 2009 Andrew Hill
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(CRecycleBin);

typedef struct
{
    DWORD dwNukeOnDelete;
    DWORD dwSerial;
    DWORD dwMaxCapacity;
} DRIVE_ITEM_CONTEXT, *PDRIVE_ITEM_CONTEXT;

static void toggleNukeOnDeleteOption(HWND hwndDlg, BOOL bEnable)
{
    if (bEnable)
    {
        SendDlgItemMessage(hwndDlg, 14001, BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, 14002), FALSE);
        SendDlgItemMessage(hwndDlg, 14003, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendDlgItemMessage(hwndDlg, 14001, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, 14002), TRUE);
        SendDlgItemMessage(hwndDlg, 14003, BM_SETCHECK, BST_UNCHECKED, 0);
    }
}


static VOID
InitializeRecycleBinDlg(HWND hwndDlg, WCHAR DefaultDrive)
{
    WCHAR CurDrive = L'A';
    WCHAR szDrive[] = L"A:\\";
    DWORD dwDrives;
    WCHAR szName[100];
    WCHAR szVolume[100];
    DWORD MaxComponent, Flags;
    DWORD dwSerial;
    LVCOLUMNW lc;
    HWND hDlgCtrl;
    LVITEMW li;
    INT itemCount;
    ULARGE_INTEGER TotalNumberOfFreeBytes, TotalNumberOfBytes, FreeBytesAvailable;
    RECT rect;
    int columnSize;
    int defIndex = 0;
    DWORD dwSize;
    PDRIVE_ITEM_CONTEXT pItem = NULL, pDefault = NULL, pFirst = NULL;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_LOCATION, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
        szVolume[0] = 0;

    GetClientRect(hDlgCtrl, &rect);

    memset(&lc, 0, sizeof(LV_COLUMN) );
    lc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;

    columnSize = 140; //FIXME
    lc.iSubItem   = 0;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    lc.cx         = columnSize;
    lc.cchTextMax = wcslen(szVolume);
    lc.pszText    = szVolume;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&lc);

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_DISKSPACE, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
        szVolume[0] = 0;

    lc.iSubItem   = 1;
    lc.cx         = rect.right - rect.left - columnSize;
    lc.cchTextMax = wcslen(szVolume);
    lc.pszText    = szVolume;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 1, (LPARAM)&lc);

    dwDrives = GetLogicalDrives();
    itemCount = 0;
    do
    {
        if ((dwDrives & 0x1))
        {
            UINT Type = GetDriveTypeW(szDrive);
            if (Type == DRIVE_FIXED) //FIXME
            {
                if (!GetVolumeInformationW(szDrive, szName, sizeof(szName) / sizeof(WCHAR), &dwSerial, &MaxComponent, &Flags, NULL, 0))
                {
                    szName[0] = 0;
                    dwSerial = -1;
                }

                swprintf(szVolume, L"%s (%c:)", szName, szDrive[0]);
                memset(&li, 0x0, sizeof(LVITEMW));
                li.mask = LVIF_TEXT | LVIF_PARAM;
                li.iSubItem = 0;
                li.pszText = szVolume;
                li.iItem = itemCount;
                SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
                if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable , &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                {
                    if (StrFormatByteSizeW(TotalNumberOfFreeBytes.QuadPart, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
                    {

                        pItem = (DRIVE_ITEM_CONTEXT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRIVE_ITEM_CONTEXT));
                        if (pItem)
                        {
                            swprintf(szName, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume\\%04X-%04X", LOWORD(dwSerial), HIWORD(dwSerial));
                            dwSize = sizeof(DWORD);
                            RegGetValueW(HKEY_CURRENT_USER, szName, L"MaxCapacity", RRF_RT_DWORD, NULL, &pItem->dwMaxCapacity, &dwSize);
                            dwSize = sizeof(DWORD);
                            RegGetValueW(HKEY_CURRENT_USER, szName, L"NukeOnDelete", RRF_RT_DWORD, NULL, &pItem->dwNukeOnDelete, &dwSize);
                            pItem->dwSerial = dwSerial;
                            li.mask = LVIF_PARAM;
                            li.lParam = (LPARAM)pItem;
                            (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                            if (CurDrive == DefaultDrive)
                            {
                                defIndex = itemCount;
                                pDefault = pItem;
                            }
                        }
                        if (!pFirst)
                            pFirst = pItem;

                        li.mask = LVIF_TEXT;
                        li.iSubItem = 1;
                        li.pszText = szVolume;
                        li.iItem = itemCount;
                        (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                    }
                }
                itemCount++;
            }
        }
        CurDrive++;
        szDrive[0] = CurDrive;
        dwDrives = (dwDrives >> 1);
    } while(dwDrives);

    if (!pDefault)
        pDefault = pFirst;
    if (pDefault)
    {
        toggleNukeOnDeleteOption(hwndDlg, pDefault->dwNukeOnDelete);
        SetDlgItemInt(hwndDlg, 14002, pDefault->dwMaxCapacity, FALSE);
    }
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_STATE;
    li.stateMask = (UINT) - 1;
    li.state = LVIS_FOCUSED | LVIS_SELECTED;
    li.iItem = defIndex;
    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);

}

static BOOL StoreDriveSettings(HWND hwndDlg)
{
    int iCount, iIndex;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    LVITEMW li;
    PDRIVE_ITEM_CONTEXT pItem;
    HKEY hKey, hSubKey;
    WCHAR szSerial[20];
    DWORD dwSize;


    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return FALSE;

    iCount = ListView_GetItemCount(hDlgCtrl);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM;

    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
            swprintf(szSerial, L"%04X-%04X", LOWORD(pItem->dwSerial), HIWORD(pItem->dwSerial));
            if (RegCreateKeyExW(hKey, szSerial, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
            {
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&pItem->dwNukeOnDelete, dwSize);
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"MaxCapacity", 0, REG_DWORD, (LPBYTE)&pItem->dwMaxCapacity, dwSize);
                RegCloseKey(hSubKey);
            }
        }
    }
    RegCloseKey(hKey);
    return TRUE;

}

static VOID FreeDriveItemContext(HWND hwndDlg)
{
    int iCount, iIndex;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    LVITEMW li;

    iCount = ListView_GetItemCount(hDlgCtrl);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM;

    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)li.lParam);
        }
    }
}

static INT
GetDefaultItem(HWND hwndDlg, LVITEMW * li)
{
    HWND hDlgCtrl;
    UINT iItemCount, iIndex;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    if (!hDlgCtrl)
        return -1;

    iItemCount = ListView_GetItemCount(hDlgCtrl);
    if (!iItemCount)
        return -1;

    ZeroMemory(li, sizeof(LVITEMW));
    li->mask = LVIF_PARAM | LVIF_STATE;
    li->stateMask = (UINT) - 1;
    for (iIndex = 0; iIndex < iItemCount; iIndex++)
    {
        li->iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)li))
        {
            if (li->state & LVIS_SELECTED)
                return iIndex;
        }
    }
    return -1;

}

static INT_PTR CALLBACK
RecycleBinDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPSHNOTIFY lppsn;
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PDRIVE_ITEM_CONTEXT pItem;
    BOOL bSuccess;
    UINT uResult;
    PROPSHEETPAGE * page;
    DWORD dwStyle;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            InitializeRecycleBinDlg(hwndDlg, (WCHAR)page->lParam);
            dwStyle = (DWORD) SendDlgItemMessage(hwndDlg, 14000, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
            SendDlgItemMessage(hwndDlg, 14000, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
            if (GetDlgCtrlID((HWND)wParam) != 14000)
            {
                SetFocus(GetDlgItem(hwndDlg, 14000));
                return FALSE;
            }
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14001:
                    toggleNukeOnDeleteOption(hwndDlg, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                case 14003:
                    toggleNukeOnDeleteOption(hwndDlg, TRUE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                case 14004:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            lppl = (LPNMLISTVIEW) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                if (GetDefaultItem(hwndDlg, &li) > -1)
                {
                    pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
                    if (pItem)
                    {
                        uResult = GetDlgItemInt(hwndDlg, 14002, &bSuccess, FALSE);
                        if (bSuccess)
                            pItem->dwMaxCapacity = uResult;
                        if (SendDlgItemMessageW(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            pItem->dwNukeOnDelete = TRUE;
                        else
                            pItem->dwNukeOnDelete = FALSE;
                    }
                }
                if (StoreDriveSettings(hwndDlg))
                {
                    SetWindowLongPtr( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            else if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                ZeroMemory(&li, sizeof(li));
                li.mask = LVIF_PARAM;
                li.iItem = lppl->iItem;
                if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&li))
                    return TRUE;

                pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
                if (!pItem)
                    return TRUE;

                if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                {
                    /* new focused item */
                    toggleNukeOnDeleteOption(lppl->hdr.hwndFrom, pItem->dwNukeOnDelete);
                    SetDlgItemInt(hwndDlg, 14002, pItem->dwMaxCapacity, FALSE);
                }
                else if ((lppl->uOldState & LVIS_FOCUSED) && !(lppl->uNewState & LVIS_FOCUSED))
                {
                    /* kill focus */
                    uResult = GetDlgItemInt(hwndDlg, 14002, &bSuccess, FALSE);
                    if (bSuccess)
                        pItem->dwMaxCapacity = uResult;
                    if (SendDlgItemMessageW(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        pItem->dwNukeOnDelete = TRUE;
                    else
                        pItem->dwNukeOnDelete = FALSE;
                }
                return TRUE;

            }
            break;
        case WM_DESTROY:
            FreeDriveItemContext(hwndDlg);
            break;
    }
    return FALSE;
}

BOOL SH_ShowRecycleBinProperties(WCHAR sDrive)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE hprop;

    BOOL ret;


    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSP_DEFAULT | PSH_PROPTITLE;
    psh.pszCaption = MAKEINTRESOURCEW(IDS_RECYCLEBIN_FOLDER_NAME);
    psh.hwndParent = NULL;
    psh.phpage = hpsp;
    psh.hInstance = shell32_hInstance;

    hprop = SH_CreatePropertySheetPage(IDD_RECYCLE_BIN_PROPERTIES, RecycleBinDlg, (LPARAM)sDrive, NULL);
    if (!hprop)
    {
        ERR("Failed to create property sheet\n");
        return FALSE;
    }
    hpsp[psh.nPages] = hprop;
    psh.nPages++;


    ret = PropertySheetW(&psh);
    if (ret < 0)
        return FALSE;
    else
        return TRUE;
}
