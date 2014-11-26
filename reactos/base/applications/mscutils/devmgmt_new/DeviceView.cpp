/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/mscutils/devmgmt/deviceview.cpp
* PURPOSE:     Implements the tree view which holds the devices
* COPYRIGHT:   Copyright 2014 Ged Murphy <gedmurphy@gmail.com>
*
*/


#include "stdafx.h"
#include "devmgmt.h"
#include "DeviceView.h"


/* DATA *********************************************/

#define DEVICE_NAME_LEN     256
#define CLASS_NAME_LEN      256
#define CLASS_DESC_LEN      256

INT_PTR
WINAPI
DevicePropertiesExW(
    IN HWND hWndParent OPTIONAL,
    IN LPCWSTR lpMachineName OPTIONAL,
    IN LPCWSTR lpDeviceID OPTIONAL,
    IN DWORD dwFlags OPTIONAL,
    IN BOOL bShowDevMgr
);

typedef INT_PTR(WINAPI *pDevicePropertiesExW)(HWND,LPCWSTR,LPCWSTR,DWORD,BOOL);


/* PUBLIC METHODS *************************************/

CDeviceView::CDeviceView(
    HWND hMainWnd
    ) :
    m_Devices(NULL),
    m_hMainWnd(hMainWnd),
    m_hTreeView(NULL),
    m_hPropertyDialog(NULL),
    m_hShortcutMenu(NULL),
    m_ListDevices(DevicesByType),
    m_ShowHidden(FALSE),
    m_ShowUnknown(TRUE)
{
    m_Devices = new CDevices();
}

CDeviceView::~CDeviceView(void)
{
    delete m_Devices;
    m_Devices = NULL;
}

BOOL
CDeviceView::Initialize()
{
    BOOL bSuccess;

    /* Initialize the devices class */
    bSuccess = m_Devices->Initialize();
    if (bSuccess == FALSE) return FALSE;

    /* Create the main treeview */
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                  WC_TREEVIEW,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES |
                                   TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT,
                                  0, 0, 0, 0,
                                  m_hMainWnd,
                                  (HMENU)IDC_TREEVIEW,
                                  g_hInstance,
                                  NULL);
    if (m_hTreeView)
    {
        /* Get the devices image list */
        m_ImageList = m_Devices->GetImageList();

        /* Set the image list against the treeview */
        (VOID)TreeView_SetImageList(m_hTreeView,
                                    m_ImageList,
                                    TVSIL_NORMAL);

        /* Display the devices */
        Refresh();
    }

    return !!(m_hTreeView);
}

BOOL
CDeviceView::Uninitialize()
{
    (VOID)m_Devices->Uninitialize();

    return TRUE;
}

VOID
CDeviceView::Size(
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy
    )
{
    /* Resize the treeview */
    SetWindowPos(m_hTreeView,
                 NULL,
                 x,
                 y,
                 cx,
                 cy,
                 SWP_NOZORDER);
}


VOID
CDeviceView::Refresh()
{
    HANDLE hThread;

    /* Run on a new thread to keep the gui responsive */
    hThread = (HANDLE)_beginthreadex(NULL,
                                     0,
                                     &ListDevicesThread,
                                     this,
                                     0,
                                     NULL);

    if (hThread) CloseHandle(hThread);
}

VOID
CDeviceView::DisplayPropertySheet()
{
#ifndef __REACTOS__
    pDevicePropertiesExW DevicePropertiesExW;
    HMODULE hModule;

    hModule = LoadLibraryW(L"devmgr.dll");
    if (hModule == NULL) return;

    DevicePropertiesExW = (pDevicePropertiesExW)GetProcAddress(hModule,
                                                               "DevicePropertiesExW");
    if (DevicePropertiesExW == NULL)
    {
        FreeLibrary(hModule);
        return;
    }
#endif

    TV_ITEM tvItem;

    tvItem.hItem = TreeView_GetSelection(m_hTreeView);
    tvItem.mask = TVIF_PARAM;

    if (TreeView_GetItem(m_hTreeView, &tvItem) &&
        (LPTSTR)tvItem.lParam != NULL)
    {
        DevicePropertiesExW(m_hTreeView,
                            NULL,
                            (LPTSTR)tvItem.lParam,
                            1,//DPF_EXTENDED,
                            FALSE);
    }

#ifndef __REACTOS__
    FreeLibrary(hModule);
#endif
}

VOID
CDeviceView::SetFocus()
{
}


/* PRIVATE METHODS ********************************************/

