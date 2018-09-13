//------------------------------------------------------------------------
//
//  File:       peerfact.cxx
//
//  Contents:   peer factories
//
//  Classes:    CPeerFactoryUrl, etc.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_SCRPTLET_H_
#define X_SCRPTLET_H_
#include "scrptlet.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifdef VSTUDIO7
#ifndef X_CODELOAD_HXX_
#define X_CODEOAD_HXX_
#include "codeload.hxx"
#endif
#endif //VSTUDIO7

///////////////////////////////////////////////////////////////////////////
//
// (doc) implementation details that are not obvious
//
///////////////////////////////////////////////////////////////////////////

/*

Typical cases how CPeerFactoryUrls may be created.

  1.    <span style = "behavior:url(http://url)" >

        When css is applied to the span, CDoc::AttachPeerCss does not find matching 
        CPeerFactoryUrl in the list, and creates CPeerFactoryUrl for the url

  2.    <span style = "behavior:url(#default#foo)" >

        When css is applied to the span, CDoc::AttachPeerCss does not find matching 
        "#default" or "#default#foo" in the list, and creates CPeerFactoryUrl "#default"; then
        it immediately clones it into "#default#foo" and inserts it before "#default".

  3.    <object id = bar ... >...</object>

        <span id = span1 style = "behavior:url(#bar#foo)" >
        <span id = span2 style = "behavior:url(#bar#zoo)" >

        When css is applied to the span1, CDoc::AttachPeerCss does not find matching 
        "#bar#foo" or "#bar" in the list, and creates CPeerFactoryUrl "#bar"; then 
        it immediately clones it into "#bar#foo" and inserts it before "#bar".

        When css is applied to the span2, CDoc::AttachPeerCss finds matching "#bar" and
        clones it into "#bar#zoo".

  4.    (olesite #bar is missing)
        <span id = span1 style = "behavior:url(#bar#foo)" >
        <span id = span2 style = "behavior:url(#bar#foo)" >

        When css is applied to span1, we can't find matching "#bar" or "#bar#foo". We then
        search the whole page trying to find olesite "#bar", and we fail. We then create CPeerFactoryUrl "#bar"
        with DOWNLOANLOADSTATE_DONE and TYPE_NULL. The factory will be failing creating any behaviors:
        it's whole purpose is to avoid any additional searches of the page for olesite "#bar". CPeerFactoryUrl
        "#bar" is then immediately cloned into CPeerFactoryUrl "#bar#foo", that is also going to be failing
        creating behaviors.

        When css is applied to span2, we find CPeerFactoryUrl "#bar#foo", and reuse it.


*/

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CPeerFactoryUrl,       CDoc,   "CPeerFactoryUrl")
MtDefine(CPeerFactoryBinary,    CDoc,   "CPeerFactoryBinary")

MtDefine(CPeerFactoryUrl_aryDeferred_pv, CPeerFactoryUrl, "CPeerFactoryUrl::_aryDeferred::_pv")

// SubobjectThunk
HRESULT (STDMETHODCALLTYPE  CVoid::*const CPeerFactoryUrl::s_apfnSubobjectThunk[])(void) =
{
#ifdef UNIX //IEUNIX: This first item is null on Unix Thunk format
    TEAROFF_METHOD_NULL
#endif
    TEAROFF_METHOD(CPeerFactoryUrl,  SubobjectThunkQueryInterface, subobjectthunkqueryinterface, (REFIID, void **))
    TEAROFF_METHOD_(CPeerFactoryUrl, SubobjectThunkAddRef,         subobjectthunkaddref,         ULONG, ())
    TEAROFF_METHOD_(CPeerFactoryUrl, SubobjectThunkSubRelease,     subobjectthunkrelease,        ULONG, ())
};

///////////////////////////////////////////////////////////////////////////
//
// misc helpers
//
///////////////////////////////////////////////////////////////////////////

LPTSTR
GetPeerNameFromUrl (LPTSTR pch)
{
    if (!pch || !pch[0])
        return NULL;

    if (_T('#') == pch[0])
        pch++;

    pch= StrChr(pch, _T('#'));
    if (pch)
    {
        pch++;
    }

    return pch;
}

