/*
 * PROJECT:     ReactOS Device Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/devmgr/devmgr/DeviceView.cpp
 * PURPOSE:     Implements the tree view which contains the devices
 * COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
 */



#include "precomp.h"
#include "devmgmt.h"
#include "DeviceView.h"


// DATA ********************************************/

#define CLASS_NAME_LEN      256
#define CLASS_DESC_LEN      256
#define ROOT_NAME_SIZE      MAX_COMPUTERNAME_LENGTH + 1

extern "C" {
INT_PTR
WINAPI
DevicePropertiesExW(
    IN HWND hWndParent OPTIONAL,
    IN LPCWSTR lpMachineName OPTIONAL,
    IN LPCWSTR lpDeviceID OPTIONAL,
    IN DWORD dwFlags OPTIONAL,
    IN BOOL bShowDevMgr
);
}
typedef INT_PTR(WINAPI *pDevicePropertiesExW)(HWND,LPCWSTR,LPCWSTR,DWORD,BOOL);

struct RefreshThreadData
{
    CDeviceView *This;
    BOOL ScanForChanges;
    BOOL UpdateView;
    LPWSTR DeviceId;
};


// PUBLIC METHODS ************************************/

CDeviceView::CDeviceView(
    HWND hMainWnd
    ) :
    m_hMainWnd(hMainWnd),
    m_hTreeView(NULL),
    m_hPropertyDialog(NULL),
    m_hMenu(NULL),
    m_ViewType(DevicesByType),
    m_ShowHidden(FALSE),
    m_RootNode(NULL)
{
    ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
}

CDeviceView::~CDeviceView(void)
{
}

bool
CDeviceView::Initialize()
{
    // Get the device image list
    m_ImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    BOOL bSuccess = SetupDiGetClassImageList(&m_ImageListData);
    if (bSuccess == FALSE) return false;

    // Create the main treeview
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                  WC_TREEVIEW,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES |
                                  TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT,
                                  0, 0, 0, 0,
                                  m_hMainWnd,
                                  (HMENU)IDC_TREEVIEW,
                                  g_hThisInstance,
                                  NULL);
    if (m_hTreeView)
    {
        // Set the image list against the treeview
        (void)TreeView_SetImageList(m_hTreeView,
                                    m_ImageListData.ImageList,
                                    TVSIL_NORMAL);

        // Give the treeview arrows instead of +/- boxes (on Win7)
        SetWindowTheme(m_hTreeView, L"explorer", NULL);
    }



    return !!(m_hTreeView);
}

bool
CDeviceView::Uninitialize()
{
    EmptyDeviceView();

    if (m_ImageListData.ImageList != NULL)
    {
        SetupDiDestroyClassImageList(&m_ImageListData);
        ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
    }

    return true;
}

LRESULT
CDeviceView::OnSize(
    _In_ int x,
    _In_ int y,
    _In_ int cx,
    _In_ int cy
    )
{
    // Resize the treeview
    SetWindowPos(m_hTreeView,
                 NULL,
                 x,
                 y,
                 cx,
                 cy,
                 SWP_NOZORDER);

    return 0;
}

LRESULT
CDeviceView::OnRightClick(
    _In_ LPNMHDR NmHdr
    )
{
    HTREEITEM hItem = TreeView_GetNextItem(NmHdr->hwndFrom, 0, TVGN_DROPHILITE);
    if (hItem)
    {
        TreeView_SelectItem(NmHdr->hwndFrom, hItem);
    }

    return 0;
}

LRESULT
CDeviceView::OnContextMenu(
    _In_ LPARAM lParam
    )
{
    HTREEITEM hSelected = TreeView_GetSelection(m_hTreeView);

    RECT rc;
    if (TreeView_GetItemRect(m_hTreeView,
                             hSelected,
                             &rc,
                             TRUE))
    {
        POINT pt;
        if (GetCursorPos(&pt) &&
            ScreenToClient(m_hTreeView, &pt) &&
            PtInRect(&rc, pt))
        {
            CNode *Node = GetSelectedNode();
            if (Node)
            {
                // Create the context menu
                HMENU hContextMenu = CreatePopupMenu();

                // Add the actions for this node
                BuildActionMenuForNode(hContextMenu, Node, false);

                INT xPos = GET_X_LPARAM(lParam);
                INT yPos = GET_Y_LPARAM(lParam);

                // Display the menu
                TrackPopupMenuEx(hContextMenu,
                                 TPM_RIGHTBUTTON,
                                 xPos,
                                 yPos,
                                 m_hMainWnd,
                                 NULL);

                DestroyMenu(hContextMenu);
            }
        }
    }

    return 0;
}