unsigned int __stdcall CDeviceView::ListDevicesThread(void *Param)
{
    CDeviceView *This = (CDeviceView *)Param;

    /* Check if there are any items in the tree */
    if (TreeView_GetRoot(This->m_hTreeView) != NULL)
    {
        /* Delete all the items */
        (VOID)TreeView_DeleteAllItems(This->m_hTreeView);
    }

    /* Reset the tree root */
    This->m_hTreeRoot = NULL;

    switch (This->m_ListDevices)
    {
    case DevicesByType:
        (VOID)This->ListDevicesByType();
        break;

    case DevicesByConnection:
        (VOID)This->ListDevicesByConnection();
        break;

    case ResourcesByType:
        break;

    case ResourcesByConnection:
        break;
    }
    return 0;
}


BOOL
CDeviceView::ListDevicesByType()
{
    HTREEITEM hDevItem = NULL;
    GUID ClassGuid;
    WCHAR ClassName[CLASS_NAME_LEN];
    WCHAR ClassDescription[CLASS_DESC_LEN];
    INT ClassIndex;
    INT ClassImage;
    LPTSTR DeviceId = NULL;
    
    BOOL IsUnknown = FALSE;
    BOOL IsHidden = FALSE;
    BOOL bSuccess;


    /* Get the details of the root of the device tree */
    bSuccess = m_Devices->GetDeviceTreeRoot(ClassName, CLASS_NAME_LEN, &ClassImage);
    if (bSuccess)
    {
        /* Add the root of the device tree to the treeview */
        m_hTreeRoot = InsertIntoTreeView(NULL,
                                         ClassName,
                                         NULL,
                                         ClassImage,
                                         0);
    }

    /* If something went wrong, bail */
    if (m_hTreeRoot == NULL) return FALSE;

    ClassIndex = 0;
    do
    {
        /* Get the next device class */
        bSuccess = m_Devices->EnumClasses(ClassIndex,
                                          &ClassGuid,
                                          ClassName,
                                          CLASS_NAME_LEN,
                                          ClassDescription,
                                          CLASS_DESC_LEN,
                                          &ClassImage,
                                          &IsUnknown,
                                          &IsHidden);
        if (bSuccess &&
            (IsUnknown == FALSE || (IsUnknown && m_ShowUnknown)) &&
            (IsHidden == FALSE || (IsHidden && m_ShowHidden)))
        {
            BOOL bDevSuccess, AddedParent;
            HANDLE Handle = NULL;
            WCHAR DeviceName[DEVICE_NAME_LEN];
            INT DeviceIndex = 0;
            BOOL MoreItems = FALSE;
            BOOL DeviceHasProblem = FALSE;
            ULONG DeviceStatus, ProblemNumber;
            ULONG OverlayImage = 0;

            AddedParent = FALSE;

            do
            {
                /* Enum all the devices that belong to this class */
                bDevSuccess = m_Devices->EnumDevicesForClass(&Handle,
                                                             &ClassGuid,
                                                             DeviceIndex,
                                                             &MoreItems,
                                                             DeviceName,
                                                             DEVICE_NAME_LEN,
                                                             &DeviceId);
                if (bDevSuccess)
                {
                    /* We have a device, we're gonna need to add the parent first */
                    if (AddedParent == FALSE)
                    {
                        /* Insert the new class under the root item */
                        hDevItem = InsertIntoTreeView(m_hTreeRoot,
                                                      ClassDescription,
                                                      NULL,
                                                      ClassImage,
                                                      0);

                        /* Don't add it again */
                        AddedParent = TRUE;
                    }

                    /* Get the status of the device */
                    if (m_Devices->GetDeviceStatus(DeviceId,
                                                   &DeviceStatus,
                                                   &ProblemNumber))
                    {
                        /* Check if the device has a problem */
                        if (DeviceStatus & DN_HAS_PROBLEM)
                        {
                            DeviceHasProblem = TRUE;
                            OverlayImage = 1;
                        }

                        /* The disabled overlay takes precidence over the problem overlay */
                        if (ProblemNumber == CM_PROB_DISABLED ||
                            ProblemNumber == CM_PROB_HARDWARE_DISABLED)
                        {
                            OverlayImage = 2;
                        }
                    }

                    /* Add the device under the class item */
                    (VOID)InsertIntoTreeView(hDevItem,
                                             DeviceName,
                                             (LPARAM)DeviceId,
                                             ClassImage,
                                             OverlayImage);

                    /* Check if there's a problem with the device */
                    if (DeviceHasProblem)
                    {
                        /* Expand the class */
                        (VOID)TreeView_Expand(m_hTreeView,
                                              hDevItem,
                                              TVE_EXPAND);
                    }
                }

                DeviceIndex++;

            } while (MoreItems);

            /* Check if this class has any devices */
            if (AddedParent == TRUE)
            {
                /* Sort the devices alphabetically */
                (VOID)TreeView_SortChildren(m_hTreeView,
                                            hDevItem,
                                            0);
            }
        }

        ClassIndex++;

    } while (bSuccess);

    /* Sort the classes alphabetically */
    (VOID)TreeView_SortChildren(m_hTreeView,
                                m_hTreeRoot,
                                0);

    /* Expand the root item */
    (VOID)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    /* Pre-select the root item */
    (VOID)TreeView_SelectItem(m_hTreeView,
                              m_hTreeRoot);

    return 0;
}

