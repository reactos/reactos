//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       docobj.cxx
//
//  Contents:   Implementation for IOleDocument and IOleDocumentView
//
//----------------------------------------------------------------------------

#include "headers.hxx"


DeclareTag(tagMsoView, "IOleDocumentView", "IOleDocumentView methods in CServer")
DeclareTag(tagMsoDoc, "IOleDocument", "IOleDocument methods in CServer")


//+-------------------------------------------------------------------------
//
//  IOleDocument implementation
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CServer::CreateView, IOleDocument
//
//  Synopsis:   Asks the document to create a new viewand optionally
//              make the view initialize its view state from the given
//              stream.  Fails if we support only one view and this
//              method has already been called.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::CreateView(
        IOleInPlaceSite * pIPSite,
        IStream * pStm,
        DWORD dwReserved,
        IOleDocumentView ** ppView)
{
    HRESULT         hr;

    TraceTag((tagMsoDoc, "CServer::CreateView"));
    Assert(_fMsoDocMode);
    Assert(pIPSite);

    if (_fMsoViewExists)
    {
        RRETURN(E_FAIL);
    }

    hr = THR(EnsureInPlaceObject());
    if (hr)
        goto Cleanup;

    hr = THR(SetInPlaceSite(pIPSite));
    if (hr)
        goto Cleanup;

    if (pStm)
    {
        IGNORE_HR(ApplyViewState(pStm));
    }

    hr = THR(PrivateQueryInterface(IID_IOleDocumentView, (void **)ppView));
    if (hr)
        goto Cleanup;

    _fMsoViewExists = TRUE;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetDocMiscStatus, IOleDocument
//
//  Synopsis:   Returns miscellaneous information about the doc object.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::GetDocMiscStatus(DWORD * pdwStatus)
{
    TraceTag((tagMsoDoc, "CServer::GetDocMiscStatus"));
    Assert(_fMsoDocMode);

    *pdwStatus = DOCMISC_CANTOPENEDIT | DOCMISC_NOFILESUPPORT;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::EnumViews, IOleDocument
//
//  Synopsis:   Enumerates the views of the document.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::EnumViews(IEnumOleDocumentViews ** ppEnumViews, IOleDocumentView ** ppView)
{
    TraceTag((tagMsoDoc, "CServer::EnumViews"));
    Assert(_fMsoDocMode);

    HRESULT hr = S_OK;

    *ppEnumViews = NULL;

    if (_fMsoViewExists)
    {
        hr = THR(PrivateQueryInterface(IID_IOleDocumentView, (void **)ppView));
    }
    else
    {
        *ppView = NULL;
    }

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  IOleDocumentView implementation
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CServer::SetInPlaceSite, IOleDocumentView
//
//  Synopsis:   Detaches from the existing docsite, inplace deactivates
//              and saves new pointer.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::SetInPlaceSite(IOleInPlaceSite * pIPSite)
{
    HRESULT hr = S_OK;
    IOleInPlaceSiteEx * pIPSiteEx = NULL;
    
    TraceTag((tagMsoView, "CServer::SetInPlaceSite"));
    Assert(_fMsoDocMode);

    IGNORE_HR(InPlaceDeactivate());
    if (pIPSite)
    {
        hr = THR(EnsureInPlaceObject());
        if (hr)
            goto Cleanup;

        // CHROME
        // When Chrome hosted we only ever want to be actived
        // windowless so don't try for a windowed site if Chrome
        // hosted
        if (   !IsChromeHosted()
            && !(THR_NOTRACE(pIPSite->QueryInterface(
                   IID_IOleInPlaceSiteEx, (void **)&pIPSiteEx))))
        {
            //
            // This is really something that supports InPlaceSiteEx
            //

            _pInPlace->_fUseExtendedSite = TRUE;
            ReplaceInterface(
                &_pInPlace->_pInPlaceSite, 
                (IOleInPlaceSite *)pIPSiteEx);
        }
        // CHROME
        // Check for Windowless site
        // NOTE: This is an optional change as CServer::ActivateInPlace
        // has the smarts to negotiate for a windowless site. Problem is
        // that if a container sets the site here, CServer::ActivateInPlace
        // will not negotaite for one. So to allow the container to set a
        // windowless site this change is necessary.
        else if (IsChromeHosted())
        {
            IOleInPlaceSiteWindowless * pIPSiteWindowless = NULL;

            // When Chrome hosted the site must support IOleInPlaceSiteWindowless.
            // Assert that we got a windowless site
            hr = THR(pIPSite->QueryInterface(
                    IID_IOleInPlaceSiteWindowless, (void **)&pIPSiteWindowless));
            Assert(SUCCEEDED(hr));
			if (FAILED(hr))
				goto Cleanup;

            //
            // This is really something that supports InPlaceSiteWindowless
            //
            _fWindowless = TRUE;
            _pInPlace->_fUseExtendedSite = TRUE;
            ReplaceInterface(&_pInPlace->_pInPlaceSite,
                            (IOleInPlaceSite *)pIPSiteWindowless);
            ReleaseInterface(pIPSiteWindowless);
        }
        else
        {
            _pInPlace->_fUseExtendedSite = FALSE;
            ReplaceInterface(&_pInPlace->_pInPlaceSite, pIPSite);
        }
        ReleaseInterface(pIPSiteEx);
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetInPlaceSite, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::GetInPlaceSite(IOleInPlaceSite ** ppIPSite)
{
    TraceTag((tagMsoView, "CServer::GetInPlaceSite"));
    Assert(_fMsoDocMode);

    if (_pInPlace && _pInPlace->_pInPlaceSite)
    {
        _pInPlace->_pInPlaceSite->AddRef();
        
        *ppIPSite = _pInPlace->_pInPlaceSite;
    }
    else
    {
        *ppIPSite = NULL;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetDocument, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::GetDocument(IUnknown ** ppUnk)
{
    TraceTag((tagMsoView, "CServer::GetDocument"));
    Assert(_fMsoDocMode);

    // cast to any non-tearoff interface
    *ppUnk = (IViewObject *)this;
    (*(IUnknown **) ppUnk)->AddRef();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::SetRect, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::SetRect(LPRECT prcView)
{
    long lDirty = 0;
    HRESULT hr = S_OK;
    SIZEL sizel;

    TraceTag((tagMsoView, "CServer::SetRect"));
    Assert(_fMsoDocMode);

    // BUGBUG (garybu): SHDOCVW incorrect calls this method
    // before we are inplace activated. Look at bug 20233
    // for more info about this if statement.
    if (State() < OS_INPLACE)
    {
        goto Cleanup;
    }

    sizel.cx = HimetricFromHPix(prcView->right - prcView->left);
    sizel.cy = HimetricFromVPix(prcView->bottom - prcView->top);

    if (sizel.cx != _sizel.cx || sizel.cy != _sizel.cy)
    {
        hr = THR(SetExtent(DVASPECT_CONTENT, &sizel));
        if (hr)
            goto Cleanup;
        //
        // BUGBUG - marka bug for 10161
        // notifications are inadvertently setting dirtiness.
        // this must be fixed for beta2
        //
        lDirty = _lDirtyVersion;
        hr = THR(SetObjectRects(ENSUREOLERECT(prcView), ENSUREOLERECT(prcView)));
        if ( ( ! lDirty ) && ( _lDirtyVersion ))
            _lDirtyVersion = 0;
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetRect, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::GetRect(LPRECT prcView)
{
    TraceTag((tagMsoView, "CServer::GetRect"));

    Assert(_pInPlace);
    Assert(_fMsoDocMode);

    *prcView = _pInPlace->_rcPos;
    OffsetRect(prcView, _pInPlace->_ptWnd.x, _pInPlace->_ptWnd.y);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::SetRectComplex, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::SetRectComplex(
        LPRECT lprcView,
        LPRECT lprcHScroll,
        LPRECT lprcVScroll,
        LPRECT lprcSizeBox)
{
    TraceTag((tagMsoView, "CServer::SetRectComplex"));
    Assert(_fMsoDocMode);

    RRETURN(E_FAIL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::Show, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::Show(BOOL fShow)
{
    HRESULT     hr = S_OK;

    TraceTag((tagMsoView, "CServer::Show"));
    Assert(_fMsoDocMode);

    if (fShow)
    {
        if (_state < OS_INPLACE)
        {
            hr = THR(TransitionTo(OS_INPLACE, NULL));
            if (hr)
                goto Cleanup;
        }

        if (_pInPlace->_hwnd)
        {
            ShowWindow(_pInPlace->_hwnd, SW_SHOWNORMAL);
        }
    }
    else
    {
        if (_state == OS_UIACTIVE)
        {
            IGNORE_HR(UIActivate(FALSE));
        }

        if (_state >= OS_INPLACE && _pInPlace->_hwnd)
        {
            ShowWindow(_pInPlace->_hwnd, SW_HIDE);
        }
    }
    
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::UIActivate, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::UIActivate(BOOL fActivate)
{
    HRESULT     hr = S_OK;
    long lDirtyBefore;
    TraceTag((tagMsoView, "CServer::UIActivate"));
    Assert(_fMsoDocMode);

    lDirtyBefore = _lDirtyVersion;
    if (fActivate && (_state < OS_UIACTIVE))
    {
        hr = THR(TransitionTo(OS_UIACTIVE, NULL));
    }
    else if ((fActivate == FALSE) && (_state == OS_UIACTIVE))
    {
        IGNORE_HR(UIDeactivate());
    }
    //
    // BUGBUG ( marka) - HACK for Bug 10161
    // to be fixed post beta 2
    //
    if ( ( ! lDirtyBefore  ) && ( _lDirtyVersion))
        _lDirtyVersion = 0;
        
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::Open, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::Open(void)
{
    TraceTag((tagMsoView, "CServer::Open"));
    Assert(_fMsoDocMode);

    //
    //  No view frame of our own
    //

    RRETURN(E_FAIL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::CloseView, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::CloseView(DWORD dwReserved)
{
    TraceTag((tagMsoView, "CServer::CloseView"));
    Assert(_fMsoDocMode);

    //
    //  Implementation of IOleDocumentView::CloseView - we support
    //  only one view.  Send the view the NULL InPlaceSite.
    //

    IGNORE_HR(SetInPlaceSite(NULL));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::SaveViewState, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::SaveViewState(IStream * pStm)
{
    TraceTag((tagMsoView, "CServer::SaveViewState"));
    Assert(_fMsoDocMode);

    //  No view state for now
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::ApplyViewState, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::ApplyViewState(IStream * pStm)
{
    TraceTag((tagMsoView, "CServer::ApplyViewState"));
    Assert(_fMsoDocMode);

    //  No view state for now
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::Clone, IOleDocumentView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

HRESULT
CServer::Clone(IOleInPlaceSite * pNewIPSite, IOleDocumentView ** ppNewView)
{
    TraceTag((tagMsoView, "CServer::Clone"));
    Assert(_fMsoDocMode);

    return E_NOTIMPL;
}

