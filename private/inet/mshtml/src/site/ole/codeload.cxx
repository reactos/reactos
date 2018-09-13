//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       codeload.cxx
//
//  Contents:   Implementation of CCodeLoad class
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_CODELOAD_HXX_
#define X_CODELOAD_HXX_
#include "codeload.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_SAFEOCX_H_
#define X_SAFEOCX_H_
#include <safeocx.h>
#endif

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

HRESULT CreateStreamOnFile(LPCTSTR lpstrFile, DWORD dwSTGM, LPSTREAM * ppstrm);

MtDefine(CCodeLoad, Dwn, "CCodeLoad")
MtDefine(OleCreateInfo, Dwn, "OLECREATEINFO")
MtDefine(CBindContextParam, Dwn, "CBindContextParam")

#define BINDCONTEXT_CMDID_BASEURL   0
#define BINDCONTEXT_CMDID_PROPBAG   1

///////////////////////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CCodeLoad::CCodeLoad
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------

CCodeLoad::CCodeLoad()
{
    _info.clsid = GUID_NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     CCodeLoad::Init
//
//  Synopsis:   Simple initializer for code download context
//
//-------------------------------------------------------------------------

HRESULT
CCodeLoad::Init(
    COleSite *pSiteOle,
    COleSite::OLECREATEINFO *pinfo)
{
    CLock   Lock(this);
    HRESULT hr = S_OK;
    CDoc *  pDoc = pSiteOle->Doc();

    Assert( pSiteOle );

    //
    // Initialize member data
    //

    _pSiteOle = pSiteOle;
    _pSiteOle->SubAddRef();

    if (pSiteOle->IsInMarkup())
        pSiteOle->GetMarkup()->EnterScriptDownload(&_dwScriptCookie);

    _fGotData = TRUE;

    Assert(!pinfo->pDataObject);

    // Make a copy of pinfo locally.
    _info.clsid = pinfo->clsid;
    if (pinfo->pStream)
    {
        _info.pStream = pinfo->pStream;
        pinfo->pStream->AddRef();
    }
    if (pinfo->pStorage)
    {
        _info.pStorage = pinfo->pStorage;
        pinfo->pStorage->AddRef();
    }
    if (pinfo->pPropBag)
    {
        _info.pPropBag = pinfo->pPropBag;
        pinfo->pPropBag->AddRef();
    }
    if (pinfo->pStreamHistory)
    {
        _info.pStreamHistory = pinfo->pStreamHistory;
        pinfo->pStreamHistory->AddRef();
    }
    if (pinfo->pBindCtxHistory)
    {
        _info.pBindCtxHistory = pinfo->pBindCtxHistory;
        pinfo->pBindCtxHistory->AddRef();
    }
    if (pinfo->pShortCutInfo)
    {
        _info.pShortCutInfo = pinfo->pShortCutInfo;
        _info.pShortCutInfo->AddRef();
    }
    _info.dwMajorVer = pinfo->dwMajorVer;
    _info.dwMinorVer = pinfo->dwMinorVer;

    MemReplaceString(Mt(OleCreateInfo), pinfo->pchSourceUrl, &_info.pchSourceUrl);
    MemReplaceString(Mt(OleCreateInfo), pinfo->pchDataUrl, &_info.pchDataUrl);
    MemReplaceString(Mt(OleCreateInfo), pinfo->pchMimeType, &_info.pchMimeType);
    MemReplaceString(Mt(OleCreateInfo), pinfo->pchClassid, &_info.pchClassid);
    MemReplaceString(Mt(OleCreateInfo), pinfo->pchFileName, &_info.pchFileName);

    _pProgSink = pDoc->GetProgSink();
    if( _pProgSink )
    {
        _pProgSink->AddRef();
        IGNORE_HR(_pProgSink->AddProgress( PROGSINK_CLASS_CONTROL, &_dwProgCookie ));
    }

    //
    // If we need to download data, create a bits context and get data
    //

    if (pinfo->pchDataUrl)
    {

        // We still download the data, even if from another domain.  But set
        // the flag so we can inject policy when we init the control.
        pSiteOle->_fDataSameDomain = pDoc->AccessAllowed(pinfo->pchDataUrl);

        hr = THR(pDoc->NewDwnCtx(DWNCTX_FILE, pinfo->pchDataUrl,
                    pSiteOle, (CDwnCtx **)&_pBitsCtx));
        if (hr)
            goto Cleanup;

        if (_pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR))
        {
            OnDwnChan(_pBitsCtx);
        }
        else
        {
            _fGotData = FALSE;
            _pBitsCtx->SetProgSink(pDoc->GetProgSink());
            _pBitsCtx->SetCallback(OnDwnChanCallback, this);
            _pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }

    SetDwnDoc(pDoc->_pDwnDoc);

    //
    // Start BTO.
    //

    hr = THR(BindToObject());
    if (!OK(hr))
        goto Cleanup;

Cleanup:
    RRETURN1(hr, MK_S_ASYNCHRONOUS);
}

//+------------------------------------------------------------------------
//
//  Method:     CCodeLoad::OnDwnChan
//
//-------------------------------------------------------------------------

void
CCodeLoad::OnDwnChan(CDwnChan * pDwnChan)
{
    CLock       Lock(this);
    ULONG       ulState = _pBitsCtx->GetState();
    HRESULT     hr;
    BOOL        fDone = FALSE;
    TCHAR *     pchExt;

    //
    // We better not have terminated yet
    //

    Assert(_pSiteOle);

    if (ulState & DWNLOAD_COMPLETE)
    {
        fDone = TRUE;

        // If unsecure download, may need to remove lock icon on Doc
        _pSiteOle->Doc()->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
        
        // Find last occurance of '.' in URL

        //
        // BUGBUG:  due to a data sniffing bug, binary .stm files
        // are downloaded and given an extension of .htm.  We then
        // need to examine the URL to get the correct file extension
        //

        pchExt = _tcsrchr(_pBitsCtx->GetUrl(), _T('.'));
        if (pchExt &&
            (!StrCmpIC(pchExt, _T(".stm")) ||
             !StrCmpIC(pchExt, _T(".ods")) ||   // NCompass data files
             !StrCmpIC(pchExt, _T(".ica"))))    // Citrix Winframe data files
        {
            hr = THR(CreateStreamFromData());
            if (hr)
                goto Cleanup;

            goto Done;
        }

        _pBitsCtx->GetFile(&_info.pchFileName);
    }
    else if (ulState & (DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        //
        // In error case, try to initNew
        //

        fDone = TRUE;
    }

Done:
    if (fDone)
    {
        _fGotData = TRUE;

        if (_punkObject)
        {
            //
            // Code download is already done, go ahead and create object.
            //

            hr = THR(_pSiteOle->CreateObjectNow(
                _iidObject,
                _punkObject,
                &_info));
        }

        if (_pBitsCtx)
        {
            _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
            _pBitsCtx->Disconnect();
            _pBitsCtx->Release();
            _pBitsCtx = NULL;
        }
    }

Cleanup:
    ;
}


//+------------------------------------------------------------------------
//
//  Member:     CCodeLoad::CreateStreamFromData
//
//  Synopsis:   Creates an OLE stream from a .stm data file.
//
//-------------------------------------------------------------------------

HRESULT
CCodeLoad::CreateStreamFromData()
{
    LARGE_INTEGER   dlibMove = {0,0};
    HRESULT         hr = E_FAIL;
    TCHAR *         pchFileName = NULL;
    CLSID           clsid;

    //
    // Retrieve clsid from stream and get control
    //

    hr = THR(_pBitsCtx->GetFile(&pchFileName));
    if (hr)
        goto Cleanup;

    hr = THR(CreateStreamOnFile(
            pchFileName,
            STGM_READ | STGM_SHARE_DENY_WRITE,
            &_info.pStream));
    if (hr)
        goto Cleanup;

    //
    // In this mode, we only know how to initialize the
    // object via a stream.
    //

    // seek to the begining of the stream
    hr = THR(_info.pStream->Seek(dlibMove, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    //
    // read the clsid from the first 16 bytes
    //

#ifdef BIG_ENDIAN
    // BUGBUG fix big endian
    Assert( 0 && "Fix big endian read of clsid" );
#endif

    hr = THR(_info.pStream->Read(&clsid, 16, NULL));
    if (hr)
        goto Cleanup;

    //
    // If we don't have a clsid yet, go ahead and set the clsid.
    //

    if (_info.clsid == g_Zero.guid)
    {
        _info.clsid = clsid;
    }

Cleanup:
    MemFreeString(pchFileName);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CCodeLoad::Terminate
//
//  Synopsis:   1st stage destruction.
//
//-------------------------------------------------------------------------

void
CCodeLoad::Terminate()
{
    CLock Lock(this);

    Assert(_pSiteOle);

    if (_pSiteOle->IsInMarkup())
        IGNORE_HR(_pSiteOle->GetMarkup()->LeaveScriptDownload(&_dwScriptCookie));

    if (_pBitsCtx)
    {
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
        _pBitsCtx = NULL;
    }

    if (!_fGotObject)
    {
        _pSiteOle->OnFailToCreate();
        _pSiteOle->GetCurLayout()->Invalidate(); // so olesite will redraw another placeholder

        if (_pbinding)
        {
            IGNORE_HR(_pbinding->Abort());
        }
    }

    if (_pbctx)
    {
        IGNORE_HR(RevokeBindStatusCallback(_pbctx, this));
        ClearInterface(&_pbctx);
    }

    _pSiteOle->OnControlReadyStateChanged(/* fForceComplete = */FALSE);

    _pSiteOle->SubRelease();
    _pSiteOle = NULL;

    ClearInterface(&_punkObject);

    if( _dwProgCookie )
    {
        Assert( _pProgSink );
        _pProgSink->DelProgress( _dwProgCookie );
    }
    ClearInterface(&_pProgSink);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::QueryInterface, IUnknown
//
//  Synopsis:   Per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IWindowForBindingUI)
    {
        *ppv = (IWindowForBindingUI *)this;
        AddRef();
        return(S_OK);
    }

    return(super::QueryInterface(iid, ppv));
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::QueryService, IServiceProvider
//
//  Synopsis:   Per IServiceProvider
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObj)
{
    if (rguidService == IID_IWindowForBindingUI)
    {
        return QueryInterface(riid, ppvObj);
    }

    return super::QueryService(rguidService, riid, ppvObj);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::GetWindow, IWindowForBindingUI
//
//  Synopsis:   Per IWindowForBindingUI
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::GetWindow(REFGUID rguidReason, HWND *phwnd)
{
    HRESULT hr;

    if (!_pSiteOle)
    {
        *phwnd = 0;
        hr = E_FAIL;
    }
    else if (_pSiteOle->Doc() &&_pSiteOle->Doc()->IsPrintDoc())
    {
        *phwnd = HWND_DESKTOP;
        hr = S_OK;
    }
    else if (_pSiteOle->Doc()->_dwLoadf & DLCTL_SILENT)
    {
        *phwnd = (HWND)INVALID_HANDLE_VALUE;
        hr = S_FALSE;
    }
    else
    {
        _pSiteOle->Doc()->GetWindowForBinding(phwnd);
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::OnObjectAvailable, IBindStatusCallback
//
//  Synopsis:   Internet Component Download will call back
//              when object is ready.  Typically a class factory.
//
//----------------------------------------------------------------------------

HRESULT
CCodeLoad::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    HRESULT        hr = E_FAIL;
    IClassFactory *pCF = NULL;

    if (!_pSiteOle)
        goto Cleanup;

    _fGotObject = TRUE;

    if (!_fGotData)
    {
        //
        // Data download is occuring.  Wait until OnChan to create object.
        //

        ReplaceInterface (&_punkObject, punk);
        _iidObject = riid;
        hr = S_OK;
    }
    else
    {
        if (_fGetClassObject && OK(punk->QueryInterface(IID_IClassFactory, (void **)&pCF)))
        {
            hr = THR(_pSiteOle->CreateObjectNow(IID_IClassFactory, pCF, &_info));
        }
        else
        {
            hr = THR(_pSiteOle->CreateObjectNow(riid, punk, &_info));
        }
    }

Cleanup:
    ReleaseInterface(pCF);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetBindInfo, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CCodeLoad::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    HRESULT hr;

    hr = THR(super::GetBindInfo(pdwBindf, pbindinfo));
    if (S_OK == hr && _fGetClassObject)
    {
        *pdwBindf |= BINDF_GETCLASSOBJECT;
    }

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::OnProgress, IBindStatusCallback
//
//  Synopsis:   Feedback on code download.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode,
    LPCWSTR pchText)
{
    LPCWSTR pszComponent = NULL;
    ULONG   ulSetProgressFlags;

    if (_pProgSink)
    {
        switch( ulCode )
        {
            case BINDSTATUS_DOWNLOADINGDATA:
            case BINDSTATUS_BEGINDOWNLOADCOMPONENTS:
            case BINDSTATUS_INSTALLINGCOMPONENTS:       
            ulSetProgressFlags = PROGSINK_SET_STATE | PROGSINK_SET_POS | PROGSINK_SET_IDS | PROGSINK_SET_MAX;
            if( pchText )
            {   // Find the component name in the passed in string:
                pszComponent = pchText + wcslen( pchText );
                while( pszComponent > pchText )
                {
                    if( pszComponent[-1] == _T('/') || pszComponent[-1] == _T('\\'))
                    {
                        break;
                    }
                    --pszComponent;
                }
                ulSetProgressFlags |= PROGSINK_SET_TEXT;
            }

            IGNORE_HR(_pProgSink->SetProgress(
              _dwProgCookie,
              ulSetProgressFlags,
              PROGSINK_STATE_LOADING,
              pszComponent,
              IDS_BINDSTATUS_INSTALLINGCOMPONENTS,
              ulPos,
              ulMax ));
            break;
        }
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::OnStartBinding, IBindStatusCallback
//
//  Synopsis:   Feedback on code download.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    if (_pSiteOle)
    {
        ReplaceInterface(&_pbinding, pbinding);
    }
    else
    {
        IGNORE_HR(pbinding->Abort());
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::OnStopBinding, IBindStatusCallback
//
//  Synopsis:   Feedback on code download.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCodeLoad::OnStopBinding(HRESULT hrBinding, LPCWSTR szErr)
{
    CLock   Lock(this);

    ClearInterface(&_pbinding);

        if (_pbctx)
        {
        IGNORE_HR(RevokeBindStatusCallback(_pbctx, this));
            ClearInterface(&_pbctx);
        }

    // if error (e.g., download was aborted or failed to find the object)
    if (hrBinding && _pSiteOle)
    {
        // If download failed due to trust violation, inform user.
        if (hrBinding == TRUST_E_FAIL)
        {
            //
            // Don't show dlg for TRUST_E_SUBJECT_NOT_TRUSTED because that
            // is only returned if user chooses no on authenticode dialog.
            // ie4 bug 38366.
            //
            NotifyHaveProtectedUserFromUnsafeContent(_pSiteOle->Doc(), IDS_OCXDISABLED);
        }

        // in stress, _pSiteOle turns up NULL at this point (not sure why - dbau)
        if (_pSiteOle)
        {
            _pSiteOle->ReleaseCodeLoad();
        }
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:     AddBindContextParam
//
//----------------------------------------------------------------------------

HRESULT
AddBindContextParam(CDoc *pDoc,
                    IBindCtx *pbctx,
                    COleSite *pOleSite, /* = NULL */
                    IPropertyBag *pPropBag /* = NULL */)
{
    HRESULT             hr;
    TCHAR *             pchBaseUrl = NULL;
    CBindContextParam * pBindContextParam = NULL;

    Assert(pDoc);
    hr = THR(pDoc->GetBaseUrl(&pchBaseUrl, pOleSite));
    if (hr)
        goto Cleanup;

    pBindContextParam = new CBindContextParam();
    if (!pBindContextParam)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pBindContextParam->Init(pchBaseUrl, pPropBag));
    if (hr)
        goto Cleanup;

    hr = THR(pbctx->RegisterObjectParam(KEY_BINDCONTEXTPARAM, pBindContextParam));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pBindContextParam)
        pBindContextParam->Release();

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodeLoad::BindToObject
//
//  Synopsis:   Start binding to code.
//
//----------------------------------------------------------------------------

HRESULT
CCodeLoad::BindToObject()
{
    HRESULT                     hr;
    CLock                       Lock(this);
    BOOL                        fClearTask = TRUE;
    IClassFactory *             pCF = NULL;
    IMoniker *                  pMk = NULL;
    IUnknown *                  pUnkObject = NULL;
    TCHAR *                     pchUrl;
    COleSite::OLECREATEINFO *   pinfo = &_info;
    IBindStatusCallback *       pBSC;
    CDoc *                      pDoc = _pSiteOle->Doc();

    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &_pbctx, 0));
    if (hr)
        goto Cleanup;

    pBSC = this;

#if DBG==1
    pBSC->AddRef();
    DbgTrackItf(IID_IBindStatusCallback, "codlod", TRUE, (void **)&pBSC);
#endif

    hr = THR(RegisterBindStatusCallback(_pbctx, pBSC, NULL, NULL));

#if DBG==1
    pBSC->Release();
#endif

    if (hr && hr != S_FALSE) // S_FALSE returned normally
        goto Cleanup;

    if (!IsEqualGUID(g_Zero.guid, pinfo->clsid) ||    // if clsid is not zero, or
        pinfo->pchMimeType)                           // there is MimeType
    {
        IActiveXSafetyProvider *    pSafetyProvider;

        hr = THR(_pSiteOle->Doc()->GetActiveXSafetyProvider(&pSafetyProvider));
        if (hr)
            goto Cleanup;

        if (pSafetyProvider) {
            //
            // An ActiveXSafetyProvider is installed.  Use it to
            // instantiate controls.
            //
            BOOL    fTreatAsUntrusted;

            hr = THR(pDoc->ProcessURLAction(URLACTION_ACTIVEX_TREATASUNTRUSTED,
                &fTreatAsUntrusted));
            if (hr)
                goto Cleanup;

            hr = pSafetyProvider->TreatControlAsUntrusted(fTreatAsUntrusted);
            if (hr)
                goto Cleanup;
            hr = pSafetyProvider->SetSecurityManager(pDoc->_pSecurityMgr);
            if (hr)
                goto Cleanup;
            hr = pSafetyProvider->SetDocumentURLW(pDoc->_cstrUrl);
            if (hr)
                goto Cleanup;
        }


        // CONSIDER: (alexz) (anandra) it would be good to have monikers support
        // urls like "clsid:......." - so that MkParseDisplayName would return
        // a moniker on clsid, and BindToObject on that moniker would instantiate
        // the object with the clsid. If that is implemented, then here we would
        // not call CoGetClassObjectFromURL but go instead to pchUrl codepath.

        if (!(pDoc->_dwLoadf & DLCTL_NO_DLACTIVEXCTLS))
        {
            //
            // main case - download of activex controls is allowed
            //

            hr =  THR(CoGetClassObjectFromURL(
                        pinfo->clsid,
                        pinfo->pchSourceUrl,
                        pinfo->dwMajorVer,
                        pinfo->dwMinorVer,
                        pinfo->pchMimeType,
                        _pbctx,
                        CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                        NULL,
                        IID_IClassFactory,
                        (void **)&pCF));
        }
        else
        {
            //
            // we are not allowed to download activex control so try to
            // instantiate from local machine (common scenario - in WebCheck)
            //

            if (pSafetyProvider) {
                hr = THR(pSafetyProvider->SafeGetClassObject(
                    pinfo->clsid,
                    CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                    NULL,
                    IID_IClassFactory,
                    (IUnknown **)&pCF));
            } else {
                hr = THR(CoGetClassObject(
                    pinfo->clsid,
                    CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                    NULL,
                    IID_IClassFactory,
                    (void **)&pCF));
            }
        }
    }
    else
    {
        ULONG   cchEaten;
        CStr cstrSpecialURL;

        if (pinfo->pchClassid)
        {
            pchUrl = pinfo->pchClassid;
            _fGetClassObject = TRUE;
        }
        else
        {
            pchUrl = pinfo->pchDataUrl;
        }

        // since the UrlCompare also calls this, it is not a problem to call it here.
        // we need it to protect against cases where .HTA can be written as .ht%61
        hr = THR(UrlUnescape( pchUrl, NULL, NULL, URL_UNESCAPE_INPLACE));
        if (hr)
            goto Cleanup;

        hr = E_FAIL;

        if (WrapSpecialUrl(pchUrl, &cstrSpecialURL, pDoc->_cstrUrl, FALSE, FALSE) != S_OK)
            goto Cleanup;

        // Binding to an HTA in this manner is a security hole.
        if (!pchUrl || !StrCmpI(PathFindExtension(pchUrl), _T(".HTA")))
            goto Cleanup;

        // we don't want to continue if the URL we have is the same
        // with the Urls of any of the parent documents.
        if (pDoc->IsUrlRecursive(pchUrl))
            goto Cleanup;

        //
        // First try MkParseDisplayName because MkParseDisplayNameEx
        // seems to have a bug in it where it doesn't forward to
        // MkParseDisplayName.  This is needed for TracySh's java:
        // moniker.  Look at ie4 bug 45662.  (anandra)
        //
        if (!_tcsnipre( _T("file:"), 5, pchUrl,  -1))
            hr = THR_NOTRACE(MkParseDisplayName(_pbctx, pchUrl, &cchEaten, &pMk));
        if (hr)
        {
            hr = THR(MkParseDisplayNameEx(_pbctx, pchUrl, &cchEaten, &pMk));
            if (hr)
                goto Cleanup;
        }

        hr = THR(::AddBindContextParam(pDoc, _pbctx, _pSiteOle, _info.pPropBag));
        if (hr)
            goto Cleanup;

        hr = THR(pMk->BindToObject(
                _pbctx,
                NULL,
                IID_IUnknown,
                (void **)&pUnkObject));
        if (!OK(hr))
            goto Cleanup;
    }

    if (S_OK == hr)
    {
        //
        // the object is immediately available, signal OnObjectAvailable
        //

        if (pCF)
        {
            hr = THR(OnObjectAvailable(IID_IClassFactory, pCF));
        }
        else
        {
            Assert(pUnkObject);
            hr = THR(OnObjectAvailable(IID_IUnknown, pUnkObject));
        }
    }
    else if (MK_S_ASYNCHRONOUS == hr)
    {
        //
        // Block this task until OnObjectAvailable
        //

        fClearTask = FALSE;
    }
    else if (TRUST_E_FAIL == hr)
    {
            NotifyHaveProtectedUserFromUnsafeContent(pDoc, IDS_OCXDISABLED);
    }


Cleanup:
    ReleaseInterface (pCF);
    ReleaseInterface (pMk);
    ReleaseInterface (pUnkObject);

    //
    // Clear up internal state in the case where the class
    // factory is already available.
    //

    if (fClearTask && _pbctx)
    {
        IGNORE_HR(RevokeBindStatusCallback(_pbctx, this));
    }

    RRETURN1(hr, MK_S_ASYNCHRONOUS);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// CBindContextParam methods
//
//

CBindContextParam::CBindContextParam()
{
    _ulRefs = 1;
    _pPropBag = NULL;
}


HRESULT
CBindContextParam::QueryInterface(REFIID iid, void ** ppv)
{
    switch (iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IOleCommandTarget)

    default:
        *ppv = NULL;
        RRETURN (E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


HRESULT
CBindContextParam::Init(TCHAR * pchBaseUrl, IPropertyBag *pPropBag)
{
    ReplaceInterface(&_pPropBag, pPropBag);
    RRETURN (THR(_cstrBaseUrl.Set(pchBaseUrl)));
}


HRESULT
CBindContextParam::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdexecopt,
    VARIANT *       pvarIn,
    VARIANT *       pvarOut)
{
    HRESULT     hr = S_OK;

    if (IsEqualGUID(*pguidCmdGroup, CGID_DownloadObjectBindContext))
    {
        if (!pvarOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        switch (nCmdID)
        {
        case BINDCONTEXT_CMDID_BASEURL:
            V_VT(pvarOut) = VT_BSTR;
            hr = THR(FormsAllocString(_cstrBaseUrl, &V_BSTR (pvarOut)));
            break;

        case BINDCONTEXT_CMDID_PROPBAG:
            if (_pPropBag)
            {
                V_VT(pvarOut) = VT_UNKNOWN;
                V_UNKNOWN(pvarOut) = _pPropBag;
                _pPropBag->AddRef();
                break;
            }
            // Fall thru

        default:
            hr = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }

Cleanup:
    RRETURN (hr);
}