void
CDeviceView::Refresh(
    _In_ ViewType Type,
    _In_ bool ScanForChanges,
    _In_ bool UpdateView,
    _In_opt_ LPWSTR DeviceId
    )
{
    // Enum devices on a seperate thread to keep the gui responsive

    m_ViewType = Type;

    RefreshThreadData *ThreadData;
    ThreadData = new RefreshThreadData;
    ThreadData->This = this;
    ThreadData->ScanForChanges = ScanForChanges;
    ThreadData->UpdateView = UpdateView;
    ThreadData->DeviceId = NULL;

    if (DeviceId)
    {
        // Node gets deleted on refresh so we copy it to another block
        size_t Length = wcslen(DeviceId) + 1;
        ThreadData->DeviceId = new WCHAR[Length];
        StringCbCopyW(ThreadData->DeviceId, Length, DeviceId);
    }


    HANDLE hThread;
    hThread = (HANDLE)_beginthreadex(NULL,
                                     0,
                                     RefreshThread,
                                     ThreadData,
                                     0,
                                     NULL);
    if (hThread) CloseHandle(hThread);
}

LRESULT
CDeviceView::OnAction(
    _In_ UINT Action
)
{
    switch (Action)
    {
        case IDC_PROPERTIES:
        {
            DisplayPropertySheet();
            break;
        }

        case IDC_SCAN_HARDWARE:
        {
            Refresh(GetCurrentView(),
                    true,
                    true,
                    NULL);
            break;
        }

        case IDC_ENABLE_DRV:
        {
            bool NeedsReboot;
            if (EnableSelectedDevice(true, NeedsReboot) &&
                NeedsReboot)
            {
                MessageBox(m_hMainWnd, L"Rebooting", L"Enable", MB_OK);
            }
            break;
        }

        case IDC_DISABLE_DRV:
        {
            bool NeedsReboot;
            EnableSelectedDevice(false, NeedsReboot);
            break;
        }

        case IDC_UPDATE_DRV:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Update Driver", MB_OK);
            break;
        }

        case IDC_UNINSTALL_DRV:
        {
            UninstallSelectedDevice();
            break;
        }

        case IDC_ADD_HARDWARE:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Add Hardware", MB_OK);
            break;
        }
    }

    return 0;
}

void
CDeviceView::DisplayPropertySheet()
{
    //
    // In ReactOS we can link to DevicePropertiesEx but
    // not in windows as it's not part of the SDK 

#ifndef __REACTOS__
    HMODULE hModule = LoadLibraryW(L"devmgr.dll");
    if (hModule == NULL) return;

    pDevicePropertiesExW DevicePropertiesExW;
    DevicePropertiesExW = (pDevicePropertiesExW)GetProcAddress(hModule,
                                                               "DevicePropertiesExW");
    if (DevicePropertiesExW == NULL)
    {
        FreeLibrary(hModule);
        return;
    }
#endif

    CNode *Node = GetSelectedNode();
    if (Node && Node->HasProperties())
    {
        DevicePropertiesExW(m_hTreeView,
                            NULL,
                            Node->GetDeviceId(),
                            1,//DPF_EXTENDED,
                            FALSE);
    }

#ifndef __REACTOS__
    FreeLibrary(hModule);
#endif
}

void
CDeviceView::SetFocus()
{
}

bool
CDeviceView::CreateActionMenu(
    _In_ HMENU OwnerMenu,
    _In_ bool MainMenu
    )
{
    CNode *Node = GetSelectedNode();
    if (Node)
    {
        BuildActionMenuForNode(OwnerMenu, Node, MainMenu);
        return true;
    }

    return false;
}

CNode*
CDeviceView::GetSelectedNode()
{
    TV_ITEM TvItem;
    TvItem.hItem = TreeView_GetSelection(m_hTreeView);
    return GetNode(&TvItem);
}



