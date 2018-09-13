//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       client.cxx
//
//  Contents:   implementation of OLE site.
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

#ifndef X_MSHTMCID_H_
#define X_MSHTMCID_H_
#include "mshtmcid.h"
#endif

#define SID_SElementBehaviorFactory IID_IElementBehaviorFactory

//+-------------------------------------------------------------------------
//
//  Method:     CClient::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

HRESULT CClient::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IOleClientSite)
        *ppv = (IOleClientSite *)this;
    else if (riid == IID_IOleCommandTarget)
        *ppv = (IOleCommandTarget *) this;
    else if (riid == IID_IOleWindow)
        *ppv = (IOleWindow *) this;
    else if (riid == IID_IOleInPlaceSite)
        *ppv = (IOleInPlaceSite *) this;
    else if (riid == IID_IOleDocumentSite)
        *ppv = (IOleDocumentSite *) this;
    else if (riid == IID_IAdviseSink)
        *ppv = (IAdviseSink *) this;
    else if (riid == IID_IServiceProvider)
        *ppv = (IServiceProvider *) this;
    else if (riid == IID_IInternetSecurityManager)
        *ppv = (IInternetSecurityManager *) this ;
    else if (riid == IID_IDocHostUIHandler)
        *ppv = (IDocHostUIHandler *) this ;
    else if (riid == IID_IDocHostShowUI)
        *ppv = (IDocHostShowUI *) this ;
    else if (riid == IID_IHTMLOMWindowServices)
        *ppv = (IHTMLOMWindowServices *) this;
    
    if (*ppv)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


/* IOleClientSite methods */

//+-------------------------------------------------------------------------
//
//  Method:     CClient::SaveObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::SaveObject()
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::GetMoniker
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR * ppmk)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::GetContainer
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::GetContainer(LPOLECONTAINER FAR * ppContainer)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::ShowObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::ShowObject()
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnShowWindow
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::RequestNewObjectLayout
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

HRESULT CClient::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}


HRESULT CClient::QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds,
        MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext)
{
    if (pguidCmdGroup != NULL)
        return (OLECMDERR_E_UNKNOWNGROUP);

    MSOCMD *    pCmd;
    INT         c;
    HRESULT     hr = S_OK;

    // Loop through each command in the ary, setting the status of each.
    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        switch (pCmd->cmdID)
        {
        case OLECMDID_ALLOWUILESSSAVEAS:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;
        }
    }

    RRETURN(hr);
}

