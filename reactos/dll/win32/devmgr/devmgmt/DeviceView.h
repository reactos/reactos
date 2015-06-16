#pragma once
#include "Node.h"

enum ViewType
{
    DevicesByType,
    DevicesByConnection,
    ResourcesByType,
    ResourcesByConnection
};


class CDeviceView
{
    CAtlList<CNode *> m_ClassNodeList;
    CAtlList<CNode *> m_DeviceNodeList;

    SP_CLASSIMAGELIST_DATA m_ImageListData;

    HWND m_hMainWnd;
    HWND m_hTreeView;
    HWND m_hPropertyDialog;
    HWND m_hShortcutMenu;
    ViewType m_ViewType;

    HTREEITEM m_hTreeRoot;
    DEVINST m_RootDevInst;

    bool m_ShowHidden;
    int m_RootClassImage;

public:
    CDeviceView(
        HWND hMainWnd
        );

    ~CDeviceView(void);

    bool Initialize();
    bool Uninitialize();

    VOID Size(
        _In_ int x,
        _In_ int y,
        _In_ int cx,
        _In_ int cy
        );

    VOID Refresh(
        _In_ ViewType Type,
        _In_ bool ScanForChanges,
        _In_ bool UpdateView
        );

    VOID DisplayPropertySheet();
    VOID SetFocus();

    //VOID SetDeviceListType(ListDevices List)
    //{
    //    m_ListDevices = List;
    //}

    VOID ShowHiddenDevices(_In_ bool ShowHidden)
    {
        m_ShowHidden = ShowHidden;
    }

    ViewType GetCurrentView() { return m_ViewType; }

    bool HasProperties(_In_ LPTV_ITEMW TvItem);
    //bool SelDeviceIsHidden();
    bool CanDisable(_In_ LPTV_ITEMW TvItem);
    bool IsDisabled(_In_ LPTV_ITEMW TvItem);
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

    VOID RecurseChildDevices(
        _In_ DEVINST ParentDevice,
        _In_ HTREEITEM hParentTreeItem
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
        _In_ HTREEITEM hParent,
        _In_ CNode *Node
        );

    VOID RecurseDeviceView(
        _In_ HTREEITEM hParentItem
        );

    VOID EmptyDeviceView(
        );

    CNode* GetNode(_In_ LPTV_ITEMW TvItem);
    CNode* GetSelectedNode();

    CNode* GetClassNode(_In_ LPGUID ClassGuid);
    CNode* GetDeviceNode(_In_ DEVINST Device);
    void EmptyLists();
};