// PRIVATE METHODS *******************************************/

bool
CDeviceView::AddRootDevice()
{
    m_hTreeRoot = InsertIntoTreeView(NULL, m_RootNode);
    return (m_hTreeRoot != NULL);
}

bool
CDeviceView::GetNextClass(
    _In_ ULONG ClassIndex,
    _Out_ LPGUID ClassGuid,
    _Out_ HDEVINFO *hDevInfo
    )
{
    CONFIGRET cr;

    // Get the next class in the list
    cr = CM_Enumerate_Classes(ClassIndex,
                              ClassGuid,
                              0);
    if (cr != CR_SUCCESS) return false;

    // Check if this is the unknown class
    if (IsEqualGUID(*ClassGuid, GUID_DEVCLASS_UNKNOWN))
    {
        // Get device info for all devices
        *hDevInfo = SetupDiGetClassDevsW(NULL,
                                         NULL,
                                         NULL,
                                         DIGCF_ALLCLASSES);
    }
    else
    {
        // We only want the devices for this class
        *hDevInfo = SetupDiGetClassDevsW(ClassGuid,
                                         NULL,
                                         NULL,
                                         DIGCF_PRESENT);
    }

    return (hDevInfo != INVALID_HANDLE_VALUE);
}

unsigned int __stdcall CDeviceView::RefreshThread(void *Param)
{
    RefreshThreadData *ThreadData = (RefreshThreadData *)Param;
    CDeviceView *This = ThreadData->This;


    // Empty the treeview
    This->EmptyDeviceView();
    This->m_hTreeRoot = NULL;

    // Refresh the devices only if requested. This means
    // switching views uses the cache and remains fast
    if (ThreadData->ScanForChanges)
    {
        This->RefreshDeviceList();
    }

    // display the type of view the user wants
    switch (This->m_ViewType)
    {
        case DevicesByType:
            (void)This->ListDevicesByType();
            break;

        case DevicesByConnection:
            (VOID)This->ListDevicesByConnection();
            break;

        case ResourcesByType:
            break;

        case ResourcesByConnection:
            break;
    }


    This->SelectNode(ThreadData->DeviceId);

    if (ThreadData->DeviceId)
        delete[] ThreadData->DeviceId;
    delete ThreadData;

    return 0;
}


bool
CDeviceView::ListDevicesByType()
{
    CClassNode *ClassNode;
    CDeviceNode *DeviceNode;
    HDEVINFO hDevInfo;
    HTREEITEM hTreeItem = NULL;
    GUID ClassGuid;
    INT ClassIndex;
    BOOL bClassSuccess, bSuccess;

    // Start by adding the root node to the tree
    bSuccess = AddRootDevice();
    if (bSuccess == false) return false;

    ClassIndex = 0;
    do
    {
        // Loop through all the device classes
        bClassSuccess = GetNextClass(ClassIndex, &ClassGuid, &hDevInfo);
        if (bClassSuccess)
        {
            bool bClassUnknown = false;
            bool AddedParent = false;
            INT DeviceIndex = 0;
            bool MoreItems = false;

            // Get the cached class node
            ClassNode = GetClassNode(&ClassGuid);
            if (ClassNode == nullptr)
            {
                ATLASSERT(FALSE);
                ClassIndex++;
                continue;
            }

            // Set a flag is this is the (special case) unknown class
            if (IsEqualGUID(ClassGuid, GUID_DEVCLASS_UNKNOWN))
                bClassUnknown = true;

            do
            {
                // Get a handle to all the devices in this class
                SP_DEVINFO_DATA DeviceInfoData;
                ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
                DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                bSuccess = SetupDiEnumDeviceInfo(hDevInfo,
                                                 DeviceIndex,
                                                 &DeviceInfoData);
                if (bSuccess == FALSE && GetLastError() == ERROR_NO_MORE_ITEMS)
                    MoreItems = false;

                if (bSuccess)
                {
                    MoreItems = true;

                    // The unknown class handle contains all devices on the system,
                    // and we're just looking for the ones with a null GUID
                    if (bClassUnknown)
                    {
                        if (IsEqualGUID(DeviceInfoData.ClassGuid, GUID_NULL) == FALSE)
                        {
                            // This is a known device, we aren't interested in it
                            DeviceIndex++;
                            continue;
                        }
                    }

                    // Get the cached device node
                    DeviceNode = GetDeviceNode(DeviceInfoData.DevInst);
                    if (DeviceNode == nullptr)
                    {
                        ATLASSERT(bClassUnknown == true);
                        DeviceIndex++;
                        continue;
                    }

                    // Check if this is a hidden device
                    if (DeviceNode->IsHidden())
                    {
                        // Ignore this device if we aren't displaying hidden devices
                        if (m_ShowHidden == FALSE)
                        {
                            DeviceIndex++;
                            continue;
                        }
                    }

                    // We have a device, we need to add the parent if it hasn't yet been added
                    if (AddedParent == false)
                    {
                        // Insert the new class under the root item
                        hTreeItem = InsertIntoTreeView(m_hTreeRoot,
                                                       ClassNode);
                        AddedParent = true;
                    }

                    // Add the device under the class item node
                    (void)InsertIntoTreeView(hTreeItem, DeviceNode);

                    // Expand the class if it has a problem device
                    if (DeviceNode->HasProblem())
                    {
                        (void)TreeView_Expand(m_hTreeView,
                                              hTreeItem,
                                              TVE_EXPAND);
                    }
                }

                DeviceIndex++;

            } while (MoreItems);

            // If this class has devices, sort them alphabetically
            if (AddedParent == true)
            {
                (void)TreeView_SortChildren(m_hTreeView,
                                            hTreeItem,
                                            0);
            }
        }

        ClassIndex++;

    } while (bClassSuccess);

    // Sort the classes alphabetically
    (void)TreeView_SortChildren(m_hTreeView,
                                m_hTreeRoot,
                                0);

    // Expand the root item
    (void)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    // Pre-select the root item
    (VOID)TreeView_SelectItem(m_hTreeView,
                              m_hTreeRoot);

    return 0;
}

