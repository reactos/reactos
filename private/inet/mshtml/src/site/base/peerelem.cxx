#include "headers.hxx"

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#define _cxx_
#include "urncoll.hdl"

//////////////////////////////////////////////////////////////////////////////
//
// misc
//
//////////////////////////////////////////////////////////////////////////////

DeclareTag(tagPeerCDocProcessPeerTask,  "Peer", "trace CElement::ProcessPeerTask queueing");
DeclareTag(tagPeerIncDecPeerDownloads,      "Peer", "trace CElement::IncPeerDownloads and CElement::DecPeerDownloads");

//////////////////////////////////////////////////////////////////////////////
//
// CElement - peer hosting code
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member: CElement::ProcessPeerTask
//
//----------------------------------------------------------------------------

HRESULT
CElement::ProcessPeerTask(PEERTASK task)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();
    CPeerMgr *      pPeerMgr;

    switch (task)
    {

    case PEERTASK_ENTERTREE_UNSTABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_ENTERTREE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            //
            // optimizations
            //

            BOOL fRequestIdentityPeer = (!HasIdentityPeer() && NeedsIdentityPeer());

            if (!pDoc->_fPeersPossible && !fRequestIdentityPeer)
                goto Cleanup;

            // don't process the task when parsing auxilary markup - the element will be 
            // spliced into target markup later and that is when we process the same task
            if (pDoc->_fMarkupServicesParsing)
                goto Cleanup; // done
        
            pPeerMgr = GetPeerMgr();

            if (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending())
                goto Cleanup; // done

            {
                CBehaviorInfo   info(GetFirstBranch());

                if (!fRequestIdentityPeer)
                {
                    hr = THR(ApplyBehaviorCss(&info));
                    if (hr)
                        goto Cleanup;
                }
                // else
                //     (we don't need css - because identity behavior requested, we will queue the element up anyway)

                if (fRequestIdentityPeer ||
                    0 < info._acstrBehaviorUrls.Size() /*||
                    (HasPeerHolder() && GetPeerHolder()->TestFlagMulti(CPeerHolder::NEEDDOCUMENTCONTEXTCHANGE))*/)
                {
                    //
                    // queue up for recomputing the behaviors at a safe moment,
                    // as well as sending documentContextChange notification
                    //

                    // trace output
#if DBG == 1
                    {
                        int i, c;
                        TraceTag((tagPeerCDocProcessPeerTask, "CElement::ProcessPeerTask, tag: %ls, tag id: %ls; queueing up to attach behaviors:", TagName(), STRVAL(GetAAid())));
                        if (fRequestIdentityPeer)
                            TraceTag((tagPeerCDocProcessPeerTask, "         '<%ls>'", STRVAL(TagName())));
                        else
                        {
                            for (i = 0, c = info._acstrBehaviorUrls.Size(); i < c; i++)
                            {
                                TraceTag((tagPeerCDocProcessPeerTask, "            '%ls'", STRVAL(info._acstrBehaviorUrls[i])));
                            }
                        }
                    }
#endif
                    hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                    if (hr)
                        goto Cleanup;

                    pPeerMgr->SetEnterExitTreeStablePending(TRUE);

                    AddRef();

                    IGNORE_HR(pDoc->PeerEnqueueTask(this, PEERTASK_ENTEREXITTREE_STABLE));
                }

                break;
            }
        }
        break;


    case PEERTASK_EXITTREE_UNSTABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_EXITTREE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            if (HasPeerHolder())
            {
                pPeerMgr = GetPeerMgr();

                if (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending())
                    goto Cleanup; // done

                hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                if (hr)
                    goto Cleanup;

                pPeerMgr->SetEnterExitTreeStablePending(TRUE);

                AddRef();

                IGNORE_HR(pDoc->PeerEnqueueTask(this, PEERTASK_ENTEREXITTREE_STABLE));
            }
        }
        break;


    case PEERTASK_ENTEREXITTREE_STABLE:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_ENTEREXITTREE_STABLE
            //
            //----------------------------------------------------------------------------

            CBehaviorInfo   info(GetFirstBranch());

            // update peer mgr state

            pPeerMgr = GetPeerMgr();
            Assert (pPeerMgr && pPeerMgr->IsEnterExitTreeStablePending());

            pPeerMgr->SetEnterExitTreeStablePending(FALSE);
            CPeerMgr::EnsureDeletePeerMgr(this);

            // ensure identity peer (this should happen regardless of if it is in markup or not)

            if (!pDoc->TestLock(FORMLOCK_UNLOADING))
            {
                hr = THR(EnsureIdentityPeer());
                if (hr)
                    goto Cleanup;
            }

            //
            // process css peers
            //

            if (IsInMarkup())
            {
                //
                // in the tree now
                //

                if (!pDoc->TestLock(FORMLOCK_UNLOADING))
                {
                    hr = THR(ApplyBehaviorCss(&info));
                    if (hr)
                        goto Cleanup;

                    IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));

                    if (HasPeerHolder())
                    {
                        GetPeerHolder()->NotifyMulti(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE);
                    }
                }

                Release(); // NOTE that this element may passivate after this release
            }
            else
            {
                //
                // out of the tree now
                //

                ULONG ulElementRefs;

                Assert (!IsInMarkup());

                if (HasPeerHolder())
                {
                    GetPeerHolder()->NotifyMulti(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE);
                }

                if (!pDoc->TestLock(FORMLOCK_UNLOADING))
                {
                    hr = THR(ApplyBehaviorCss(&info));
                    if (hr)
                        goto Cleanup;

                    // because 0 == info._acstrBehaviorUrls, this will cause all css-attached behaviors to be removed
                    IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));
                }
        
                ulElementRefs = GetObjectRefs() - 1;        // (can't use return value of Release())
                                                            // this should happen after AttachPeersCss
                Release();                                  // NOTE that this element may passivate after this release

                if (ulElementRefs && HasPeerHolder())    // if still has not passivated and has peers
                {                                           // then there is risk of refcount loops
                    if (pDoc->TestLock(FORMLOCK_UNLOADING))
                    {
                        DelPeerHolder()->Release();      // delete the ptr and release peer holders,
                    }                                       // thus breaking possible loops right here
                    else
                    {
                        pDoc->RequestReleaseNotify(this);   // defer breaking refcount loops (until CDoc::UnloadContents)
                    }
                }
            }
        }
        break;


    case PEERTASK_RECOMPUTEBEHAVIORS:
        {
            //+---------------------------------------------------------------------------
            //
            // PEERTASK_RECOMPUTEBEHAVIORS
            //
            //----------------------------------------------------------------------------

            CBehaviorInfo   info(GetFirstBranch());

            hr = THR(ApplyBehaviorCss(&info));
            if (hr)
                goto Cleanup;

            IGNORE_HR(pDoc->AttachPeersCss(this, &info._acstrBehaviorUrls));
        }
        break;

    case PEERTASK_APPLYSTYLE_UNSTABLE:

            //+---------------------------------------------------------------------------
            //
            // PEERTASK_APPLYSTYLE_UNSTABLE
            //
            //----------------------------------------------------------------------------

            if (HasPeerHolder())
            {
                // don't post the task if already did that; update peer mgr state

                pPeerMgr = GetPeerMgr();

                if (pPeerMgr && pPeerMgr->IsApplyStyleStablePending())
                    goto Cleanup; // done

                hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
                if (hr)
                    goto Cleanup;

                pPeerMgr->SetApplyStyleStablePending(TRUE);

                // post the task

                AddRef();

                IGNORE_HR(pDoc->PeerEnqueueTask(this, PEERTASK_APPLYSTYLE_STABLE));
            }
            break;

            
    case PEERTASK_APPLYSTYLE_STABLE:

            //+---------------------------------------------------------------------------
            //
            // PEERTASK_APPLYSTYLE_STABLE
            //
            //----------------------------------------------------------------------------

            // update peer mgr state

            pPeerMgr = GetPeerMgr();
            Assert (pPeerMgr && pPeerMgr->IsApplyStyleStablePending());

            pPeerMgr->SetApplyStyleStablePending(FALSE);
            // don't do "CPeerMgr::EnsureDeletePeerMgr(this)" to avoid frequent memallocs

            // do the notification

            if (HasPeerHolder())
            {
                //::StartCAP();
                IGNORE_HR(GetPeerHolder()->ApplyStyleMulti());
                //::StopCAP();
            }

            Release(); // NOTE that this element may passivate after this release

            break;


    } // eo switch (task)

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CElement::ApplyBehaviorCss
//
//--------------------------------------------------------------------