//+-------------------------------------------------------------------
//
//  Helper:     FindOleSite
//
//--------------------------------------------------------------------

HRESULT
FindOleSite(LPTSTR pchName, CMarkup * pMarkup, COleSite ** ppOleSite)
{
    HRESULT                 hr;
    CElementAryCacheItem    cacheItem;
    CBase *                 pBase;
    int                     i, c;

    Assert (ppOleSite);

    *ppOleSite = NULL;

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pMarkup->CollectionCache()->BuildNamedArray(
        CMarkup::ELEMENT_COLLECTION,
        pchName,
        FALSE,              // fTagName
        &cacheItem,
        0,                  // iStartFrom
        FALSE));            // fCaseSensitive
    if (hr)
        goto Cleanup;

    hr = E_FAIL;
    for (i = 0, c = cacheItem.Length(); i < c; i++)
    {
        pBase = (CBase*)(cacheItem.GetAt(i));
        if (pBase->BaseDesc()->_dwFlags & CElement::ELEMENTDESC_OLESITE)
        {
            hr = S_OK;
            *ppOleSite = DYNCAST(COleSite, pBase);
            break;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Helper:     QuerySafePeerFactory
//
//-------------------------------------------------------------------------

HRESULT
QuerySafePeerFactory(COleSite * pOleSite, IElementBehaviorFactory ** ppPeerFactory)
{
    HRESULT                     hr;

    Assert (ppPeerFactory);

    hr = THR_NOTRACE (pOleSite->QueryControlInterface(IID_IElementBehaviorFactory, (void**)ppPeerFactory));
    if (S_OK == hr && (*ppPeerFactory))
    {
        if (!pOleSite->IsSafeToScript() ||                             // do not allow unsafe activex controls
            !pOleSite->IsSafeToInitialize(IID_IPersistPropertyBag2))   // to workaround safety settings by
        {                                                              // exposing scriptable or loadable peers
            ClearInterface(ppPeerFactory);
            hr = E_ACCESSDENIED;
        }
    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Helper:     IsDefaultUrl
//
//--------------------------------------------------------------------

BOOL
IsDefaultUrl(LPTSTR pchUrl, LPTSTR * ppchName = NULL)
{
    BOOL    fDefault = (0 == StrCmpNIC(_T("#default"), pchUrl, 8) &&
                        (0 == pchUrl[8] || _T('#') == pchUrl[8]));

    if (fDefault && ppchName)
    {
        *ppchName = pchUrl + 8;         // advance past "#default"

        if (_T('#') == (*ppchName)[0])
        {
            (*ppchName)++;              // advance past second "#"
        }
        else
        {
            Assert (0 == (*ppchName)[0]);
            (*ppchName) = NULL;
        }
    }

    return fDefault;
}

//+-------------------------------------------------------------------
//
//  Helper:     FindPeer
//
//--------------------------------------------------------------------

HRESULT
FindPeer(
    IElementBehaviorFactory *   pFactory,
    LPTSTR                      pchName,
    LPTSTR                      pchUrl,
    IElementBehaviorSite *      pSite,
    IElementBehavior**          ppPeer)
{
    HRESULT     hr;
    BSTR        bstrName = NULL;
    BSTR        bstrUrl  = NULL;

    hr = THR(FormsAllocString(pchName, &bstrName));
    if (hr)
        goto Cleanup;

    hr = THR(FormsAllocString(pchUrl, &bstrUrl));
    if (hr)
        goto Cleanup;

    hr = THR(pFactory->FindBehavior(bstrName, bstrUrl, pSite, ppPeer));

Cleanup:
    FormsFreeString(bstrName);
    FormsFreeString(bstrUrl);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryUrl
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CPeerFactoryUrl
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CPeerFactoryUrl::CPeerFactoryUrl(CDoc * pDoc)
{
    _pDoc = pDoc;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::~CPeerFactoryUrl
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CPeerFactoryUrl::~CPeerFactoryUrl()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Passivate
//
//-------------------------------------------------------------------------

void
CPeerFactoryUrl::Passivate()
{
    StopBinding();

    ClearInterface (&_pFactory);
    ClearInterface (&_pMoniker);

    if (_pOleSite)
    {
        _pOleSite->PrivateRelease();
        _pOleSite = NULL;
    }

    _aryDeferred.ReleaseAll();

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::QueryInterface
//
//-------------------------------------------------------------------------

//..todo move it to core\include\qi_impl.h
#define Data1_IWindowForBindingUI 0x79EAC9D5

STDMETHODIMP
CPeerFactoryUrl::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS2(this, IUnknown, IWindowForBindingUI)
    QI_INHERITS(this, IWindowForBindingUI)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::QueryInterface(iid, ppv));
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Create
//
//--------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Create(LPTSTR pchUrl, CDoc * pDoc, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT             hr;

    Assert (pchUrl && ppFactory);

    (*ppFactory) = new CPeerFactoryUrl(pDoc);
    if (!(*ppFactory))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_T('#') != pchUrl[0] || IsDefaultUrl(pchUrl))       
    {
        //
        // if "http://", "file://", etc., or "#default" or "#default#foo"
        //

        hr = THR((*ppFactory)->Init(pchUrl));           // this will launch download if necessary
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // if local page reference
        //

        // assert that it is "#foo" but not "#foo#bar"
        Assert (_T('#') == pchUrl[0] && NULL == StrChr(pchUrl + 1, _T('#')));

        COleSite *          pOleSite;

        hr = THR(FindOleSite(pchUrl + 1, pMarkup, &pOleSite));
        if (hr)
            goto Cleanup;

        hr = THR((*ppFactory)->Init(pchUrl, pOleSite));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr)
    {   // we get here when #foo is not found on the page
        (*ppFactory)->Release();
        (*ppFactory) = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Init(LPTSTR pchUrl)
{
    HRESULT     hr;
    HRESULT     hr2;
    LPTSTR      pchName;

    Assert (pchUrl);

    hr = THR(_cstrUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    if (HostOverrideBehaviorFactory())
    {
        _type = TYPE_DEFAULT;
        _downloadStatus = DOWNLOADSTATUS_DONE;
        goto Cleanup; // done
    }

    if (_T('#') != pchUrl[0])
    {
        _type = TYPE_CLASSFACTORY;
        _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

        hr = THR(LaunchUrlDownload(_cstrUrl));
    }
    else
    {
        if (IsDefaultUrl(pchUrl, &pchName))
        {
            TCHAR   achUrlDownload[pdlUrlLen];

            hr2 = THR_NOTRACE(_pDoc->FindDefaultBehavior1(
                pchName,
                pchUrl,
                (IElementBehaviorFactory**)&_pFactory,
                achUrlDownload, ARRAY_SIZE(achUrlDownload)));
            if (S_OK == hr2)
            {
                if (_pFactory)
                {
                    _type = TYPE_PEERFACTORY;
                    _downloadStatus = DOWNLOADSTATUS_DONE;
                }
                else
                {
                    _type = TYPE_CLASSFACTORY;
                    _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

                    hr = THR(LaunchUrlDownload(achUrlDownload));
                }
            }
            else
            {
                _type = TYPE_DEFAULT;
                _downloadStatus = DOWNLOADSTATUS_DONE;
            }
        }
        else
        {
            _type = TYPE_NULL;
            _downloadStatus = DOWNLOADSTATUS_DONE;
        }
    }

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Init(LPTSTR pchUrl, COleSite * pOleSite)
{
    HRESULT     hr;

    Assert (pchUrl && _T('#') == pchUrl[0]);

    hr = THR(_cstrUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    _type = TYPE_PEERFACTORY;
    _downloadStatus = DOWNLOADSTATUS_DONE;

    // NOTE that it is valid for pOleSite to be null. In this case this CPeerFactoryUrl only serves as a bit
    // of cached information to avoid search for the olesite again

    _pOleSite = pOleSite;

    if (!_pOleSite)
        goto Cleanup;

    _pOleSite->AddRef();        // balanced in passivate

    if (READYSTATE_LOADED <= _pOleSite->_lReadyState)
    {
        hr = THR(OnOleObjectAvailable());
    }
    else
    {
        hr = THR(_oleReadyStateSink.SinkReadyState()); // causes OnStartBinding, OnOleObjectAvailable, OnStopBinding
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Clone
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Clone(LPTSTR pchUrl, CPeerFactoryUrl ** ppFactory)
{
    HRESULT     hr;

    Assert (
        pchUrl && _T('#') == pchUrl[0] &&
        (TYPE_PEERFACTORY == _type || TYPE_DEFAULT == _type) &&
        0 == StrCmpNIC(_cstrUrl, pchUrl, _cstrUrl.Length()));

    *ppFactory = new CPeerFactoryUrl(_pDoc);
    if (!(*ppFactory))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_pOleSite)
    {
        hr = THR((*ppFactory)->Init(pchUrl, _pOleSite));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR((*ppFactory)->Init(pchUrl));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::LaunchUrlDownload
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::LaunchUrlDownload(LPTSTR pchUrl)
{
    HRESULT                 hr;
    IMoniker *              pMk = NULL;
    IBindCtx *              pBindContext = NULL;
    IUnknown *              pUnk = NULL;
    IBindStatusCallback *   pBSC = NULL;
#ifdef VSTUDIO7
    ULONG                   cchEaten;
#endif //VSTUDIO7

    hr = THR(SubobjectThunkQueryInterface(IID_IBindStatusCallback, (void**)&pBSC));
    if (hr)
        goto Cleanup;

    hr = THR(CreateAsyncBindCtx(0, pBSC, NULL, &pBindContext));
    if (hr)
        goto Cleanup;

#ifndef VSTUDIO7
    hr = THR(CreateURLMoniker(NULL, pchUrl, &pMk));
    if (hr)
        goto Cleanup;
#else 
    hr = E_FAIL;
    if (!_tcsnipre( _T("file:"), 5, pchUrl,  -1))
        hr = THR_NOTRACE(MkParseDisplayName(pBindContext, pchUrl, &cchEaten, &pMk));
    if (hr)
    {
        hr = THR(MkParseDisplayNameEx(pBindContext, pchUrl, &cchEaten, &pMk));
        if (hr)
            goto Cleanup;
    }

    hr = THR(AddBindContextParam(_pDoc, pBindContext));
    if (hr)
        goto Cleanup;
#endif //VSTUDIO7

    _pMoniker = pMk;        // this has to be done before BindToObject, so that if OnObjectAvailable
    _pMoniker->AddRef();    // happens synchronously _pMoniker is available

    hr = THR(pMk->BindToObject(pBindContext, NULL, IID_IUnknown, (void**)&pUnk));
    if (S_ASYNCHRONOUS == hr)
    {
        hr = S_OK;
    }
    else if (S_OK == hr && pUnk)
    {
        pUnk->Release();
    }

Cleanup:

    ReleaseInterface (pMk);
    ReleaseInterface (pBindContext);
    ReleaseInterface (pBSC);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::AttachPeer
//
//  Synopsis:   either creates and attaches peer, or defers  it until download
//              complete
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::AttachPeer (CPeerHolder * pPeerHolder, BOOL fAfterDownload)
{
    HRESULT     hr = S_OK;

    // this should be set up before creation of peer, and before even download is completed, so that we
    // don't attach same behavior twice
    pPeerHolder->_pPeerFactoryUrl = this;    

    switch (_downloadStatus)
    {
    case DOWNLOADSTATUS_NOTSTARTED:
        Assert (0 && "AttachPeer called on CPeerFactoryUrl before Init");
        break;

    case DOWNLOADSTATUS_INPROGRESS:

        //
        // download in progress - defer attaching peer until download completed
        //

        pPeerHolder->_pElement->IncPeerDownloads();

        hr = THR(_aryDeferred.Append (pPeerHolder));
        if (hr)
            goto Cleanup;

        pPeerHolder->AddRef(); // so it won't go away before we attach peer to it

        break;

    case DOWNLOADSTATUS_DONE:

        //
        // download ready - attach right now
        //

        if (TYPE_NULL != _type)
        {
            TCHAR       achPeerName[512];

            Assert (!pPeerHolder->_pPeer);

            hr = THR(GetPeerName(pPeerHolder->_pElement, achPeerName, ARRAY_SIZE(achPeerName)));
            if (hr)
                goto Cleanup;

            IGNORE_HR(pPeerHolder->Create(
                achPeerName[0] ? achPeerName : NULL,    // pchPeerName
                this));

#ifdef PEER_NOTIFICATIONS_AFTER_INIT
            pPeerHolder->EnsureNotificationsSent();
#endif
        }

        if (fAfterDownload)
        {
            pPeerHolder->_pElement->DecPeerDownloads();
        }

        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetPeerName
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::GetPeerName(CElement * pElement, LPTSTR pchName, LONG cchName)
{
    HRESULT     hr;

    hr = THR(Format(0, pchName, cchName, _T("<0s>"), STRVAL(GetPeerNameFromUrl(_cstrUrl))));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::AttachAllDeferred
//
//  Synopsis:   attaches the peer to all elements put on hold before
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::AttachAllDeferred()
{
    int             c;
    CPeerHolder **  ppPeerHolder;

    Assert (DOWNLOADSTATUS_DONE == _downloadStatus);

    for (c = _aryDeferred.Size(), ppPeerHolder = _aryDeferred; 
         c; 
         c--, ppPeerHolder++)
    {
        IGNORE_HR (AttachPeer(*ppPeerHolder, /* fAfterDownload = */ TRUE));
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::FindBehavior, per IElementBehaviorFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::FindBehavior(
    LPTSTR                  pchPeerName,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT hr = E_FAIL;

    if (HostOverrideBehaviorFactory())
    {
        hr = THR_NOTRACE(_pDoc->FindHostBehavior(pchPeerName, _cstrUrl, pSite, ppPeer));
        goto Cleanup;   // done
    }
    
    switch (_type)
    {
    case TYPE_CLASSFACTORYEX:
        if (_pFactory)
        {
            hr = THR(((IClassFactoryEx*)_pFactory)->CreateInstanceWithContext(
                pSite, NULL, IID_IElementBehavior, (void **)ppPeer));
        }
        break;

    case TYPE_CLASSFACTORY:
        if (_pFactory)
        {
            hr = THR(((IClassFactory*)_pFactory)->CreateInstance(NULL, IID_IElementBehavior, (void **)ppPeer));
        }
        break;

    case TYPE_PEERFACTORY:
        if (_pFactory)
        {
            hr = THR(FindPeer((IElementBehaviorFactory *)_pFactory, pchPeerName, _cstrUrl, pSite, ppPeer));
        }
        break;

    case TYPE_DEFAULT:
        hr = THR_NOTRACE(_pDoc->FindDefaultBehavior2(pchPeerName, _cstrUrl, pSite, ppPeer));
        goto Cleanup;   // done

    default:
        Assert (0 && "wrong _type");
        break;
    }

    if (hr)
        goto Cleanup;

    IGNORE_HR(PersistMonikerLoad(*ppPeer, /* fLoadOnce = */FALSE));

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::SubobjectThunkQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::SubobjectThunkQueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT     hr;
    IUnknown *  punk;

    hr = THR_NOTRACE(QueryInterface(riid, (void**)&punk));
    if (S_OK == hr)
    {
        hr = THR(CreateTearOffThunk(
                punk,
                *(void **)punk,
                NULL,
                ppv,
                this,
                (void **)s_apfnSubobjectThunk,
                QI_MASK | ADDREF_MASK | RELEASE_MASK,
                NULL));

        punk->Release();

        if (S_OK == hr)
        {
            ((IUnknown*)*ppv)->AddRef();
        }
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetBindInfo, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    HRESULT hr;

    hr = THR(super::GetBindInfo(pdwBindf, pbindinfo));

    if (S_OK == hr)
    {
        *pdwBindf |= BINDF_GETCLASSOBJECT;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStartBinding, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStartBinding()
{
    _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

    IGNORE_HR(_pDoc->GetProgSink()->AddProgress (PROGSINK_CLASS_CONTROL, &_dwBindingProgCookie));

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStopBinding, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStopBinding()
{
    _downloadStatus = DOWNLOADSTATUS_DONE;

    // note that even if _pFactory is NULL, we still want to do AttachAllDeferred so to balance Inc/DecPeersPending
    IGNORE_HR(AttachAllDeferred());

    _aryDeferred.ReleaseAll();

    _pDoc->GetProgSink()->DelProgress (_dwBindingProgCookie);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStartBinding, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStartBinding(DWORD grfBSCOption, IBinding * pBinding)
{
    IGNORE_HR(super::OnStartBinding(grfBSCOption, pBinding));

    IGNORE_HR(OnStartBinding());
    
    _pBinding = pBinding;
    _pBinding->AddRef();

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStopBinding, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStopBinding(HRESULT hrErr, LPCWSTR szErr)
{
    IGNORE_HR(super::OnStopBinding(hrErr, szErr));

    ClearInterface(&_pBinding);

    IGNORE_HR(OnStopBinding());

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnProgress, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode, LPCWSTR pszText)
{
    IGNORE_HR(super::OnProgress(ulPos, ulMax, ulCode, pszText));

    if (    (ulCode == BINDSTATUS_REDIRECTING)
        &&  _pDoc && !_pDoc->AccessAllowed(pszText))
    {
        if (_pBinding)
            _pBinding->Abort();
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::PersistMonikerLoad, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::PersistMonikerLoad(IUnknown * pUnk, BOOL fLoadOnce)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    IPersistMoniker *   pPersistMoniker;

    if (!_pMoniker || !pUnk)
        goto Cleanup;

    hr2 = THR(pUnk->QueryInterface(IID_IPersistMoniker, (void**)&pPersistMoniker));
    if (S_OK == hr2)
    {
        IGNORE_HR(pPersistMoniker->Load(FALSE, _pMoniker, NULL, NULL));

        if (fLoadOnce)
        {
            ClearInterface(&_pMoniker);
        }

        ReleaseInterface (pPersistMoniker);
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnObjectAvailable, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnObjectAvailable(REFIID riid, IUnknown * pUnk)
{
    HRESULT     hr2;
    IUnknown*   pUnk2;

    IGNORE_HR(super::OnObjectAvailable(riid, pUnk));

    hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IElementBehaviorFactory, (void**)&pUnk2));
    if (S_OK == hr2 && pUnk2)
    {
        _type = TYPE_PEERFACTORY;
        _pFactory = pUnk2;
    }
    else
    {
        hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IClassFactoryEx, (void**)&pUnk2));
        if (S_OK == hr2 && pUnk2)
        {
            _type = TYPE_CLASSFACTORYEX;
            _pFactory = pUnk2;
        }
        else
        {
            hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IClassFactory, (void**)&pUnk2));
            if (S_OK == hr2 && pUnk2)
            {
                Assert (TYPE_CLASSFACTORY == _type);
                _pFactory = pUnk2;
            }
        }
    }

    IGNORE_HR(PersistMonikerLoad(_pFactory, /* fLoadOnce = */TRUE));

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnOleObjectAvailable, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnOleObjectAvailable()
{
    HRESULT     hr;

    hr = THR(QuerySafePeerFactory(_pOleSite, (IElementBehaviorFactory**)&_pFactory));
    if (hr)
    {
        hr = S_OK;
        _type = TYPE_NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::StopBinding, helper
//
//-------------------------------------------------------------------------

void
CPeerFactoryUrl::StopBinding()
{
    if (_pBinding)
        _pBinding->Abort();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetWindow, per IWindowForBindingUI
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::GetWindow(REFGUID rguidReason, HWND *phwnd)
{
    HRESULT hr;

    hr = THR(_pDoc->GetWindow(phwnd));

    RRETURN1(hr, S_FALSE);
}

#ifdef VSTUDIO7
///////////////////////////////////////////////////////////////////////////
//
// CIdentityPeerFactoryUrl
//
///////////////////////////////////////////////////////////////////////////

CIdentityPeerFactoryUrl::CIdentityPeerFactoryUrl(CDoc *pDoc)
    : CPeerFactoryUrl(pDoc)
{
    _fContextSleeping = FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CIdentityPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CIdentityPeerFactoryUrl::Init(IIdentityBehaviorFactory *pFactory)
{
    HRESULT                 hr = S_OK;

    Assert (pFactory);

    _type = TYPE_PEERFACTORY;
    _downloadStatus = DOWNLOADSTATUS_DONE;
    _pFactory = pFactory;
    _pFactory->AddRef();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CIdentityPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CIdentityPeerFactoryUrl::Init(LPTSTR pchUrl)
{
    HRESULT hr;

    hr = CPeerFactoryUrl::Init(pchUrl);
    if (hr)
        goto Cleanup;

    // Will be woken by OnStopBinding().
    if (!_pFactory)
    {
        _fContextSleeping = TRUE;
        _pDoc->PrimaryMarkup()->EnterScriptDownload(&_dwScriptCookie);
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CIdentityPeerFactoryUrl::OnStopBinding, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CIdentityPeerFactoryUrl::OnStopBinding(HRESULT hrErr, LPCWSTR szErr)
{
    HRESULT    hr = S_OK;

    if (_fContextSleeping)
    {
        _fContextSleeping = FALSE;
        _pDoc->PrimaryMarkup()->LeaveScriptDownload(&_dwScriptCookie);
    }

    hr = CPeerFactoryUrl::OnStopBinding(hrErr, szErr);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CIdentityPeerFactoryUrl::GetPeerName
//
//-------------------------------------------------------------------------

HRESULT
CIdentityPeerFactoryUrl::GetPeerName(CElement * pElement, LPTSTR pchName, LONG cchName)
{
    HRESULT     hr;
    LPTSTR      pchCodebase = NULL;
    CHtmCtx *   pCtx = NULL;

    Assert (pElement->NamespaceHtml() && pElement->TagName());
    
    pCtx = _pDoc->HtmCtx();
    Assert(pCtx);

    hr = THR(Format(
        0, pchName, cchName,
        _T("<0s>:<1s>"),
        pElement->NamespaceHtml(), pElement->TagName()));
    
    if (hr)
        goto Cleanup;

    pchCodebase = pCtx->GetCodeBaseFromFactory(pchName);
    if (pchCodebase)
    {
        hr = THR(Format(
            0, pchName, cchName,
            _T("<0s>:<1s>#<2s>"),
            pElement->NamespaceHtml(), pElement->TagName(), pchCodebase));
    }

Cleanup:

    RRETURN (hr);
}
#endif //VSTUDIO7

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryBuiltin class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBuiltin::FindBehavior, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBuiltin::FindBehavior(
    LPTSTR                  pchBehaviorName,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT     hr = E_FAIL;

    Assert (_pTagDesc);

    switch (_pTagDesc->type)
    {
    case CBuiltinGenericTagDesc::TYPE_HTC:

        hr = THR(CHtmlComponent::FindBehavior(_pTagDesc->extraInfo.htcBehaviorType, pSite, ppPeer));

        break;

    case CBuiltinGenericTagDesc::TYPE_OLE:

        hr = THR(CoCreateInstance(
            _pTagDesc->extraInfo.clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehavior, (void **)ppPeer));

        break;

    default:
        Assert ("invalid BuiltinGenericTagDesc");
        hr = E_FAIL;
        break;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryBinary class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary constructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinary::CPeerFactoryBinary()
{
    _pFactory = NULL;
    _pchUrl = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary destructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinary::~CPeerFactoryBinary()
{
    ReleaseInterface(_pFactory);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::AttachPeer
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinary::AttachPeer (CPeerHolder * pPeerHolder, LPTSTR pchUrl)
{
    HRESULT     hr;

    _pchUrl = pchUrl;

    hr = THR_NOTRACE(pPeerHolder->Create(GetPeerNameFromUrl(pchUrl), this));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::FindBehavior, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinary::FindBehavior(
    LPTSTR                  pchPeerName,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT     hr;

    if (_pFactory)
    {
        hr = THR(FindPeer(_pFactory, pchPeerName, _pchUrl, pSite, ppPeer));
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryUrl::CContext class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CContext::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::CContext::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IServiceProvider *)this, IUnknown)
    QI_INHERITS((IServiceProvider *)this, IServiceProvider)
    QI_INHERITS((IScriptletSite *)this, IScriptletSite)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CContext::QueryService, per IServiceProvider
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::CContext::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    HRESULT     hr;

    if (IsEqualGUID(rguidService, SID_ScriptletSite))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
    }
    else
    {
        hr = THR_NOTRACE(PFU()->_pDoc->QueryService(rguidService, riid, ppvObject));
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CContext::OnEvent, per IScriptletSite
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::CContext::OnEvent(DISPID dispid, int cArg, VARIANT *prgvarArg, VARIANT *pvarRes)
{
    HRESULT     hr = S_OK;

    switch (dispid)
    {
    case DISPID_ERROREVENT:
        // Display the error box.
        if (cArg == 1 && V_VT(prgvarArg) == VT_UNKNOWN)
        {
            IScriptletError * pScriptletErr = NULL;

            hr = THR(V_UNKNOWN(prgvarArg)->QueryInterface(IID_IScriptletError, (void **)&pScriptletErr));
            if (S_OK == hr)
            {
                ErrorRecord     errRec;

                hr = THR(errRec.Init(pScriptletErr, PFU()->_cstrUrl));
                if (S_OK == hr)
                {
                    hr = THR(PFU()->_pDoc->ReportScriptError(errRec));
                }

                ReleaseInterface(pScriptletErr);
            }
        }
        break;
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CContext::GetProperty, per IScriptletSite
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::CContext::GetProperty(DISPID dispid, VARIANT *pvarRes)
{
    HRESULT     hr = S_OK;

    if (dispid == DISPID_AMBIENT_LOCALEID)
    {
        // Return the user locale ID, this make VBS locale sensitive and means
        // that VBS pages can not be written to be locale neutral (a page cannot
        // be written to run on different locales).  Only JScript can do this. 
        // However, VBS can be written for Intra-net style apps where functions
        // like string comparisons, UCase, LCase, etc. are used.  Also, some
        // pages could be written in both VBS and JS.  Where JS has the locale 
        // neutral things like floating point numbers, etc.
        if (pvarRes)
        {
            V_VT(pvarRes) = VT_I4;
            V_I4(pvarRes) = g_lcidUserDefault;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryUrl::COleReadyStateSink class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::COleReadyStateSink::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IDispatch)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::COleReadyStateSink::SinkReadyState()
{
    HRESULT         hr = S_OK;
    BSTR            bstrEvent = NULL;

    hr = THR(FormsAllocString(_T("onreadystatechange"), &bstrEvent));
    if (hr)
        goto Cleanup;
    
    hr = THR(PFU()->_pOleSite->attachEvent(bstrEvent, (IDispatch*)this, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(PFU()->OnStartBinding());

Cleanup:
    FormsFreeString(bstrEvent);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::COleReadyStateSink::Invoke(
    DISPID          dispid,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          puArgErr)
{
    HRESULT         hr = S_OK;
    BSTR            bstrEvent = NULL;

    Assert (PFU()->_pOleSite && DISPID_VALUE == dispid);

    if (READYSTATE_LOADED <= PFU()->_pOleSite->_lReadyState &&
        PFU()->_downloadStatus < DOWNLOADSTATUS_DONE)
    {
        hr = THR(PFU()->OnOleObjectAvailable());
        if (hr)
            goto Cleanup;
        
        hr = THR(PFU()->OnStopBinding());
        if (hr)
            goto Cleanup;

        hr = THR(FormsAllocString(_T("onreadystatechange"), &bstrEvent));
        if (hr)
            goto Cleanup;
        
        // BUGBUG (alexz) can't do the following: this will prevent it from firing into the next sink point
        // hr = THR(PFU()->_pOleSite->detachEvent(bstrEvent, (IDispatch*)this));
    }

Cleanup:
    FormsFreeString(bstrEvent);

    RRETURN (hr);
}