BOOL
CDeviceView::ListDevicesByConnection()
{
    WCHAR DeviceName[DEVICE_NAME_LEN];
    INT ClassImage;
    BOOL bSuccess;

    /* Get the details of the root of the device tree */
    bSuccess = m_Devices->GetDeviceTreeRoot(DeviceName, DEVICE_NAME_LEN, &ClassImage);
    if (bSuccess)
    {
        /* Add the root of the device tree to the treeview */
        m_hTreeRoot = InsertIntoTreeView(NULL,
                                         DeviceName,
                                         NULL,
                                         ClassImage,
                                         0);
    }

    /* Walk the device tree and add all the devices */
    RecurseChildDevices(m_Devices->GetRootDevice(), m_hTreeRoot);

    /* Expand the root item */
    (VOID)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    return TRUE;
}

VOID
CDeviceView::RecurseChildDevices(
    _In_ DEVINST ParentDevice,
    _In_ HTREEITEM hParentTreeItem
    )
{
    HTREEITEM hDevItem = NULL;
    DEVINST Device;
    WCHAR DeviceName[DEVICE_NAME_LEN];
    LPTSTR DeviceId = NULL;
    INT ClassImage;
    BOOL IsUnknown = FALSE;
    BOOL IsHidden = FALSE;
    BOOL bSuccess;

    /* Check if the parent has any child devices */
    if (m_Devices->GetChildDevice(ParentDevice, &Device) == FALSE)
        return;

    /* Lookup the info about this device */
    bSuccess = m_Devices->GetDevice(Device,
                                    DeviceName,
                                    DEVICE_NAME_LEN,
                                    &DeviceId,
                                    &ClassImage,
                                    &IsUnknown,
                                    &IsHidden);
    if (bSuccess)
    {
        /* Add this device to the tree under its parent */
        hDevItem = InsertIntoTreeView(hParentTreeItem,
                                        DeviceName,
                                        (LPARAM)DeviceId,
                                        ClassImage,
                                        0);
        if (hDevItem)
        {
            /* Check if this child has any children itself */
            RecurseChildDevices(Device, hDevItem);
        }
    }


    for (;;)
    {
        /* Check if the parent device has anything at the same level */
        bSuccess = m_Devices->GetSiblingDevice(Device, &Device);
        if (bSuccess == FALSE) break;

        /* Lookup the info about this device */
        bSuccess = m_Devices->GetDevice(Device,
                                        DeviceName,
                                        DEVICE_NAME_LEN,
                                        &DeviceId,
                                        &ClassImage,
                                        &IsUnknown,
                                        &IsHidden);
        if (bSuccess)
        {
            /* Add this device to the tree under its parent */
            hDevItem = InsertIntoTreeView(hParentTreeItem,
                                            DeviceName,
                                            (LPARAM)DeviceId,
                                            ClassImage,
                                            0);
            if (hDevItem)
            {
                /* Check if this child has any children itself */
                RecurseChildDevices(Device, hDevItem);
            }
        }
    }

    (void)TreeView_SortChildren(m_hTreeView,
                                hParentTreeItem,
                                0);
}

 HTREEITEM
CDeviceView::InsertIntoTreeView(
    _In_ HTREEITEM hParent,
    _In_z_ LPWSTR lpLabel,
    _In_ LPARAM lParam,
    _In_ INT DevImage,
    _In_ UINT OverlayImage
    )
{
    TV_ITEMW tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = wcslen(lpLabel);
    tvi.lParam = lParam;
    tvi.iImage = DevImage;
    tvi.iSelectedImage = DevImage;

    if (OverlayImage != 0)
    {
        tvi.mask |= TVIF_STATE;
        tvi.stateMask = TVIS_OVERLAYMASK;
        tvi.state = INDEXTOOVERLAYMASK(OverlayImage);
    }

    tvins.item = tvi;
    tvins.hParent = hParent;

    return TreeView_InsertItem(m_hTreeView, &tvins);
}
