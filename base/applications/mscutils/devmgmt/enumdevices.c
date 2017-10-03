/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/devmgmt/enumdevices.c
 * PURPOSE:     Enumerates all devices on the local machine
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#include <cfgmgr32.h>
#include <dll/devmgr/devmgr.h>
#include <initguid.h>
#include <devguid.h>

static SP_CLASSIMAGELIST_DATA ImageListData;


VOID
FreeDeviceStrings(HWND hTreeView)
{
    HTREEITEM hItem;

    hItem = TreeView_GetRoot(hTreeView);

    if (hItem)
    {
        hItem = TreeView_GetChild(hTreeView,
                                  hItem);
        /* loop the parent items */
        while (hItem)
        {
            hItem = TreeView_GetChild(hTreeView,
                                      hItem);
            if (hItem == NULL)
                break;

            /* loop the child items and free the DeviceID */
            while (TRUE)
            {
                HTREEITEM hOldItem;
                TV_ITEM tvItem;
                //TCHAR Buf[100];

                tvItem.hItem = hItem;
                tvItem.mask = TVIF_PARAM;// | TVIF_TEXT;
                //tvItem.pszText = Buf;
                //tvItem.cchTextMax = 99;

                (void)TreeView_GetItem(hTreeView, &tvItem);

                //MessageBox(NULL, Buf, NULL, 0);

                HeapFree(GetProcessHeap(),
                         0,
                         (LPTSTR)tvItem.lParam);

                hOldItem = hItem;

                hItem = TreeView_GetNextSibling(hTreeView,
                                                hItem);
                if (hItem == NULL)
                {
                    hItem = hOldItem;
                    break;
                }
            }

            hItem = TreeView_GetParent(hTreeView,
                                       hItem);
            hItem = TreeView_GetNextSibling(hTreeView,
                                            hItem);
        }
    }
}


VOID
OpenPropSheet(HWND hTreeView,
              HTREEITEM hItem)
{
    TV_ITEM tvItem;

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;

    if (TreeView_GetItem(hTreeView, &tvItem) &&
        (LPTSTR)tvItem.lParam != NULL)
    {
        DevicePropertiesExW(hTreeView,
                            NULL,
                            (LPTSTR)tvItem.lParam,
                            DPF_EXTENDED,
                            FALSE);
    }
}


static HTREEITEM
InsertIntoTreeView(HWND hTreeView,
                   HTREEITEM hRoot,
                   LPTSTR lpLabel,
                   LPTSTR DeviceID,
                   INT DevImage,
                   UINT OverlayImage)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = lstrlen(lpLabel);
    tvi.lParam = (LPARAM)DeviceID;
    tvi.iImage = DevImage;
    tvi.iSelectedImage = DevImage;

    if (OverlayImage != 0)
    {
        tvi.mask |= TVIF_STATE;
        tvi.stateMask = TVIS_OVERLAYMASK;
        tvi.state = INDEXTOOVERLAYMASK(OverlayImage);
    }

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTreeView, &tvins);
}


static
ULONG
GetClassCount(VOID)
{
    ULONG ulClassIndex;
    GUID ClassGuid;
    CONFIGRET cr;

    for (ulClassIndex = 0; ; ulClassIndex++)
    {
        cr = CM_Enumerate_Classes(ulClassIndex,
                                  &ClassGuid,
                                  0);
        if (cr == CR_NO_SUCH_VALUE)
            return ulClassIndex;
    }
}


static
PDEVCLASS_ENTRY
GetClassFromClassGuid(
    PDEVCLASS_ENTRY pClassArray,
    ULONG ulClassCount,
    GUID *pGuid)
{
    PDEVCLASS_ENTRY pClass, pUnknownClass = NULL;
    ULONG i;

    for (i = 0; i < ulClassCount; i++)
    {
        pClass = &pClassArray[i];

        if (IsEqualGUID(&pClass->ClassGuid, &GUID_DEVCLASS_UNKNOWN))
            pUnknownClass = pClass;

        if (IsEqualGUID(&pClass->ClassGuid, pGuid))
            return pClass;
    }

    return pUnknownClass;
}


