#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

DeclareTag(tagPeerCDocAttachPeerCss,                    "Peer", "trace CDoc::AttachPeerCss")
DeclareTag(tagPeerCDocAttachPeers,                      "Peer", "trace CDoc::AttachPeers[Css]")
DeclareTag(tagPeerCDocAttachPeersEmptyPH,               "Peer", "CDoc::AttachPeers: add empty peer holders to the list")

MtDefine(CDoc_aryPeerFactoriesUrl_pv, CDoc, "CDoc::_aryPeerFactoriesUrl::_pv")
MtDefine(CDoc_aryPeerQueue_pv,        CDoc, "CDoc::_aryPeerQueue::_pv")
MtDefine(CDoc_AttachPeers_aryUrls_pv, CDoc, "CDoc::AttachPeers::aryUrls")

#ifdef VSTUDIO7
extern ELEMENT_TAG ETagFromTagId ( ELEMENT_TAG_ID tagID );
#endif //VSTUDIO7

HRESULT CLSIDFromHtmlString(TCHAR *pchClsid, CLSID *pclsid);

//////////////////////////////////////////////////////////////////////
//
// CDoc - parsing generic elements
//
//////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsGenericElement
//
//--------------------------------------------------------------------

ELEMENT_TAG
#ifdef VSTUDIO7
CDoc::IsGenericElement (LPTSTR pchFullName, LPTSTR pchColon, BOOL *pfDerived /* = NULL */)
#else
CDoc::IsGenericElement (LPTSTR pchFullName, LPTSTR pchColon)
#endif //VSTUDIO7
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;

#ifdef VSTUDIO7
    if (pfDerived)
        *pfDerived = FALSE;
#endif //VSTUDIO7
    
    if (pchColon)
    {
#ifdef VSTUDIO7
        //
        // factory supplied element? Is there a tagsource entry?
        //

        etag = IsGenericElementFactory(pchFullName, pchColon);
        if (ETAG_UNKNOWN != etag)
        {
            if (pfDerived)
                *pfDerived = TRUE;
            goto Cleanup;           // done
        }
#endif //VSTUDIO7
        
        //
        // sprinkle in a host defined namespace?
        //

        etag = IsGenericElementHost(pchFullName, pchColon);
        if (ETAG_UNKNOWN != etag)
            goto Cleanup;           // done
    }
    else
    {
        //
        // builtin tag?
        //

        if (GetBuiltinGenericTag(pchFullName))
            etag = ETAG_GENERIC_BUILTIN;
    }

Cleanup:

    return etag;
}

#ifdef VSTUDIO7
//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsGenericElementFactory
//
//--------------------------------------------------------------------

ELEMENT_TAG
CDoc::IsGenericElementFactory(LPTSTR pchFullName, LPTSTR pchColon)
{
    CHtmCtx *                  pCtx = NULL;

    pCtx = HtmCtx();
    Assert(pCtx);

    return (ELEMENT_TAG)pCtx->GetBaseTagFromFactory(pchFullName);
}
#endif //VSTUDIO7

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsGenericElementHost
//
//--------------------------------------------------------------------

ELEMENT_TAG
CDoc::IsGenericElementHost (LPTSTR pchFullName, LPTSTR pchColon)
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;
    LPTSTR          pchHost;
    int             l;

    Assert (pchColon);

    pchHost = _cstrHostNS;
    
    // for debugging
    // pchHost = _T("FOO;A;AA;AAA;B");
    
    if (pchHost)
    {
        l = pchColon - pchFullName;

        // CONSIDER (alexz): use an optimal linear algorithm here instead of this one with quadratic behavior

        for (;;)
        {
            if (0 == StrCmpNIC(pchHost, pchFullName, l) &&
                (0 == pchHost[l]) || _T(';') == pchHost[l])
            {
                etag = ETAG_GENERIC;
                goto Cleanup;
            }

            pchHost = StrChr(pchHost, _T(';'));
            if (!pchHost)
                break;

            pchHost++;  // advance past ';'
        }
    }

Cleanup:

    return etag;
}

//////////////////////////////////////////////////////////////////////
//
// CDoc - behavior hosting code
//
//////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Member:     CDoc::FindHostBehavior
//
//--------------------------------------------------------------------