bool
CDeviceView::ListDevicesByConnection()
{
    bool bSuccess;

    // Start by adding the root node to the tree
    bSuccess = AddRootDevice();
    if (bSuccess == false) return false;

    // Walk the device tree and add all the devices 
    (void)RecurseChildDevices(m_RootNode->GetDeviceInst(), m_hTreeRoot);

    // Expand the root item 
    (void)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    return true;
}

bool
CDeviceView::RecurseChildDevices(
    _In_ DEVINST ParentDevice,
    _In_ HTREEITEM hParentTreeItem
    )
{
    HTREEITEM hDevItem = NULL;
    DEVINST Device;
    bool HasProblem = false;
    bool bSuccess;

    // Check if the parent has any child devices 
    if (GetChildDevice(ParentDevice, &Device) == FALSE)
        return true;

    // Get the cached device node
    CDeviceNode *DeviceNode;
    DeviceNode = dynamic_cast<CDeviceNode *>(GetDeviceNode(Device));
    if (DeviceNode == nullptr)
    {
        ATLASSERT(FALSE);
        return false;
    }

    // Don't show hidden devices if not requested
    if ((m_ShowHidden == TRUE) || (!(DeviceNode->IsHidden())))
    {
        // Add this device to the tree under its parent 
        hDevItem = InsertIntoTreeView(hParentTreeItem,
                                      DeviceNode);
        if (hDevItem)
        {
            // Check if this child has any children itself 
            if (!RecurseChildDevices(Device, hDevItem))
                HasProblem = true;
        }

        if (DeviceNode->HasProblem())
        {
            HasProblem = true;
        }
    }


    // Check for siblings
    for (;;)
    {
        // Check if the parent device has anything at the same level 
        bSuccess = GetSiblingDevice(Device, &Device);
        if (bSuccess == FALSE) break;

        DeviceNode = dynamic_cast<CDeviceNode *>(GetDeviceNode(Device));
        if (DeviceNode == nullptr)
        {
            ATLASSERT(FALSE);
        }

        // Don't show hidden devices if not requested
        if ((m_ShowHidden == TRUE) || (!(DeviceNode->IsHidden())))
        {
            if (DeviceNode->HasProblem())
            {
                HasProblem = true;
            }

            // Add this device to the tree under its parent 
            hDevItem = InsertIntoTreeView(hParentTreeItem,
                                          DeviceNode);
            if (hDevItem)
            {
                // Check if this child has any children itself 
                if (!RecurseChildDevices(Device, hDevItem))
                    HasProblem = true;
            }
        }
    }

    (void)TreeView_SortChildren(m_hTreeView,
                                hParentTreeItem,
                                0);

    // Expand the class if it has a problem device
    if (HasProblem == true)
    {
        (void)TreeView_Expand(m_hTreeView,
                              hParentTreeItem,
                              TVE_EXPAND);
    }

    // If there was a problem, expand the ancestors
    if (HasProblem) return false;

    return true;
}