static
VOID
EnumDeviceClasses(
    HWND hTreeView,
    HTREEITEM hRoot,
    PDEVCLASS_ENTRY pClassArray,
    ULONG ClassCount)
{
    WCHAR ClassName[MAX_DEV_LEN];
    WCHAR ClassDesc[MAX_DEV_LEN];
    PDEVCLASS_ENTRY pClass;
    ULONG ClassIndex;
    DWORD dwSize;
    LONG lSize;
    HKEY hKey;
    CONFIGRET cr;

    for (ClassIndex = 0; ClassIndex < ClassCount; ClassIndex++)
    {
        pClass = &pClassArray[ClassIndex];

        cr = CM_Enumerate_Classes(ClassIndex,
                                  &pClass->ClassGuid,
                                  0);
        if (cr == CR_NO_SUCH_VALUE)
            return;

        dwSize = MAX_CLASS_NAME_LEN;
        if (!SetupDiClassNameFromGuid(&pClass->ClassGuid,
                                      ClassName,
                                      dwSize,
                                      &dwSize))
        {
            ClassName[0] = _T('\0');
        }

        if (!SetupDiGetClassImageIndex(&ImageListData,
                                       &pClass->ClassGuid,
                                       &pClass->ClassImage))
        {
            /* FIXME: can we do this?
             * Set the blank icon: IDI_SETUPAPI_BLANK = 41
             * it'll be image 24 in the imagelist */
            pClass->ClassImage = 24;
        }

        hKey = SetupDiOpenClassRegKeyEx(&pClass->ClassGuid,
                                        MAXIMUM_ALLOWED,
                                        DIOCR_INSTALLER,
                                        NULL,
                                        0);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            lSize = MAX_DEV_LEN;
            if (RegQueryValue(hKey,
                              NULL,
                              ClassDesc,
                              &lSize) != ERROR_SUCCESS)
            {
                ClassDesc[0] = _T('\0');
            }

            RegCloseKey(hKey);
        }

        pClass->hItem = InsertIntoTreeView(hTreeView,
                                           hRoot,
                                           (ClassDesc[0] != _T('\0')) ? ClassDesc : ClassName,
                                           NULL,
                                           pClass->ClassImage,
                                           0);
    }
}


static
VOID
EnumDevices(
    HWND hTreeView,
    PDEVCLASS_ENTRY pClassArray,
    ULONG ulClassCount,
    BOOL bShowHidden)
{
    HDEVINFO hDevInfo =  INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    ULONG Status, Problem;
    DWORD DevIdSize;
    TCHAR DeviceName[MAX_DEV_LEN];
    DWORD DevIndex;
    LPTSTR InstanceId;
    PDEVCLASS_ENTRY pClass;
    UINT OverlayImage;
    CONFIGRET cr;


    /* Get device info for all devices of a particular class */
    hDevInfo = SetupDiGetClassDevs(NULL,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    for (DevIndex = 0; ; DevIndex++)
    {
        ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        InstanceId = NULL;
        DeviceName[0] = _T('\0');
        OverlayImage = 0;

        if (!SetupDiEnumDeviceInfo(hDevInfo,
                                   DevIndex,
                                   &DeviceInfoData))
            break;

        if (bShowHidden == FALSE &&
            (IsEqualGUID(&DeviceInfoData.ClassGuid, &GUID_DEVCLASS_LEGACYDRIVER) ||
             IsEqualGUID(&DeviceInfoData.ClassGuid, &GUID_DEVCLASS_VOLUME)))
            continue;

        pClass = GetClassFromClassGuid(pClassArray,
                                       ulClassCount,
                                       &DeviceInfoData.ClassGuid);

        /* get the device ID */
        if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                        &DeviceInfoData,
                                        NULL,
                                        0,
                                        &DevIdSize))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                InstanceId = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                             0,
                                             DevIdSize * sizeof(TCHAR));
                if (InstanceId != NULL)
                {
                    if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                                    &DeviceInfoData,
                                                    InstanceId,
                                                    DevIdSize,
                                                    NULL))
                    {
                        HeapFree(GetProcessHeap(),
                                 0,
                                 InstanceId);
                        InstanceId = NULL;
                    }
                }
            }
        }

        /* Skip the root device */
        if (InstanceId != NULL &&
            _tcscmp(InstanceId, _T("HTREE\\ROOT\\0")) == 0)
        {
            HeapFree(GetProcessHeap(),
                     0,
                     InstanceId);
            InstanceId = NULL;
            continue;
        }

        /* Get the device's friendly name */
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              0,
                                              (BYTE*)DeviceName,
                                              MAX_DEV_LEN,
                                              NULL))
        {
            /* If the friendly name fails, try the description instead */
            SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DeviceInfoData,
                                             SPDRP_DEVICEDESC,
                                             0,
                                             (BYTE*)DeviceName,
                                             MAX_DEV_LEN,
                                             NULL);
        }

        cr = CM_Get_DevNode_Status_Ex(&Status,
                                      &Problem,
                                      DeviceInfoData.DevInst,
                                      0,
                                      NULL);
        if ((cr == CR_SUCCESS) &&
            (Status & DN_HAS_PROBLEM))
        {
            if (Problem == CM_PROB_DISABLED ||
                Problem == CM_PROB_HARDWARE_DISABLED)
                OverlayImage = 2;
            else
                OverlayImage = 1;
        }

        InsertIntoTreeView(hTreeView,
                           pClass->hItem,
                           DeviceName,
                           InstanceId,
                           pClass->ClassImage,
                           OverlayImage);

        if (OverlayImage != 0)
        {
             /* Expand the class if the device has a problem */
             (void)TreeView_Expand(hTreeView,
                                   pClass->hItem,
                                   TVE_EXPAND);
        }

        pClass->bUsed = TRUE;
    }

    if (hDevInfo !=  INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(hDevInfo);
}


