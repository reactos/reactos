//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       frame.cxx
//
//  Contents:   Implementation of frame object
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTMLAppFrame, CHTMLApp, HTMLApp, _Frame)

CHTMLAppFrame::CHTMLAppFrame()
    : _pActiveObj(NULL)
{
}

void CHTMLAppFrame::Passivate()
{
}

void CHTMLAppFrame::Resize()
{
    // Note:  We provide the client area rectangle because:
    //
    //  1.  No frame-level adornments are allowed.
    //  2.  No status bar is implemented.
    //
    //  if either condition changes, we should revist this code so that the
    //  appropriate area is excluded from this rect.
    //
    
    if (_pActiveObj)
    {
        RECT rc;
        HTMLApp()->GetViewRect(&rc);
        _pActiveObj->ResizeBorder( &rc, this, TRUE);
    }
}

HRESULT
CHTMLAppFrame::TranslateKeyMsg(MSG * pMsg)
{
    HRESULT hr = S_FALSE;

    // Only allow keyboard messages to be translated
    if (_pActiveObj && ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST)))
    {
        hr = _pActiveObj->TranslateAccelerator(pMsg);
    }
    
    RRETURN1(hr, S_FALSE);
}

HRESULT CHTMLAppFrame::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IOleInPlaceFrame ||
        riid == IID_IOleWindow ||
        riid == IID_IUnknown)
    {
        *ppv = (IOleInPlaceFrame *) this;
    }
    else if (riid == IID_IOleInPlaceUIWindow)
    {
        *ppv = (IOleInPlaceUIWindow *) this;
    }

    if (*ppv)
    {
        AddRef();
        return S_OK;
    }
    
    RRETURN(E_NOINTERFACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::GetWindow(HWND * phWnd)
{
    *phWnd = HTMLApp()->_hwnd;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::GetBorder
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::GetBorder(LPOLERECT prcBorder)
{
    HTMLApp()->GetViewRect(prcBorder);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::RequestBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::RequestBorderSpace(LPCBORDERWIDTHS pbw)
{
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::SetBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::SetBorderSpace(LPCBORDERWIDTHS pbw)
{
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::SetActiveObject
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::SetActiveObject(
        LPOLEINPLACEACTIVEOBJECT    pActiveObj,
        LPCTSTR                     pstrObjName)
{
    ReplaceInterface(&_pActiveObj, pActiveObj);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::InsertMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{        
    
    pmgw->width[0] = pmgw->width[2] = pmgw->width[4] = 0;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::SetMenu
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND 
hwndActiveObject)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::RemoveMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::RemoveMenus(HMENU hmenuShared)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::SetStatusText
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::SetStatusText(LPCTSTR szStatusText)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::EnableModeless
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::EnableModeless(BOOL fEnable)
{
    //  BUGBUG: should probably disable ourselves?
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLAppFrame::TranslateAccelerator
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLAppFrame::TranslateAccelerator(LPMSG pmsg, WORD wID)
{
    return S_FALSE;
}


