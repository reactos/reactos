/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    ctvctl.cpp

Abstract:

    This module implements TreeView OCX for Device Manager snapin

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// CTVCtl.cpp : Implementation of the CTVCtrl OLE control class.
#include "stdafx.h"
#include <afxcmn.h>
#include "ctv.h"
#include "CTVCtl.h"
#include "CTVPpg.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define GET_X_LPARAM(lParam) (int)(short)LOWORD(lParam)
#define GET_Y_LPARAM(lParam) (int)(short)HIWORD(lParam)

IMPLEMENT_DYNCREATE(CTVCtrl, COleControl)

BEGIN_INTERFACE_MAP(CTVCtrl, COleControl)
    INTERFACE_PART(CTVCtrl, IID_IDMTVOCX, DMTVOCX)
END_INTERFACE_MAP()


const IID IID_IDMTVOCX = {0x142525f2,0x59d8,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}};
const IID IID_ISnapinCallback = {0x8e0ba98a,0xd161,0x11d0,{0x83,0x53,0x00,0xa0,0xc9,0x06,0x40,0xbf}};



ULONG EXPORT CTVCtrl::XDMTVOCX::AddRef()
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->ExternalAddRef();
}
ULONG EXPORT CTVCtrl::XDMTVOCX::Release()
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->ExternalRelease();
}

HRESULT EXPORT CTVCtrl::XDMTVOCX::QueryInterface(
    REFIID iid,
    void ** ppvObj
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->ExternalQueryInterface(&iid, ppvObj);
}

HTREEITEM EXPORT CTVCtrl::XDMTVOCX::InsertItem(
    LPTV_INSERTSTRUCT pis
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->InsertItem(pis);
}

HRESULT EXPORT CTVCtrl::XDMTVOCX::DeleteItem(
    HTREEITEM hitem
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->DeleteItem(hitem);
}

HRESULT EXPORT CTVCtrl::XDMTVOCX::DeleteAllItems(
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->DeleteAllItems();
}



HIMAGELIST EXPORT CTVCtrl::XDMTVOCX::SetImageList(
    INT iImage,
    HIMAGELIST himl
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SetImageList(iImage, himl);
}


HRESULT EXPORT CTVCtrl::XDMTVOCX::SetItem(
    TV_ITEM* pitem
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SetItem(pitem);
}


HRESULT EXPORT CTVCtrl::XDMTVOCX::Expand(
    UINT Flags,
    HTREEITEM hitem
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->Expand(Flags, hitem);
}

HRESULT EXPORT CTVCtrl::XDMTVOCX::SelectItem(
    UINT Flags,
    HTREEITEM hitem
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SelectItem(Flags, hitem);
}


HRESULT EXPORT CTVCtrl::XDMTVOCX::SetStyle(
    DWORD dwStyle
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SetStyle(dwStyle);
}

HWND EXPORT CTVCtrl::XDMTVOCX::GetWindowHandle(
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetWindowHandle();
}


HRESULT EXPORT CTVCtrl::XDMTVOCX::GetItem(
    TV_ITEM* pti
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetItem(pti);
}

HTREEITEM EXPORT CTVCtrl::XDMTVOCX::GetNextItem(
    UINT Flags,
    HTREEITEM htiRef
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetNextItem(Flags, htiRef);
}



HRESULT EXPORT CTVCtrl::XDMTVOCX::SelectItem(
    HTREEITEM hti
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SelectItem(hti);
}


UINT EXPORT CTVCtrl::XDMTVOCX::GetCount(
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetCount();
}

HTREEITEM EXPORT CTVCtrl::XDMTVOCX::GetSelectedItem(
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetSelectedItem();
}

HRESULT EXPORT CTVCtrl::XDMTVOCX::Connect(
    IComponent* pIComponent,
    MMC_COOKIE  cookie
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->Connect(pIComponent, cookie);
}
HRESULT EXPORT CTVCtrl::XDMTVOCX::SetActiveConnection(
    MMC_COOKIE cookie
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SetActiveConnection(cookie);
}
MMC_COOKIE EXPORT CTVCtrl::XDMTVOCX::GetActiveConnection()
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->GetActiveConnection();
}