bool
CDeviceView::EnableSelectedDevice(
    _In_ bool Enable,
    _Out_ bool &NeedsReboot
    )
{
    CDeviceNode *Node = dynamic_cast<CDeviceNode *>(GetSelectedNode());
    if (Node == nullptr) return false;

    if (Enable == false)
    {
        CAtlStringW str;
        if (str.LoadStringW(g_hThisInstance, IDS_CONFIRM_DISABLE))
        {
            if (MessageBoxW(m_hMainWnd,
                str,
                Node->GetDisplayName(),
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
            {
                return false;
            }
        }
    }

    if (Node->EnableDevice(Enable, NeedsReboot))
    {
        Refresh(m_ViewType, true, true, Node->GetDeviceId());
        return true;
    }

    return false;
}

bool
CDeviceView::UninstallSelectedDevice(
    )
{
    CDeviceNode *Node = dynamic_cast<CDeviceNode *>(GetSelectedNode());
    if (Node == nullptr) return false;

    return Node->UninstallDevice();
}

bool
CDeviceView::GetChildDevice(
    _In_ DEVINST ParentDevInst,
    _Out_ PDEVINST DevInst
)
{
    CONFIGRET cr;
    cr = CM_Get_Child(DevInst,
                      ParentDevInst,
                      0);
    return (cr == CR_SUCCESS);
}

bool
CDeviceView::GetSiblingDevice(
    _In_ DEVINST PrevDevice,
    _Out_ PDEVINST DevInst
)
{
    CONFIGRET cr;
    cr = CM_Get_Sibling(DevInst,
                        PrevDevice,
                        0);
    return (cr == CR_SUCCESS);
}

HTREEITEM
CDeviceView::InsertIntoTreeView(
    _In_opt_ HTREEITEM hParent,
    _In_ CNode *Node
    )
{
    LPWSTR lpLabel;
    lpLabel = Node->GetDisplayName();

    TV_ITEMW tvi;
    TV_INSERTSTRUCT tvins;
    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = wcslen(lpLabel);
    tvi.lParam = (LPARAM)Node;
    tvi.iImage = Node->GetClassImage();
    tvi.iSelectedImage = Node->GetClassImage();

    // try to cast it to a device node. This will only suceed if it's the correct type
    CDeviceNode *DeviceNode = dynamic_cast<CDeviceNode *>(Node);
    if (DeviceNode && DeviceNode->GetOverlayImage())
    {
        tvi.mask |= TVIF_STATE;
        tvi.stateMask = TVIS_OVERLAYMASK;
        tvi.state = INDEXTOOVERLAYMASK(DeviceNode->GetOverlayImage());
    }

    tvins.item = tvi;
    tvins.hParent = hParent;

    return TreeView_InsertItem(m_hTreeView, &tvins);
}

void
CDeviceView::BuildActionMenuForNode(
    _In_ HMENU OwnerMenu,
    _In_ CNode *Node,
    _In_ bool MainMenu
    )
{
    // Create a seperator structure 
    MENUITEMINFOW MenuSeperator = { 0 };
    MenuSeperator.cbSize = sizeof(MENUITEMINFOW);
    MenuSeperator.fType = MFT_SEPARATOR;

    // Setup the 
    MENUITEMINFOW MenuItemInfo = { 0 };
    MenuItemInfo.cbSize = sizeof(MENUITEMINFOW);
    MenuItemInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_SUBMENU;
    MenuItemInfo.fType = MFT_STRING;

    CAtlStringW String;
    int i = 0;

    // Device nodes have extra data
    if (Node->GetNodeType() == DeviceNode)
    {
        CDeviceNode *DeviceNode = dynamic_cast<CDeviceNode *>(Node);

        if (DeviceNode->CanUpdate())
        {
            String.LoadStringW(g_hThisInstance, IDS_MENU_UPDATE);
            MenuItemInfo.wID = IDC_UPDATE_DRV;
            MenuItemInfo.dwTypeData = String.GetBuffer();
            InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
            i++;
        }

        if (DeviceNode->IsDisabled())
        {
            String.LoadStringW(g_hThisInstance, IDS_MENU_ENABLE);
            MenuItemInfo.wID = IDC_ENABLE_DRV;
            MenuItemInfo.dwTypeData = String.GetBuffer();
            InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
            i++;
        }

        if (DeviceNode->CanDisable() && !DeviceNode->IsDisabled())
        {
            String.LoadStringW(g_hThisInstance, IDS_MENU_DISABLE);
            MenuItemInfo.wID = IDC_DISABLE_DRV;
            MenuItemInfo.dwTypeData = String.GetBuffer();
            InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
            i++;
        }

        if (DeviceNode->CanUninstall())
        {
            String.LoadStringW(g_hThisInstance, IDS_MENU_UNINSTALL);
            MenuItemInfo.wID = IDC_UNINSTALL_DRV;
            MenuItemInfo.dwTypeData = String.GetBuffer();
            InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
            i++;
        }

        InsertMenuItemW(OwnerMenu, i, TRUE, &MenuSeperator);
        i++;
    }

    // All nodes have the scan option
    String.LoadStringW(g_hThisInstance, IDS_MENU_SCAN);
    MenuItemInfo.wID = IDC_SCAN_HARDWARE;
    MenuItemInfo.dwTypeData = String.GetBuffer();
    InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
    i++;

    if ((Node->GetNodeType() == RootNode) || (MainMenu == true))
    {
        String.LoadStringW(g_hThisInstance, IDS_MENU_ADD);
        MenuItemInfo.wID = IDC_ADD_HARDWARE;
        MenuItemInfo.dwTypeData = String.GetBuffer();
        InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
        i++;
    }

    if (Node->HasProperties())
    {
        InsertMenuItemW(OwnerMenu, i, TRUE, &MenuSeperator);
        i++;

        String.LoadStringW(g_hThisInstance, IDS_MENU_PROPERTIES);
        MenuItemInfo.wID = IDC_PROPERTIES;
        MenuItemInfo.dwTypeData = String.GetBuffer();
        InsertMenuItemW(OwnerMenu, i, TRUE, &MenuItemInfo);
        i++;

        SetMenuDefaultItem(OwnerMenu, IDC_PROPERTIES, FALSE);
    }
}

HTREEITEM
CDeviceView::RecurseFindDevice(
    _In_ HTREEITEM hParentItem,
    _In_ LPWSTR DeviceId
    )
{
    HTREEITEM FoundItem;
    HTREEITEM hItem;
    TVITEMW tvItem;
    CNode *Node;

    // Check if this node has any children
    hItem = TreeView_GetChild(m_hTreeView, hParentItem);
    if (hItem == NULL) return NULL;

    // The lParam contains the node pointer data
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvItem) &&
        tvItem.lParam != NULL)
    {
        // check for a matching deviceid
        Node = reinterpret_cast<CNode *>(tvItem.lParam);
        if (Node->GetDeviceId() &&
            (wcscmp(Node->GetDeviceId(), DeviceId) == 0))
        {
            return hItem;
        }
    }

    // This node may have its own children
    FoundItem = RecurseFindDevice(hItem, DeviceId);
    if (FoundItem) return FoundItem;

    // Loop all the siblings
    for (;;)
    {
        // Get the next item at this level
        hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
        if (hItem == NULL) break;

        // The lParam contains the node pointer data
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_PARAM;
        if (TreeView_GetItem(m_hTreeView, &tvItem))
        {
            // check for a matching deviceid
            Node = reinterpret_cast<CNode *>(tvItem.lParam);
            if (Node->GetDeviceId() && 
                wcscmp(Node->GetDeviceId(), DeviceId) == 0)
            {
                return hItem;
            }
        }

        // This node may have its own children 
        FoundItem = RecurseFindDevice(hItem, DeviceId);
        if (FoundItem) return FoundItem;
    }

    return hItem;
}

