/*
 * PROJECT:     ReactOS Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Confirm Removal Dialog
 * COPYRIGHT:   Copyright 2023 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "hotplug.h"

#include <initguid.h>
#include <devguid.h>

static
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
InsertConfirmDeviceListItem(
    _In_ HWND hwndCfmDeviceList,
    _In_ DEVINST DevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    WCHAR szDisplayName[40];
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    DWORD dwSize;
    GUID ClassGuid;
    INT nClassImage;
    CONFIGRET cr;
    LVITEMW lvItem;

    /* Get the device description */
    dwSize = sizeof(szDisplayName);
    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                          CM_DRP_DEVICEDESC,
                                          NULL,
                                          szDisplayName,
                                          &dwSize,
                                          0);
    if (cr != CR_SUCCESS)
        wcscpy(szDisplayName, L"Unknown Device");

    /* Get the class GUID */
    dwSize = sizeof(szGuidString);
    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                          CM_DRP_CLASSGUID,
                                          NULL,
                                          szGuidString,
                                          &dwSize,
                                          0);
    if (cr == CR_SUCCESS)
    {
        pSetupGuidFromString(szGuidString, &ClassGuid);
    }
    else
    {
        memcpy(&ClassGuid, &GUID_DEVCLASS_UNKNOWN, sizeof(GUID));
    }

    /* Get the image for the class this device is in */
    SetupDiGetClassImageIndex(&pHotplugData->ImageListData,
                              &ClassGuid,
                              &nClassImage);

    /* Add it to the confirm device list */
    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvItem.iItem = ListView_GetItemCount(hwndCfmDeviceList);
    lvItem.pszText = szDisplayName;
    lvItem.iImage = nClassImage;
    lvItem.lParam = (LPARAM)DevInst;

    ListView_InsertItem(hwndCfmDeviceList, &lvItem);
}

static
VOID
CfmListRecursiveInsertSubDevices(
    _In_ HWND hwndCfmDeviceList,
    _In_ DEVINST ParentDevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    DEVINST ChildDevInst;
    CONFIGRET cr;

    cr = CM_Get_Child(&ChildDevInst, ParentDevInst, 0);
    if (cr != CR_SUCCESS)
        return;

    InsertConfirmDeviceListItem(hwndCfmDeviceList, ChildDevInst, pHotplugData);
    CfmListRecursiveInsertSubDevices(hwndCfmDeviceList, ChildDevInst, pHotplugData);

    for (;;)
    {
        cr = CM_Get_Sibling(&ChildDevInst, ChildDevInst, 0);
        if (cr != CR_SUCCESS)
            return;

        InsertConfirmDeviceListItem(hwndCfmDeviceList, ChildDevInst, pHotplugData);
        CfmListRecursiveInsertSubDevices(hwndCfmDeviceList, ChildDevInst, pHotplugData);
    }
}

static
VOID
FillConfirmDeviceList(
    _In_ HWND hwndCfmDeviceList,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    DEVINST DevInst;
    LVCOLUMNW lvColumn;

    DevInst = GetDeviceInstForRemoval(pHotplugData);
    if (DevInst == 0)
        return;

    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_FMT;
    lvColumn.fmt = LVCFMT_LEFT | LVCFMT_IMAGE;
    ListView_InsertColumn(hwndCfmDeviceList, 0, &lvColumn);

    InsertConfirmDeviceListItem(hwndCfmDeviceList, DevInst, pHotplugData);
    CfmListRecursiveInsertSubDevices(hwndCfmDeviceList, DevInst, pHotplugData);

    ListView_SetColumnWidth(hwndCfmDeviceList, 0, LVSCW_AUTOSIZE_USEHEADER);
}

static
VOID
SafeRemoveDevice(
    _In_ DEVINST DevInst)
{
    PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
    CONFIGRET cr;

    cr = CM_Request_Device_EjectW(DevInst, &VetoType, NULL, 0, 0);
    if (cr != CR_SUCCESS && VetoType == PNP_VetoTypeUnknown)
    {
        WCHAR szError[64];
        swprintf(szError, L"Failed to remove device (0x%x)", cr);
        MessageBoxW(NULL, szError, NULL, MB_ICONEXCLAMATION | MB_OK);
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
                    SafeRemoveDevice(GetDeviceInstForRemoval(pHotplugData));
                    EndDialog(hwndDlg, 0);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
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
