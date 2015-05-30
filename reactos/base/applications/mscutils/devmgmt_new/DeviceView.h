#pragma once
#include "Devices.h"

enum ListDevices
{
    DevicesByType,
    DevicesByConnection,
    ResourcesByType,
    ResourcesByConnection
};

class CDeviceView : public CDevices
{
    CDevices *m_Devices;
    HWND m_hMainWnd;
    HWND m_hTreeView;
    HWND m_hPropertyDialog;
    HWND m_hShortcutMenu;
    ListDevices m_ListDevices;

    HIMAGELIST m_ImageList;
    HTREEITEM m_hTreeRoot;

    BOOL m_ShowHidden;

public:
    CDeviceView(
        HWND hMainWnd,
        ListDevices List
        );

    ~CDeviceView(void);

    BOOL Initialize();
    BOOL Uninitialize();

    VOID Size(
        _In_ INT x,
        _In_ INT y,
        _In_ INT cx,
        _In_ INT cy
        );

    VOID Refresh();
    VOID DisplayPropertySheet();
    VOID SetFocus();

    VOID SetDeviceListType(ListDevices List)
    {
        m_ListDevices = List;
    }

    VOID ShowHiddenDevices(_In_ BOOL ShowHidden)
    {
        m_ShowHidden = ShowHidden;
    }

private:
    static DWORD CALLBACK ListDevicesThread(PVOID Param);

    BOOL ListDevicesByConnection(
        );
    BOOL ListDevicesByType(
        );

    VOID RecurseChildDevices(
        _In_ DEVINST ParentDevice,
        _In_ HTREEITEM hParentTreeItem
        );

    HTREEITEM InsertIntoTreeView(
        _In_ HTREEITEM hParent,
        _In_z_ LPWSTR lpLabel,
        _In_ LPARAM lParam,
        _In_ INT DevImage,
        _In_ UINT OverlayImage
        );

    VOID RecurseDeviceView(
        _In_ HTREEITEM hParentItem
        );

    VOID EmptyDeviceView(
        );
};