void
CDeviceView::SelectNode(
    _In_ LPWSTR DeviceId
    )
{
    HTREEITEM hRoot, hItem;

    // Check if there are any items in the tree
    hRoot = TreeView_GetRoot(m_hTreeView);
    if (hRoot == NULL) return;

    // If we don't want to set select a node, just select root
    if (DeviceId == NULL)
    {
        TreeView_SelectItem(m_hTreeView, hRoot);
        return;
    }

    // Scan the tree looking for the node we want
    hItem = RecurseFindDevice(hRoot, DeviceId);
    if (hItem)
    {
        TreeView_SelectItem(m_hTreeView, hItem);
        TreeView_Expand(m_hTreeView, hItem, TVM_EXPAND);
    }
    else
    {
        TreeView_SelectItem(m_hTreeView, hRoot);
    }
}


void
CDeviceView::EmptyDeviceView()
{
    (VOID)TreeView_DeleteAllItems(m_hTreeView);
}


CClassNode*
CDeviceView::GetClassNode(
    _In_ LPGUID ClassGuid
    )
{
    POSITION Pos;
    CClassNode *Node;

    Pos = m_ClassNodeList.GetHeadPosition();
    if (Pos == NULL) return nullptr;

    do
    {
        Node = m_ClassNodeList.GetNext(Pos);
        if (IsEqualGUID(*Node->GetClassGuid(), *ClassGuid))
        {
            ATLASSERT(Node->GetNodeType() == ClassNode);
            break;
        }

        Node = NULL;

    } while (Pos != NULL);

    return Node;
}

