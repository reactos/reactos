#pragma once
#include "DeviceNode.h"
#include "ClassNode.h"
#include "RootNode.h"

enum ViewType
{
    DevicesByType,
    DevicesByConnection,
    ResourcesByType,
    ResourcesByConnection
};


class CDeviceView
{

    HWND m_hMainWnd;
    HWND m_hTreeView;
    HWND m_hPropertyDialog;
    HMENU m_hMenu;
    ViewType m_ViewType;
    HTREEITEM m_hTreeRoot;
    bool m_ShowHidden;

    CRootNode *m_RootNode;
    CAtlList<CClassNode *> m_ClassNodeList;
    CAtlList<CDeviceNode *> m_DeviceNodeList;
    SP_CLASSIMAGELIST_DATA m_ImageListData;

public:
    CDeviceView(
        HWND hMainWnd
        );

    ~CDeviceView(void);

    bool Initialize();
    bool Uninitialize();

    LRESULT OnSize(
        _In_ int x,
        _In_ int y,
        _In_ int cx,
        _In_ int cy
        );

    LRESULT OnDoubleClick(
        _In_ LPNMHDR NmHdr
        );

    LRESULT OnRightClick(
        _In_ LPNMHDR NmHdr
        );

    LRESULT OnContextMenu(
        _In_ LPARAM lParam
        );

    LRESULT OnAction(
        UINT Action
        );

    VOID Refresh(
        _In_ ViewType Type,
        _In_ bool ScanForChanges,
        _In_ bool UpdateView
        );

    VOID DisplayPropertySheet();
    VOID SetFocus();

    VOID SetHiddenDevices(_In_ bool ShowHidden)
    {
        m_ShowHidden = ShowHidden;
    }

    ViewType GetCurrentView() { return m_ViewType; }

    bool CreateActionMenu(
        _In_ HMENU OwnerMenu,
        _In_ bool MainMenu
        );

    CNode* GetSelectedNode(
        );

    bool SelDeviceIsStarted();
    bool SelDeviceIsInstalled();

private:
    bool AddRootDevice();

    bool RefreshDeviceList();

    static unsigned int __stdcall RefreshThread(
        void *Param
        );

    bool ListDevicesByConnection(
        );
    bool ListDevicesByType(
        );

    bool GetNextClass(
        _In_ ULONG ClassIndex,
        _Out_ LPGUID ClassGuid,
        _Out_ HDEVINFO *hDevInfo
        );

    bool RecurseChildDevices(
        _In_ DEVINST ParentDevice,
        _In_ HTREEITEM hParentTreeItem
        );

    bool EnableSelectedDevice(
        _In_ bool Enable,
        _Out_ bool &NeedsReboot
        );

    bool UpdateSelectedDevice(
        _Out_ bool &NeedsReboot
        );

    bool UninstallSelectedDevice(
        );

    bool RunAddHardwareWizard(
        );

    bool GetChildDevice(
        _In_ DEVINST ParentDevInst,
        _Out_ PDEVINST DevInst
        );

    bool GetSiblingDevice(
        _In_ DEVINST PrevDevice,
        _Out_ PDEVINST DevInst
        );

    HTREEITEM InsertIntoTreeView(
        _In_opt_ HTREEITEM hParent,
        _In_ CNode *Node
        );

    void BuildActionMenuForNode(
        _In_ HMENU OwnerMenu,
        _In_ CNode *Node,
        _In_ bool MainMenu
        );

    HTREEITEM RecurseFindDevice(
        _In_ HTREEITEM hParentItem,
        _In_ CNode *Node
        );

    void SelectNode(
        _In_ CNode *Node
        );

    void EmptyDeviceView(
        );

    CNode* GetNode(
        _In_ LPTV_ITEMW TvItem
        );

    CClassNode* GetClassNode(
        _In_ LPGUID ClassGuid
        );
    CDeviceNode* GetDeviceNode(
        _In_ DEVINST Device
        );
    void EmptyLists(
        );
};