static
VOID
CleanupDeviceClasses(
    HWND hTreeView,
    PDEVCLASS_ENTRY pClassArray,
    ULONG ulClassCount)
{
    PDEVCLASS_ENTRY pClass;
    ULONG i;

    for (i = 0; i < ulClassCount; i++)
    {
        pClass = &pClassArray[i];

        if (pClass->bUsed == FALSE)
            (void)TreeView_DeleteItem(hTreeView,
                                      pClass->hItem);
        else
            (void)TreeView_SortChildren(hTreeView,
                                        pClass->hItem,
                                        0);
    }
}


VOID
ListDevicesByType(HWND hTreeView,
                  HTREEITEM hRoot,
                  BOOL bShowHidden)
{
    PDEVCLASS_ENTRY pClassArray;
    ULONG ulClassCount;

    ulClassCount = GetClassCount();

    pClassArray = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            ulClassCount * sizeof(DEVCLASS_ENTRY));
    if (pClassArray == NULL)
        return;

    EnumDeviceClasses(hTreeView,
                      hRoot,
                      pClassArray,
                      ulClassCount);

    EnumDevices(hTreeView,
                pClassArray,
                ulClassCount,
                bShowHidden);

    CleanupDeviceClasses(hTreeView,
                         pClassArray,
                         ulClassCount);

    if (pClassArray != NULL)
        HeapFree(GetProcessHeap(), 0, pClassArray);

    (void)TreeView_Expand(hTreeView,
                          hRoot,
                          TVE_EXPAND);

    (void)TreeView_SortChildren(hTreeView,
                                hRoot,
                                0);

    (void)TreeView_SelectItem(hTreeView,
                              hRoot);
}


