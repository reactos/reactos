//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       proppage.cxx
//
//  Contents:   Display property dialog.
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_PROPPAGE_HXX_
#define X_PROPPAGE_HXX_
#include "proppage.hxx"
#endif

const CBase::CLASSDESC CPropertyPage::s_classdesc = 
{
    NULL,                               // _pclsid
    0,                                  // _idrBase
    NULL,                               // _apClsidPages
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    NULL,                               // _piidDispinterface
    NULL,                               // _apHdlDesc
};

#ifdef WIN16
CPropertyPage::CPropertyPage(IUnknown *pUnkOuter)
        : _cstrTitle(CSTR_NOINIT),
          _cstrDocString(CSTR_NOINIT) 
{
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Passivate
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

void
CPropertyPage::Passivate()
{
    // clean up our window.
    //
    if (_hwnd)
    {
        SetWindowLong(_hwnd, DWL_USER, 0xffffffff);
        DestroyWindow(_hwnd);
    }

    // release all the objects we're holding on to.
    //
    ReleaseAllObjects();

    // release the site
    //
    ClearInterface(&_pPropertyPageSite);
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::QueryInterface
//
//  Synopsis:   we support IPP and IPP2.
//
//
//-------------------------------------------------------------------------

HRESULT
CPropertyPage::PrivateQueryInterface(REFIID iid, void **ppvObj)
{
    *ppvObj = NULL;

    if ( IsEqualIID(iid, IID_IPropertyPage) )
    {
        *ppvObj = (IPropertyPage *)this;
    }
    else if ( IsEqualIID(iid, IID_IPropertyPage2) )
    {
        *ppvObj = (IPropertyPage2 *)this;
    }
    else
    {
        RRETURN(super::PrivateQueryInterface(iid, ppvObj));
    }

    ((IUnknown*)(*ppvObj))->AddRef();

    return S_OK;
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Init
//
//  Synopsis:   Init.
//
//  Note:       Load up the descriptor strings
//
//-------------------------------------------------------------------------

HRESULT
CPropertyPage::Init(void)
{
    HRESULT hr;
    int cch;
    TCHAR * pch;

    hr = LoadString(GetResourceHInst(), ProppageDesc()->wTitleId, &cch, &pch);
    if ( hr )
        goto Cleanup;

    hr = _cstrTitle.Set(pch, cch);
    if ( hr )
        goto Cleanup;

    hr = LoadString(GetResourceHInst(), ProppageDesc()->wDocStringId, &cch, &pch);
    if ( hr )
        goto Cleanup;

    hr = _cstrDocString.Set(pch, cch);
    if ( hr )
        goto Cleanup;


Cleanup:
    RRETURN(hr);
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::SetPageSite
//
//  Synopsis:   Set the proppage site on the proppage dialog
//
//  Arguments:  pPropertyPageSite  - [in] new site.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::SetPageSite(IPropertyPageSite *pPropertyPageSite)
{
    if ( pPropertyPageSite && _pPropertyPageSite )
        return E_UNEXPECTED;

    ReplaceInterface(&_pPropertyPageSite, pPropertyPageSite);

    return S_OK;
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Activate
//
//  Synopsis:   instructs the page to create its
//              display window as a child of hwndparent
//              and to position it according to prc
//
//  Arguments:  hwndParent      the parent window
//              prcBounds       the rect to use for display
//              fModal          or not
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Activate(HWND    hwndParent,
                        LPCRECT prcBounds,
                        BOOL    fModal)
{
    HRESULT hr = S_OK;
    BOOL fActivating = _fActivating;

    _fActivating = TRUE;

    // first make sure the dialog window is loaded and created.
    //
    if (_hwnd)
        goto Cleanup;

    _hwnd = CreateDialogParam(GetResourceHInst(),
                              MAKEINTRESOURCE(GetProppageDesc()->wDlgResourceId),
                              hwndParent,
                              &CPropertyPage::DlgProc,
                              (LPARAM)this);
    if ( ! _hwnd )
        goto Win32Error;

    _fActive = TRUE;

    THR_NOTRACE(ConnectObjects());

    // now move ourselves to where we're told to be and show ourselves
    //
    Move(prcBounds);

Cleanup:
    _fActivating = fActivating;
    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Deactivate
//
//  Synopsis:   instructs the page to destroy the window created in activate
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Deactivate(void)
{
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
    }

    _hwnd = NULL;
    _fActive = FALSE;

    Disconnect();

    return S_OK;
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::GetPageInfo
//
//  Synopsis:   asks the page to fill a PROPPAGEINFO structure
//
//  Arguments:  pPropPageInfo - [out] where to put info.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::GetPageInfo(PROPPAGEINFO * pPropPageInfo)
{
    HRESULT hr = S_OK;
    RECT rc;

    if ( ! pPropPageInfo )
        return E_POINTER;


    memset(pPropPageInfo, 0, sizeof(PROPPAGEINFO));
    pPropPageInfo->cb = sizeof(PROPPAGEINFO);

    hr = TaskAllocString(_cstrTitle, &pPropPageInfo->pszTitle);
    if ( hr )
        goto Cleanup;

    TaskAllocString(_cstrDocString, &pPropPageInfo->pszDocString);
    TaskAllocString(ProppageDesc()->szHelpFile, &pPropPageInfo->pszHelpFile);
    pPropPageInfo->dwHelpContext = ProppageDesc()->dwHelpContextId;

    // if we've got a window yet, go and set up the size information they want.
    //
    if (_hwnd)
    {
        GetWindowRect(_hwnd, &rc);
        pPropPageInfo->size.cx = rc.right - rc.left;
        pPropPageInfo->size.cy = rc.bottom - rc.top;
    }
    else
    {
        DLGTEMPLATE * pdt;

        pdt = (DLGTEMPLATE *)GetResource(GetResourceHInst(),
                                         MAKEINTRESOURCE(ProppageDesc()->wDlgResourceId),
                                         RT_DIALOG,
                                         NULL);

        if ( pdt )
        {
            hr = ComputeDialogSize(pdt, &pPropPageInfo->size);
        }
    }


Cleanup:
    RRETURN(hr);

}







//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::SetObjects
//
//  Synopsis:   provides the page with the objects being affected by the changes.
//
//  Arguments:  cObjects        - [in] count of objects.
//              ppUnkObjects    - [in] objects.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::SetObjects(ULONG       cObjects,
                          IUnknown ** ppUnkObjects)
{
    HRESULT hr = S_OK;
    ULONG   i;
    IUnknown * pUnk;

    ReleaseAllObjects();

    if (!cObjects)
        goto Cleanup;

    // now go and set up the new ones.
    //

    hr = _aryUnk.EnsureSize(cObjects);
    if ( hr )
        goto Cleanup;


    //  Loop through and copy over all the objects.
    //  Get only those supporting our requested primary interface

    for (i = 0; i < cObjects; i++)
    {
        if ( ppUnkObjects[i] &&
             OK(ppUnkObjects[i]->QueryInterface(*GetProppageDesc()->piidPrimary, (void**)&pUnk)) )
        {
            hr = _aryUnk.Append(pUnk);
            if ( hr )
                goto Cleanup;
        }

    }


    _fDirty = FALSE;


Cleanup:
    RRETURN(hr);
}






//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Show
//
//  Synopsis:   asks the page to show or hide its window
//
//  Arguments:  nCmdShow    - [in] whether to show or hide
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Show(UINT nCmdShow)
{
    if ( ! _hwnd )
        return E_UNEXPECTED;

    ShowWindow(_hwnd, nCmdShow);

    return S_OK;
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Move
//
//  Synopsis:   asks the page to relocate and resize itself
//              to a position other than what was specified through Activate
//
//  Arguments:  prcBounds    - [in] new position and size
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Move(LPCRECT prcBounds)
{
    if ( ! _hwnd )
    {
        return E_UNEXPECTED;
    }

    MoveWindow(_hwnd,
               prcBounds->left,
               prcBounds->top,
               prcBounds->right - prcBounds->left,
               prcBounds->bottom - prcBounds->top,
               TRUE);

    return S_OK;
}






//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::IsPageDirty
//
//  Synopsis:   asks the page whether it has changed its state
//
//  Returns:    S_OK    dirty
//              S_FALSE not dirty
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::IsPageDirty(void)
{
    return _fDirty ? S_OK : S_FALSE;
}







//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Apply
//
//  Synopsis:   instructs the page to send its changes
//              to all the objects passed through SetObjects()
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Apply(void)
{
    HRESULT hr = S_OK;

    if (_fDirty)
    {
        _fDirty = FALSE;
        if (_pPropertyPageSite)
        {
            _pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    }

    RRETURN(hr);
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Help
//
//  Synopsis:   instructs the page that the help button was clicked.
//
//  Arguments:  pszHelpDir  - [in] help directory
//
//  Note:       Default implementation bounces back with E_NOTIMPL
//              forcing caller to use help info from GetPageInfo.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::Help(LPCOLESTR pszHelpDir)
{
    if ( _hwnd )
        return E_UNEXPECTED;

    return E_NOTIMPL;   //  force prop sheet to use GetPageInfo.
}








//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::TranslateAccelerator
//
//  Synopsis:   informs the page of keyboard events,
//              allowing it to implement its own keyboard interface.
//
//  Arguments:  pmsg   - [in] message that triggered this
//
//  Returns:    S_OK    if consumed
//              S_FALSE if not processed
//              E_xxxx  if error
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::TranslateAccelerator(LPMSG pmsg)
{
    Assert(_hwnd && "How can we get a TranslateAccelerator call if we're not visible?");
    Assert(_pPropertyPageSite);

    // just pass this message on to the dialog proc and see if they want it.
    //
    RRETURN1(_pPropertyPageSite->TranslateAccelerator(pmsg),S_FALSE);
}







//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::EditProperty
//
//  Synopsis:   instructs the page to set the focus
//              to the property matching the dispid.
//
//  Arguments:  dispid   - [in] dispid of property to set focus to.
//
//  Note:       Derived should overload
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::EditProperty(DISPID dispid)
{
    return E_NOTIMPL;
}






//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::ReleaseAllObjects, protected
//
//  Synopsis:   releases all the objects that we're working with
//
//  Note:       Derived class can overload to catch the action
//
//-------------------------------------------------------------------------

void
CPropertyPage::ReleaseAllObjects(void)
{
    //  Be robust. Objects are disconnected in Deactivate but
    //  we want ot be able to survive the lack of it

    Disconnect();
    _aryUnk.ReleaseAll();
}





//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::ComputeDialogSize, protected
//
//  Synopsis:   releases all the objects that we're working with
//
//  Note:       Derived class can overload to catch the action
//
//-------------------------------------------------------------------------

HRESULT
CPropertyPage::ComputeDialogSize(DLGTEMPLATE * pdt, SIZE * psize)
{
    HRESULT hr = S_OK;
    int i;
    SIZE sizeBaseUnit = /* BUGBUG:INIT */ {0,0};
    IFont *pFont = 0;

    Assert(pdt);
    Assert(psize);

    if ( pdt->style & DS_SETFONT )
    {
        short * ps;
        int iFontSize;
        TCHAR * pszFontName;

        //  Walk the dialog template to get at the specified font

        ps = (short*)((char*)pdt + sizeof(DLGTEMPLATE));

        //  Skip the menu and class descriptors
        for ( i = 0; i < 2; i++ )
        {
            switch (*ps)
            {
                case 0:
                    //  Non-existent
                    ps++;
                    break;

                case 0xffff:
                    //  Specified via resourceID/atom,
                    //  next short is the ID
                    ps += 2;
                    break;

                default:
                    //  Null-terminated Unicode string
                    while (*ps++);
                    break;
            }
        }

        while(*ps++);

        iFontSize = *ps++;
        pszFontName = (TCHAR *)ps;

        //  Get the font

        if ( iFontSize && pszFontName )
        {
            FONTDESC fd;
            TEXTMETRICOLE tm;

            memset(&fd, 0, sizeof(fd));
            fd.cbSizeofstruct = sizeof(fd);
            fd.lpstrName = pszFontName;
            fd.cySize.Hi = iFontSize;

            hr = OleCreateFontIndirect(&fd, IID_IFont, (void**)&pFont);
            if ( hr )
                goto Cleanup;

            hr = pFont->QueryTextMetrics(&tm);
            if ( hr )
                goto Cleanup;

            sizeBaseUnit.cx = tm.tmAveCharWidth;
            sizeBaseUnit.cy = tm.tmHeight;
        }
    }
    else
    {
        DWORD dwBaseUnits;

        dwBaseUnits = GetDialogBaseUnits();
        sizeBaseUnit.cx = LOWORD(dwBaseUnits);
        sizeBaseUnit.cy = HIWORD(dwBaseUnits);
    }

    psize->cx = ( pdt->cx * sizeBaseUnit.cx) / 4;
    psize->cy = ( pdt->cy * sizeBaseUnit.cy) / 8;


Cleanup:
    ReleaseInterface(pFont);
    RRETURN(hr);
}







//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::PropPageDlgProc
//
//  Synopsis:   static global helper dialog proc
//              that gets called before we pass the message on to anybody
//
//  Arguments:  Like a Dialog Proc. See Win32
//
//-------------------------------------------------------------------------

BOOL CALLBACK
CPropertyPage::DlgProc(HWND    hwnd,
                       UINT    msg,
                       WPARAM  wParam,
                       LPARAM  lParam)
{
    BOOL rc = TRUE;
    CPropertyPage *pPropertyPage = (CPropertyPage *)GetWindowLong(hwnd, DWL_USER);

    switch (msg)
    {
        case WM_INITDIALOG:
            SetWindowLong(hwnd, DWL_USER, lParam);
            pPropertyPage = (CPropertyPage *)lParam;
            return pPropertyPage->OnInitDialog(hwnd);
            break;

#if NEVER
        case WM_CLOSE:
            pPropertyPage->Close();
            break;

        case WM_NOTIFY:
            pPropertyPage->OnNotify((int)wParam, (LPNMHDR)lParam);
            break;

        case WM_COMMAND:
            pPropertyPage->OnCommand(GET_WM_COMMAND_CMD(wParam, lParam),
                                     GET_WM_COMMAND_ID(wParam, lParam), 
                                     GET_WM_COMMAND_HWND(wParam, lParam));
            break;
#endif

        default:
            rc = pPropertyPage ? pPropertyPage->DialogProc(hwnd, msg, wParam, lParam) : FALSE;
            break;
    }

    return rc;
}






//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::MakeDirty
//
//  Synopsis:   marks a page as dirty.
//
//-------------------------------------------------------------------------

void
CPropertyPage::MakeDirty(void)
{
    if ( ! _fActivating )
    {
        _fDirty = TRUE;
        if (_pPropertyPageSite)
            _pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY|PROPPAGESTATUS_VALIDATE);
    }
}





//+---------------------------------------------------------------------------
//
//  Member:     CPropertyPage::ConnectObjects
//
//  Synopsis:   Connection to current selected objectes
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CPropertyPage::ConnectObjects(void)
{
    HRESULT         hr;
    int             i;
    IUnknown **     ppUnk = NULL;
    DWORD *         pdw;

    Disconnect();   //  the old ones

    // Get space for connection IDs.
    hr = _aryAdvise.EnsureSize(_aryUnk.Size());
    if (hr)
        goto Cleanup;

    // Register as sink to each object.
    ppUnk = _aryUnk;
    pdw = _aryAdvise;

    for (i = 0, ppUnk = _aryUnk;
         i < _aryUnk.Size();
         i++, ppUnk++)
    {
        //  Ignore errors
        IGNORE_HR(ConnectSink(*ppUnk, IID_IPropertyNotifySink, &_PropertyPagePNS, pdw));

        // Next object.
        pdw++;
    }

    //  Set the correct count on the cookie array
    _aryAdvise.SetSize(_aryUnk.Size());

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPropertyPage::Disconnect
//
//  Synopsis:   Disconnect to current selected objected
//
//-------------------------------------------------------------------------
void
CPropertyPage::Disconnect(void)
{
    HRESULT         hr = S_OK;
    int             i;
    IUnknown **     ppUnk = NULL;
    DWORD *         pdw;

    //
    // Revoke as property notify sink from each object.
    //

    for (i = 0, ppUnk = _aryUnk, pdw = _aryAdvise;
         i < min ( _aryUnk.Size(), _aryAdvise.Size() );
         i++, ppUnk++)
    {
        IGNORE_HR(DisconnectSink(*ppUnk, IID_IPropertyNotifySink, pdw));

        // Next object.
        pdw++;
    }

    _aryAdvise.SetSize(0);
}




//+------------------------------------------------------------------------
//
//  CPropertyPagePNS implementation.
//
//-------------------------------------------------------------------------


IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CPropertyPage::CPropertyPagePNS, CPropertyPage, _PropertyPagePNS)


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyPage::CPropertyPagePNS::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::CPropertyPagePNS::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyNotifySink))
    {
        *ppv = (IPropertyNotifySink *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyPage::CPropertyPagePNS::OnChanged
//
//  Synopsis:   Refreshes property page.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::CPropertyPagePNS::OnChanged(DISPID dispid)
{
    if (MyCPropertyPage()->_fInApply)
        return S_OK;

    //  BUGBUG: Need to implement this functionality

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyPage::CPropertyPagePNS::OnRequestEdit
//
//  Synopsis:   Does nothing.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPropertyPage::CPropertyPagePNS::OnRequestEdit(DISPID dispid)
{
    return S_OK;
}