long EXPORT CTVCtrl::XDMTVOCX::SetRedraw(BOOL Redraw)
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->SetRedraw(Redraw);
}

BOOL EXPORT CTVCtrl::XDMTVOCX::EnsureVisible(
    HTREEITEM hitem
    )
{
    METHOD_PROLOGUE(CTVCtrl, DMTVOCX)
    return pThis->EnsureVisible(hitem);
}



/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CTVCtrl, COleControl)
        //{{AFX_MSG_MAP(CTVCtrl)
        ON_WM_DESTROY()
        ON_WM_CONTEXTMENU()
        //}}AFX_MSG_MAP
        ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
        ON_MESSAGE(OCM_NOTIFY, OnOcmNotify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CTVCtrl, COleControl)
        //{{AFX_DISPATCH_MAP(CTVCtrl)
        //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CTVCtrl, COleControl)
        //{{AFX_EVENT_MAP(CTVCtrl)
        // NOTE - ClassWizard will add and remove event map entries
        //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CTVCtrl, 1)
        PROPPAGEID(CTVPropPage::guid)
END_PROPPAGEIDS(CTVCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CTVCtrl, "CTREEVIEW.CTreeViewCtrl.1",
        0xcd6c7868, 0x5864, 0x11d0, 0xab, 0xf0, 0, 0x20, 0xaf, 0x6b, 0xb, 0x7a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CTVCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DTV =
                { 0xcd6c7866, 0x5864, 0x11d0, { 0xab, 0xf0, 0, 0x20, 0xaf, 0x6b, 0xb, 0x7a } };
const IID BASED_CODE IID_DTVEvents =
                { 0xcd6c7867, 0x5864, 0x11d0, { 0xab, 0xf0, 0, 0x20, 0xaf, 0x6b, 0xb, 0x7a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwTVOleMisc =
        OLEMISC_ACTIVATEWHENVISIBLE |
        OLEMISC_SETCLIENTSITEFIRST |
        OLEMISC_INSIDEOUT |
        OLEMISC_CANTLINKINSIDE |
        OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CTVCtrl, IDS_TV, _dwTVOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::CTVCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CTVCtrl

BOOL CTVCtrl::CTVCtrlFactory::UpdateRegistry(BOOL bRegister)
{
        // TODO: Verify that your control follows apartment-model threading rules.
        // Refer to MFC TechNote 64 for more information.
        // If your control does not conform to the apartment-model rules, then
        // you must modify the code below, changing the 6th parameter from
        // afxRegApartmentThreading to 0.

        if (bRegister)
                return AfxOleRegisterControlClass(
                        AfxGetInstanceHandle(),
                        m_clsid,
                        m_lpszProgID,
                        IDS_TV,
                        IDB_TV,
                        afxRegApartmentThreading,
                        _dwTVOleMisc,
                        _tlid,
                        _wVerMajor,
                        _wVerMinor);
        else
                return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::CTVCtrl - Constructor

CTVCtrl::CTVCtrl()
{
        InitializeIIDs(&IID_DTV, &IID_DTVEvents);

        m_nConnections = 0;
        m_pIComponent =  NULL;
        m_pISnapinCallback = NULL;
        m_Destroyed = FALSE;
        // TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::~CTVCtrl - Destructor

CTVCtrl::~CTVCtrl()
{
        // TODO: Cleanup your control's instance data here.

    if (m_pISnapinCallback)
        m_pISnapinCallback->Release();
    if (m_pIComponent)
        m_pIComponent->Release();

}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::OnDraw - Drawing function

void CTVCtrl::OnDraw(
                        CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
        DoSuperclassPaint(pdc, rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::DoPropExchange - Persistence support

void CTVCtrl::DoPropExchange(CPropExchange* pPX)
{
        ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
        COleControl::DoPropExchange(pPX);

        // TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::OnResetState - Reset control to default state

void CTVCtrl::OnResetState()
{
        COleControl::OnResetState();  // Resets defaults found in DoPropExchange

        // TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CTVCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
        cs.lpszClass = _T("SysTreeView32");
        // Turn off WS_EX_NOPARENTNOTIFY style bit so that our parents
        // receive mouse clicks on our window. I do not know why MFC
        // fundation class turns this on for an OCX.
        cs.dwExStyle &= ~(WS_EX_NOPARENTNOTIFY);
        return COleControl::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::IsSubclassedControl - This is a subclassed control

BOOL CTVCtrl::IsSubclassedControl()
{
        return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl::OnOcmCommand - Handle command messages

LRESULT CTVCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
        WORD wNotifyCode = HIWORD(wParam);
#else
        WORD wNotifyCode = HIWORD(lParam);
#endif

        // TODO: Switch on wNotifyCode here.

        return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CTVCtrl message handlers



///////////////////////////////////////////////////////////////////////
///
/// Tree View functions
///

HRESULT
CTVCtrl::Connect(
    IComponent* pIComponent,
    MMC_COOKIE cookie
    )
{
    HRESULT hr = S_OK;

    if (0 == m_nConnections)
    {
        ASSERT(NULL == m_pIComponent);

        m_pIComponent  = pIComponent;
        m_pIComponent->AddRef();
        hr = m_pIComponent->QueryInterface(IID_ISnapinCallback,
                            reinterpret_cast<void**>(&m_pISnapinCallback)
                            );
    }

    // A single snapin may have multiple nodes that uses us as result pane
    // display media, therefore, we may be connected mutlple times.
    // However, we get created only when MMC creates a new snapin instance.
    // This means, every connection call must provide the same
    // pIComponent and pConsole.
    //
    ASSERT(m_pIComponent == pIComponent);


    if (SUCCEEDED(hr))
    {
        m_nConnections++;
        hr = SetActiveConnection(cookie);
    }
    return hr;
}

HRESULT
CTVCtrl::SetActiveConnection(
    MMC_COOKIE cookie
    )
{
    m_ActiveCookie = cookie;
    return S_OK;
}
MMC_COOKIE
CTVCtrl::GetActiveConnection()
{
    return m_ActiveCookie;
}

HTREEITEM CTVCtrl::InsertItem(
    LPTV_INSERTSTRUCT pis
    )
{
    return (HTREEITEM)SendMessage(TVM_INSERTITEM, 0, (LPARAM)pis);
}

HRESULT CTVCtrl::DeleteItem(
    HTREEITEM hitem
    )
{
    if (SendMessage(TVM_DELETEITEM, 0, (LPARAM)hitem))
        return S_OK;
    else
        return E_UNEXPECTED;
}

HRESULT CTVCtrl::DeleteAllItems()
{
    return DeleteItem((HTREEITEM)TVI_ROOT);

}
HIMAGELIST CTVCtrl::SetImageList(
    INT iImage,
    HIMAGELIST hmil
    )
{
    return (HIMAGELIST)SendMessage(TVM_SETIMAGELIST, (WPARAM)iImage, (LPARAM)hmil);
}

HRESULT CTVCtrl::SetItem(
    TV_ITEM* pitem
    )
{
    if (SendMessage(TVM_SETITEM, 0, (LPARAM)pitem))
        return S_OK;
    else
        return E_UNEXPECTED;
}

HRESULT CTVCtrl::Expand(
    UINT Flags,
    HTREEITEM hitem
    )
{
    if (SendMessage(TVM_EXPAND, (WPARAM) Flags, (LPARAM)hitem))
        return S_OK;
    else
        return E_UNEXPECTED;
}

HRESULT CTVCtrl::SelectItem(
    UINT Flags,
    HTREEITEM hitem
    )
{
    if (SendMessage(TVM_SELECTITEM, (WPARAM)Flags, (LPARAM)hitem))
        return S_OK;
    else
        return E_UNEXPECTED;
}

HRESULT CTVCtrl::SetStyle(
    DWORD dwStyle
    )
{
    if (ModifyStyle(0, dwStyle))
        return S_OK;
    else
        return E_UNEXPECTED;
}

HWND CTVCtrl::GetWindowHandle(
    )
{
    return m_hWnd;
}


HRESULT CTVCtrl::GetItem(
    TV_ITEM* pti
    )
{
    if (SendMessage(TVM_GETITEM, 0, (LPARAM)pti))
        return S_OK;
    else
        return E_UNEXPECTED;

}

HTREEITEM CTVCtrl::GetNextItem(
    UINT Flags,
    HTREEITEM htiRef
    )
{
    return (HTREEITEM) SendMessage(TVM_GETNEXTITEM, (WPARAM)Flags, (LPARAM)htiRef);
}

HRESULT CTVCtrl::SelectItem(
    HTREEITEM hti
    )
{
    if (SendMessage(TVM_SELECTITEM, 0, (LPARAM) hti))
        return S_OK;

    else
        return S_FALSE;
}
UINT CTVCtrl::GetCount(
    )
{
    return (UINT)SendMessage(TVM_GETCOUNT, 0, 0);
}


HTREEITEM CTVCtrl::HitTest(
    LONG x,
    LONG y,
    UINT* pFlags
    )
{
    POINT pt;
    pt.x = x;
    pt.y = y;
    
    ScreenToClient(&pt);
    
    TV_HITTESTINFO tvhti;
    tvhti.pt = pt;
    
    HTREEITEM hti = (HTREEITEM)SendMessage(TVM_HITTEST, 0, (LPARAM)&tvhti);

    if (hti && pFlags)
        *pFlags = tvhti.flags;
    
    return hti;
}
HTREEITEM CTVCtrl::GetSelectedItem(
    )
{
    return (HTREEITEM)SendMessage(TVM_GETNEXTITEM, TVGN_CARET, 0);
}

HRESULT CTVCtrl::SetRedraw(
    BOOL Redraw
    )
{
    if (Redraw)
        Invalidate();
    return S_OK;
}

BOOL CTVCtrl::EnsureVisible(
    HTREEITEM hitem
    )
{
    return (BOOL)SendMessage(TVM_ENSUREVISIBLE, 0, (LPARAM)hitem);
}

LRESULT
CTVCtrl::OnOcmNotify(
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPARAM param, arg;
    MMC_COOKIE cookie;

    HRESULT hr = S_FALSE;
    TV_NOTIFY_CODE NotifyCode;
    TV_ITEM TI;

    NotifyCode = TV_NOTIFY_CODE_UNKNOWN;
    switch (((NMHDR*)lParam)->code)
    {
        case NM_RCLICK:
            NotifyCode = TV_NOTIFY_CODE_RCLICK;
            break;

        //case NM_RCLICK:
        case NM_RDBLCLK:
        case NM_CLICK:
        case NM_DBLCLK:
            NotifyCode = DoMouseNotification(((NMHDR*)lParam)->code, &cookie,
                                               &arg, &param);
            break;
        case TVN_KEYDOWN:
            TI.hItem = GetSelectedItem();
            TI.mask = TVIF_PARAM;
            if (TI.hItem && SUCCEEDED(GetItem(&TI)))
            {
                cookie = (MMC_COOKIE)TI.lParam;
                NotifyCode = TV_NOTIFY_CODE_KEYDOWN;
                param = ((TV_KEYDOWN*)lParam)->wVKey;
                arg = (LPARAM)TI.hItem;
            }
            break;
        case NM_SETFOCUS:
            TI.hItem = GetSelectedItem();
            TI.mask = TVIF_PARAM;
            if (TI.hItem && SUCCEEDED(GetItem(&TI)))
            {
                cookie = (MMC_COOKIE)TI.lParam;
                NotifyCode = TV_NOTIFY_CODE_FOCUSCHANGED;
                param = 1;
                arg = (LPARAM)TI.hItem;
            }
            break;
#if 0

        case TVN_GETDISPINFOA:
        case TVN_GETDISPINFOW:
            TI.hItem = ((TV_DISPINFO*)lParam)->item.hItem;
            TI.mask = TVIF_PARAM;
            if (TI.hItem && SUCCEEDED(GetItem(&TI)))
            {
                NotifyCode = TV_NOTIFY_CODE_GETDISPINFO;
                arg = (LPARAM)TI.hItem;
                cookie = (MMC_COOKIE)TI.lParam;
                param = (LPARAM)&((TV_DISPINFO*)lParam)->item;
            }
            break;
        case TVN_SELCHANGINGA:
        case TVN_SELCHANGINGW:
            NotifyCode = TV_NOTIFY_CODE_SELCHANGING;
            arg = (LPARAM)((NM_TREEVIEW*)lParam)->itemNew.hItem;
            cookie = (MMC_COOKIE)((NM_TREEVIEW*)lParam)->itemNew.lParam;
            param = (LPARAM)((NM_TREEVIEW*)lParam)->action;
            break;


        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            NotifyCode = TV_NOTIFY_CODE_EXPANDING;
            arg = (LPARAM)((NM_TREEVIEW*)lParam)->itemNew.hItem;
            cookie = (MMC_COOKIE)((NM_TREEVIEW*)lParam)->itemNew.lParam;
            param = (LPARAM)((NM_TREEVIEW*)lParam)->action;
            break;
#endif

        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            NotifyCode = TV_NOTIFY_CODE_SELCHANGED;
            arg = (LPARAM)((NM_TREEVIEW*)lParam)->itemNew.hItem;
            cookie = (MMC_COOKIE)((NM_TREEVIEW*)lParam)->itemNew.lParam;
            param = (LPARAM)((NM_TREEVIEW*)lParam)->action;
            break;
        case TVN_ITEMEXPANDEDA:
        case TVN_ITEMEXPANDEDW:
            NotifyCode = TV_NOTIFY_CODE_EXPANDED;
            arg = (LPARAM)((NM_TREEVIEW*)lParam)->itemNew.hItem;
            cookie = (MMC_COOKIE)((NM_TREEVIEW*)lParam)->itemNew.lParam;
            param = (LPARAM)((NM_TREEVIEW*)lParam)->action;
            break;

        default:
            NotifyCode = TV_NOTIFY_CODE_UNKNOWN;
            break;
    }

    if (TV_NOTIFY_CODE_UNKNOWN != NotifyCode && m_pISnapinCallback)
    {
        hr = m_pISnapinCallback->tvNotify(*this, cookie, NotifyCode, arg, param);
        if (S_FALSE == hr)
        {
            //
            // Translate RCLICK to context menu
            //
            if (TV_NOTIFY_CODE_RCLICK == NotifyCode)
            {
                SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos());
                hr = S_OK;
            }
            
            //
            // Translate Shift-F10 or VK_APPS to context menu 
            //
            else if (TV_NOTIFY_CODE_KEYDOWN == NotifyCode && 
                     (VK_F10 == param && GetKeyState(VK_SHIFT) < 0) ||
                     (VK_APPS == param))
            {
                RECT rect;
                *((HTREEITEM*)&rect) = (HTREEITEM)arg;
                if (SendMessage(TVM_GETITEMRECT, TRUE, (LPARAM)&rect))
                {
                    POINT pt;
                    pt.x = (rect.left + rect.right) / 2;
                    pt.y = (rect.top + rect.bottom) / 2;
                    ClientToScreen(&pt);
                    SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(pt.x, pt.y));
                    hr = S_OK;
                }
            }
        }
    }
    
    ASSERT(S_OK == hr || S_FALSE == hr);
    
    if (S_OK == hr)
    {
        return 1;
    }
    
    return 0;
}

TV_NOTIFY_CODE
CTVCtrl::DoMouseNotification(
    UINT Code,
    MMC_COOKIE* pcookie,
    LPARAM* parg,
    LPARAM* pparam
    )
{
    DWORD MsgPos;
    POINT point;

    ASSERT(pparam && parg && pcookie);
    *pparam = 0;
    *parg = 0;
    MsgPos = GetMessagePos();
    point.x = GET_X_LPARAM(MsgPos);
    point.y = GET_Y_LPARAM(MsgPos);
    UINT htFlags;
    HTREEITEM hti = HitTest(point.x, point.y, &htFlags);

    TV_NOTIFY_CODE NotifyCode = TV_NOTIFY_CODE_UNKNOWN;
    
    if (hti && (htFlags & TVHT_ONITEM))
    {
        TV_ITEM TI;
        TI.hItem = hti;
        TI.mask = TVIF_PARAM;
        
        if (SUCCEEDED(GetItem(&TI)))
        {
            switch (Code)
            {
                case NM_RCLICK:
                    NotifyCode = TV_NOTIFY_CODE_RCLICK;
                    break;
                case NM_RDBLCLK:
                    NotifyCode = TV_NOTIFY_CODE_RDBLCLK;
                    break;
                case NM_CLICK:
                    NotifyCode = TV_NOTIFY_CODE_CLICK;
                    break;
                case NM_DBLCLK:
                    NotifyCode = TV_NOTIFY_CODE_DBLCLK;
                    break;
                default:
                    NotifyCode = TV_NOTIFY_CODE_UNKNOWN;
                    break;
            }
            
            if (TV_NOTIFY_CODE_UNKNOWN != NotifyCode)
            {
                *parg = (LPARAM)hti;
                *pparam = htFlags;
                *pcookie = (MMC_COOKIE)TI.lParam;
            }
        }
    }
    
    return NotifyCode;
}

// OnDestroy may be called on two occasions:
// (1). We are the current active result pane window and MMC
//      is destroying our parent window(MDI client). Note that
//      if we are not the active result pane window, this function
//      will not get called until (2).
// (2). our reference count has reached zero.
//
//  When (1) happens, the snapin may be still holding reference to us
//  thus, even though our window has been destroyed (2) still happens
//  (unless PostNcDestory is done which MFC reset m_hWnd)and we end up
//  destoying the window twice.
//  So, we keep an eye on OnDestroy and do nothing after it has been called.
//  We can not wait for PostNcDestroy because we have no idea when it would
//  come.
//
void CTVCtrl::OnDestroy()
{
    if (!m_Destroyed)
    {
        COleControl::OnDestroy();
        m_Destroyed = TRUE;
    }
}

void CTVCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
    // TODO: Add your message handler code here
    POINT pt = point;
    UINT htFlags;
    HTREEITEM hti = HitTest(pt.x, pt.y, &htFlags);

    if (hti)
    {
        TV_ITEM TI;
        TI.hItem = hti;
        TI.mask = TVIF_PARAM;
        if (SUCCEEDED(GetItem(&TI)))
        {
            m_pISnapinCallback->tvNotify(*this, (MMC_COOKIE)TI.lParam,
                                        TV_NOTIFY_CODE_CONTEXTMENU,
                                        (LPARAM)hti, (LPARAM)&point );
        }
    }
}

// The upmost frame window may have its own accelerator table and may take
// away certain key combinations we really need.
BOOL CTVCtrl::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add your specialized code here and/or call the base class
    if (WM_KEYDOWN == pMsg->message &&
       (VK_DELETE == pMsg->wParam ||
        VK_RETURN == pMsg->wParam))
    {
        OnKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
        return TRUE;
    }
        
    else if (WM_SYSKEYDOWN == pMsg->message && VK_F10 == pMsg->wParam &&
             GetKeyState(VK_SHIFT) < 0)
    {
        // Shift-F10 will be translated to WM_CONTEXTMENU
        OnSysKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
        return TRUE;
    }

    return COleControl::PreTranslateMessage(pMsg);
}
