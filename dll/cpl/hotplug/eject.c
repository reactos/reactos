/*
 * PROJECT:     ReactOS Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device Removal Support
 * COPYRIGHT:   Copyright 2023 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "hotplug.h"

DEVINST
GetDeviceInstForRemoval(
    _In_ PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hTreeItem;
    TVITEMW tvItem;

    hTreeItem = TreeView_GetSelection(pHotplugData->hwndDeviceTree);
    if (!hTreeItem)
        return 0;

    /* Find top-level parent item */
    while (TreeView_GetParent(pHotplugData->hwndDeviceTree, hTreeItem))
    {
        hTreeItem = TreeView_GetParent(pHotplugData->hwndDeviceTree, hTreeItem);
    }

    ZeroMemory(&tvItem, sizeof(tvItem));
    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = hTreeItem;

    TreeView_GetItem(pHotplugData->hwndDeviceTree, &tvItem);

    return tvItem.lParam;
}

static
VOID
FillConfirmDeviceList(
    _In_ HWND hwndCfmDeviceList,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    LVCOLUMNW lvColumn;

    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_FMT;
    lvColumn.fmt = LVCFMT_LEFT | LVCFMT_IMAGE;
    ListView_InsertColumn(hwndCfmDeviceList, 0, &lvColumn);

    CfmListEnumDevices(hwndCfmDeviceList, pHotplugData);

    ListView_SetColumnWidth(hwndCfmDeviceList, 0, LVSCW_AUTOSIZE_USEHEADER);
}

static
VOID
SafeRemoveDevice(
    _In_ DEVINST DevInst,
    _In_opt_ HWND hwndDlg)
{
    PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
    CONFIGRET cr;

    cr = CM_Request_Device_EjectW(DevInst, &VetoType, NULL, 0, 0);
    if (cr != CR_SUCCESS && VetoType == PNP_VetoTypeUnknown)
    {
        WCHAR szError[64];
        swprintf(szError, L"Failed to remove device (0x%x)", cr);
        MessageBoxW(hwndDlg, szError, NULL, MB_ICONEXCLAMATION | MB_OK);
    }
}

INT_PTR
CALLBACK
ConfirmRemovalDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PHOTPLUG_DATA pHotplugData;

    pHotplugData = (PHOTPLUG_DATA)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pHotplugData = (PHOTPLUG_DATA)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pHotplugData);

            ListView_SetImageList(GetDlgItem(hwndDlg, IDC_CONFIRM_STOP_DEVICE_LIST),
                                  pHotplugData->ImageListData.ImageList,
                                  LVSIL_SMALL);

            FillConfirmDeviceList(GetDlgItem(hwndDlg, IDC_CONFIRM_STOP_DEVICE_LIST),
                                  pHotplugData);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    SafeRemoveDevice(GetDeviceInstForRemoval(pHotplugData), hwndDlg);
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;
            }

            break;
        }

        case WM_DESTROY:
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            break;
    }

    return FALSE;
}