static HTREEITEM
AddDeviceToTree(HWND hTreeView,
                HTREEITEM hRoot,
                DEVINST dnDevInst,
                BOOL bShowHidden)
{
    TCHAR DevName[MAX_DEV_LEN];
    TCHAR FriendlyName[MAX_DEV_LEN];
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuid;
    ULONG ulLength;
    LPTSTR DeviceID = NULL;
    INT ClassImage = 24;
    CONFIGRET cr;

    ulLength = MAX_GUID_STRING_LEN * sizeof(TCHAR);
    cr = CM_Get_DevNode_Registry_Property(dnDevInst,
                                          CM_DRP_CLASSGUID,
                                          NULL,
                                          ClassGuidString,
                                          &ulLength,
                                          0);
    if (cr == CR_SUCCESS)
    {
        pSetupGuidFromString(ClassGuidString, &ClassGuid);

        if (bShowHidden == FALSE &&
            (IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_LEGACYDRIVER) ||
             IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_VOLUME)))
            return NULL;
    }
    else
    {
        /* It's a device with no driver */
        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }

    cr = CM_Get_Device_ID(dnDevInst,
                          DevName,
                          MAX_DEV_LEN,
                          0);
    if (cr != CR_SUCCESS)
        return NULL;

    ulLength = MAX_DEV_LEN * sizeof(TCHAR);
    cr = CM_Get_DevNode_Registry_Property(dnDevInst,
                                          CM_DRP_FRIENDLYNAME,
                                          NULL,
                                          FriendlyName,
                                          &ulLength,
                                          0);
    if (cr != CR_SUCCESS)
    {
        ulLength = MAX_DEV_LEN * sizeof(TCHAR);
        cr = CM_Get_DevNode_Registry_Property(dnDevInst,
                                              CM_DRP_DEVICEDESC,
                                              NULL,
                                              FriendlyName,
                                              &ulLength,
                                              0);
        if (cr != CR_SUCCESS)
            return NULL;
    }

    if (!SetupDiGetClassImageIndex(&ImageListData,
                                   &ClassGuid,
                                   &ClassImage))
    {
        /* FIXME: can we do this?
         * Set the blank icon: IDI_SETUPAPI_BLANK = 41
         * it'll be image 24 in the imagelist */
        ClassImage = 24;
    }

    if (DevName != NULL)
    {
        DeviceID = HeapAlloc(GetProcessHeap(),
                             0,
                             (lstrlen(DevName) + 1) * sizeof(TCHAR));
        if (DeviceID == NULL)
        {
            return NULL;
        }

        lstrcpy(DeviceID, DevName);
    }

    return InsertIntoTreeView(hTreeView,
                              hRoot,
                              FriendlyName,
                              DeviceID,
                              ClassImage,
                              0);
}


static VOID
EnumChildDevices(HWND hTreeView,
                 HTREEITEM hRoot,
                 DEVINST dnParentDevInst,
                 BOOL bShowHidden)
{
    HTREEITEM hDevItem;
    DEVINST dnDevInst;
    CONFIGRET cr;

    cr = CM_Get_Child(&dnDevInst,
                      dnParentDevInst,
                      0);
    if (cr != CR_SUCCESS)
        return;

    hDevItem = AddDeviceToTree(hTreeView,
                               hRoot,
                               dnDevInst,
                               bShowHidden);
    if (hDevItem != NULL)
    {
        EnumChildDevices(hTreeView,
                         hDevItem,
                         dnDevInst,
                         bShowHidden);
    }

    while (cr == CR_SUCCESS)
    {
        cr = CM_Get_Sibling(&dnDevInst,
                            dnDevInst,
                            0);
        if (cr != CR_SUCCESS)
            break;

        hDevItem = AddDeviceToTree(hTreeView,
                                   hRoot,
                                   dnDevInst,
                                   bShowHidden);
        if (hDevItem != NULL)
        {
            EnumChildDevices(hTreeView,
                             hDevItem,
                             dnDevInst,
                             bShowHidden);
        }
    }

    (void)TreeView_SortChildren(hTreeView,
                                hRoot,
                                0);
}


VOID
ListDevicesByConnection(HWND hTreeView,
                        HTREEITEM hRoot,
                        BOOL bShowHidden)
{
    DEVINST devInst;
    CONFIGRET cr;

    cr = CM_Locate_DevNode(&devInst,
                           NULL,
                           CM_LOCATE_DEVNODE_NORMAL);
    if (cr == CR_SUCCESS)
        EnumChildDevices(hTreeView,
                         hRoot,
                         devInst,
                         bShowHidden);

    (void)TreeView_Expand(hTreeView,
                          hRoot,
                          TVE_EXPAND);

    (void)TreeView_SelectItem(hTreeView,
                              hRoot);
}


HTREEITEM
InitTreeView(HWND hTreeView)
{
    HTREEITEM hRoot;
    TCHAR ComputerName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    INT RootImage;

    (void)TreeView_DeleteAllItems(hTreeView);

    /* Get the device image list */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    (void)TreeView_SetImageList(hTreeView,
                                ImageListData.ImageList,
                                TVSIL_NORMAL);

    if (!GetComputerName(ComputerName,
                         &dwSize))
    {
        ComputerName[0] = _T('\0');
    }

    /* Get the image index of the computer class */
    SetupDiGetClassImageIndex(&ImageListData,
                              &GUID_DEVCLASS_COMPUTER,
                              &RootImage);

    /* Insert the root item into the tree */
    hRoot = InsertIntoTreeView(hTreeView,
                               NULL,
                               ComputerName,
                               NULL,
                               RootImage,
                               0);

    return hRoot;
}