HRESULT
CDoc::FindHostBehavior(
    LPTSTR                  pchPeerName,
    LPTSTR                  pchPeerUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT hr = E_FAIL;

    if (_pHostPeerFactory)
    {
#ifdef VSTUDIO7
        // BUGBUG COM+ (sreeramn): This is purely a VM hack. It cannot
        // handle interface sub-classing properly. Once we move
        // to COM+, we shouldn't need this anymore.
        if (_fHasHostIdentityPeerFactory)
        {
            IIdentityBehaviorFactory * pFactory = NULL;

            hr = _pHostPeerFactory->QueryInterface(IID_IIdentityBehaviorFactory,
                    (void **)&pFactory);
            if (hr)
                goto Cleanup;
        
            hr = THR_NOTRACE(pFactory->FindBehavior(pchPeerName, pchPeerUrl, pSite, ppPeer));

            ReleaseInterface(pFactory);

            goto Cleanup; // done
        }
#endif //VSTUDIO7

        hr = THR_NOTRACE(FindPeer(_pHostPeerFactory, pchPeerName, pchPeerUrl, pSite, ppPeer));
    }

#ifdef VSTUDIO7
Cleanup:
#endif //VSTUDIO7

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Helper:     GetUrlFromDefaultBehaviorName
//
//--------------------------------------------------------------------

HRESULT
GetUrlFromDefaultBehaviorName(LPCTSTR pchName, LPTSTR pchUrl, UINT cchUrl)
{
    HRESULT     hr = S_OK;
    LONG        lRet;
    HKEY        hKey = NULL;
    DWORD       dwBufSize;
    DWORD       dwKeyType;

    //
    // read registry
    //

    lRet = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Internet Explorer\\Default Behaviors"),
        0,
        KEY_QUERY_VALUE,
        &hKey);
    if (ERROR_SUCCESS != lRet || !hKey)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    dwBufSize = cchUrl;
    lRet = RegQueryValueEx(hKey, pchName, NULL, &dwKeyType, (LPBYTE)pchUrl, &dwBufSize);
    if (ERROR_SUCCESS != lRet || REG_SZ != dwKeyType)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    if (hKey)
        RegCloseKey (hKey);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::FindDefaultBehavior1
//
//  Synopsis:   lookup in registry for a default behavior.
//              Here we are looking up for a factory of peer in a form
//              of either COM IElementBehaviorFactory, or url
//
//--------------------------------------------------------------------

HRESULT
CDoc::FindDefaultBehavior1(
    LPTSTR                      pchName,
    LPTSTR                      pchUrl,
    IElementBehaviorFactory **  ppFactory,
    LPTSTR                      pchUrlDownload,
    UINT                        cchUrlDownload)
{
    HRESULT     hr = E_FAIL;
    HRESULT     hr2;
    TCHAR       achUrl[pdlUrlLen];
    uCLSSPEC    classpec;

    if (!pchName)
        goto Cleanup;

    Assert (ppFactory);
    *ppFactory = NULL;

    //
    // look it up in registry and expand url
    //

    hr = THR_NOTRACE(GetUrlFromDefaultBehaviorName(pchName, achUrl, ARRAY_SIZE(achUrl)));
    if (hr)
        goto Cleanup;

    //
    // found the name in registry. Now make sure host does not implement this behavior
    //

    hr2 = THR_NOTRACE(FindHostBehavior(pchName, pchUrl, NULL, NULL));
    if (S_OK == hr2)
    {
        // in this case we will request the behavior in FindDefaultBehavior2
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // prepare the factory info from url
    //

    hr2 = THR_NOTRACE(CLSIDFromHtmlString(achUrl, &classpec.tagged_union.clsid));
    if (S_OK == hr2)
    {
        //
        // if this is a "CLSID:", ensure it is JIT downloaded and cocreate it's factory;
        //

        classpec.tyspec = TYSPEC_CLSID;

        hr = THR(FaultInIEFeatureHelper(GetHWND(), &classpec, NULL, 0));
        Assert(S_FALSE != hr);
        if (hr)
            goto Cleanup;

        hr = THR(CoCreateInstance(
            classpec.tagged_union.clsid,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehaviorFactory,
            (void **)ppFactory));

        Assert (*ppFactory);
    }
    else
    {
        //
        // if this is a usual URL, expand it and return it
        //

        hr = THR(ExpandUrl(achUrl, cchUrlDownload, pchUrlDownload, NULL));
    }

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::FindDefaultBehavior2
//
//  Synopsis:   lookup in host and iepeers.dll.
//              Here we are looking up for an instance of peer
//
//--------------------------------------------------------------------

HRESULT
CDoc::FindDefaultBehavior2(
    LPTSTR                  pchPeerName,
    LPTSTR                  pchPeerUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    static const CLSID CLSID_DEFAULTPEERFACTORY = {0x3050f4cf, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

    HRESULT hr;

    *ppPeer = NULL;
    
    //
    // try the host
    //

    hr = THR_NOTRACE(FindHostBehavior(pchPeerName, pchPeerUrl, pSite, ppPeer));
    if (S_OK == hr)
        goto Cleanup;       // done;

    //
    // lookup in iepeers.dll
    //

    if (!_fDefaultPeerFactoryEnsured)
    {
        _fDefaultPeerFactoryEnsured = TRUE;

        hr = THR(CoCreateInstance(
            CLSID_DEFAULTPEERFACTORY,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehaviorFactory,
            (void **)&_pDefaultPeerFactory));
        if (hr)
            _pDefaultPeerFactory = NULL;
    }

    if (!_pDefaultPeerFactory)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR_NOTRACE(FindPeer(_pDefaultPeerFactory, pchPeerName, pchPeerUrl, pSite, ppPeer));

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::SetPeersPossible
//
//--------------------------------------------------------------------

void
CDoc::SetPeersPossible()
{
    if (_fPeersPossible)        // if already set
        return;

    if (_dwLoadf & DLCTL_NO_BEHAVIORS)
        return;

    _fPeersPossible = TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::StopPeerFactoriesDownloads
//
//--------------------------------------------------------------------

void
CDoc::StopPeerFactoriesDownloads()
{
    int                 c;
    CPeerFactoryUrl **  ppFactory;

    for (c = _aryPeerFactoriesUrl.Size(), ppFactory = _aryPeerFactoriesUrl;
         c;
         c--, ppFactory++)
    {
        (*ppFactory)->StopBinding();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeersCss
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeersCss(
    CElement *      pElement,
    CAtomTable *    pacstrBehaviorUrls)
{
    HRESULT                 hr = S_OK;

    //
    // control reentrance: if we are in process of attaching peer, do not
    // allow to attach any other peer
    //

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);

        HRESULT                 hr2;
        int                     i, cUrls;
        CPeerHolder *           pPeerHolder;
        LPTSTR                  pchUrl;
        TCHAR                   achUrl[pdlUrlLen];
        CPeerHolder::CListMgr   ListPrev;
        CPeerHolder::CListMgr   ListNew;

        //
        // Pass 1: build new list of peer holders:
        //          -   preserving existing peer holders and peers that go in the new list intact, and
        //          -   creating new peer holders with peers
        //

        TraceTag((tagPeerCDocAttachPeers,
                  "CDoc::AttachPeersCss, tag: %ls, tag id: %ls; peers to be attached:",
                  pElement->TagName(), STRVAL(pElement->GetAAid())));

        cUrls = pacstrBehaviorUrls->Size();

        ListPrev.Init (pElement->DelPeerHolder());
        ListNew.StartBuild (pElement);

        for (i = 0; i < cUrls; i++)
        {
            //
            // handle url
            //

            pchUrl = (LPTSTR)(*pacstrBehaviorUrls)[i];
            if (!pchUrl || 0 == pchUrl[0])  // if empty
                continue;

            if (_T('#') != pchUrl[0])   // if does not begin with '#'
            {                           // then expand it
                IGNORE_HR(ExpandUrl(pchUrl, ARRAY_SIZE(achUrl), achUrl, pElement));
                pchUrl = achUrl;
            }

            TraceTag((tagPeerCDocAttachPeers, "           '%ls'", pchUrl));

            //
            // attempt to find existing peer holder for the url.
            // if found, move it to the new list; otherwise, create a new empty peer holder
            //

            if (ListPrev.Find(pchUrl))
            {
                ListPrev.MoveCurrentToTailOf(&ListNew);
            }
            else
            {
                pPeerHolder = new CPeerHolder(pElement);
                if (!pPeerHolder)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                //
                // AttachPeerCss
                //

                pPeerHolder->SetFlag(CPeerHolder::CSSPEER);

                hr2 = THR_NOTRACE(AttachPeerCss(pPeerHolder, pchUrl));

                if (S_OK == hr2)
                {
                    ListNew.AddToTail(pPeerHolder);
                }
                else
                {
                    // ( BUGBUG (alexz) temporary don't report the error when printing )
                    if (E_ACCESSDENIED == hr2 && !IsPrintDoc())
                    {
                        TCHAR           achMessage[pdlUrlLen + 32];
                        ErrorRecord     errorRecord;

                        Format(0, achMessage, ARRAY_SIZE(achMessage), _T("Access is denied to: <0s>"), pchUrl);
                        errorRecord.Init(hr2, achMessage, _cstrUrl);

                        ReportScriptError(errorRecord);
                    }

                    pPeerHolder->Release();
                }
            }
        }

        //
        // Pass 2: release all css created peer holders left in the old list
        // (and leave host, identity and "attachBehavior" behaviors intact)
        //

        ListPrev.Reset();
        while (!ListPrev.IsEnd())
        {
            if (ListPrev.Current()->IsCssPeer())            // if css peer to save and remove
            {
                ListPrev.DetachCurrent(/*fSave = */TRUE);
            }
            else if (ListPrev.Current()->IsIdentityPeer())  // if identity peer (there can only be one)
            {
                ListPrev.MoveCurrentToHeadOf(&ListNew);
            }
            else                                            // behavior added by addBehavior
            {
                ListPrev.MoveCurrentToTailOf(&ListNew);
            }
        }

        //
        // debug stuff
        //

#if DBG == 1
        if (IsTagEnabled(tagPeerCDocAttachPeersEmptyPH))
        {
            if (!ListNew.Head() || !ListNew.Head()->IsIdentityPeer())
            {
                ListNew.AddToHead(new CPeerHolder(pElement));
            }
            ListNew.AddToTail(new CPeerHolder(pElement));
        }
#endif
        //
        // finalize
        //

        hr = THR(ListNew.DoneBuild());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeer
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeer(
    CElement *              pElement,
    LPTSTR                  pchUrl,
    CPeerFactoryBinary *    pFactoryBinary /* = NULL */,
    LONG *                  pCookie /* = NULL */)
{
    HRESULT                 hr = S_OK;

    if (!pchUrl || 0 == pchUrl[0])  // if empty
        goto Cleanup;

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);

        CPeerHolder *           pPeerHolder;
        TCHAR                   achUrl[pdlUrlLen];
        CPeerHolder::CListMgr   List;

        List.StartBuild (pElement);

        //
        // expand url and check if the peer already exists
        //

        if (_T('#') != pchUrl[0])   // if does not begin with '#'
        {                           // then expand it
            IGNORE_HR(ExpandUrl(pchUrl, ARRAY_SIZE(achUrl), achUrl, pElement));
            pchUrl = achUrl;
        }

        TraceTag((tagPeerCDocAttachPeers,
                  "CDoc::AttachPeer, tag: %ls, tag id: %ls; peer to be attached:",
                  pElement->TagName(), STRVAL(pElement->GetAAid())));
        TraceTag((tagPeerCDocAttachPeers, "           '%ls'", pchUrl));

        if (!List.Find(pchUrl))
        {
            //
            // create peer holder and initiate attaching peer
            //

            pPeerHolder = new CPeerHolder(pElement);
            if (!pPeerHolder)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            if (!pFactoryBinary)
            {
                hr = THR_NOTRACE(AttachPeerCss(pPeerHolder, pchUrl));
            }
            else
            {
                hr = THR_NOTRACE(pFactoryBinary->AttachPeer(pPeerHolder, pchUrl));
            }

            if (S_OK == hr)
            {
                List.AddToTail(pPeerHolder);

                if (pCookie)
                {
                    (*pCookie) = pPeerHolder->CookieID();
                }
            }
            else // if error
            {
                pPeerHolder->Release();
                goto Cleanup;
            }
        } // eo if not found

        //
        // finalize
        //

        hr = THR(List.DoneBuild());
        if (hr)
            goto Cleanup;

    } // eo if lock

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::RemovePeer
//
//--------------------------------------------------------------------

HRESULT
CDoc::RemovePeer(
    CElement *      pElement,
    LONG            cookie,
    VARIANT_BOOL *  pfResult)
{
    HRESULT     hr = E_UNEXPECTED;

    if (!pfResult)
        RRETURN (E_POINTER);

    *pfResult = VB_FALSE;

    if (!pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
    {
        CElement::CLock         lock(pElement, CElement::ELEMENTLOCK_ATTACHPEER);

        CPeerHolder::CListMgr   List;

        List.StartBuild (pElement);

        // search for the cookie

        while (!List.IsEnd())
        {
            if (cookie == List.Current()->CookieID())
            {
                *pfResult = VB_TRUE;
                List.DetachCurrent(/*fSave = */TRUE);
                break;
            }

            List.Step();
        }

        // finalize

        hr = THR(List.DoneBuild());
    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::AttachPeerCss
//
//--------------------------------------------------------------------

HRESULT
CDoc::AttachPeerCss(CPeerHolder * pPeerHolder, LPTSTR pchUrl)
{
    HRESULT             hr;
    int                 idx, cnt;
    BOOL                fClone;
    LPTSTR              pchUrlSecondPound;
    int                 nUrlSecondPound;
    CPeerFactoryUrl *   pFactory = NULL;

    Assert (pchUrl && pchUrl[0]);

    TraceTag((tagPeerCDocAttachPeerCss,
              "CDoc::AttachPeer, attaching peer to tag: %ls, tag id: %ls; url: %ls",
              pPeerHolder->_pElement->TagName(),
              STRVAL(pPeerHolder->_pElement->GetAAid()),
              pchUrl));

    //
    // security check
    // BUGBUG: (anandra) temp check for cdl: and allow all of them through
    //

    if (_T('#') != pchUrl[0] && !_tcsnipre(_T("cdl:"), 4, pchUrl, -1) && !AccessAllowed(pchUrl))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    //
    // try to find an existing factory for this url
    //

    // (for more details see also comments in the beginning of peerfact.cxx file)
    // as a result of execution of the end of this function, _aryPeerFactoriesUrl can be
    // modified in the following way:
    //  - no change may happen, if pchUrl matches _aryPeerFactoriesUrl[i]->_cstrUrl for some i
    //  - new peer factory may be cloned out of _aryPeerFactoriesUrl[i], for operation "#foo" -> "#foo#bar"
    //      (operation "#foo#bar" -> "#foo#zoo" is not possible). The clone factory is inserted before
    //      parent factory, which prevents us from cloning out the same factory in the future
    //  - new peer factory or factories may be created if none of the two above applies
    //

    pchUrlSecondPound = (_T('#') == pchUrl[0]) ? StrChr(pchUrl + 1, _T('#')) : NULL;
    nUrlSecondPound = pchUrlSecondPound ? pchUrlSecondPound - pchUrl : 0;

    fClone = FALSE;

    for (idx = 0, cnt = _aryPeerFactoriesUrl.Size(); idx < cnt; idx++)
    {
        pFactory = _aryPeerFactoriesUrl[idx];

        if (0 == StrCmpIC(pchUrl, pFactory->_cstrUrl))
        {
            break;
        }

        if (nUrlSecondPound &&
            0 == StrCmpNIC(pchUrl, pFactory->_cstrUrl, nUrlSecondPound) &&
            0 == pFactory->_cstrUrl[nUrlSecondPound])
        {
            // found the peer factory "#foo", but need to clone it into a more specific "#foo#cat"
            fClone = TRUE;
            break;
        }
    }

    //
    // create new CPeerFactoryUrl if necessary and/or clone existing if necessary
    //

    if (idx == cnt)
    {
        CStringNullTerminator     nullTerminator(pchUrlSecondPound); // convert url "#foo#bar" to "#foo" temporary

        hr = THR(CPeerFactoryUrl::Create(
            pchUrl, this, pPeerHolder->_pElement->GetMarkup(), &pFactory));
        if (hr)
            goto Cleanup;

        hr = THR(_aryPeerFactoriesUrl.Append(pFactory));
        if (hr)
            goto Cleanup;

        idx = _aryPeerFactoriesUrl.Size() - 1;  // (the idx is also used in cloning codepath)
        fClone = (NULL != pchUrlSecondPound);   // request cloning only if necessary

    }

    if (fClone)
    {
        // clone existing peer factory into more specific factory and insert it *before* less specific

        hr = THR(pFactory->Clone(pchUrl, &pFactory));
        if (hr)
            goto Cleanup;

        hr = THR(_aryPeerFactoriesUrl.Insert(idx, pFactory));
        if (hr)
            goto Cleanup;
    }

    //
    // now attach
    //

    Assert (pFactory);

    hr = THR_NOTRACE(pFactory->AttachPeer(pPeerHolder));

Cleanup:

    RRETURN1 (hr, S_ASYNCHRONOUS);
}

#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureIdentityFactory
//
//-------------------------------------------------------------------------

HRESULT
CDoc::EnsureIdentityFactory(LPTSTR pchCodebase /* = NULL */)
{
    HRESULT                    hr = S_OK;
    CIdentityPeerFactoryUrl *  pFactory = NULL;
    IIdentityBehaviorFactory * pIFactory = NULL;
    
    if (_fHasIdentityPeerFactory)
    {
#ifdef DEBUG
        if (pchCodebase)
            Assert(0 == StrCmpIC(_cstrIdentityFactoryUrl, pchCodebase));
#endif //DEBUG
        goto Cleanup;
    }

    Assert(_aryPeerFactoriesUrl.Size() == 0);

    pFactory = new CIdentityPeerFactoryUrl(this);
    if (!pFactory)
        goto Cleanup;
    
    hr = THR(_aryPeerFactoriesUrl.Append(pFactory));
    if (hr)
        goto Cleanup;

    // Make sure that the host hasn't already provided one.
    if (!_fHasHostIdentityPeerFactory)
    {
        Assert(pchCodebase);
        _cstrIdentityFactoryUrl.Set(pchCodebase);
        
        hr = THR(((CPeerFactoryUrl *)pFactory)->Init((LPTSTR)_cstrIdentityFactoryUrl)); // this will launch download if necessary
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert(_pHostPeerFactory);
        hr = THR(_pHostPeerFactory->QueryInterface(IID_IIdentityBehaviorFactory,
                    (void **)&pIFactory));
        if (hr)
            goto Cleanup;
        
        if (pIFactory)
        {
            hr = THR(pFactory->Init(pIFactory));
            if (hr)
                goto Cleanup;
        }
    }

    _fHasIdentityPeerFactory = TRUE;
    SetPeersPossible();

Cleanup:

    ReleaseInterface(pIFactory);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetIdentityFactory
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetIdentityFactory(IIdentityBehaviorFactory **ppFactory)
{
    HRESULT                    hr = S_OK;
    IIdentityBehaviorFactory * pFactory = NULL;
    IUnknown *                 pUnkFactory = NULL;

    if (!_fHasIdentityPeerFactory)
        goto Cleanup;
    
    pUnkFactory = _aryPeerFactoriesUrl[0]->_pFactory;
    if (pUnkFactory)
    {
        hr = pUnkFactory->QueryInterface(IID_IIdentityBehaviorFactory, (void **)&pFactory);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    *ppFactory = pFactory;

    RRETURN(pFactory ? S_OK : E_FAIL);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetBaseTagsFromFactory
//
//  Synopsis:   Get the intrinsic tag info from the identity factory
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetBaseTagsFromFactory(LPTSTR pchCodebase /* = NULL */)
{
    HRESULT                    hr = S_OK;
    IIdentityBehaviorFactory * pFactory = NULL;
    BSTR *                     rgbstrTagNames = NULL;
    ELEMENT_TAG *              rgBaseTagIds = NULL;
    INT                        cTags = 0;
    INT                        iTag;
    CHtmCtx *                  pCtx = NULL;
    
    hr = THR(EnsureIdentityFactory(pchCodebase));
    if (hr)
        goto Cleanup;
    
    hr = THR(GetIdentityFactory(&pFactory));
    if (hr)
        goto Cleanup;

    hr = THR(pFactory->GetBaseTags((INT *)&rgbstrTagNames, (INT *)&rgBaseTagIds, &cTags));
    if (hr)
        goto Cleanup;
    
    for (iTag = 0; iTag < cTags; ++iTag)
        rgBaseTagIds[iTag] = ETagFromTagId((ELEMENT_TAG_ID)rgBaseTagIds[iTag]);

    pCtx = HtmCtx();
    Assert(pCtx);
  
    hr = THR(pCtx->AddTagsources(rgbstrTagNames, (ELEMENT_TAG *)rgBaseTagIds, NULL, cTags));

Cleanup:
    
    if (rgbstrTagNames)
    {
        for (iTag = 0; iTag < cTags; ++iTag)
            SysFreeString(rgbstrTagNames[iTag]);
        CoTaskMemFree(rgbstrTagNames);
    }

    if (rgBaseTagIds)
        CoTaskMemFree(rgBaseTagIds);

    ReleaseInterface(pFactory);

    RRETURN(hr);
}
#endif //VSTUDIO7

//+-------------------------------------------------------------------
//
//  Member:     CDoc::PeerEnqueueTask
//
//--------------------------------------------------------------------

HRESULT
CDoc::PeerEnqueueTask(CBase * pTarget, PEERTASK task)
{
    HRESULT             hr = S_OK;
    CPeerQueueItem *    pQueueItem;

    //
    // post message to commit, if not posted yet (and not unloading)
    //

    if (0 == _aryPeerQueue.Size() && !TestLock(FORMLOCK_UNLOADING))
    {
        IProgSink * pProgSink;

        hr = THR(GWPostMethodCall(
            this, ONCALL_METHOD(CDoc, PeerDequeueTasks, peerdequeuetasks),
            0, FALSE, "CDoc::PeerDequeueTasks"));
        if (hr)
            goto Cleanup;

        pProgSink = GetProgSink();

        Assert (0 == _dwPeerQueueProgressCookie);

        if (pProgSink)
        {
            pProgSink->AddProgress (PROGSINK_CLASS_CONTROL, &_dwPeerQueueProgressCookie);
        }
    }

    //
    // add to the queue
    //

    pQueueItem = _aryPeerQueue.Append();
    if (!pQueueItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pQueueItem->Init(pTarget, task);

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::PeerDequeueTasks
//
//--------------------------------------------------------------------

void
CDoc::PeerDequeueTasks(DWORD_PTR)
{
    int                 i;
    ULONG               cDie;
    CBase *             pTarget;
    CPeerQueueItem *    pQueueItem;

    //
    // go up to interactive state if necessary
    //

    // if need to be inplace active in order execute scripts
    if (!TestLock(FORMLOCK_UNLOADING) &&
        _fNeedInPlaceActivation &&
        (_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) &&
        State() < OS_INPLACE)
    {
        // request in-place activation by going interactive
        SetInteractive(0);

        // and wait until activated - the activation codepath will call PeerDequeueTasks again
        goto Cleanup;
    }

    //
    // dequeue all the elements in queue
    //

    // note that the queue might get blown away while we are in this loop.
    // this can happen in CDoc::UnloadContents, resulted from document.write, executed
    // during creation of a behavior (e.g. in inline script of a scriptlet). We use cDie
    // to be robust for this reentrance.

    cDie = _cDie;

    for (i = 0; i < _aryPeerQueue.Size() && cDie == _cDie; i++)
    {
        pQueueItem = &_aryPeerQueue[i];

        pTarget = pQueueItem->_pTarget;

        if (!pTarget)                  // if we have re-entered the method, bail out
            goto Cleanup;

        pQueueItem->_pTarget = NULL;

        if (PEERTASK_ELEMENT_FIRST <= pQueueItem->_task && pQueueItem->_task <= PEERTASK_ELEMENT_LAST)
        {
            IGNORE_HR(DYNCAST(CElement, pTarget)->ProcessPeerTask(pQueueItem->_task));
        }
        else
        {
            Assert (PEERTASK_MARKUP_FIRST <= pQueueItem->_task && pQueueItem->_task <= PEERTASK_MARKUP_LAST);

            IGNORE_HR(DYNCAST(CMarkup, pTarget)->ProcessPeerTask(pQueueItem->_task));
        }
    }

    _aryPeerQueue.DeleteAll();

    //
    // remove progress
    //

    if (0 != _dwPeerQueueProgressCookie)
    {
        IProgSink * pProgSink = GetProgSink();

        //
        // progsink must have been available in order to AddProgress return _dwPeerQueueProgressCookie.
        // Now it can be released only when the doc is unloading (progsink living on tree is destroyed
        // before we dequeue the queue)
        //
        Assert (pProgSink || TestLock(FORMLOCK_UNLOADING));

        if (pProgSink)
        {
            pProgSink->DelProgress (_dwPeerQueueProgressCookie);
        }

        _dwPeerQueueProgressCookie = 0;
    }

Cleanup:
    return;
}