CDeviceNode*
CDeviceView::GetDeviceNode(
    _In_ DEVINST Device
    )
{
    POSITION Pos;
    CDeviceNode *Node;

    Pos = m_DeviceNodeList.GetHeadPosition();
    if (Pos == NULL) return nullptr;

    do
    {
        Node = m_DeviceNodeList.GetNext(Pos);
        if (Node->GetDeviceInst() == Device)
        {
            ATLASSERT(Node->GetNodeType() == DeviceNode);
            break;
        }

        Node = NULL;

    } while (Pos != NULL);

    return Node;
}

CNode* CDeviceView::GetNode(
    _In_ LPTV_ITEMW TvItem
    )
{
    TvItem->mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, TvItem))
    {
        return (CNode *)TvItem->lParam;
    }
    return NULL;
}

void
CDeviceView::EmptyLists()
{
    CNode *Node;

    while (!m_ClassNodeList.IsEmpty())
    {
        Node = m_ClassNodeList.RemoveTail();
        delete Node;
    }

    while (!m_DeviceNodeList.IsEmpty())
    {
        Node = m_DeviceNodeList.RemoveTail();
        delete Node;
    }
}

bool
CDeviceView::RefreshDeviceList()
{
    GUID ClassGuid;
    CClassNode *ClassNode;
    CDeviceNode *DeviceNode;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i;
    BOOL Success;

    ULONG ClassIndex = 0;

    EmptyLists();

    if (m_RootNode) delete m_RootNode;
    m_RootNode = new CRootNode(&m_ImageListData);
    m_RootNode->SetupNode();

    // Loop through all the classes
    do
    {
        Success = GetNextClass(ClassIndex, &ClassGuid, &hDevInfo);
        if (Success)
        {
            // Create a new class node and add it to the list
            ClassNode = new CClassNode(&ClassGuid, &m_ImageListData);
            if (ClassNode->SetupNode())
            {
                m_ClassNodeList.AddTail(ClassNode);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }
        ClassIndex++;
    } while (Success);


    // Get all the devices on the local machine
    hDevInfo = SetupDiGetClassDevsW(NULL,
                                    0,
                                    0,
                                    DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // loop though all the devices
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0;; i++)
    {
        // Get the devinst for this device
        Success = SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData);
        if (Success == FALSE) break;

        // create a new device node and add it to the list
        DeviceNode = new CDeviceNode(DeviceInfoData.DevInst, &m_ImageListData);
        if (DeviceNode->SetupNode())
        {
            m_DeviceNodeList.AddTail(DeviceNode);
        }
        else
        {
            ATLASSERT(FALSE);
        }

    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return TRUE;
}