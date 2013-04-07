/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/devmgmt/enumdevices.c
 * PURPOSE:     Enumerates all devices on the local machine
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static SP_CLASSIMAGELIST_DATA ImageListData;
static HDEVINFO hDevInfo;

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

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
                   LONG DevProb)
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

    if (DevProb != 0)
    {
        tvi.stateMask = TVIS_OVERLAYMASK;

        if (DevProb == CM_PROB_DISABLED)
        {
            /* FIXME: set the overlay icon */
        }

    }

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTreeView, &tvins);
}


static INT
EnumDeviceClasses(INT ClassIndex,
                  BOOL ShowHidden,
                  LPTSTR DevClassName,
                  LPTSTR DevClassDesc,
                  BOOL *DevPresent,
                  INT *ClassImage,
                  BOOL *IsUnknown,
                  BOOL *IsHidden)
{
    GUID ClassGuid;
    HKEY KeyClass;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD RequiredSize = MAX_CLASS_NAME_LEN;
    UINT Ret;

    *DevPresent = FALSE;
    *DevClassName = _T('\0');
    *IsHidden = FALSE;

    Ret = CM_Enumerate_Classes(ClassIndex,
                               &ClassGuid,
                               0);
    if (Ret != CR_SUCCESS)
    {
        /* all classes enumerated */
        if(Ret == CR_NO_SUCH_VALUE)
        {
            return -1;
        }

        if (Ret == CR_INVALID_DATA)
        {
            ; /*FIXME: what should we do here? */
        }

        /* handle other errors... */
    }

    /* This case is special because these devices don't show up with normal class enumeration */
    *IsUnknown = IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_UNKNOWN);

    if (ShowHidden == FALSE &&
        (IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_LEGACYDRIVER) ||
         IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_VOLUME)))
        *IsHidden = TRUE;

    if (SetupDiClassNameFromGuid(&ClassGuid,
                                 ClassName,
                                 RequiredSize,
                                 &RequiredSize))
    {
        lstrcpy(DevClassName, ClassName);
    }

    if (!SetupDiGetClassImageIndex(&ImageListData,
                                   &ClassGuid,
                                   ClassImage))
    {
        /* FIXME: can we do this?
         * Set the blank icon: IDI_SETUPAPI_BLANK = 41
         * it'll be image 24 in the imagelist */
        *ClassImage = 24;
    }

    /* Get device info for all devices of a particular class */
    hDevInfo = SetupDiGetClassDevs(*IsUnknown ? NULL : &ClassGuid,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT | (*IsUnknown ? DIGCF_ALLCLASSES : 0));
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    KeyClass = SetupDiOpenClassRegKeyEx(&ClassGuid,
                                        MAXIMUM_ALLOWED,
                                        DIOCR_INSTALLER,
                                        NULL,
                                        0);
    if (KeyClass != INVALID_HANDLE_VALUE)
    {

        LONG dwSize = MAX_DEV_LEN;

        if (RegQueryValue(KeyClass,
                          NULL,
                          DevClassDesc,
                          &dwSize) != ERROR_SUCCESS)
        {
            *DevClassDesc = _T('\0');
        }
    }
    else
    {
        return 0;
    }

    *DevPresent = TRUE;

    RegCloseKey(KeyClass);

    return 0;
}


static LONG
EnumDevices(INT index,
            LPTSTR DeviceClassName,
            LPTSTR DeviceName,
            LPTSTR *DeviceID)
{
    SP_DEVINFO_DATA DeviceInfoData;
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    DWORD DevIdSize;

    *DeviceName = _T('\0');
    *DeviceID = NULL;

    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiEnumDeviceInfo(hDevInfo,
                               index,
                               &DeviceInfoData))
    {
        /* no such device */
        return -1;
    }

    if (DeviceClassName == NULL && !IsEqualGUID(&DeviceInfoData.ClassGuid, &GUID_NULL))
    {
        /* we're looking for unknown devices and this isn't one */
        return -2;
    }

    /* get the device ID */
    if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                    &DeviceInfoData,
                                    NULL,
                                    0,
                                    &DevIdSize))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            (*DeviceID) = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                            0,
                                            DevIdSize * sizeof(TCHAR));
            if (*DeviceID)
            {
                if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                                &DeviceInfoData,
                                                *DeviceID,
                                                DevIdSize,
                                                NULL))
                {
                    HeapFree(GetProcessHeap(),
                             0,
                             *DeviceID);
                    *DeviceID = NULL;
                }
            }
        }
    }

    if (DeviceID != NULL &&
        _tcscmp(*DeviceID, _T("HTREE\\ROOT\\0")) == 0)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 *DeviceID);
        *DeviceID = NULL;
        return -1;
    }

    /* get the device's friendly name */
    if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                          &DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          0,
                                          (BYTE*)DeviceName,
                                          MAX_DEV_LEN,
                                          NULL))
    {
        /* if the friendly name fails, try the description instead */
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         0,
                                         (BYTE*)DeviceName,
                                         MAX_DEV_LEN,
                                         NULL);
    }

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DeviceInfoData.DevInst,
                                  0,
                                  NULL);
    if (cr == CR_SUCCESS && (Status & DN_HAS_PROBLEM))
    {
        return ProblemNumber;
    }

    return 0;
}


