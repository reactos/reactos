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

VOID
FreeDeviceStrings(HWND hTV)
{
    HTREEITEM hItem;

    hItem = TreeView_GetRoot(hTV);

    if (hItem)
    {
        hItem = TreeView_GetChild(hTV,
                                  hItem);
        /* loop the parent items */
        while (hItem)
        {
            hItem = TreeView_GetChild(hTV,
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

                (void)TreeView_GetItem(hTV, &tvItem);

                //MessageBox(NULL, Buf, NULL, 0);

                HeapFree(GetProcessHeap(),
                         0,
                         (LPTSTR)tvItem.lParam);

                hOldItem = hItem;

                hItem = TreeView_GetNextSibling(hTV,
                                                hItem);
                if (hItem == NULL)
                {
                    hItem = hOldItem;
                    break;
                }
            }

            hItem = TreeView_GetParent(hTV,
                                       hItem);
            hItem = TreeView_GetNextSibling(hTV,
                                            hItem);
        }
    }
}


VOID
OpenPropSheet(HWND hTV,
              HTREEITEM hItem)
{
    TV_ITEM tvItem;

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;

    if (TreeView_GetItem(hTV, &tvItem) &&
        (LPTSTR)tvItem.lParam != NULL)
    {
        DevicePropertiesExW(hTV,
                            NULL,
                            (LPTSTR)tvItem.lParam,
                            0,
                            FALSE);
    }

}


static HTREEITEM
InsertIntoTreeView(HWND hTV,
                   HTREEITEM hRoot,
                   LPTSTR lpLabel,
                   LPTSTR DeviceID,
                   INT DevImage)
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

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTV, &tvins);
}


static INT
EnumDeviceClasses(INT ClassIndex,
                  LPTSTR DevClassName,
                  LPTSTR DevClassDesc,
                  BOOL *DevPresent,
                  INT *ClassImage)
{
    GUID ClassGuid;
    HKEY KeyClass;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD RequiredSize = MAX_CLASS_NAME_LEN;
    UINT Ret;

    *DevPresent = FALSE;
    *DevClassName = _T('\0');

    Ret = CM_Enumerate_Classes(ClassIndex,
                               &ClassGuid,
                               0);
    if (Ret != CR_SUCCESS)
    {
        /* all classes enumerated */
        if(Ret == CR_NO_SUCH_VALUE)
        {
            hDevInfo = NULL;
            return -1;
        }

        if (Ret == CR_INVALID_DATA)
            ; /*FIXME: what should we do here? */

        /* handle other errors... */
    }

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
    hDevInfo = SetupDiGetClassDevs(&ClassGuid,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        hDevInfo = NULL;
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
        return -3;
    }

    *DevPresent = TRUE;

    RegCloseKey(KeyClass);

    return 0;
}


static INT
EnumDevices(INT index,
            LPTSTR DeviceClassName,
            LPTSTR DeviceName,
            LPTSTR *DeviceID)
{
    SP_DEVINFO_DATA DeviceInfoData;
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
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DeviceInfoData,
                                              SPDRP_DEVICEDESC,
                                              0,
                                              (BYTE*)DeviceName,
                                              MAX_DEV_LEN,
                                              NULL))
        {
            /* if the description fails, just give up! */
            return -2;
        }
    }

    return 0;
}


VOID
ListDevicesByType(PMAIN_WND_INFO Info,
                  HTREEITEM hRoot)
{
    HTREEITEM hDevItem;
    TCHAR DevName[MAX_DEV_LEN];
    TCHAR DevDesc[MAX_DEV_LEN];
    LPTSTR DeviceID = NULL;
    BOOL DevExist = FALSE;
    INT ClassRet;
    INT index = 0;
    INT DevImage;

    do
    {
        ClassRet = EnumDeviceClasses(index,
                                     DevName,
                                     DevDesc,
                                     &DevExist,
                                     &DevImage);

        if ((ClassRet != -1) && (DevExist))
        {
            TCHAR DeviceName[MAX_DEV_LEN];
            INT Ret, DevIndex = 0;

            if (DevDesc[0] != _T('\0'))
            {
                hDevItem = InsertIntoTreeView(Info->hTreeView,
                                              hRoot,
                                              DevDesc,
                                              NULL,
                                              DevImage);
            }
            else
            {
                hDevItem = InsertIntoTreeView(Info->hTreeView,
                                              hRoot,
                                              DevName,
                                              NULL,
                                              DevImage);
            }

            do
            {
                Ret = EnumDevices(DevIndex,
                                  DevName,
                                  DeviceName,
                                  &DeviceID);
                if (Ret == 0)
                {
                    InsertIntoTreeView(Info->hTreeView,
                                       hDevItem,
                                       DeviceName,
                                       DeviceID,
                                       DevImage);
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
            if (!TreeView_GetChild(Info->hTreeView,
                                   hDevItem))
            {
                (void)TreeView_DeleteItem(Info->hTreeView,
                                          hDevItem);
            }
            else
            {
                (void)TreeView_SortChildren(Info->hTreeView,
                                            hDevItem,
                                            0);
            }
        }

        index++;

    } while (ClassRet != -1);

    (void)TreeView_Expand(Info->hTreeView,
                          hRoot,
                          TVE_EXPAND);

    (void)TreeView_SortChildren(Info->hTreeView,
                                hRoot,
                                0);
}


HTREEITEM
InitTreeView(PMAIN_WND_INFO Info)
{
    HTREEITEM hRoot;
    HBITMAP hComp;
    TCHAR ComputerName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    INT RootImage;

    (void)TreeView_DeleteAllItems(Info->hTreeView);

    /* get the device image List */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    hComp = LoadBitmap(hInstance,
                       MAKEINTRESOURCE(IDB_ROOT_IMAGE));

    ImageList_Add(ImageListData.ImageList,
                  hComp,
                  NULL);

    DeleteObject(hComp);

    (void)TreeView_SetImageList(Info->hTreeView,
                                ImageListData.ImageList,
                                TVSIL_NORMAL);

    if (!GetComputerName(ComputerName,
                         &dwSize))
    {
        ComputerName[0] = _T('\0');
    }

    RootImage = ImageList_GetImageCount(ImageListData.ImageList) - 1;

    /* insert the root item into the tree */
    hRoot = InsertIntoTreeView(Info->hTreeView,
                               NULL,
                               ComputerName,
                               NULL,
                               RootImage);

    return hRoot;
}