HRESULT
CElement::ApplyBehaviorCss(CBehaviorInfo * pInfo)
{
    HRESULT      hr = S_OK;
    CAttrArray * pInLineStyleAA;

    // NOTE per rules of css application, inline styles take precedence so they should be applied last

    if (IsInMarkup())
    {
        // apply markup rules
        hr = THR(GetMarkup()->ApplyStyleSheets(pInfo, APPLY_Behavior));
        if (hr)
            goto Cleanup;
    }
    
    // apply inline style rules
    pInLineStyleAA = GetInLineStyleAttrArray();
    if (pInLineStyleAA)
    {
        hr = THR(ApplyAttrArrayValues(pInfo, &pInLineStyleAA, NULL, APPLY_Behavior));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CElement::addBehavior
//
//----------------------------------------------------------------------------

HRESULT
CElement::addBehavior(BSTR bstrUrl, VARIANT * pvarFactory, LONG * pCookie)
{
    HRESULT                 hr;
    CPeerFactoryBinary      factory;
    CPeerFactoryBinary *    pFactory = NULL;
    LONG                    lCookieTemp;

    //
    // startup
    //

    if (!bstrUrl)
        RRETURN (E_INVALIDARG);

    if (!pCookie)
    {
        pCookie = &lCookieTemp;
    }

    *pCookie = 0;

    //
    // extract factory if any
    //

    if (pvarFactory)
    {
        HRESULT hr2;

        // dereference
        if (V_VT(pvarFactory) & VT_BYREF)
            pvarFactory = (VARIANT*) V_BYREF(pvarFactory);

        // get IElementBehaviorFactory
        if (pvarFactory &&
            (VT_UNKNOWN  == V_VT(pvarFactory) ||
             VT_DISPATCH == V_VT(pvarFactory)))
        {
            hr2 = THR_NOTRACE(V_UNKNOWN(pvarFactory)->QueryInterface(
                IID_IElementBehaviorFactory, (void**)&factory._pFactory));
            if (S_OK == hr2)
            {
                pFactory = &factory;
            }
        }
    }

    //
    // attach
    //

    hr = THR(Doc()->AttachPeer(this, bstrUrl, pFactory, pCookie));

    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member: CElement::removeBehavior
//
//----------------------------------------------------------------------------

HRESULT
CElement::removeBehavior(LONG cookie, VARIANT_BOOL * pfResult)
{
    HRESULT     hr;

    hr = THR(Doc()->RemovePeer(this, cookie, pfResult));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::NeedsIdentityPeer
//
//----------------------------------------------------------------------------

BOOL
CElement::NeedsIdentityPeer()
{
    //
    // builtin or literal?
    //

    //
    // BUGBUG (alexz)   currently if the tag is generic literal tag it implies that it has
    //                  identity behavior (XML behavior in particular). This will not be true
    //                  when we fully support literally parsed sprinkles: there will be literal
    //                  tags that do not have identity behaviors.
    //

    switch (Tag())
    {
    case ETAG_GENERIC_BUILTIN:
    case ETAG_GENERIC_LITERAL:
        return TRUE;
    }

    //
    // needs vsforms identity behavior?
    //

#ifdef VSTUDIO7
    if (NeedsVStudioPeer())
        return TRUE;
#endif //VSTUDIO7

    return FALSE;
}

#ifdef VSTUDIO7
//+---------------------------------------------------------------------------
//
//  Member:     CElement::NeedsVStudioPeer
//
//----------------------------------------------------------------------------

BOOL
CElement::NeedsVStudioPeer()
{
    if (Doc()->_fHasIdentityPeerFactory)
    {
        LPCTSTR pchNamespace = Namespace();
        // Add only intrinsics with ID or non-intrinsics to the
        // Identity peer queue.
        if ((StrCmp(pchNamespace, _T("WFC")) == 0) ||
            (!pchNamespace && GetAAid()))
        {
            return TRUE;
        }
    }

    return FALSE;
}
#endif //VSTUDIO7

//+------------------------------------------------------------------------
//
//  Member:     CElement::EnsureIdentityPeer()
//
//-------------------------------------------------------------------------
HRESULT
CElement::EnsureIdentityPeer(BOOL fForceCreate /* = FALSE */)
{
    HRESULT                 hr;
    LPTSTR                  pchName;
    LPTSTR                  pchColon;
    CPeerFactoryBuiltin     factory;
    CPeerFactory *          pFactory;
    CPeerHolder *           pPeerHolder = NULL;
#ifdef VSTUDIO7
    TCHAR                   achPeerName[512];
#endif //VSTUDIO7

    //
    // NOTE In the startup of this method use "RRETURN" instead of "goto Cleanup" and
    // don't initialize any local vars here so to keep this function maximally fast
    // in the most common case when no identity behaviors created for the element.
    // Reason: this function is called for every element when parsing or recomputing css.
    //

#ifdef VSTUDIO7
    if (TestLock(CElement::ELEMENTLOCK_ATTACHPEER))
        return S_OK;
#endif //VSTUDIO7

    if (HasIdentityPeer())
        return S_OK;

    //
    // bail out if no identity behavior needs to be created
    //
    
#ifdef VSTUDIO7
    if (fForceCreate || NeedsVStudioPeer())
    {
        // BUGBUG "(CPeerFactory*) Doc()->_aryPeerFactoriesUrl[0]" should be replaced with "pDoc->GetIdentityPeerFactory()"
        pFactory = DYNCAST(CIdentityPeerFactoryUrl, Doc()->_aryPeerFactoriesUrl[0]);
        
        hr = DYNCAST(CIdentityPeerFactoryUrl, pFactory)->GetPeerName(this, achPeerName, ARRAY_SIZE(achPeerName));
        if (hr)
            RRETURN (hr);
        pchName = achPeerName;
    }
    else
#endif //VSTUDIO7
    {
        if (Tag() != ETAG_GENERIC_BUILTIN &&
            Tag() != ETAG_GENERIC_LITERAL)
            return S_OK;
        
        pchName  = (LPTSTR)TagName();
        pchColon = StrChr(pchName, _T(':'));
        if (pchColon)
            pchName = pchColon + 1;

        factory._pTagDesc = GetBuiltinGenericTag(pchName);
        if (!factory._pTagDesc ||                                               // if not a builtin tag, or
            (CBuiltinGenericTagDesc::TYPE_HTC == factory._pTagDesc->type &&     // generic HTC tag with no behavior
             HTC_BEHAVIOR_NONE == factory._pTagDesc->extraInfo.htcBehaviorType))
        {
            return S_OK;
        }

        pFactory = &factory;
    }

    //
    // create now
    //

    pPeerHolder = new CPeerHolder(this);
    if (!pPeerHolder)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
#ifdef VSTUDIO7
        CElement::CLock lock(this, CElement::ELEMENTLOCK_ATTACHPEER);
#endif //VSTUDIO7

        hr = THR(pPeerHolder->Create(pchName, pFactory));
        if (hr)
            goto Cleanup;
    
        pPeerHolder->SetFlag(CPeerHolder::IDENTITYPEER);
        CPeerHolder::CListMgr::AttachPeerHolderToElement(pPeerHolder, this);
    }

Cleanup:
    if (hr && pPeerHolder)
        pPeerHolder->Release();

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasIdentityPeer
//
//----------------------------------------------------------------------------

BOOL
CElement::HasIdentityPeer()
{
    CPeerHolder *pPeerHolder = GetPeerHolder();

    if (!pPeerHolder)
        return FALSE;

#if DBG == 1
    pPeerHolder->AssertCorrectIdentityPeerHolder();
#endif

    if (pPeerHolder->IsIdentityPeer())
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetIdentityPeerHolder
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetIdentityPeerHolder()
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();

    if (!pPeerHolder)
        return NULL;

#if DBG == 1
    pPeerHolder->AssertCorrectIdentityPeerHolder();
#endif

    if (pPeerHolder->IsIdentityPeer())
        return pPeerHolder;

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasPeerWithUrn
//
//----------------------------------------------------------------------------

BOOL
CElement::HasPeerWithUrn(LPCTSTR Urn)
{
    CPeerHolder::CListMgr   List;

    List.Init(GetPeerHolder());

    return List.HasPeerWithUrn(Urn);
}

//+------------------------------------------------------------------------------
//
//  Member:     get_behaviorUrns
//
//  Synopsis:   returns the urn collection for the behaviors attached to this element
//      
//+------------------------------------------------------------------------------

HRESULT
CElement::get_behaviorUrns(IDispatch ** ppDispUrns)
{
    HRESULT                 hr = S_OK;
    CPeerUrnCollection *    pCollection = NULL;

    if (!ppDispUrns)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pCollection = new CPeerUrnCollection(this);
    if (!pCollection)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pCollection->QueryInterface (IID_IDispatch, (void **) ppDispUrns));
    if ( hr )
        goto Cleanup;

Cleanup:
    if (pCollection)
        pCollection->Release();

    RRETURN(SetErrorInfoPGet(hr, DISPID_CElement_behaviorUrns));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::PutUrnAtom, helper
//
//----------------------------------------------------------------------------

HRESULT
CElement::PutUrnAtom (LONG urnAtom)
{
    HRESULT     hr;

    Assert (-1 != urnAtom);

    hr = THR(AddSimple(DISPID_A_URNATOM, urnAtom, CAttrValue::AA_Internal));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetUrnAtom, helper
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetUrnAtom(LONG * pAtom)
{
    HRESULT         hr = S_OK;
    AAINDEX         aaIdx;

    Assert (pAtom);

    aaIdx = FindAAIndex(DISPID_A_URNATOM, CAttrValue::AA_Internal);
    if (AA_IDX_UNKNOWN != aaIdx)
    {
        hr = THR(GetSimpleAt(aaIdx, (DWORD*)pAtom));
    }
    else
    {
        *pAtom = -1;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetUrn(LPTSTR * ppchUrn)
{
    HRESULT     hr;
    LONG        urnAtom;

    Assert (ppchUrn);

    hr = THR(GetUrnAtom(&urnAtom));
    if (hr)
        goto Cleanup;

    if (-1 != urnAtom)
    {
        Assert (Doc()->_pXmlUrnAtomTable);      // should have been ensured when we stored urnAtom

        hr = THR(Doc()->_pXmlUrnAtomTable->GetUrn(urnAtom, ppchUrn));
    }
    else
    {
        *ppchUrn = NULL;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::put_tagUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::put_tagUrn (BSTR bstrUrn)
{
    HRESULT             hr;
    LPCTSTR             pchNamespace = Namespace();
    CXmlUrnAtomTable *  pUrnAtomTable;
    LONG                urnAtom;

    if (!pchNamespace)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(Doc()->EnsureXmlUrnAtomTable(&pUrnAtomTable));
    if (hr)
        goto Cleanup;

    hr = THR(pUrnAtomTable->EnsureUrnAtom(bstrUrn, &urnAtom));
    if (hr)
        goto Cleanup;

    hr = THR(PutUrnAtom(urnAtom));

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::get_tagUrn, per IHTMLElement2
//
//----------------------------------------------------------------------------

HRESULT
CElement::get_tagUrn(BSTR * pbstrUrn)
{
    HRESULT             hr = S_OK;
    LPCTSTR             pchNamespace = Namespace();
    LPTSTR              pchUrn = NULL;

    if (!pbstrUrn)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pchNamespace)
    {
        hr = THR(GetUrn(&pchUrn));
        if (hr)
            goto Cleanup;
    }

    if (pchUrn)
    {
        hr = THR(FormsAllocString(pchUrn, pbstrUrn));
    }
    else
    {
        // CONSIDER: (anandra, alexz) return urn of html
        *pbstrUrn = NULL;
    }

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetReadyState
//
//----------------------------------------------------------------------------

long
CElement::GetReadyState()
{
    CPeerMgr       *pPeerMgr = GetPeerMgr();

    if (pPeerMgr)
    {
        return pPeerMgr->_readyState;
    }
    else
    {
        return READYSTATE_COMPLETE;
    } 
}


HRESULT
CElement::get_readyStateValue(long *plRetValue)
{
    HRESULT     hr = S_OK;

    if (!plRetValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plRetValue = READYSTATE_COMPLETE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CElement::OnReadyStateChange()
{
    if (ElementDesc()->TestFlag(ELEMENTDESC_OLESITE))
    {
        COleSite::OLESITE_TAG olesiteTag = DYNCAST(COleSite, this)->OlesiteTag();

        if (olesiteTag == COleSite::OSTAG_ACTIVEX || olesiteTag == COleSite::OSTAG_APPLET)
        {
            DYNCAST(CObjectElement, this)->Fire_onreadystatechange();

            return; // done
        }
    }

    Fire_onreadystatechange();
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement:get_readyState
//
//+------------------------------------------------------------------------------

HRESULT
CElement::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Doc()->PeerDequeueTasks(0);

    hr = THR(s_enumdeschtmlReadyState.StringFromEnum(GetReadyState(), &V_BSTR(pVarRes)));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::IncPeerDownloads
//
//----------------------------------------------------------------------------

void
CElement::IncPeerDownloads()
{
    HRESULT     hr;
    CPeerMgr *  pPeerMgr;

#if DBG==1
    if (IsTagEnabled(tagPeerIncDecPeerDownloads))
    {
        TraceTag((0, "CElement::IncPeerDownloads on <%ls> element (SSN = %ld, %lx)", TagName(), SN(), this));
        TraceCallers(0, 0, 12);
    }
#endif

    hr = THR(CPeerMgr::EnsurePeerMgr(this, &pPeerMgr));
    if (hr)
        goto Cleanup;

    pPeerMgr->IncPeerDownloads();
    pPeerMgr->UpdateReadyState();

Cleanup:
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DecPeerDownloads
//
//----------------------------------------------------------------------------

void
CElement::DecPeerDownloads()
{
    CPeerMgr *  pPeerMgr = GetPeerMgr();

#if DBG==1
    if (IsTagEnabled(tagPeerIncDecPeerDownloads))
    {
        TraceTag((0, "CElement::DecPeerDownloads on <%ls> element (SSN = %ld, %lx)", TagName(), SN(), this));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert (pPeerMgr);
    if (!pPeerMgr)
        return;

    pPeerMgr->DecPeerDownloads();
    pPeerMgr->UpdateReadyState();

    CPeerMgr::EnsureDeletePeerMgr(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::OnPeerListChange
//
//----------------------------------------------------------------------------

HRESULT
CElement::OnPeerListChange()
{
    HRESULT         hr = S_OK;

    CPeerMgr::UpdateReadyState(this);

#ifdef PEER_NOTIFICATIONS_AFTER_INIT
    if (HasPeerHolder())
    {
        IGNORE_HR(GetPeerHolder()->EnsureNotificationsSentMulti());
    }
#endif

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetRenderPeerHolder
//
//----------------------------------------------------------------------------

CPeerHolder *
CElement::GetRenderPeerHolder()
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();
    return pPeerHolder ? pPeerHolder->GetRenderPeerHolder() : NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasRenderPeerHolder
//
//----------------------------------------------------------------------------

BOOL
CElement::HasRenderPeerHolder()
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();
    return pPeerHolder && pPeerHolder->GetRenderPeerHolder() ? TRUE : FALSE;
}