VOID
ListDevicesByType(HWND hTreeView,
                  HTREEITEM hRoot,
                  BOOL bShowHidden)
{
    HTREEITEM hDevItem;
    TCHAR DevName[MAX_DEV_LEN];
    TCHAR DevDesc[MAX_DEV_LEN];
    LPTSTR DeviceID = NULL;
    BOOL DevExist = FALSE;
    INT ClassRet;
    INT index = 0;
    INT DevImage;
    BOOL IsUnknown = FALSE;
    BOOL IsHidden = FALSE;

    do
    {
        ClassRet = EnumDeviceClasses(index,
                                     bShowHidden,
                                     DevName,
                                     DevDesc,
                                     &DevExist,
                                     &DevImage,
                                     &IsUnknown,
                                     &IsHidden);

        if ((ClassRet != -1) && (DevExist) && !IsHidden)
        {
            TCHAR DeviceName[MAX_DEV_LEN];
            INT DevIndex = 0;
            LONG Ret;

            if (DevDesc[0] != _T('\0'))
            {
                hDevItem = InsertIntoTreeView(hTreeView,
                                              hRoot,
                                              DevDesc,
                                              NULL,
                                              DevImage,
                                              0);
            }
            else
            {
                hDevItem = InsertIntoTreeView(hTreeView,
                                              hRoot,
                                              DevName,
                                              NULL,
                                              DevImage,
                                              0);
            }

            do
            {
                Ret = EnumDevices(DevIndex,
                                  IsUnknown ? NULL : DevName,
                                  DeviceName,
                                  &DeviceID);
                if (Ret >= 0)
                {
                    InsertIntoTreeView(hTreeView,
                                       hDevItem,
                                       DeviceName,
                                       DeviceID,
                                       DevImage,
                                       Ret);
                    if (Ret != 0)
                    {
                        /* Expand the class if the device has a problem */
                        (void)TreeView_Expand(hTreeView,
                                              hDevItem,
                                              TVE_EXPAND);
                    }
                }

                DevIndex++;

            } while (Ret != -1);

            /* kill InfoList initialized in EnumDeviceClasses */
            if (hDevInfo)
            {
                SetupDiDestroyDeviceInfoList(hDevInfo);
                hDevInfo = NULL;
            }

            /* don't insert classes with no devices */
            if (!TreeView_GetChild(hTreeView,
                                   hDevItem))
            {
                (void)TreeView_DeleteItem(hTreeView,
                                          hDevItem);
            }
            else
            {
                (void)TreeView_SortChildren(hTreeView,
                                            hDevItem,
                                            0);
            }
        }

        index++;

    } while (ClassRet != -1);

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
    HBITMAP hComp;
    TCHAR ComputerName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    INT RootImage;

    (void)TreeView_DeleteAllItems(hTreeView);

    /* get the device image List */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    hComp = LoadBitmap(hInstance,
                       MAKEINTRESOURCE(IDB_ROOT_IMAGE));

    ImageList_Add(ImageListData.ImageList,
                  hComp,
                  NULL);

    DeleteObject(hComp);

    (void)TreeView_SetImageList(hTreeView,
                                ImageListData.ImageList,
                                TVSIL_NORMAL);

    if (!GetComputerName(ComputerName,
                         &dwSize))
    {
        ComputerName[0] = _T('\0');
    }

    RootImage = ImageList_GetImageCount(ImageListData.ImageList) - 1;

    /* insert the root item into the tree */
    hRoot = InsertIntoTreeView(hTreeView,
                               NULL,
                               ComputerName,
                               NULL,
                               RootImage,
                               0);

    return hRoot;
}