HRESULT CClient::Exec(const GUID * pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut)
{
    HRESULT hr = S_OK;

    if ( ! pguidCmdGroup )
    {
        switch (nCmdID)
        {
        case OLECMDID_SETTITLE:
            if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
            {
                App()->SetTitle(V_BSTR(pvarargIn));
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }
            break;

        case OLECMDID_CLOSE:
            App()->Close();
            break;
            
        default:
            hr = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else if (pguidCmdGroup && IsEqualGUID(CGID_MSHTML,*pguidCmdGroup ))
    {
        switch (nCmdID)
        {
            case IDM_PARSECOMPLETE:

                // Document is fully parsed.  Create the application window.
                App()->CreateAppWindow();
                break;
                
            default:
                hr = OLECMDERR_E_NOTSUPPORTED;
                break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }

    RRETURN(hr);
}


/* IOleWindow methods */

//+-------------------------------------------------------------------------
//
//  Method:     CClient::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

HRESULT CClient::GetWindow(HWND FAR *lpHwnd)
{
    HRESULT hr = S_OK;
    
    if (!lpHwnd)
        return E_POINTER;
    
    *lpHwnd = App()->_hwnd;
    
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

HRESULT CClient::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}


/* IOleInPlaceSite methods */

//+-------------------------------------------------------------------------
//
//  Method:     CClient::CanInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::CanInPlaceActivate()
{
    HRESULT hr = S_OK;
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnInPlaceActivate()
{
    HRESULT hr = S_OK;
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnUIActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnUIActivate()
{
    HRESULT hr = S_OK;
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::GetWindowContext
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::GetWindowContext(
                    LPOLEINPLACEFRAME FAR* lplpFrame,
                    LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                    LPOLERECT lprcPosRect,
                    LPOLERECT lprcClipRect,
                    LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    HRESULT hr = S_OK;

    CHTMLAppFrame *pFrame = Frame();
    Assert(pFrame);
    
    *lplpFrame = (LPOLEINPLACEFRAME)pFrame;
    *lplpDoc = (LPOLEINPLACEUIWINDOW)pFrame;

    App()->GetViewRect(lprcPosRect);
    *lprcClipRect = *lprcPosRect;

    // Nothing much interesting in this struct.  We don't have any
    // menus or accelerators.
    Assert(lpFrameInfo->cb >= sizeof(OLEINPLACEFRAMEINFO));
    lpFrameInfo->fMDIApp = FALSE ;
    lpFrameInfo->hwndFrame = App()->_hwnd ;
    lpFrameInfo->haccel = 0 ;
    lpFrameInfo->cAccelEntries = 0 ;

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::Scroll
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::Scroll(OLESIZE scrollExtent)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnUIActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnUIDeactivate(BOOL fUndoable)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnInPlaceDeactivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnInPlaceDeactivate()
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::DiscardUndoState
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::DiscardUndoState()
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::DeactivateAndUndo
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::DeactivateAndUndo()
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnPosRectChange
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

HRESULT CClient::OnPosRectChange(LPCOLERECT lprcPosRect)
{
    return E_NOTIMPL;
}

                    
/* IOleDocumentSite methods */

//+-------------------------------------------------------------------------
//
//  Method:     CClient::ActivateMe
//
//  Synopsis:   Per IOleDocumentSite
//
//--------------------------------------------------------------------------

HRESULT CClient::ActivateMe(IOleDocumentView * pViewToActivate)
{
    HRESULT hr = S_OK;
    
    // BUGBUG if pViewToActivate is null, call CreateView.
    ReleaseInterface(_pView);

    _pView = pViewToActivate;
    if (_pView)
    {
        // We're going to hold on to this, so AddRef it.
        _pView->AddRef();

        hr = _pView->SetInPlaceSite(this);
        TEST(hr);
        
        hr = _pView->UIActivate(TRUE);
        TEST(hr);
        
        hr = _pView->Show(TRUE);
        TEST(hr);
    }
    
Cleanup:

    RRETURN(hr);
}


/* IAdviseSink methods */

//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnDataChange
//
//  Synopsis:   Per IAdviseSink
//
//--------------------------------------------------------------------------

void CClient::OnDataChange(FORMATETC * pFormatetc, STGMEDIUM * pmedium)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnViewChange
//
//  Synopsis:   Per IAdviseSink
//
//--------------------------------------------------------------------------

void CClient::OnViewChange(DWORD dwAspect, long lindex)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnRename
//
//  Synopsis:   Per IAdviseSink
//
//--------------------------------------------------------------------------

void CClient::OnRename(LPMONIKER pmk)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnSave
//
//  Synopsis:   Per IAdviseSink
//
//--------------------------------------------------------------------------

void CClient::OnSave()
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::OnClose
//
//  Synopsis:   Per IAdviseSink
//
//--------------------------------------------------------------------------

void CClient::OnClose()
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CClient::QueryService
//
//  Synopsis:   Per IServiceProvider
//
//--------------------------------------------------------------------------

HRESULT CClient::QueryService(REFGUID guidService, REFIID riid, void ** ppv)
{
    HRESULT hr = E_FAIL;

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;
    
    if (IsEqualGUID(guidService, SID_SElementBehaviorFactory))
    {
        *ppv = &App()->_PeerFactory;
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }
    else if (IsEqualGUID(guidService, IID_IInternetSecurityManager) ||
             IsEqualGUID(guidService, IID_IHTMLOMWindowServices))
    {
        hr = QueryInterface(riid, ppv);
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::SetSecuritySite
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::SetSecuritySite(IInternetSecurityMgrSite *pSite)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::GetSecuritySite
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::GetSecuritySite(IInternetSecurityMgrSite **ppSite)
{
    *ppSite = NULL;
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::MapURLToZone
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::MapUrlToZone(LPCWSTR pwszUrl, DWORD* pdwZone, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}
                    
//+------------------------------------------------------------------------
//
//  Member:     CClient::ProcessURLAction
//
//  Synopsis:   Per IInternetSecurityManager.  Return URLPOLICY_ALLOW for
//              most actions if this comes from a trusted source.  Security is 
//              disabled in this context.
//
//-------------------------------------------------------------------------

HRESULT
CClient::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy,
                          BYTE* pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    DWORD dwPolicy = URLPOLICY_ALLOW;
    HRESULT hr = S_OK;
    
    // If this is not coming from a trusted source, do default.
    if (!(dwFlags & PUAF_TRUSTED))
    {
        return INET_E_DEFAULT_ACTION;
    }
    
    // Policies are DWORD values, so we need at least 4 bytes.
    if (cbPolicy < 4)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    switch(dwAction)
    {
        case URLACTION_CREDENTIALS_USE:
            dwPolicy = URLPOLICY_CREDENTIALS_SILENT_LOGON_OK;
            break;
        case URLACTION_AUTHENTICATE_CLIENT:
            dwPolicy = URLPOLICY_AUTHENTICATE_CLEARTEXT_OK;
            break;
        case URLACTION_JAVA_PERMISSIONS:
            dwPolicy = URLPOLICY_JAVA_LOW;
            break;
        case URLACTION_CHANNEL_SOFTDIST_PERMISSIONS:
            dwPolicy = URLPOLICY_CHANNEL_SOFTDIST_AUTOINSTALL;
            break;
    }

    if (dwPolicy != URLPOLICY_ALLOW)
        hr = S_FALSE;
        
    *pPolicy = dwPolicy;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::GetSecurityID
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::GetSecurityId(LPCWSTR pwszUrl, BYTE __RPC_FAR *pbSecurityId,
                       DWORD __RPC_FAR *pcbSecurityId, DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::SetZoneMapping
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::SetZoneMapping  (DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::QueryCustomPolicy
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::QueryCustomPolicy (LPCWSTR pwszUrl, REFGUID guidKey, BYTE** ppPolicy,
                            DWORD* pcbPolicy, BYTE* pContext, DWORD cbContext,
                            DWORD dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}
                        
//+------------------------------------------------------------------------
//
//  Member:     CClient::GetZoneMappings
//
//  Synopsis:   Per IInternetSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CClient::GetZoneMappings (DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT
CClient::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    HRESULT hr = S_OK;

    Assert(pInfo);
    if (pInfo->cbSize < sizeof(DOCHOSTUIINFO))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pInfo->dwFlags = 0;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    pInfo->pchHostCss = (OLECHAR *)CoTaskMemAlloc(sizeof(SZ_APPLICATION_BEHAVIORCSS)+1);
    if ( !pInfo->pchHostCss )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pInfo->pchHostNS = (OLECHAR *)CoTaskMemAlloc(sizeof(SZ_APPLICATION_BEHAVIORNAMESPACE)+1);
    if ( !pInfo->pchHostNS )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    StrCpyW(pInfo->pchHostCss, SZ_APPLICATION_BEHAVIORCSS);
    StrCpyW(pInfo->pchHostNS, SZ_APPLICATION_BEHAVIORNAMESPACE);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::ShowContextMenu
//
//  Synopsis:   Called by Trident when its about to show its context menu.
//
//  Returns:    S_OK to disable context menu
//              S_FALSE to allow Trident to show its context menu
//
//-------------------------------------------------------------------------
HRESULT
CClient::ShowContextMenu(DWORD dwID, POINT * pptPosition,
                         IUnknown * pcmdtReserved,
                         IDispatch * pDispatchObjectHit)
{
    if (!theApp.contextMenu())
       return S_OK;

    return S_FALSE;
}

HRESULT
CClient::GetExternal(IDispatch ** ppDisp)
{
    if (!ppDisp)
        return E_POINTER;

    *ppDisp = NULL;
    return S_OK;
}

HRESULT
CClient::ShowMessage(HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption,
            DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT *plResult)
{
    TCHAR szBuf[512];
    int iReturn = GetWindowText(App()->_hwnd, szBuf, ARRAY_SIZE(szBuf));

    // GetWindowText will return zero if there is no window text, or upon failure.  Rather
    // than have a blank title for this message box, default to "HTML Application".
    
    if (iReturn)
        iReturn = MessageBox(hwnd, lpstrText, szBuf, dwType);
    else
        iReturn = MessageBox(hwnd, lpstrText, _T("HTML Application"), dwType);

    if (plResult)
        *plResult = iReturn;
        
    return (iReturn ? S_OK : S_FALSE);
}


//==================================================================
//  method implementatiosn for IHTMLOMWindowServices
//
//  this interface is used by the hta to implement the window sevices
//  that COmWindow delagates to. e.g .moveTo, resizeby
//
//==================================================================


//+-----------------------------------------------------
//      per IHTMLOMWindowServices
//-------------------------------------------------------
HRESULT
CClient::moveTo(long x, long y)
{
    HRESULT  hr;
    RECT     rect;
    HWND     hwnd= App()->_hwnd;
    
    // If the main window hasn't been created yet - remember the new position
    // and apply it at window creation time.
    if (!IsWindow(hwnd))
    {
        hr = App()->Pending_moveTo(x,y);
        goto Cleanup;
    }

    if (!GetWindowRect(hwnd, &rect))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // adjust to the requested position
    rect.bottom = rect.bottom-rect.top + y;
    rect.right  = rect.right-rect.left + x;
    rect.top = y;
    rect.left = x;

    // verify that the window is still visible.  This uses
    // the same helper as the dialogs and is necessary for 
    // security reasons.

    // but HTA's shouldn't have to do this.
    //VerifyWindowRect(&rect, hwnd, 0, 0);

    // now we've the go-ahead to actually set the position
    hr = (SetWindowPos(hwnd,
                 NULL, 
                 rect.left, 
                 rect.top, 
                 rect.right-rect.left, 
                 rect.bottom-rect.top, 
                 SWP_NOZORDER | SWP_SHOWWINDOW )) ? 
            S_OK : E_FAIL;

Cleanup:
    return hr;
}

//+-----------------------------------------------------
//      per IHTMLOMWindowServices
//-------------------------------------------------------
HRESULT
CClient::moveBy(long x, long y)
{
    HRESULT  hr;
    RECT     rect;
    HWND     hwnd= App()->_hwnd;

    // If the main window hasn't been created yet - remember the new relative 
    // position and apply it at window creation time.
    if (!IsWindow(hwnd))
    {
        hr = App()->Pending_moveBy(x,y);
        goto Cleanup;
    }

    // If the window has been created, but we can't get the rect, something
    // is badly wrong.  Fail this call.
    if (!GetWindowRect(hwnd, &rect))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // adjust to the requested position
    rect.bottom += y;
    rect.right  += x;
    rect.top += y;
    rect.left += x;

    // verify that the window is still visible.  This uses
    // the same helper as the dialogs and is necessary for 
    // security reasons.

    // but HTA's shouldn't have to do this.
    // VerifyWindowRect(&rect, hwnd, 0, 0);

    // do we actually need to even move?
    hr = (SetWindowPos(hwnd,
                 NULL, 
                 rect.left, 
                 rect.top, 
                 rect.right-rect.left, 
                 rect.bottom-rect.top, 
                 SWP_NOZORDER | SWP_SHOWWINDOW )) ? 
            S_OK : E_FAIL;

Cleanup:
    return hr;
}

//+-----------------------------------------------------
//      per IHTMLOMWindowServices
//-------------------------------------------------------
HRESULT
CClient::resizeTo(long x, long y)
{
    HRESULT  hr;
    RECT     rect;
    HWND     hwnd= App()->_hwnd;

    // If the main window hasn't been created yet - remember the new size
    // and apply it at window creation time.
    if (!IsWindow(hwnd))
    {
        hr = App()->Pending_resizeTo(x,y);
        goto Cleanup;
    }

    // get the current window rect
    if (!GetWindowRect(hwnd, &rect))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // adjust to the requested Size 
    rect.bottom = rect.top + y;
    rect.right  = rect.left + x;

    // verify that the window is still visible.  This uses
    // the same helper as the dialogs and is necessary for 
    // security reasons.

    // but HTA's shouldn't have to do this.
    // VerifyWindowRect(&rect, hwnd, 0, 0);


    // set our new (secure) size
    hr = (SetWindowPos(hwnd,
                 NULL, 
                 rect.left, 
                 rect.top, 
                 rect.right-rect.left, 
                 rect.bottom-rect.top, 
                 SWP_NOZORDER | SWP_SHOWWINDOW )) ? 
            S_OK : E_FAIL;

Cleanup:
    return hr;
}

//+-----------------------------------------------------
//      per IHTMLOMWindowServices
//-------------------------------------------------------
HRESULT
CClient::resizeBy(long x, long y)
{
    HRESULT hr;
    RECT    rect;
    HWND    hwnd = App()->_hwnd;

    // If the main window hasn't been created yet - remember the new relative size
    // and apply it at window creation time.
    if (!IsWindow(hwnd))
    {
        hr = App()->Pending_resizeBy(x,y);
        goto Cleanup;
    }


    // If the window has been created, but we can't get the rect, something
    // is badly wrong.  Fail this call.
    if (!GetWindowRect(hwnd, &rect))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // adjust to the requested Size 
    rect.bottom += y;
    rect.right  += x;

    // verify that the new sized window is still visible.  This uses
    // the same helper as the dialogs and is necessary for 
    // security reasons.

    // but HTA's shouldn't have to do this.
//    VerifyWindowRect(&rect, hwnd, 0, 0);

    // set our new (secure) size
    hr = (SetWindowPos(hwnd,
                 NULL, 
                 rect.left, 
                 rect.top, 
                 rect.right-rect.left, 
                 rect.bottom-rect.top, 
                 SWP_NOZORDER | SWP_SHOWWINDOW )) ? 
            S_OK : E_FAIL;

Cleanup:
    return hr;
}

