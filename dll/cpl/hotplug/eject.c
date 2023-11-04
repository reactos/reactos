/*
 * PROJECT:     Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device removal support
 * COPYRIGHT:   Copyright 2023 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "hotplug.h"

#include <strsafe.h>

DEVINST
GetDeviceInstForRemoval(
    _In_ PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hItem, hParentItem;
    TVITEMW tvItem;

    hItem = TreeView_GetSelection(pHotplugData->hwndDeviceTree);
    if (!hItem)
        return 0;

    /* Find root item */
    hParentItem = TreeView_GetParent(pHotplugData->hwndDeviceTree, hItem);
    while (hParentItem)
    {
        hItem = hParentItem;
        hParentItem = TreeView_GetParent(pHotplugData->hwndDeviceTree, hItem);
    }

    ZeroMemory(&tvItem, sizeof(tvItem));
    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = hItem;

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
        LPCWSTR pszFormat = L"";
        WCHAR szError[64];

        /* NOTES: IDS_EJECT_ERROR_FORMAT resource has to be explicitly NULL-terminated
         * so we can use the string directly without having to make a copy of it. */
        LoadStringW(hApplet, IDS_EJECT_ERROR_FORMAT, (LPWSTR)&pszFormat, 0);
        StringCbPrintfW(szError, sizeof(szError), pszFormat, cr);

        MessageBoxW(hwndDlg, szError, NULL, MB_ICONEXCLAMATION | MB_OK);
    }
}

INT_PTR
CALLBACK
ConfirmRemovalDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PHOTPLUG_DATA pHotplugData;

    pHotplugData = (PHOTPLUG_DATA)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndDevList;

            pHotplugData = (PHOTPLUG_DATA)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pHotplugData);

            hwndDevList = GetDlgItem(hwndDlg, IDC_CONFIRM_STOP_DEVICE_LIST);

            ListView_SetImageList(hwndDevList,
                                  pHotplugData->ImageListData.ImageList,
                                  LVSIL_SMALL);

            FillConfirmDeviceList(hwndDevList, pHotplugData);

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
