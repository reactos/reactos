//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dlgsite.cxx
//
//  Contents:   Implementation of the site for hosting html dialogs
//
//  History:    06-14-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#define SID_SOmWindow IID_IHTMLWindow2

// BUGBUG need to move the CMDID_SCRIPTSITE_HTMLDLGTRUST definition from formkrnl.hxx
// to a header file visible here or include formkrnl.hxx here
#define CMDID_SCRIPTSITE_HTMLDLGTRUST 1

DeclareTag(tagHTMLDlgSiteMethods, "HTML Dialog Site", "Methods on the html dialog site")

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTMLDlgSite, CHTMLDlg, HTMLDlg, _Site);

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IOleClientSite ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleClientSite *) this;
    }
    else if (iid == IID_IOleInPlaceSite ||
             iid == IID_IOleWindow)
    {
        *ppv = (IOleInPlaceSite *) this;
    }
    else if (iid == IID_IOleControlSite)
    {
        *ppv = (IOleControlSite *) this;
    }
    else if (iid == IID_IDispatch)
    {
        *ppv = (IDispatch *) this;
    }
    else if (iid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }
    else if (iid == IID_ITargetFrame)
    {
        *ppv = (ITargetFrame *)this;
    }
    else if (iid == IID_ITargetFrame2)
    {
        *ppv = (ITargetFrame2 *)this;
    }
    else if (iid == IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *)this;
    }
    else if (iid == IID_IOleCommandTarget)
    {
        *ppv = (IOleCommandTarget *)this;
    }
    else if (iid == IID_IInternetSecurityManager)
    {
        *ppv = (IInternetSecurityManager *) this ;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::SaveObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::SaveObject()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::SaveObject"));
    
    // BUGBUG: (anandra) Possibly call the apply method?
    return S_OK;    // Fail silently
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::GetMoniker
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetMoniker(
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER * ppmk)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::GetMoniker"));

    //lookatme
    *ppmk = NULL;
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::GetContainer
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetContainer(LPOLECONTAINER * ppContainer)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::GetContainer"));
    
    *ppContainer = NULL;
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::ShowObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::ShowObject()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::ShowObject"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnShowWindow
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnShowWindow(BOOL fShow)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::OnShowWindow"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::RequestNewObjectLayout
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::RequestNewObjectLayout( )
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleClientSite::RequestNewObjectLayout"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetWindow(HWND * phwnd)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleWindow::GetWindow"));

    *phwnd = HTMLDlg()->_hwnd;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleWindow::ContextSensitiveHelp"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::CanInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::CanInPlaceActivate()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::CanInPlaceActivate"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnInPlaceActivate()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::OnInPlaceActivate"));
    
    HRESULT     hr;

    hr = THR(HTMLDlg()->_pOleObj->QueryInterface(
            IID_IOleInPlaceObject,
            (void **) &HTMLDlg()->_pInPlaceObj));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnUIActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnUIActivate( )
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::OnUIActivate"));
    
    // Clean up any of our ui.
    IGNORE_HR(HTMLDlg()->_Frame.SetMenu(NULL, NULL, NULL));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::GetWindowContext
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetWindowContext(
        LPOLEINPLACEFRAME  *    ppFrame,
        LPOLEINPLACEUIWINDOW  * ppDoc,
        LPOLERECT               prcPosRect,
        LPOLERECT               prcClipRect,
        LPOLEINPLACEFRAMEINFO   pFI)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::GetWindowContext"));
    
    *ppFrame = &HTMLDlg()->_Frame;
    (*ppFrame)->AddRef();

    *ppDoc = NULL;

#ifndef WIN16
    HTMLDlg()->GetViewRect(prcPosRect);
#else
    RECTL rcView;
    HTMLDlg()->GetViewRect(&rcView);
    CopyRect(prcPosRect, &rcView);
