#include "StdAfx.h"
#include "devmgmt.h"
#include "DeviceView.h"


CDeviceView::CDeviceView(HWND hMainWnd) :
    m_hMainWnd(hMainWnd),
    m_hTreeView(NULL),
    m_hPropertyDialog(NULL),
    m_hShortcutMenu(NULL)
{
}


CDeviceView::~CDeviceView(void)
{
}

BOOL
CDeviceView::Initialize()
{
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
        
    }

    return !!(m_hTreeView);

    return TRUE;
}

VOID
CDeviceView::Size(INT x,
                  INT y,
                  INT cx,
                  INT cy)
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

BOOL
CDeviceView::Uninitialize()
{
    return TRUE;
}

VOID
CDeviceView::Refresh()
{
}

VOID
CDeviceView::DisplayPropertySheet()
{
}

VOID
CDeviceView::SetFocus()
{
}