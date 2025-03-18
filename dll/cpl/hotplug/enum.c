/*
 * PROJECT:     Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device enumeration
 * COPYRIGHT:   Copyright 2020 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2023 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "hotplug.h"

#include <initguid.h>
#include <devguid.h>
#include <setupapi_undoc.h>

#define MAX_DEVICE_DISPLAYNAME_LEN 256

static
VOID
GetDeviceDisplayInfo(
    _In_ DEVINST DevInst,
    _In_ PHOTPLUG_DATA pHotplugData,
    _Out_writes_z_(cchDesc) LPWSTR pszDesc,
    _In_ ULONG cchDesc,
    _Out_ PINT pImageIndex)
{
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuid;
    ULONG ulSize;
    CONFIGRET cr;

    /* Get the device description */
    ulSize = cchDesc * sizeof(WCHAR);
    cr = CM_Get_DevNode_Registry_PropertyW(DevInst,
                                           CM_DRP_FRIENDLYNAME,
                                           NULL,
                                           pszDesc,
                                           &ulSize,
                                           0);
    if (cr != CR_SUCCESS)
    {
        ulSize = cchDesc * sizeof(WCHAR);
        cr = CM_Get_DevNode_Registry_PropertyW(DevInst,
                                               CM_DRP_DEVICEDESC,
                                               NULL,
                                               pszDesc,
                                               &ulSize,
                                               0);
        if (cr != CR_SUCCESS)
            LoadStringW(hApplet, IDS_UNKNOWN_DEVICE, pszDesc, cchDesc);
    }

    /* Get the class GUID */
    ulSize = sizeof(szGuidString);
    cr = CM_Get_DevNode_Registry_PropertyW(DevInst,
                                           CM_DRP_CLASSGUID,
                                           NULL,
                                           szGuidString,
                                           &ulSize,
                                           0);
    if (cr == CR_SUCCESS)
    {
        pSetupGuidFromString(szGuidString, &ClassGuid);
    }
    else
    {
        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }

    /* Get the image for the class this device is in */
    SetupDiGetClassImageIndex(&pHotplugData->ImageListData,
                              &ClassGuid,
                              pImageIndex);
}

static
HTREEITEM
InsertDeviceTreeItem(
    _In_ HTREEITEM hParent,
    _In_ DEVINST DevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    WCHAR szDisplayName[MAX_DEVICE_DISPLAYNAME_LEN];
    INT nClassImage;
    TVINSERTSTRUCTW tvItem;

    GetDeviceDisplayInfo(DevInst,
                         pHotplugData,
                         szDisplayName,
                         ARRAYSIZE(szDisplayName),
                         &nClassImage);

    ZeroMemory(&tvItem, sizeof(tvItem));
    tvItem.hParent = hParent;
    tvItem.hInsertAfter = TVI_LAST;

    tvItem.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvItem.item.state = TVIS_EXPANDED;
    tvItem.item.stateMask = TVIS_EXPANDED;
    tvItem.item.pszText = szDisplayName;
    tvItem.item.iImage = nClassImage;
    tvItem.item.iSelectedImage = nClassImage;
    tvItem.item.lParam = (LPARAM)DevInst;

    return TreeView_InsertItem(pHotplugData->hwndDeviceTree, &tvItem);
}

static
VOID
DevTreeRecursiveInsertSubDevices(
    _In_ HTREEITEM hParentItem,
    _In_ DEVINST ParentDevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hTreeItem;
    DEVINST ChildDevInst;
    CONFIGRET cr;

    cr = CM_Get_Child(&ChildDevInst, ParentDevInst, 0);
    if (cr != CR_SUCCESS)
        return;

    hTreeItem = InsertDeviceTreeItem(hParentItem,
                                     ChildDevInst,
                                     pHotplugData);
    if (hTreeItem != NULL)
    {
        DevTreeRecursiveInsertSubDevices(hTreeItem,
                                         ChildDevInst,
                                         pHotplugData);
    }

    for (;;)
    {
        cr = CM_Get_Sibling(&ChildDevInst, ChildDevInst, 0);
        if (cr != CR_SUCCESS)
            return;

        hTreeItem = InsertDeviceTreeItem(hParentItem,
                                         ChildDevInst,
                                         pHotplugData);
        if (hTreeItem != NULL)
        {
            DevTreeRecursiveInsertSubDevices(hTreeItem,
                                             ChildDevInst,
                                             pHotplugData);
        }
    }
}

VOID
EnumHotpluggedDevices(
    _In_ PHOTPLUG_DATA pHotplugData)
{
    SP_DEVINFO_DATA did = { 0 };
    HDEVINFO hdev;
    int idev;
    DWORD dwCapabilities, dwSize;
    ULONG ulStatus, ulProblem;
    HTREEITEM hTreeItem;
    CONFIGRET cr;

    TreeView_DeleteAllItems(pHotplugData->hwndDeviceTree);

    hdev = SetupDiGetClassDevs(NULL, NULL, 0, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hdev == INVALID_HANDLE_VALUE)
        return;

    did.cbSize = sizeof(did);

    /* Enumerate all the attached devices */
    for (idev = 0; SetupDiEnumDeviceInfo(hdev, idev, &did); idev++)
    {
        ulStatus = 0;
        ulProblem = 0;

        cr = CM_Get_DevNode_Status(&ulStatus,
                                   &ulProblem,
                                   did.DevInst,
                                   0);
        if (cr != CR_SUCCESS)
            continue;

        dwCapabilities = 0,
        dwSize = sizeof(dwCapabilities);
        cr = CM_Get_DevNode_Registry_Property(did.DevInst,
                                              CM_DRP_CAPABILITIES,
                                              NULL,
                                              &dwCapabilities,
                                              &dwSize,
                                              0);
        if (cr != CR_SUCCESS)
            continue;

        /* Add devices that require safe removal to the device tree */
        if ( (dwCapabilities & CM_DEVCAP_REMOVABLE) &&
            !(dwCapabilities & CM_DEVCAP_DOCKDEVICE) &&
            !(dwCapabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
            ((dwCapabilities & CM_DEVCAP_EJECTSUPPORTED) || (ulStatus & DN_DISABLEABLE)) &&
            ulProblem == 0)
        {
            hTreeItem = InsertDeviceTreeItem(TVI_ROOT,
                                             did.DevInst,
                                             pHotplugData);

            if ((hTreeItem != NULL) && (pHotplugData->dwFlags & HOTPLUG_DISPLAY_DEVICE_COMPONENTS))
            {
                DevTreeRecursiveInsertSubDevices(hTreeItem,
                                                 did.DevInst,
                                                 pHotplugData);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hdev);
}

static
VOID
InsertConfirmDeviceListItem(
    _In_ HWND hwndCfmDeviceList,
    _In_ DEVINST DevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    WCHAR szDisplayName[MAX_DEVICE_DISPLAYNAME_LEN];
    INT nClassImage;
    LVITEMW lvItem;

    GetDeviceDisplayInfo(DevInst,
                         pHotplugData,
                         szDisplayName,
                         ARRAYSIZE(szDisplayName),
                         &nClassImage);

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

VOID
CfmListEnumDevices(
    _In_ HWND hwndCfmDeviceList,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    DEVINST DevInst;

    DevInst = GetDeviceInstForRemoval(pHotplugData);
    if (DevInst != 0)
    {
        InsertConfirmDeviceListItem(hwndCfmDeviceList, DevInst, pHotplugData);
        CfmListRecursiveInsertSubDevices(hwndCfmDeviceList, DevInst, pHotplugData);
    }
}
