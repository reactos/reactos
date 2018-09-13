//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dlgsite.cxx
//
//  Contents:   Implementation of the frame for hosting html dialogs
//
//  History:    06-14-96  AnandRa   Created
//
//  Notes:      This frame does not supply a real frame window if
//              it is acting as a property page.  If it's acting as 
//              a dialog, then it does.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

DeclareTag(tagHTMLDlgFrameMethods, "HTML Dialog Frame", "Methods on the html dialog frame")

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTMLDlgFrame, CHTMLDlg, HTMLDlg, _Frame);


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IOleInPlaceFrame ||
        iid == IID_IOleWindow ||
        iid == IID_IOleInPlaceUIWindow ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleInPlaceFrame *) this;
        AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::GetWindow(HWND * phWnd)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleWindow::GetWindow"));
    
    *phWnd = HTMLDlg()->_hwnd;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleWindow::ContextSensitiveHelp"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::GetBorder
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::GetBorder(LPOLERECT prcBorder)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceUIWindow::GetBorder"));

#ifndef WIN16    
    HTMLDlg()->GetViewRect(prcBorder);
#else
    RECTL rcBorder;
    HTMLDlg()->GetViewRect(&rcBorder);
    CopyRect(prcBorder, &rcBorder);
#endif
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::RequestBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::RequestBorderSpace(LPCBORDERWIDTHS pbw)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceUIWindow::RequestBorderSpace"));
    
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::SetBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::SetBorderSpace(LPCBORDERWIDTHS pbw)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceUIWindow::SetBorderSpace"));
    
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::SetActiveObject
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::SetActiveObject(
        LPOLEINPLACEACTIVEOBJECT    pActiveObj,
        LPCTSTR                     pstrObjName)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceUIWindow::SetActiveObject"));
    
    ReplaceInterface(&HTMLDlg()->_pInPlaceActiveObj, pActiveObj);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::InsertMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{        
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::InsertMenus"));
    
    pmgw->width[0] = pmgw->width[2] = pmgw->width[4] = 0;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::SetMenu
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::SetMenu"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::RemoveMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::RemoveMenus(HMENU hmenuShared)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::RemoveMenus"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::SetStatusText
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::SetStatusText(LPCTSTR szStatusText)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::SetStatusText"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::EnableModeless
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::EnableModeless(BOOL fEnable)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::EnableModeless"));
    
    //  BUGBUG: (anandra) should probably disable ourselves?
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgFrame::TranslateAccelerator
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgFrame::TranslateAccelerator(LPMSG pmsg, WORD wID)
{
    TraceTag((tagHTMLDlgFrameMethods, "IOleInPlaceFrame::TranslateAccelerator"));
    
    return S_FALSE;
}