#endif
    *prcClipRect = *prcPosRect;

    pFI->fMDIApp = FALSE;
    pFI->hwndFrame = HTMLDlg()->_hwnd;
    pFI->haccel = NULL;
    pFI->cAccelEntries = 0;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::Scroll
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::Scroll(OLESIZE scrollExtent)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::Scroll"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnUIDeactivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnUIDeactivate(BOOL fUndoable)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::OnUIDeactivate"));
    
    // Set focus back to the frame.
    SetFocus(HTMLDlg()->_hwnd);

    // Clean up any of our ui.
    IGNORE_HR(HTMLDlg()->_Frame.SetMenu(NULL, NULL, NULL));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnInPlaceDeactivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnInPlaceDeactivate( )
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::OnInPlaceDeactivate"));
    
    ClearInterface(&HTMLDlg()->_pInPlaceObj);
    ClearInterface(&HTMLDlg()->_pInPlaceActiveObj);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::DiscardUndoState
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::DiscardUndoState( )
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::DiscardUndoState"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::DeactivateAndUndo
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::DeactivateAndUndo( )
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::DeactivateAndUndo"));
    
    RRETURN(THR(HTMLDlg()->_pInPlaceObj->UIDeactivate()));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnPosRectChange
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnPosRectChange(LPCOLERECT prcPosRect)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleInPlaceSite::OnPosRectChange"));
    
    Assert(FALSE);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnControlInfoChanged
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnControlInfoChanged()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::OnControlInfoChanged"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::LockInPlaceActive
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::LockInPlaceActive(BOOL fLock)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::LockInPlaceActive"));
    
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::GetExtendedControl
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetExtendedControl(IDispatch **ppDispCtrl)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::GetExtendedControl"));
    
    RRETURN(HTMLDlg()->QueryInterface(IID_IDispatch, (void **)ppDispCtrl));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::TransformCoords
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::TransformCoords(
    POINTL* pptlHimetric,
    POINTF* pptfContainer,
    DWORD dwFlags)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::TransformCoords"));
    
    //lookatme
    // This tells the object that we deal entirely with himetric
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::TranslateAccelerator
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::TranslateAccelerator(LPMSG lpmsg, DWORD grfModifiers)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::TranslateAccelerator"));

    HRESULT hr;
    
    //
    // Pass this on up to the property frame thru the page site, if one
    // exists.
    //

    if (HTMLDlg()->_pPageSite)
    {
        hr = THR(HTMLDlg()->_pPageSite->TranslateAccelerator(lpmsg));
    }
    else
    {
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::OnFocus
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::OnFocus(BOOL fGotFocus)
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::OnFocus"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLDlgSite::ShowPropertyFrame
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::ShowPropertyFrame()
{
    TraceTag((tagHTMLDlgSiteMethods, "IOleControlSite::ShowPropertyFrame"));
    
    // To disallow control showing prop-pages
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetTypeInfo
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** ppTypeInfo)
{
    TraceTag((tagHTMLDlgSiteMethods, "IDispatch::GetTypeInfo"));
    
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetTypeInfoCount
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetTypeInfoCount(UINT * pctinfo)
{
    TraceTag((tagHTMLDlgSiteMethods, "IDispatch::GetTypeInfoCount"));
    
    *pctinfo = 0;
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetIDsOfNames
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetIDsOfNames(
    REFIID riid, 
    LPOLESTR * rgszNames, 
    UINT cNames, 
    LCID lcid, 
    DISPID * rgdispid)
{
    TraceTag((tagHTMLDlgSiteMethods, "IDispatch::GetIDsOfNames"));
    
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::Invoke
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::Invoke(
    DISPID dispidMember, 
    REFIID riid, 
    LCID lcid, 
    WORD wFlags,
    DISPPARAMS * pdispparams, 
    VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo, 
    UINT * puArgErr)
{
    TraceTag((tagHTMLDlgSiteMethods, "IDispatch::Invoke"));
    
    HRESULT     hr = DISP_E_MEMBERNOTFOUND;

    if (wFlags & DISPATCH_PROPERTYGET)
    {
        if (!pvarResult)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        VariantInit(pvarResult);

        switch (dispidMember)
        {
        case DISPID_AMBIENT_SHOWHATCHING:
        case DISPID_AMBIENT_SHOWGRABHANDLES:
            //
            // We don't want the ui-active control to show standard ole
            // hatching or draw grab handles.
            //

            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = (VARIANT_BOOL)0;
            hr = S_OK;
            break;
            
        case DISPID_AMBIENT_LOCALEID:
            hr = S_OK;
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = HTMLDlg()->_lcid;
            break;
        }
    }
    
Cleanup:    
    return hr;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::QueryService
//
//  Synopsis:   per IServiceProvider
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::QueryService(REFGUID sid, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;

    if (NULL != HTMLDlg()->_pHostServiceProvider)
    {
        hr = HTMLDlg()->_pHostServiceProvider->QueryService(sid, iid, ppv);
        
        if (!hr)
        {
            return hr;
        }
    }

    if (sid == IID_ITargetFrame || sid == IID_ITargetFrame2)
    {
        hr = THR_NOTRACE(QueryInterface(iid, ppv));
    }
    else if (sid == IID_IHTMLDialog)
    {
        hr = THR(HTMLDlg()->QueryInterface(iid, ppv));
    }
    else if ( sid == IID_IInternetSecurityManager)
    {
        //
        // marka - if we are in a trusted Dialog
        // We then provide our own custom security manager (that allows everything)
        // otherwise - we don't provide this service. 
        //
        if ( HTMLDlg()->_fTrusted )
            hr = THR_NOTRACE( QueryInterface( iid, ppv ));
        else
        {    
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }                
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetFrameOptions
//
//  Synopsis:   per ITargetFrame
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetFrameOptions(DWORD *pdwFlags)
{
    *pdwFlags = HTMLDlg()->_dwFrameOptions;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::QueryStatus
//
//  Synopsis:   per IOleCommandTarget
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::QueryStatus(
                const GUID * pguidCmdGroup,
                ULONG cCmds,
                MSOCMD rgCmds[],
                MSOCMDTEXT * pcmdtext)
{
    return OLECMDERR_E_UNKNOWNGROUP ;
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::Exec
//
//  Synopsis:   per IOleCommandTarget
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::Exec(
                const GUID * pguidCmdGroup,
                DWORD nCmdID,
                DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut)
{
    HRESULT  hr = S_OK;

    if(!pguidCmdGroup)
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
        goto Cleanup;
    }


    if(CLSID_HTMLDialog == *pguidCmdGroup)
    {
        // we should only get this message
        // when onload is fired.  It uses nCmdID == 0
        // so any other command is an error.
        Assert(nCmdID == 0);

        if(HTMLDlg()->_fAutoExit)
        {
            HTMLDlg()->close();
        }
        else
        {
            hr = OLECMDERR_E_NOTSUPPORTED;
        }
    }
    else if (CGID_ScriptSite == *pguidCmdGroup)
    {
        switch (nCmdID)
        {
        case CMDID_SCRIPTSITE_HTMLDLGTRUST:
            if (!pvarargOut)
            {
                hr = E_POINTER;
                goto Cleanup;
            }
    
            V_VT(pvarargOut) = VT_BOOL;
            V_BOOL(pvarargOut) = HTMLDlg()->_fTrusted;

            break;

        default:
            hr = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else
        hr = OLECMDERR_E_UNKNOWNGROUP;


Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetHostInfo
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetHostInfo(DOCHOSTUIINFO * pInfo)
{
    Assert(pInfo);

    if (pInfo->cbSize < sizeof(DOCHOSTUIINFO)) 
        return E_INVALIDARG;

    pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetExternal
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::GetExternal(IDispatch **ppDisp)
{
    HRESULT hr;

    if (!ppDisp)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = THR(HTMLDlg()->QueryInterface(IID_IDispatch, (void **)ppDisp));
    }

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::TranslateUrl
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::TranslateUrl(
    DWORD dwTranslate, 
    OLECHAR *pchURLIn, 
    OLECHAR **ppchURLOut)
{
    HRESULT hr;

    if (!ppchURLOut)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppchURLOut = NULL;
        hr = S_OK;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::FilterDataObject
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLDlgSite::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    HRESULT     hr;

    if (!ppDORet)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppDORet = NULL;
        hr = S_OK;
    }
   
    RRETURN(hr);
}


//+=======================================================================
//
//
// IInternetSecurityManager Methods
//
// marka - CHTMLDlgSite now implements an IInternetSecurityManager
// this is to BYPASS the normal Security settings for a trusted HTML Dialog in a "clean" way
//
// QueryService - for IID_IInternetSecurity Manager - will return that interface
// if we are a _trusted Dialog. Otherwise we fail this QueryService and use the "normal"
// security manager.
//
// RATIONALE:
//
//    - Moving dialog code to mshtmled.dll requires implementing COptionsHolder as an
//      embedded object. Hence if we were honoring the user's Security Settings - if they had 
//    ActiveX turned off - they could potentially break dialogs.
//
//  - If you're in a "trusted" HTML dialog (ie invoked via C-code), your potential to do
//      anything "unsafe" is infinite - so why not allow all security actions ?
//
//
//
//==========================================================================


//+------------------------------------------------------------------------
//
//  Member:     CHtmlDlgSite::SetSecuritySite
//
//    Implementation of IInternetSecurityManager::SetSecuritySite
//
//  Synopsis:   Sets the Security Site Manager
//
//
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::SetSecuritySite( IInternetSecurityMgrSite *pSite )
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlDlgSite::GetSecuritySite
//
//    Implementation of IInternetSecurityManager::GetSecuritySite
//
//  Synopsis:   Returns the Security Site Manager
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetSecuritySite( IInternetSecurityMgrSite **ppSite )
{
    *ppSite = NULL;
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlDlgSite::MapURLToZone
//
//    Implementation of IInternetSecurityManager::MapURLToZone
//
//  Synopsis:   Returns the ZoneIndex for a given URL
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::MapUrlToZone(
                        LPCWSTR     pwszUrl,
                        DWORD*      pdwZone,
                        DWORD       dwFlags
                    )
{
    return INET_E_DEFAULT_ACTION;
}
                    
//+------------------------------------------------------------------------
//
//  Member:     CHtmlDlgSite::ProcessURLAction
//
//    Implementation of IInternetSecurityManager::ProcessURLAction
//
//  Synopsis:   Query the security manager for a given action
//              and return the response.  This is basically a true/false
//              or allow/disallow return value.
//
//                (marka) We assume that for HTML Dialogs - a Security Manager is *ONLY*
//                created if we are in a Trusted Dialog ( see QueryService above)
//
//                Hence we then allow *ALL* actions
//
//
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::ProcessUrlAction(
                        LPCWSTR     pwszUrl,
                        DWORD       dwAction,
                        BYTE*   pPolicy,    // output buffer pointer
                        DWORD   cbPolicy,   // output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwFlags,    // See enum PUAF for details.
                        DWORD   dwReserved)
{
    if (cbPolicy == sizeof(DWORD))
        *(DWORD*)pPolicy = URLPOLICY_ALLOW;
    else if (cbPolicy == sizeof(WORD))
        *(WORD*)pPolicy = URLPOLICY_ALLOW;
    else // BYTE or unknown type
        *pPolicy = URLPOLICY_ALLOW;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetSecurityID
//
//    Implementation of IInternetSecurityManager::GetSecurityID
//
//  Synopsis:   Retrieves the Security Identification of the given URL
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetSecurityId( 
            LPCWSTR pwszUrl,
            BYTE __RPC_FAR *pbSecurityId,
            DWORD __RPC_FAR *pcbSecurityId,
            DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::SetZoneMapping
//
//    Implementation of IInternetSecurityManager::SetZoneMapping
//
//  Synopsis:   Sets the mapping of a Zone
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::SetZoneMapping  (
                    DWORD   dwZone,        // absolute zone index
                    LPCWSTR lpszPattern,   // URL pattern with limited wildcarding
                    DWORD   dwFlags       // add, change, delete
)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::QueryCustomPolicy
//
//    Implementation of IInternetSecurityManager::QueryCustomPolicy
//
//  Synopsis:   Retrieves the custom policy associated with the URL and
//                specified key in the given context
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::QueryCustomPolicy (
                        LPCWSTR     pwszUrl,
                        REFGUID     guidKey,
                        BYTE**  ppPolicy,   // pointer to output buffer pointer
                        DWORD*  pcbPolicy,  // pointer to output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwReserved )
 {
    return INET_E_DEFAULT_ACTION;
 }
                        
//+------------------------------------------------------------------------
//
//  Member:     CHTMLDlgSite::GetZoneMappings
//
//    Implementation of IInternetSecurityManager::GetZoneMapping
//
//  Synopsis:   Sets the mapping of a Zone
//
//-------------------------------------------------------------------------

HRESULT
CHTMLDlgSite::GetZoneMappings (
                    DWORD   dwZone,        // absolute zone index
                    IEnumString  **ppenumString,   // output buffer size
                    DWORD   dwFlags        // reserved, pass 0
)
{
    return INET_E_DEFAULT_ACTION;
}
