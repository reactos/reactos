//+---------------------------------------------------------------------
//
//   File:      frmsite.cxx
//
//  Contents:   frame site implementation
//
//  Classes:    CFrameSite, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"     // for IWebBrowser
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for ITargetFrame, ITargetEmbedding
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_PSPOOLER_HXX_
#define X_PSPOOLER_HXX_
#include "pspooler.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h>
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#define _cxx_
#include "frmsite.hdl"

////////////////////////////////////////////////////////////////////////////////////////

HRESULT GetSIDOfDispatch(IDispatch *pDisp, BYTE *pbSID, DWORD *pcbSID);


////////////////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_HTML_TEAROFF(this, IHTMLFrameBase, NULL)
    else
    if IID_TEAROFF(this, IDispatchEx, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::CreateObject()
//
//  Synopsis:   Helper to instantiate the web browser ocx
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::CreateObject()
{
    HRESULT             hr = S_OK;
    OLECREATEINFO       info;
    DWORD               dw;
    IPersistHistory *   pPH = NULL;
    IBindCtx *          pbc = NULL;
    IHistoryBindInfo *  phbi = NULL;
    INamedPropertyBag * pINPB = NULL;
    CDoc *              pDoc = Doc();
    IOleObject *        pOleObject = NULL;
    IOleControl *       pOleControl = NULL;
    DWORD               dwMiscStatus = 0;
    CVariant            var;
    
    if (IsOverflowFrame())
        return S_OK;

    pDoc->AddRef();

    hr = THR(pDoc->_clsTab.AssignWclsid(pDoc, CLSID_WebBrowser, &_wclsid));
    if (hr)
        goto Error;

    // If host want to disable download, don't create object
    if (pDoc->_dwLoadf & DLCTL_NO_FRAMEDOWNLOAD)
    {
        _lReadyState = READYSTATE_COMPLETE;
        goto Error;
    }

    // Register for release object notifications from the Doc.
    RegisterForRelease();


    // See if there is an "Application" attribute on this element, and if there is,
    // set a flag that is checked from CDocument::SetClientSite.
    hr = getAttribute(_T("Application"), 0, &var);

    if (SUCCEEDED(hr) && (V_VT(&var) == VT_BSTR))
    {
        if (!StrCmpIC(V_BSTR(&var), _T("Yes")))
            _fTrustedFrame = TRUE;
    }

    //
    // First stage control creation.
    //

    hr = THR_OLE(CoCreateInstance(CLSID_WebBrowser,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUnknown,
                              (void **) &_pUnkCtrl));
    if (hr)
        goto Cleanup;
        
    hr = THR_OLE(QueryControlInterface(
                IID_IOleObject, (void **)&pOleObject));
    if (hr)
        goto Error;
        
    hr = THR_OLE(pOleObject->GetMiscStatus(
            DVASPECT_CONTENT,
            &dwMiscStatus));
    if (hr)
        goto Error;

    SetMiscStatusFlags(dwMiscStatus);
    hr = THR_OLE(SetClientSite(&_Client));
    if (hr)
        goto Error;

    AfterObjectInstantiation();
    
    //
    // Load/InitNew the control
    //

    // BUGBUG (jenlc) IE5 Raid 29992
    //
    if (Tag() == ETAG_IFRAME)
    {
        hr = InitNewObject();
        if (hr)
            goto Error;
    }

    // first make sure that, if there is a history we load it.
    // we need to do this since our call to super:CO is at the
    // end of this function and we need the info earlier.
    if (!_pStreamHistory)
    {
        THR(pDoc->GetLoadHistoryStream(GetSourceIndex(), HistoryCode(), &_pStreamHistory));
    }

    if (_pStreamHistory)
    {
        // Send history info downstream
        if (pDoc->_pDwnDoc)
        {
            hr = THR(CreateBindCtx(NULL, &pbc));
            if (hr)
                goto Error;

            hr = THR(CreateHistoryBindInfo(&phbi));
            if (hr)
                goto Error;

            dw = pDoc->_pDwnDoc->GetBindf();
            hr = THR(phbi->SetOption(HISTORYOPTION_BINDF, &dw, sizeof(dw)));
            if (hr)
                goto Error;

            dw = pDoc->_pDwnDoc->GetRefresh();
            hr = THR(phbi->SetOption(HISTORYOPTION_REFRESH, &dw, sizeof(dw)));
            if (hr)
                goto Error;

            hr = THR(pbc->RegisterObjectParam(WSZ_HISTORYBINDINFO, (IUnknown*)phbi));
            if (hr)
                goto Error;
        }
        
        hr = THR(LoadHistoryStream(_pStreamHistory, pbc));
        
        // Could not load history for some reason... Load normally
        if (hr)
        {
            hr = S_OK;
            goto OrdinaryLoad;
        }
    }
    else
    {
OrdinaryLoad:
        _lReadyState = READYSTATE_UNINITIALIZED;
        IGNORE_HR(OnPropertyChange_Src());
    }

    //
    // Second stage control creation
    //

    hr = THR_OLE(QueryControlInterface(IID_IViewObject, (void **) &_pVO));
    if (hr)
        goto Error;

    {
        IPropertyNotifySink *pPNS = &_Client;

#if DBG==1
        IAdviseSink *pAdviseSink = &_Client;
        pAdviseSink->AddRef();
        DbgTrackItf(IID_IAdviseSink, "CClient", FALSE, (void **)&pAdviseSink);
        pPNS->AddRef();
        DbgTrackItf(IID_IPropertyNotifySink, "cclient", FALSE, (void **)&pPNS);
#endif
        IGNORE_HR(_pVO->SetAdvise(DVASPECT_CONTENT, 0, &_Client));
        hr = THR_OLE(ConnectSink(
                _pUnkCtrl,
                IID_IPropertyNotifySink,
                pPNS,
                &_dwPropertyNotifyCookie));
#if DBG==1
        pAdviseSink->Release();
        pPNS->Release();
#endif
        if (hr)
            goto Error;
    }

    //
    // Turn off events for the duration of creation
    //
    if ( S_OK == QueryControlInterface(IID_IOleControl, (void **) &pOleControl) )
    {
        IGNORE_HR(pOleControl->FreezeEvents(TRUE));

        if (pDoc->_cFreeze)
            IGNORE_HR(pOleControl->FreezeEvents(TRUE));
    }
    
    //
    // Set bit informing world that the control is downloaded and
    // ready.
    //

    _fObjAvailable = TRUE;
    _state = OS_LOADED;
    OnControlReadyStateChanged(FALSE);

    // if we are in the process of restoring a shortcut, then try to get the src
    // for this element.  If it is not there, do the normal load thing.  if it is
    // there, then set the src to that.

    // QFE: The BASEURL could be there, but stale.  This will happen if the top-level page has changed
    // to point the subframe to a different URL.
    // Provide a mechanism to compare and invalidate the persisted URL if this is the case.

    if (pDoc->_pShortcutUserData &&
        !pDoc->_pPrimaryMarkup->MetaPersistEnabled(htmlPersistStateFavorite) )
    {
        hr = THR_NOTRACE(pDoc->_pShortcutUserData->
                        QueryInterface(IID_INamedPropertyBag,
                                      (void**) &pINPB));
        if (!hr)
        {
            PROPVARIANT  varBASEURL = {0};
            PROPVARIANT  varORIGURL = {0};
            BSTR         strName = GetPersistID();
            BOOL         bRestoreFavorite = TRUE;

            // Check the shortcut for a BASEURL

            V_VT(&varBASEURL) = VT_BSTR;
            hr = THR_NOTRACE(pINPB->ReadPropertyNPB(strName, _T("BASEURL"), &varBASEURL));

            if (!hr && V_VT(&varBASEURL) == VT_BSTR)
            {
                // The shortcut has a BASEURL.  Now see if it has an ORIGURL (original URL)

                V_VT(&varORIGURL) = VT_BSTR;
                hr = THR_NOTRACE(pINPB->ReadPropertyNPB(strName, _T("ORIGURL"), &varORIGURL));

                if (!hr && V_VT(&varORIGURL) == VT_BSTR)
                {
                    // The shortcut has an ORIGURL.  Get the URL from the markup, and compare.

                    const TCHAR * pchUrl = GetAAsrc();

                    if (pchUrl && UrlCompare(pchUrl, V_BSTR(&varORIGURL), TRUE) != 0)
                    {
                        // They're different.  Invalidate the BASEURL by not setting the attribute.

                        bRestoreFavorite = FALSE;
                    }

                    SysFreeString(V_BSTR(&varORIGURL));
                    V_BSTR(&varORIGURL) = NULL;
                }
                
                // Restore the saved URL if either (1) there is no ORIGURL, or (2) there is an ORIGURL and it matches
                // the URL currently in the markup.

                if (bRestoreFavorite)
                {
                    hr = THR(SetAAsrc(V_BSTR(&varBASEURL)));
                    if (!hr)
                    {
                        // we're done, We're successful.  Get out of here
                        // and since we navigated due to a shortcut Note the Navigation
                        // so that subsequent saves behave consistently
                        SysFreeString(strName);
                        IGNORE_HR(OnPropertyChange_Src());
                        NoteNavEvent();

                        // Free the BSTRs since we're branching out.
                        if (V_BSTR(&varORIGURL))
                            SysFreeString(V_BSTR(&varORIGURL));
                        if (V_BSTR(&varBASEURL))
                            SysFreeString(V_BSTR(&varBASEURL));
                        goto Cleanup;
                    }
                }

                // for any other reason/failure fall through and be normal

                SysFreeString(V_BSTR(&varBASEURL));
            }
            SysFreeString(strName);
        }

        hr = S_OK;
    }

    // BUGBUG (to fix 25644): The correct call is just ResizeElement(),
    // but since framesets do not listen to text change notifications, our owner
    // won't hear about our sudden appearance, and won't recalc us when we need
    // to be recalced. So here we circumvent ResizeElement's _fSizeThis optimization
    // which tries to avoid an initial recalc on creation, and we send a notification
    // requesting the recalc on creation, no matter what.
    // Framesets should really probably listen to change notifications instead. (dbau)
    
    SendNotification(NTYPE_ELEMENT_RESIZE);

    if (pDoc->State() >= OS_INPLACE)
    {
        IGNORE_HR(TransitionToBaselineState(pDoc->State()));
    }

Cleanup:
    pDoc->Release();
    ReleaseInterface(pINPB);
    ReleaseInterface(pPH);
    ReleaseInterface(pbc);
    ReleaseInterface(phbi);
    ReleaseInterface(pOleObject);
    ReleaseInterface(pOleControl);
    ClearInterface(&_pStreamHistory);
    RRETURN(hr);

Error:
    //
    // this codepath completely aborts creation process
    //

    //
    // If an error occurred make for darn sure that any progsink
    // we created is deleted.
    //
    if (_dwProgCookie)
    {
        IGNORE_HR(pDoc->GetProgSink()->DelProgress(_dwProgCookie));
        _dwProgCookie = 0;
    }

    SetClientSite(NULL);
    ClearInterface(&_pUnkCtrl);
    _state = OS_LOADED;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::Init2
//
//  Synopsis:   2nd phase of initialization
//
//-------------------------------------------------------------------------
HRESULT
CFrameSite::Init2(CInit2Context * pContext)
{
    HRESULT hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    if (Tag() == ETAG_IFRAME)
    {
        // frameBorder form <iframe> should be calculated here.
        //
        LPCTSTR    pStrFrameBorder = GetAAframeBorder();

        _fFrameBorder = !pStrFrameBorder
                      || pStrFrameBorder[0] == _T('y')
                      || pStrFrameBorder[0] == _T('Y')
                      || pStrFrameBorder[0] == _T('1');
        Doc()->_fFrameBorderCacheValid = TRUE;
    }
    else
    {
        Doc()->_fFrameBorderCacheValid = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::Notify
//
//  Synopsis:   Called to notify of a change
//
//-------------------------------------------------------------------------

void
CFrameSite::Notify(CNotification *pNF)
{
    CDoc *      pDoc;
    VARIANT *   pvar;
    HRESULT     hr = S_OK;
    
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_GOTMNEMONIC:
        {
            CSetFocus *                 psf                     = (CSetFocus *) pNF->DataAsPtr();
            CMessage *                  pmsg                    = psf->_pMessage;
            MSG                         msg                     = *(MSG *)pmsg;
            IOleInPlaceActiveObject *   pInPlaceActiveObject    = NULL;

            // Convert accesskey into a TAB message and send it in. We should not send
            // the accesskey itself because that would mislead the inner object - it will
            // start searching for a matching element within itself!
            // (bug 65040 in IE4 db)
            if (!IsFrameTabKey(pmsg) && !IsTabKey(pmsg))
            {
                msg.message = WM_KEYDOWN;
                msg.wParam = VK_TAB;
            }
            msg.lParam = 0; // tell the inner object that this is coming from the outside

            if (S_OK == THR_OLE(QueryControlInterface(IID_IOleInPlaceActiveObject, (void **) &pInPlaceActiveObject)))
            {
                IGNORE_HR(THR(pInPlaceActiveObject->TranslateAccelerator(&msg)));
                pInPlaceActiveObject->Release();
            }
        }
        break;

    case NTYPE_BEFORE_REFRESH:
    case NTYPE_SET_CODEPAGE:
    case NTYPE_KILL_SELECTION:
        {
            CNotification   nf;
            
            hr = THR(GetCDoc(&pDoc));
            if (hr)
                goto Cleanup;

            Assert(pDoc);

            nf.Initialize(pNF->Type(), pDoc->PrimaryRoot(), NULL, pNF->DataAsPtr());
            pDoc->BroadcastNotify(&nf);
        }
        break;

    case NTYPE_GET_FRAME_ZONE:
        pNF->Data((void **)&pvar);

        hr = THR(GetCDoc(&pDoc));
        if (hr)
            goto Cleanup;

        Assert(pDoc);
        hr = THR(pDoc->GetFrameZone(pvar));
        if (hr)
            goto Cleanup;

        break;

    case NTYPE_FAVORITES_SAVE:
        // if persist is not turned on, do the 'default stuff' (if it is, the 
        //    special handling already happened in the call to super::
        if (!Doc()->_pPrimaryMarkup->MetaPersistEnabled(htmlPersistStateFavorite))
        {
            BSTR                    bstrName;
            BSTR                    bstrTemp;
            FAVORITES_NOTIFY_INFO * psni;
            BOOL                    fContinue = (hr != S_FALSE);  // if super:: returned S_FALSE
                                            // then the continuation 'bubbling' was canceled.
            
            pNF->Data((void **)&psni);
            hr = THR(GetCDoc(&pDoc));
            if (hr)
                goto Cleanup;

            Assert(pDoc);

            bstrName = GetPersistID(psni->bstrNameDomain);

            // fire on persist to give the event an oppurtunity to cancel
            // the default behavior.

            hr = THR(PersistFavoritesData(psni->pINPB, bstrName));

            if (fContinue)
            {
                void *          pv;
                CNotification   nf;
                
                // do the broadcast notify, but use the new name for nesting purposes
                bstrTemp = psni->bstrNameDomain;
                psni->bstrNameDomain = bstrName;
                pNF->Data(&pv);
                nf.FavoritesSave(pDoc->PrimaryRoot(), pv);
                pDoc->BroadcastNotify(&nf);

                psni->bstrNameDomain = bstrTemp;
            }
            SysFreeString(bstrName);
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if (_pTargetFrame)
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        ClearInterface(&_pTargetFrame);
        break;
    
    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange( DISPID_CFrameSite_src, ((PROPERTYDESC *)&s_propdescCFrameSitesrc)->GetdwFlags());
        break;
    }

Cleanup:
    super::Notify(pNF);
}


//+--------------------------------------------------------------------------------
//
//  member : PersistFavoritesData   
//
//  Synopsis : this method is responsible for saveing the default frame information
//      into the shortcut file.  This include filtering for the meta tags/ xtag
//      specification of what values to store.
//
//      For the frame we want to save a number of different pieces of information
//      this includes:
//          frame name/id/unique identifier
//          frame URL
//          frame's body's scroll position
//          frame postition or size
//
//      TODO: add the filtering logic
//
//---------------------------------------------------------------------------------

HRESULT
CFrameSite::PersistFavoritesData(INamedPropertyBag * pINPB, BSTR bstrSection)
{
    HRESULT       hr = S_OK;
    PROPVARIANT   varValue;
    TCHAR         achTemp[pdlUrlLen];

    Assert (pINPB);

    // only do the save for this frame if it has actually navigated.
    if (_fHasNavigated)
    {
        hr = THR(GetCurrentFrameURL(achTemp, ARRAY_SIZE(achTemp) ));
        if (hr)
            goto Cleanup;

        // First store off the url of the frame
        V_VT(&varValue) = VT_BSTR;
        V_BSTR(&varValue) = SysAllocString(achTemp);

        IGNORE_HR(pINPB->WritePropertyNPB(bstrSection,
                                          _T("BASEURL"),
                                          &varValue));
        SysFreeString(V_BSTR(&varValue));

        // Second, store off the original SRC URL
        const TCHAR * pchUrl = GetAAsrc();

        V_VT(&varValue) = VT_BSTR;
        V_BSTR(&varValue) = SysAllocString(pchUrl);

        IGNORE_HR(pINPB->WritePropertyNPB(bstrSection,
                                          _T("ORIGURL"),
                                          &varValue));
        SysFreeString(V_BSTR(&varValue));
    }

Cleanup:
    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member:     CacheTargetFrame
//
//  Synopsis:   caches the target frame if can.
//
//-------------------------------------------------------------------------

void
CFrameSite::CacheTargetFrame()
{
    HRESULT             hr;
    ITargetEmbedding *  pTargetEmbedding = NULL;

    if (_pTargetFrame)
        goto Cleanup;

    hr = THR_NOTRACE(QueryControlInterface(IID_ITargetEmbedding, (void**)&pTargetEmbedding));
    if (hr)
        goto Cleanup;

    IGNORE_HR(pTargetEmbedding->GetTargetFrame(&_pTargetFrame));

Cleanup:
    ReleaseInterface(pTargetEmbedding);
}


//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::NoResize()
//
//  Note:       Called by CFrameSetSite to determine if the site is resizeable
//
//-------------------------------------------------------------------------

BOOL CFrameSite::NoResize()
{
    return GetAAnoResize() != 0;
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT   hr   = S_OK;
    CDoc    * pDoc = Doc();

    switch (dispid)
    {
    case DISPID_CFrameSite_src:
    {
        BOOL fSaveTempfileForPrinting = pDoc && pDoc->_fSaveTempfileForPrinting;

        // While we are saving frames or iframes out to tempfiles, we are rewiring
        // the src property of the (i)frame temporarily, but we don't want any
        // property change notifications to occur because they would alter the
        // document inside the browser.
        if (!fSaveTempfileForPrinting)
        {
            hr = THR(OnPropertyChange_Src());
        }
        break;
    }

    case DISPID_CFrameSite_scrolling:
        hr = THR(OnPropertyChange_Scrolling());
        break;

    case DISPID_CFrameSite_noResize:
        hr = THR(OnPropertyChange_NoResize());
        break;

    case DISPID_CFrameSite_frameBorder:
        if (Tag() == ETAG_IFRAME)
        {
            LPCTSTR pStrFrameBorder = GetAAframeBorder();
            _fFrameBorder = !pStrFrameBorder
                          || pStrFrameBorder[0] == _T('y')
                          || pStrFrameBorder[0] == _T('Y')
                          || pStrFrameBorder[0] == _T('1');
        }
        else
        {
            Assert(pDoc->GetPrimaryElementClient()->Tag() == ETAG_FRAMESET);
            pDoc->_fFrameBorderCacheValid = FALSE;
        }
        break;
    }

    if (!hr)
    {
        hr = THR(super::OnPropertyChange(dispid, dwFlags));
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_Src
//
//  Note:       Called after src property has changed
//
//-------------------------------------------------------------------------

BOOL IsSpecialUrl(TCHAR *pchUrl);

HRESULT
CFrameSite::OnPropertyChange_Src()
{
    HRESULT         hr = S_OK;

    CacheTargetFrame();

    if (_pTargetFrame)
    {
        DWORD         dwBindf = 0;
        const TCHAR * pchUrl = GetAAsrc();
        TCHAR         cBuf[pdlUrlLen];
        TCHAR *       pchExpandedUrl = cBuf;
        CDoc *        pDoc = Doc();

        cBuf[0] = _T('\0');

        if (pDoc->_pDwnDoc)
            dwBindf = pDoc->_pDwnDoc->GetBindf();

        if (!pchUrl || !*pchUrl)
        {
            pchUrl = _T("about:blank");
        }

        // check to see if this URL is in one of our parent frames

        pDoc->ExpandUrl(pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, this);

        if (pDoc->IsUrlRecursive(pchExpandedUrl))
        {
            TraceTag((tagWarning, "Found %ls recursively, not displaying", pchExpandedUrl));
            pchUrl = _T("about:blank");
        }
        
        // If url is not secure but is on a secure page, we should query now
        // Note that even if we don't want to load the page, we do the navgiation anyway;
        // When loading the nested instance, we call ValidateSecureUrl again and fail the load.
        // This is so that the nested shdocvw has the correct URL in case of "refresh". (dbau)
        
        if (pDoc->GetRootDoc()->_sslPrompt == SSL_PROMPT_QUERY && !IsSpecialUrl(pchExpandedUrl))
        {
            // special urls need to be wrapped before they're checked; so the checking is
            // done on the other (load) side
            
            pDoc->ValidateSecureUrl(pchExpandedUrl, FALSE, FALSE);
        }

        hr = THR(pDoc->FollowHyperlink(
                pchUrl, 
                NULL, 
                this, 
                NULL, 
                FALSE, 
                FALSE,
                _pTargetFrame, 
                dwBindf));

        NoteNavEvent();
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_Scrolling
//
//  Note:       Called after scrolling property has changed
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange_Scrolling()
{
    HRESULT     hr = S_OK;

    CacheTargetFrame();

    if (_pTargetFrame)
    {
        DWORD   dwFlags;
        CDoc *  pDoc;

        hr = THR(_pTargetFrame->GetFrameOptions(&dwFlags));
        if (hr)
            goto Cleanup;

        // Only update scrolling option
        dwFlags &= ~(FRAMEOPTIONS_SCROLL_AUTO | FRAMEOPTIONS_SCROLL_YES | FRAMEOPTIONS_SCROLL_NO);
        hr = THR(_pTargetFrame->SetFrameOptions(dwFlags | (DWORD)GetAAscrolling()));
        if (hr)
            goto Cleanup;

        hr = THR(GetCDoc(&pDoc));
        if (hr)
            goto Cleanup;

        hr = THR(pDoc->OnFrameOptionScrollChange());
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_NoResize
//
//  Note:       Called after NoResize property has changed
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange_NoResize()
{
    HRESULT     hr = S_OK;

    CacheTargetFrame();

    if (_pTargetFrame)
    {
        DWORD   dwFlags;

        hr = THR(_pTargetFrame->GetFrameOptions(&dwFlags));
        if (hr)
            goto Cleanup;

        // Only update noResize option
        dwFlags &= ~FRAMEOPTIONS_NORESIZE;
        hr = THR(_pTargetFrame->SetFrameOptions(dwFlags | (DWORD)GetAAnoResize()));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetCDoc
//
//  Note:       drills into WebBrowser control to get
//              the doc living there
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetCDoc (CDoc ** ppDoc)
{
    HRESULT             hr = S_OK;
    IWebBrowser *       pWB = NULL;
    IDispatch *         pDispDoc = NULL;

    if (!ppDoc)
        RRETURN (E_POINTER);

    *ppDoc = NULL;

    hr = THR_OLE(QueryControlInterface(IID_IWebBrowser,(void**)&pWB));
    if (hr)
        goto Cleanup;

    hr = THR(pWB->get_Document(&pDispDoc));
    if (hr)
        goto Cleanup;

    if (!pDispDoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pDispDoc->QueryInterface(CLSID_HTMLDocument, (void**)ppDoc));

Cleanup:
    ReleaseInterface (pWB);
    ReleaseInterface (pDispDoc);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetIPrintObject
//
//  Note:       drills into WebBrowser control to get
//              the IPrint-supporter living there
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetIPrintObject(IPrint ** ppPrint)
{
    HRESULT             hr = S_OK;
    IWebBrowser *       pWB = NULL;
    IDispatch *         pDispDoc = NULL;

    if (!ppPrint)
        RRETURN (E_POINTER);

    *ppPrint = NULL;

    hr = THR_OLE(QueryControlInterface(IID_IWebBrowser,(void**)&pWB));
    if (hr)
        goto Cleanup;

    hr = THR(pWB->get_Document(&pDispDoc));
    if (hr)
        goto Cleanup;

    if (!pDispDoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pDispDoc->QueryInterface(IID_IPrint, (void**)ppPrint));

Cleanup:
    ReleaseInterface (pWB);
    ReleaseInterface (pDispDoc);

    // Don't RRETURN because in most cases, we won't find an IPrint interface.
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetCurrentFrameURL
//
//  Note:       drills into WebBrowser control to get
//              the URL of the current html or external doc living there
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetCurrentFrameURL(TCHAR *pchUrl, DWORD cchUrl)
{
    IWebBrowser *pWebBrowser = NULL;
    HRESULT hr;
    CDoc *  pDoc = Doc();

    if (!pDoc)
    {
        Assert(!"Framesite without doc!!!");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR_OLE(QueryControlInterface(IID_IWebBrowser,(void**)&pWebBrowser));
    if (!hr && pWebBrowser)
    {
        BSTR bstrActiveFrameUrl;

        hr = THR( pWebBrowser->get_LocationURL(&bstrActiveFrameUrl) );
        if (!hr && bstrActiveFrameUrl)
        {
            DWORD cchTemp;

            // Make sure we have the base URL.
            Assert(!!pDoc->_cstrUrl);

            if (!pDoc->_cstrUrl)
            {
                if (cchUrl)
                    *pchUrl = _T('\0');
            }
            else
            {
                // Obtain absolute Url.
                hr = THR(CoInternetCombineUrl(pDoc->_cstrUrl, bstrActiveFrameUrl,
                    URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, pchUrl, cchUrl, &cchTemp, 0));
            }
            SysFreeString(bstrActiveFrameUrl);
        }

        ReleaseInterface(pWebBrowser);
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetOmWindow
//
//  Note:       drills into WebBrowser control to get om window of
//              the doc living there
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetOmWindow (IHTMLWindow2 ** ppOmWindow)
{
    HRESULT             hr = S_OK;
    IWebBrowser *       pWB = NULL;
    IServiceProvider *psp = NULL;

    if (!ppOmWindow)
        RRETURN (E_POINTER);

    *ppOmWindow = NULL;

    hr = THR_OLE(QueryControlInterface(IID_IWebBrowser,(void**)&pWB));
    if (hr)
        goto Cleanup;

    //  This method of getting the OmWindow for the frame works
    //  a. even if there is no active docobject yet
    //  b. the active docobject turns out to be not TRIDENT CDoc
    hr = THR_OLE(pWB->QueryInterface(IID_IServiceProvider, (LPVOID *) &psp));
    if (hr)
        goto Cleanup;

    hr = THR_OLE(psp->QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (LPVOID *) ppOmWindow));

Cleanup:
    ReleaseInterface (psp);
    ReleaseInterface (pWB);

    RRETURN (hr);
}


//+----------------------------------------------------------------------------
//
// Member:    CFrameSite::VerifyReadyState
//
//-----------------------------------------------------------------------------
#if DBG==1

ExternTag(tagReadystateAssert);

void
CFrameSite::VerifyReadyState(LONG lReadyState)
{
     CDoc * pDoc;

     if (IsTagEnabled(tagReadystateAssert) &&
         GetCDoc(&pDoc) == S_OK &&
         lReadyState > pDoc->_readyState)
     {
         char ach[256];

         wsprintfA(ach, "CFrameSite detects inconsistent readyState.  "
             "CDoc says %s.  Shdocvw says %s.",
             pDoc->_readyState == READYSTATE_UNINITIALIZED ? "UNINITIALIZED" :
             pDoc->_readyState == READYSTATE_LOADING ? "LOADING" :
             pDoc->_readyState == READYSTATE_LOADED ? "LOADED" :
             pDoc->_readyState == READYSTATE_INTERACTIVE ? "INTERACTIVE" :
             pDoc->_readyState == READYSTATE_COMPLETE ? "COMPLETE" : "(Unknown)",
             lReadyState == READYSTATE_UNINITIALIZED ? "UNINITIALIZED" :
             lReadyState == READYSTATE_LOADING ? "LOADING" :
             lReadyState == READYSTATE_LOADED ? "LOADED" :
             lReadyState == READYSTATE_INTERACTIVE ? "INTERACTIVE" :
             lReadyState == READYSTATE_COMPLETE ? "COMPLETE" : "(Unknown)");

         AssertSz(FALSE, ach);
     }
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     HandleMessage
//
//  Synopsis:   Handle messages bubbling when the passed site is non null
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT BUGCALL
CFrameSite::HandleMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;

    switch (pMessage->message)
    {
#ifndef WIN16
    case WM_MOUSEWHEEL:
        CDoc * pDoc;
        POINT  pt;
        RECT   rc;

        // 59283 - If we already sent in this msg, don't re-send it!
        if (_fWheelMsgSentIn)
            break;

        hr    = THR(GetCDoc(&pDoc));
        if (hr)
            break;

        Assert(pDoc);
        hr = S_FALSE;

        if (pDoc->InPlace() && pDoc->InPlace()->_hwnd)
        {
            Assert(pDoc->InPlace()->_hwnd);
            pt.x = MAKEPOINTS(pMessage->lParam).x;
            pt.y = MAKEPOINTS(pMessage->lParam).y;
            ::GetWindowRect(pDoc->InPlace()->_hwnd, &rc);

            if (PtInRect(&rc, pt))
            {
                // Mouse wheel is rotated inside the frame, send WM_MOUSEWHEEL
                // message down to scroll the inner CDoc.
                //
                _fWheelMsgSentIn = TRUE;
                SendMessage(pDoc->InPlace()->_hwnd,
                        pMessage->message,
                        pMessage->wParam,
                        pMessage->lParam);
                _fWheelMsgSentIn = FALSE;
                hr = S_OK;
            }
        }

        break;
#endif //ndef WIN16
    }

    if (hr == S_FALSE)
    {
        hr = THR(super::HandleMessage(pMessage));
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSite::afterObjectInstantiation
//
//  Synopsis:   setup the frameoptions
//
//----------------------------------------------------------------------------

void CFrameSite::AfterObjectInstantiation()
{
    HRESULT hr;

    CacheTargetFrame();
    if (_pTargetFrame)
    {
        DWORD dwFlags;

        // Set default frame option
        hr = THR(_pTargetFrame->GetFrameOptions(&dwFlags));
        if (!hr)
        {
            DWORD   dwFlagScroll = (DWORD)GetAAscrolling();
            CDoc  * pDoc = Doc();
            CElement * pElemClient = GetMarkup()->GetElementClient();

            // If GetAAscrolling() does not contain FRAMEOPTIONS_SCROLL_NO
            // default option contains FRAMEOPTIONS_SCROLL_AUTO,
            // In both cases, always contains FRAMEOPTIONS_NORESIZE
            //
            if (!pDoc->_fFrameBorderCacheValid && pElemClient)
            {
                Assert(pElemClient->Tag() == ETAG_FRAMESET);
                DYNCAST(CFrameSetSite, pElemClient)->FrameBorderAttribute(TRUE, FALSE);
                pDoc->_fFrameBorderCacheValid = TRUE;
            }

            if (dwFlagScroll & FRAMEOPTIONS_SCROLL_NO)
            {
                dwFlags |= FRAMEOPTIONS_NORESIZE
                        |  (_fFrameBorder ? 0 : FRAMEOPTIONS_NO3DBORDER)
                        |  dwFlagScroll;
            }
            else
            {
                dwFlags |= FRAMEOPTIONS_SCROLL_AUTO
                        |  FRAMEOPTIONS_NORESIZE
                        |  (_fFrameBorder ? 0 : FRAMEOPTIONS_NO3DBORDER)
                        |  dwFlagScroll;
            }

            IGNORE_HR(_pTargetFrame->SetFrameOptions(dwFlags));


            CUnitValue puvHeight = GetAAmarginHeight();
            CUnitValue puvWidth  = GetAAmarginWidth();
            long iExtra = CFrameSetSite::iPixelFrameHighlightWidth;

            // if we have only one of (marginWidth, marginHeight) defined,
            // use 0 for the other. This is for Netscape compability.
            //
            int iWidth;
            int iHeight;

            iWidth  = (!puvWidth.IsNull())
                      ? max(iExtra, puvWidth.GetPixelValue())
                      : ((!puvHeight.IsNull()) ? 0 : -1);
            iHeight = (!puvHeight.IsNull())
                      ? max(iExtra, puvHeight.GetPixelValue())
                      : ((!puvWidth.IsNull()) ? 0 : -1);

            IGNORE_HR(_pTargetFrame->SetFrameMargins(iWidth, iHeight));
        }

        // even if one of these calls fails, the object still can live
        // though may be not fully initialized
        IGNORE_HR(_pTargetFrame->SetFrameName(GetAAname()));
    }
}


#ifndef NO_DATABINDING
class CDBindMethodsFrame : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsFrame() : super(VT_BSTR, DBIND_ONEWAY) {}
    ~CDBindMethodsFrame()   {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

};

static const CDBindMethodsFrame DBindMethodsFrame;

const CDBindMethods *
CFrameSite::GetDBindMethods()
{
    Assert(Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME);
    return &DBindMethodsFrame;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound image.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [pvData]  - pointer to data to transfer, in this case a bstr.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsFrame::BoundValueToElement(CElement *pElem,
                                        LONG,
                                        BOOL,
                                        LPVOID pvData) const
{
    RRETURN(DYNCAST(CFrameSite, pElem)->put_UrlHelper(*(BSTR *)pvData, (PROPERTYDESC *)&s_propdescCFrameSitesrc));
}
#endif // ndef NO_DATABINDING

HRESULT
CFrameSite::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    BOOL    fSaveToTempFile = FALSE;
    CDoc *  pDoc = Doc();
    BOOL    fSaveTempfileForPrinting = pDoc && pDoc->_fSaveTempfileForPrinting;
    TCHAR   achTempBuffer[pdlUrlLen];
    TCHAR   achTempLocation[pdlUrlLen];
    LPCTSTR pstrSrc = NULL;
    HRESULT hr;

    if (fSaveTempfileForPrinting)
    {
        // BUGBUG: A very ugly hack that we are doing to work around an UrlMon / Word97 bug
        // where the Word document disappears from the browser whenever someone else binds
        // to it.  We therefore don't even reload IPrint objects. (43440)
        if (_fHoldingIPrint)
        {
            // Reset the flag after end.
            _fHoldingIPrint = !fEnd;
            hr = S_OK;
            goto DontSaveThisFrame;
        }

        if (!fEnd)
        {
            CDoc  *  pDoc = NULL;   // unaddref'ed doc
            IPrint * pPrint = NULL; // addref'ed IPrint

            hr = THR(GetCDoc(&pDoc));

            if (pDoc && pDoc->_readyState >= READYSTATE_LOADED)
            {
                if (!StrCmpN(pDoc->_cstrUrl, _T("about:"), 6))
                {
                    // BUGBUG: Don't save "about:" docs because there is nothing
                    // to be gained and trouble to be had (51582).
                    goto DontCreateTempfile;
                }

                _tcscpy(achTempLocation, _T("file://"));

                // Obtain a temporary file name
                if (!GetTempPath( MAX_PATH, achTempBuffer ))
                    goto DontCreateTempfile;
                if (!GetHTMLTempFileName( achTempBuffer, _T("tri"), 0, ((TCHAR *)achTempLocation)+7 ))
                    goto DontCreateTempfile;

                pDoc->_fSaveTempfileForPrinting = TRUE;

                hr = THR( pDoc->Save(((TCHAR *)achTempLocation)+7, FALSE) );

                pDoc->_fSaveTempfileForPrinting = FALSE;

                if (!hr)
                {
                    CSpooler * pSpooler = NULL;

                    if (   S_OK == ::IsSpooler()
                        && S_OK == THR(::GetSpooler(&pSpooler))
                        && pSpooler)
                    {
                        pSpooler->AddTempFile(((TCHAR *)achTempLocation)+7);
                    }
                }
            }
            else if (Tag() == ETAG_IFRAME && S_OK == GetIPrintObject(&pPrint) && pPrint)
            {
                // BUGBUG:  ShDocVw doesn't let us print Office docobjects via IViewObject::Draw()
                // and Word97 has a bug where their docs disappear from hosts if another host
                // requests an instance, so consider those IFrames to be blank to avoid this
                // nasty side-effect.  (52158)
                StrCpy(achTempLocation, _T("about:blank"));
                ClearInterface(&pPrint);
                hr = S_OK;
            }
            else
            {
                hr = THR( GetCurrentFrameURL(achTempLocation, ARRAY_SIZE(achTempLocation) ));
            }

            if (!hr)
            {
                // remember original src.
                pstrSrc = GetAAsrc();

                if ( pstrSrc )
                {
                    _tcscpy(achTempBuffer, GetAAsrc());
                    pstrSrc = achTempBuffer;
                }

                hr = THR( SetAAsrc(achTempLocation) );

                fSaveToTempFile = TRUE;
            }
        }
    }

DontCreateTempfile:

    hr = THR( COleSite::Save(pStreamWrBuff, fEnd) );

DontSaveThisFrame:

    if (fSaveToTempFile)
    {
        IGNORE_HR( SetAAsrc(pstrSrc) );
    }

    RRETURN(hr);
}

