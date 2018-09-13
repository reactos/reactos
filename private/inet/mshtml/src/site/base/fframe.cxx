//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  Contents:   Fake frame implementation.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

inline CFormInPlace * CFakeUIWindow::InPlace()
{ 
    return Doc()->InPlace(); 
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::AddRef
//
//  Synopsis:   Per IUnknown
//
//-------------------------------------------------------------------------

ULONG CFakeUIWindow::AddRef()
{
    return Doc()->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::Release
//
//  Synopsis:   Per IUnknown
//
//-------------------------------------------------------------------------

ULONG CFakeUIWindow::Release()
{
    return Doc()->SubRelease();
}


//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IOleWindow ||
        iid == IID_IOleInPlaceUIWindow)
    {
        *ppv = (IOleInPlaceUIWindow *)this;
    }
    else if (this == &Doc()->_FakeInPlaceFrame &&
        iid == IID_IOleInPlaceFrame)
    {
        *ppv = (IOleInPlaceFrame *)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::GetWindow, IOleWindow
//
//  Synopsis:   Get HWND
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::GetWindow(HWND *phwnd)
{
    RRETURN(THR(RealInPlaceUIWindow()->GetWindow(phwnd)));
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::ContextSensitiveHelp, IOleWindow
//
//  Synopsis:   Controls enabling of context sensitive help.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::ContextSensitiveHelp(BOOL fEnterMode)
{
    RRETURN(THR(RealInPlaceUIWindow()->ContextSensitiveHelp(fEnterMode)));
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::GetBorderSpace, IOleInplaceUIWindow
//
//  Synopsis:   Get frame size.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::GetBorder(LPOLERECT lprectBorder)
{
    RRETURN(THR(RealInPlaceUIWindow()->GetBorder(lprectBorder)));
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::RequestBorderSpace, IOleInplaceUIWindow
//
//  Synopsis:   Get border space for tools.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    RRETURN(THR(RealInPlaceUIWindow()->RequestBorderSpace(pborderwidths)));
}


//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::SetBorderSpace, IOleInplaceUIWindow
//
//  Synopsis:   Get border space for tools.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    HRESULT hr = S_OK;

#ifndef NO_OLEUI
    if (pborderwidths)
    {
        Doc()->InPlace()->_fForwardSetBorderSpace = TRUE;
        Doc()->RemoveUI();
        hr = THR(RealInPlaceUIWindow()->SetBorderSpace(pborderwidths));
    }
    else
    {
        if (Doc()->InPlace()->_fForwardSetBorderSpace)
        {
            hr = THR(RealInPlaceUIWindow()->SetBorderSpace(pborderwidths));
            if (hr)
                RRETURN(hr);
            Doc()->InPlace()->_fForwardSetBorderSpace = FALSE;
        }
        
        if (Doc()->InPlace()->_fUIDown &&
            !Doc()->InPlace()->_fChildActivating &&
            !Doc()->InPlace()->_fDeactivating &&
            Doc()->State() >= OS_UIACTIVE)
        {
            hr = THR(Doc()->InstallUI(FALSE));
        }
    }
#endif // NO_OLEUI

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::SetActiveObject, IOleInplaceUIWindow
//
//  Synopsis:   Notify currently active object.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    IOleInPlaceUIWindow *   p = RealInPlaceUIWindow();
    HRESULT                 hr = S_OK;

    //
    // If an ocx is not going active and another ocx calls SetActiveObject
    // with NULL and we happen to be UI-Active, tell frame that we're the
    // active object instead.
    //
    
    if (p)
    {
        if (!pActiveObject && 
            !Doc()->InPlace()->_fChildActivating &&
            !Doc()->InPlace()->_fDeactivating &&
            Doc()->State() >= OS_UIACTIVE)
        {
            hr = THR(Doc()->SetActiveObject());
        }
        else
        {
            hr = THR(p->SetActiveObject(pActiveObject, pszObjName));
        }
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::InsertMenus, IOleInPlaceFrame
//
//  Synopsis:   Allow container to insert menus.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
#ifdef NO_OLEUI
	RRETURN(S_OK);
#else
    Doc()->RemoveUI();
    RRETURN(THR(InPlace()->_pFrame->InsertMenus(hmenuShared, lpMenuWidths)));
#endif // NO_OLEUI
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::SetMenu, IOleInPlaceFrame
//
//  Synopsis:   Add composite menu to window frame.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    HRESULT             hr = S_OK;

#ifndef NO_OLEUI
    if (InPlace() && InPlace()->_pFrame)
    {
        if (hmenuShared && holemenu)
        {
            Doc()->InPlace()->_fForwardSetMenu = TRUE;
            Doc()->RemoveUI();
            hr = THR(InPlace()->_pFrame->SetMenu(hmenuShared, holemenu, hwndActiveObject));
        }
        else
        {
            if (Doc()->InPlace()->_fForwardSetMenu)
            {
                hr = THR(InPlace()->_pFrame->SetMenu(
                        NULL, 
                        NULL, 
                        hwndActiveObject));
                if (hr)
                    RRETURN(hr);
                Doc()->InPlace()->_fForwardSetMenu = FALSE;
            }
            
            if (Doc()->InPlace()->_fUIDown &&
                !Doc()->InPlace()->_fChildActivating &&
                !Doc()->InPlace()->_fDeactivating &&
                Doc()->State() >= OS_UIACTIVE)
            {
                hr = THR(Doc()->InstallUI(FALSE));
            }
        }
    }
#endif // NO_OLEUI
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::SetMenu, IOleInPlaceFrame
//
//  Synopsis:   Remove container's menus.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::RemoveMenus(HMENU hmenuShared)
{
    HRESULT hr;

    if (hmenuShared && InPlace() && InPlace()->_pFrame)
    {
        hr = THR(InPlace()->_pFrame->RemoveMenus(hmenuShared));
    }
    else
    {
        hr = S_OK;
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::SetStatusText, IOleInPlaceFrame
//
//  Synopsis:   Set and display status text.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::SetStatusText(LPCOLESTR pszStatusText)
{
    HRESULT hr;
    
    if (InPlace() && InPlace()->_pFrame)
    {
        hr = THR(InPlace()->_pFrame->SetStatusText(pszStatusText));
    }
    else
    {
        hr = S_OK;
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::EnableModeless, IOleInPlaceFrame
//
//  Synopsis:   Enable / Disable modeless dialogs.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::EnableModeless(BOOL fEnable)
{
    HRESULT hr;
    
    if (InPlace() && InPlace()->_pFrame)
    {
        hr = THR(InPlace()->_pFrame->EnableModeless(fEnable));
    }
    else
    {
        hr = S_OK;
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeUIWindow::TranslateAccelerator, IOleInPlaceFrame
//
//  Synopsis:   Translate keystrokes.
//
//-------------------------------------------------------------------------

HRESULT
CFakeUIWindow::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    HRESULT hr;

    if (InPlace() && InPlace()->_pFrame)
    {
        hr = THR(InPlace()->_pFrame->TranslateAccelerator(lpmsg, wID));
    }
    else
    {
        hr = S_FALSE;
    }
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CFakeInPlaceFrame::RealInPlaceUIWindow, CFakeUIWindow
//
//  Synopsis:   Return the true frame.
//
//-------------------------------------------------------------------------

IOleInPlaceUIWindow *
CFakeInPlaceFrame::RealInPlaceUIWindow()
{
    CDoc *pDoc = Doc();
    return pDoc->InPlace() ? pDoc->InPlace()->_pFrame : NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CFakeDocUIWindow::RealInPlaceUIWindow, CFakeUIWindow
//
//  Synopsis:   Return the true document window.
//
//-------------------------------------------------------------------------

IOleInPlaceUIWindow *
CFakeDocUIWindow::RealInPlaceUIWindow()
{
    return Doc()->InPlace()->_pDoc;
}

CDoc * CFakeDocUIWindow::Doc()
{
    return CONTAINING_RECORD(this, CDoc, _FakeDocUIWindow);
}   

CDoc * CFakeInPlaceFrame::Doc()
{
    return CONTAINING_RECORD(this, CDoc, _FakeInPlaceFrame);
}    

