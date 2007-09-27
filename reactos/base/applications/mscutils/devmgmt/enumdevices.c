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
                                  DevName,
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
