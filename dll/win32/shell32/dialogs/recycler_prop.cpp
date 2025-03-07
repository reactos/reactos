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
    ULARGE_INTEGER FreeBytesAvailable;
    DWORD dwSerial;
    DWORD dwMaxCapacity;
    DWORD dwNukeOnDelete;
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

    // FIXME: Max capacity not implemented yet, disable for now (CORE-13743)
    EnableWindow(GetDlgItem(hwndDlg, 14002), FALSE);
}

static VOID
InitializeRecycleBinDlg(HWND hwndDlg, WCHAR DefaultDrive)
{
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

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_LOCATION, szVolume, _countof(szVolume)))
        szVolume[0] = 0;

    GetClientRect(hDlgCtrl, &rect);

    ZeroMemory(&lc, sizeof(lc));
    lc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;

    columnSize = 140; //FIXME
    lc.iSubItem   = 0;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    lc.cx         = columnSize;
    lc.cchTextMax = wcslen(szVolume);
    lc.pszText    = szVolume;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&lc);

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_DISKSPACE, szVolume, _countof(szVolume)))
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
        if (dwDrives & 0x1)
        {
            UINT Type = GetDriveTypeW(szDrive);
            if (Type == DRIVE_FIXED) //FIXME
            {
                if (!GetVolumeInformationW(szDrive, szName, _countof(szName), &dwSerial, &MaxComponent, &Flags, NULL, 0))
                {
                    szName[0] = 0;
                    dwSerial = -1;
                }

                swprintf(szVolume, L"%s (%c:)", szName, szDrive[0]);
                ZeroMemory(&li, sizeof(li));
                li.mask = LVIF_TEXT | LVIF_PARAM;
                li.iSubItem = 0;
                li.pszText = szVolume;
                li.iItem = itemCount;
                SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
                if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                {
                    if (StrFormatByteSizeW(TotalNumberOfFreeBytes.QuadPart, szVolume, _countof(szVolume)))
                    {
                        pItem = (DRIVE_ITEM_CONTEXT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRIVE_ITEM_CONTEXT));
                        if (pItem)
                        {
                            pItem->FreeBytesAvailable = FreeBytesAvailable;
                            pItem->dwSerial = dwSerial;

                            swprintf(szName, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume\\%04X-%04X", LOWORD(dwSerial), HIWORD(dwSerial));

                            dwSize = sizeof(DWORD);
                            if (RegGetValueW(HKEY_CURRENT_USER, szName, L"MaxCapacity", RRF_RT_DWORD, NULL, &pItem->dwMaxCapacity, &dwSize))
                                pItem->dwMaxCapacity = ~0;

                            /* Check if the maximum capacity doesn't exceed the available disk space (in megabytes), and truncate it if needed */
                            FreeBytesAvailable.QuadPart = (FreeBytesAvailable.QuadPart / (1024 * 1024));
                            pItem->dwMaxCapacity = min(pItem->dwMaxCapacity, FreeBytesAvailable.LowPart);

                            dwSize = sizeof(DWORD);
                            RegGetValueW(HKEY_CURRENT_USER, szName, L"NukeOnDelete", RRF_RT_DWORD, NULL, &pItem->dwNukeOnDelete, &dwSize);

                            li.mask = LVIF_PARAM;
                            li.lParam = (LPARAM)pItem;
                            (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                            if (szDrive[0] == DefaultDrive)
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
        szDrive[0]++;
        dwDrives = (dwDrives >> 1);
    } while (dwDrives);

    if (!pDefault)
        pDefault = pFirst;
    if (pDefault)
    {
        toggleNukeOnDeleteOption(hwndDlg, pDefault->dwNukeOnDelete);
        SetDlgItemInt(hwndDlg, 14002, pDefault->dwMaxCapacity, FALSE);
    }
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_STATE;
    li.stateMask = (UINT)-1;
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

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return FALSE;

    iCount = ListView_GetItemCount(hDlgCtrl);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM;

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
            swprintf(szSerial, L"%04X-%04X", LOWORD(pItem->dwSerial), HIWORD(pItem->dwSerial));
            if (RegCreateKeyExW(hKey, szSerial, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
            {
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"MaxCapacity", 0, REG_DWORD, (LPBYTE)&pItem->dwMaxCapacity, dwSize);
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&pItem->dwNukeOnDelete, dwSize);
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

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            HeapFree(GetProcessHeap(), 0, (PVOID)li.lParam);
        }
    }
}

static INT
GetSelectedDriveItem(HWND hwndDlg, LVITEMW* li)
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
    li->stateMask = (UINT)-1;
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
    LPARAM lParam)
{
    enum { WM_NEWDRIVESELECTED = WM_APP, WM_UPDATEDRIVESETTINGS };
    LPPSHNOTIFY lppsn;
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PDRIVE_ITEM_CONTEXT pItem;
    BOOL bSuccess;
    UINT uResult;
    PROPSHEETPAGE * page;
    DWORD dwStyle;
    ULARGE_INTEGER FreeBytesAvailable;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            InitializeRecycleBinDlg(hwndDlg, (WCHAR)page->lParam);
            SendDlgItemMessage(hwndDlg, 14004, BM_SETCHECK, !SHELL_GetSetting(SSF_NOCONFIRMRECYCLE, fNoConfirmRecycle), 0);
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
                case 14002:
                    if (HIWORD(wParam) == EN_CHANGE)
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
                SHELLSTATE ss;
                ss.fNoConfirmRecycle = SendDlgItemMessage(hwndDlg, 14004, BM_GETCHECK, 0, 0) == BST_UNCHECKED;
                SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, TRUE);

                if (GetSelectedDriveItem(hwndDlg, &li) > -1)
                {
                    SendMessage(hwndDlg, WM_UPDATEDRIVESETTINGS, 0, li.lParam);
                }
                if (StoreDriveSettings(hwndDlg))
                {
                    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR );
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
                    // New focused item, delay handling until after kill focus has been processed
                    PostMessage(hwndDlg, WM_NEWDRIVESELECTED, 0, (LPARAM)pItem);
                }
                else if ((lppl->uOldState & LVIS_FOCUSED) && !(lppl->uNewState & LVIS_FOCUSED))
                {
                    // Kill focus
                    SendMessage(hwndDlg, WM_UPDATEDRIVESETTINGS, 0, (LPARAM)pItem);
                }
                return TRUE;

            }
            break;
        case WM_NEWDRIVESELECTED:
            if (lParam)
            {
                pItem = (PDRIVE_ITEM_CONTEXT)lParam;
                toggleNukeOnDeleteOption(hwndDlg, pItem->dwNukeOnDelete);
                SetDlgItemInt(hwndDlg, 14002, pItem->dwMaxCapacity, FALSE);
            }
            break;
        case WM_UPDATEDRIVESETTINGS:
            if (lParam)
            {
                pItem = (PDRIVE_ITEM_CONTEXT)lParam;
                uResult = GetDlgItemInt(hwndDlg, 14002, &bSuccess, FALSE);
                if (bSuccess)
                {
                    /* Check if the maximum capacity doesn't exceed the available disk space (in megabytes), and truncate it if needed */
                    FreeBytesAvailable = pItem->FreeBytesAvailable;
                    FreeBytesAvailable.QuadPart = (FreeBytesAvailable.QuadPart / (1024 * 1024));
                    pItem->dwMaxCapacity = min(uResult, FreeBytesAvailable.LowPart);
                    SetDlgItemInt(hwndDlg, 14002, pItem->dwMaxCapacity, FALSE);
                }
                if (SendDlgItemMessageW(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    pItem->dwNukeOnDelete = TRUE;
                else
                    pItem->dwNukeOnDelete = FALSE;
            }
            break;
        case WM_DESTROY:
            FreeDriveItemContext(hwndDlg);
            break;
    }
    return FALSE;
}

HRESULT RecycleBin_AddPropSheetPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp = SH_CreatePropertySheetPage(IDD_RECYCLE_BIN_PROPERTIES, RecycleBinDlg, 0, NULL);
    return AddPropSheetPage(hpsp, pfnAddPage, lParam);
}
