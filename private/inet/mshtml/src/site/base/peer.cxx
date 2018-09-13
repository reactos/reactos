//+---------------------------------------------------------------------
//
//  File:       peer.cxx
//
//  Contents:   peer holder
//
//  Classes:    CPeerHolder
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CPeerHolder, Elements, "CPeerHolder")
MtDefine(CPeerHolder_CEventsBag, Elements, "CPeerHolder::CEventsBag")
MtDefine(CPeerHolder_CEventsBag_aryEvents, Elements, "CPeerHolder::CEventsBag::_aryEvents")
MtDefine(CPeerHolder_CRenderBag, Elements, "CPeerHolder::CRenderBag")

DeclareTag(tagPeerAttach,       "Peer", "trace CPeerHolder::AttachPeer")
DeclareTag(tagPeerPassivate,    "Peer", "trace CPeerHolder::Passivate")
DeclareTag(tagPeerNoOM,         "Peer", "Don't expose OM of peers (fail CPeerHolder::EnsureDispatch)")
DeclareTag(tagPeerNoRendering,  "Peer", "Don't allow peers to do drawing (disable CPeerHolder::Draw)")
DeclareTag(tagPeerNoHitTesting, "Peer", "Don't allow peers to do hit testing (clear hit testing bit in CPeerHolder::InitRender)")
DeclareTag(tagPeerApplyStyle,   "Peer", "trace CPeerHolder::ApplyStyleMulti")
ExternTag(tagDisableLockAR);

// implemented in olesite.cxx
extern HRESULT
InvokeDispatchWithNoThis (
    IDispatch *         pDisp,
    DISPID              dispidMember,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo);

//+---------------------------------------------------------------------------
//
//  Helper function:    GetNextUniquePeerNumber
//
//----------------------------------------------------------------------------

DWORD
GetNextUniquePeerNumber(CElement * pElement)
{
    HRESULT         hr;
    DWORD           n = 0;
    AAINDEX         aaIdx;

    // get previous number

    aaIdx = pElement->FindAAIndex(DISPID_A_UNIQUEPEERNUMBER, CAttrValue::AA_Internal);
    if (AA_IDX_UNKNOWN != aaIdx)
    {
        hr = THR_NOTRACE(pElement->GetSimpleAt(aaIdx, &n));
    }

    // increment it

    n++;
    // (the very first access here will return '1')

    // store it

    IGNORE_HR(pElement->AddSimple(DISPID_A_UNIQUEPEERNUMBER, n, CAttrValue::AA_Internal));

    return n;
}

//////////////////////////////////////////////////////////////////////////////
//
// CPeerHolder::CListMgr methods
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr constructor
//
//----------------------------------------------------------------------------

CPeerHolder::CListMgr::CListMgr()
{
    _pHead     = NULL;
    _pCurrent  = NULL;
    _pPrevious = NULL;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Init
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::Init(CPeerHolder * pPeerHolder)
{
    HRESULT     hr = S_OK;

    _pHead    = pPeerHolder;
    _pCurrent = _pHead;
    Assert (NULL == _pPrevious);

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::StartBuild
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::StartBuild(CElement * pElement)
{
    HRESULT     hr;

    _pElement = pElement;

    hr = THR(Init(_pElement->DelPeerHolder()));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::DoneBuild
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::DoneBuild()
{
    HRESULT     hr;

    Assert (_pElement);

    // assert that we are exclusively building peer holder list for the element
    Assert (!_pElement->HasPeerHolder());

    if (!IsEmpty())
    {
        hr = THR(_pElement->SetPeerHolder(Head()));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pElement->OnPeerListChange());

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::AddTo
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::AddTo(CPeerHolder * pItem, BOOL fAddToHead)
{
    if (!_pHead)
    {
        _pHead = _pCurrent = pItem;
        Assert (!_pPrevious);
    }
    else
    {
        if (fAddToHead)
        {
            // add to head

            pItem->_pPeerHolderNext = _pHead;

            _pPrevious = NULL;
            _pHead = _pCurrent = pItem;
        }
        else
        {
            // add to tail

            // if not at the end of the list, move there
            if (!_pCurrent || _pCurrent->_pPeerHolderNext)
            {
                Assert(_pHead);
                
                for (_pCurrent = _pHead; 
                     _pCurrent->_pPeerHolderNext; 
                     _pCurrent = _pCurrent->_pPeerHolderNext)
                     ;
            }
            
            // add
            _pPrevious = _pCurrent;
            _pCurrent  = pItem;
            _pPrevious->_pPeerHolderNext = _pCurrent;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Reset
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::Reset()
{
    _pPrevious = NULL;
    _pCurrent = _pHead;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Step
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::Step()
{
    Assert (!IsEnd());

    _pPrevious = _pCurrent;
    _pCurrent = _pCurrent->_pPeerHolderNext;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::Find
//
//----------------------------------------------------------------------------

BOOL
CPeerHolder::CListMgr::Find(LPTSTR pchUrl)
{
    Assert (pchUrl);
    Reset();
    while  (!IsEnd())
    {
        if (_pCurrent->_pPeerFactoryUrl &&
            0 == StrCmpIC(_pCurrent->_pPeerFactoryUrl->_cstrUrl, pchUrl))
        {
            return TRUE;
        }
        Step();
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::MoveCurrentTo
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::MoveCurrentTo(CListMgr * pTargetList, BOOL fHead, BOOL fSave /* = FALSE */)
{
    CPeerHolder * pNext;

    Assert (!IsEmpty() && !IsEnd());

    pNext = _pCurrent->_pPeerHolderNext;

    _pCurrent->_pPeerHolderNext = NULL;             // disconnect current from this list

    if (pTargetList)
    {
        pTargetList->AddTo(_pCurrent, fHead);       // add current to the new list
    }
    else
    {
        if (fSave)
        {
            _pCurrent->Save();
        }

        _pCurrent->Release();                       // or release
    }

    if (_pHead == _pCurrent)                        // update _pHead member in this list
    {
        _pHead = pNext;
    }

    _pCurrent = pNext;                              // update _pCurrent

    if (_pPrevious)                                 // update _pPrevious->_pPeerHolderNext
    {
        _pPrevious->_pPeerHolderNext = _pCurrent;
    }
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::InsertInHeadOf
//
//  Note:   ! assumes that the list was iterated to the end before calling here
//
//----------------------------------------------------------------------------

void
CPeerHolder::CListMgr::InsertInHeadOf (CListMgr * pTargetList)
{
    if (IsEmpty())  // if empty
        return;     // nothing to insert

    Assert (IsEnd() && _pPrevious); // assert that we've been iterated to the end

    // connect tail of this list to target list
    _pPrevious->_pPeerHolderNext = pTargetList->_pHead;

    // set target list head to this list
    pTargetList->_pHead = _pHead;
    pTargetList->Reset();
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::AttachPeerHolderToElement, static helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CListMgr::AttachPeerHolderToElement(CPeerHolder * pPeerHolder, CElement * pElement)
{
    HRESULT     hr;

    if (pElement->HasPeerHolder())
    {
        pPeerHolder->_pPeerHolderNext = pElement->GetPeerHolder();
        pElement->DelPeerHolder();
    }

    hr = THR(pElement->SetPeerHolder(pPeerHolder));
    if (hr)
        goto Cleanup;

    hr = THR(pElement->OnPeerListChange());

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::GetPeerHolderCount
//
//  CONSIDER: (tomfakes) keep the count cached to avoid this list traversal?
//
//----------------------------------------------------------------------------

long
CPeerHolder::CListMgr::GetPeerHolderCount(BOOL fNonEmptyOnly)
{
    long        lCount = 0;

    Reset();
    while  (!IsEnd())
    {
        if (Current()->_pPeer || !fNonEmptyOnly)
        {
            lCount++;
        }
        Step();
    }

    return lCount;
}


//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::GetPeerHolderByIndex
//
//----------------------------------------------------------------------------

CPeerHolder *           
CPeerHolder::CListMgr::GetPeerHolderByIndex(long lIndex, BOOL fNonEmptyOnly)
{
    long            lCurrent = 0;
    CPeerHolder *   pPeerHolder;

    Reset();
    while  (!IsEnd())
    {
        if (Current()->_pPeer || !fNonEmptyOnly) // if we either have peer, or we were not asked to always have peer
        {
            if (lCurrent == lIndex)
                break;

            lCurrent++;
        }
        Step();
    }

    if (!IsEnd())
    {
        Assert(lCurrent == lIndex && (Current()->_pPeer || !fNonEmptyOnly));
        pPeerHolder = Current();
    }
    else
    {
        pPeerHolder = NULL;
    }

    return pPeerHolder;
}

//+---------------------------------------------------------------------------
//
//  CPeerHolder::CListMgr::HasPeerWithUrn
//
//  Returns TRUE if we find a peer with the given Urn
//----------------------------------------------------------------------------

BOOL
CPeerHolder::CListMgr::HasPeerWithUrn(LPCTSTR Urn)
{
    Reset();
    while  (!IsEnd())
    {
        if (!Current()->_cstrUrn.IsNull() && 0 == StrCmpIC(Current()->_cstrUrn, Urn))
            return TRUE;

        Step();
    }

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//
// CPeerHolder methods
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerHolder
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CPeerHolder::CPeerHolder(CElement * pElement)
{
    _pElement = pElement;
    _pElement->SubAddRef();     // lock the element in memory while peer holder exists;
                                // the balancing SubRelease is in CPeerHolder::Passivate

    AddRef();                   // set  refcount to 1
    SubAddRef();                // and subrefcount to 1
                                // this SubAddRef is balanced by SubRelease made in Release
                                // when main refcount drops to zero

    // this should be done as early as possible, so that during creation peer could use the _dispidBase.
    // Another reason to do it here is that it is also used as a unique identier of the peer holder
    // on the element (e.g. in addBehavior)
    _dispidBase = UniquePeerNumberToBaseDispid(GetNextUniquePeerNumber(pElement));

#if DBG == 1
    _PeerSite._pPeerHolderDbg = this;

    // check integrity of notifications enums with flags
    Assert (NEEDCONTENTREADY          == FlagFromNotification(BEHAVIOREVENT_CONTENTREADY         ));
    Assert (NEEDDOCUMENTREADY         == FlagFromNotification(BEHAVIOREVENT_DOCUMENTREADY        ));
    Assert (NEEDAPPLYSTYLE            == FlagFromNotification(BEHAVIOREVENT_APPLYSTYLE           ));
    Assert (NEEDDOCUMENTCONTEXTCHANGE == FlagFromNotification(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE));

    Assert (BEHAVIOREVENT_CONTENTREADY          == NotificationFromFlag(NEEDCONTENTREADY         ));
    Assert (BEHAVIOREVENT_DOCUMENTREADY         == NotificationFromFlag(NEEDDOCUMENTREADY        ));
    Assert (BEHAVIOREVENT_APPLYSTYLE            == NotificationFromFlag(NEEDAPPLYSTYLE           ));
    Assert (BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE == NotificationFromFlag(NEEDDOCUMENTCONTEXTCHANGE));
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::~CPeerHolder
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CPeerHolder::~CPeerHolder()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Passivate
//
//-------------------------------------------------------------------------

void
CPeerHolder::Passivate()
{
    TraceTag((tagPeerPassivate, "CPeerHolder::Passivate, peer holder: %ld", CookieID()));

    if (_pPeer)
    {
        // Tell it to Detach from us
        _pPeer->Detach();

        // Remove all refs on the peer
        ClearInterface (&_pDisp);
        ClearInterface (&_pPeerUI);
        ClearInterface (&_pPeerCmdTarget);

        delete _pEventsBag;
        delete _pRenderBag;

        // now let it go
        ClearInterface (&_pPeer);
    }

    if (_pPeerHolderNext)
    {
        _pPeerHolderNext->Release();
        _pPeerHolderNext = NULL;
    }

    if (_pElement)
    {
        _pElement->SubRelease();
        _pElement = NULL;
    }
}

//+---------------------------------------------------------------
//
//  Member:     CPeerHolder::CLock methods
//
//---------------------------------------------------------------

CPeerHolder::CLock::CLock(CPeerHolder * pPeerHolder, FLAGS enumFlags)
{
    _pPeerHolder = pPeerHolder;

    if (!_pPeerHolder)
        return;

    // assert that only lock flags are passed in here
    Assert (0 == ((WORD)enumFlags & ~LOCKFLAGS));

    _wPrevFlags = _pPeerHolder->_wFlags;
    _pPeerHolder->_wFlags |= (WORD) enumFlags;

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        _pPeerHolder->AddRef();
       if (_pPeerHolder->_pElement)
       {
            // AddRef the element so it does not passivate while are keeping peer and peer holder alive
            _pPeerHolder->_pElement->AddRef();
       }
    }
}


CPeerHolder::CLock::~CLock()
{
    if (!_pPeerHolder)
        return;

    // restore only lock flags; don't touch others
    _pPeerHolder->_wFlags = (_wPrevFlags & LOCKFLAGS) | (_pPeerHolder->_wFlags & ~LOCKFLAGS);

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        if (_pPeerHolder->_pElement)
        {
            _pPeerHolder->_pElement->Release();
        }
        _pPeerHolder->Release();
    }
}

//+---------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryInterface
//
//
//  CPeerHolder objects do not have COM object identity!
//
//  The only reason why QI, AddRef and Release exist on peer holder
//  is to serve as IUnknown part of interface thunks created when
//  peer QI-s element for an interface. CElement, when performing
//  PrivateQueryInterface, always checks if there is a peer holder
//  attached (and if it is in QueryInterface (PEERHOLDERLOCK_INQI)) and then returns
//  thunk with IUnknown part wired to CPeerHolder. As a result, QI on
//  CElement, if originated from peer, does not increase CElement
//  main refcount (_ulRefs), but increases subrefcount (_ulAllRefs).
//  This logic breaks refcount loops when peer caches element pointer
//  received from IElementBehaviorSite::GetElement - peer has no way
//  to touch main refcount of it's element and therefore delay moment
//  of passivation of the element (with a few exceptions - see below).
//  Because QI of the thunks is also wired to this method, all
//  subsequent QIs on the returned interfaces will also go through
//  this method and the bit PEERHOLDERLOCK_INQI will be set correctly.
//
//  The only way a peer can touch main refcount of CElement it is attached
//  to is by QI-ing for IUnknown - but we do not wrap that case so to
//  preserve object identity of CElement. However, that case is naturally
//  not dangerous in terms of creating refloops because there is no really
//  reason to cache IUnknown in peer when caching of IHTMLElement is so much
//  more natural (a peer has to have malicious intent to create refcount loop
//  to do it).
//
//  Note that as a result of this logic, no object but tearoff thunks or
//  CPeerSite should be able to get to this method directly. The thunks
//  and peer site use the method only as a helper which sets proper bits
//  and then QIs CElement - they expect interface returned to be identity
//  of CElement this peer is attached to.
//
//---------------------------------------------------------------

HRESULT
CPeerHolder::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr;

    Assert (!TestFlag(LOCKINQI));

    if (!TestFlag(LOCKINQI))
    {
        CLock   lock(this, LOCKINQI);

        hr = THR_NOTRACE(_pElement->PrivateQueryInterface(riid, ppv));
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::AddRef
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::AddRef()
{
    _ulRefs++;
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Release
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::Release()
{
    _ulRefs--;
    if (0 == _ulRefs)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        _ulRefs = 0;
        SubRelease();
        return 0;
    }
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SubAddRef
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::SubAddRef()
{
    _ulAllRefs++;
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SubRelease
//
//-------------------------------------------------------------------------

ULONG
CPeerHolder::SubRelease()
{
    _ulAllRefs--;
    if (0 == _ulAllRefs)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryPeerInterface
//
//  Synposis:   for explicit QI's into the peer
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::QueryPeerInterface(REFIID riid, void ** ppv)
{
    RRETURN(_pPeer ? _pPeer->QueryInterface(riid, ppv) : E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::QueryPeerInterfaceMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::QueryPeerInterfaceMulti(REFIID riid, void ** ppv, BOOL fIdentityOnly)
{
    HRESULT             hr2;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (fIdentityOnly && !itr.PH()->IsIdentityPeer())
            continue;

        hr2 = THR_NOTRACE(itr.PH()->QueryPeerInterface(riid, ppv));
        if (S_OK == hr2)
            RRETURN (S_OK);
    }

    RRETURN (E_NOTIMPL);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Create
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Create(
    LPTSTR          pchPeerName,
    CPeerFactory *  pPeerFactory)
{
    HRESULT                 hr;
    CDoc *                  pDoc = _pElement->Doc();
    IElementBehavior *      pPeer = NULL;
    
    Assert (pPeerFactory);

    AssertSz(!_pElement->IsInMarkup() || !_pElement->GetMarkup()->__fDbgLockTree,
             "CPeerHolder::Create appears to be called at an unsafe moment of time");

    //
    // setup peer holder
    //

    // _pElement should be set before FindPeer so at the moment of creation
    // the peer has valid site and element available
    Assert (_pElement);
    
    {
        CElement::CLock lock(_pElement);    // if the element is blown away by peer while we create the peer,
                                            // this should keep us from crashing
        //
        // get the peer
        //

        hr = THR_NOTRACE(pPeerFactory->FindBehavior(pchPeerName, &_PeerSite, &pPeer));
        if (hr)
            goto Cleanup;
    }

    //
    // attach peer to the peer holder and peer holder to the element
    //
    
    hr = THR(AttachPeer(pPeer));
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

    pDoc->SetPeersPossible();

Cleanup:
    ReleaseInterface (pPeer);
    // do not do ReleaseInterface(pPeerSite);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::AttachPeer
//
//  Synopsis:   attaches the peer to this peer holder
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::AttachPeer(IElementBehavior * pPeer)
{
    HRESULT     hr = S_OK;
    
    if (!pPeer)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert (pPeer);
    _pPeer = pPeer;
    _pPeer->AddRef();

    //
    // initialize and QI
    //

    hr = THR(_pPeer->Init(&_PeerSite));
    if (hr)
        goto Cleanup;

    hr = THR(InitAttributes()); // dependencies: this should happen before InitRender, InitUI, or InitCategory
    if (hr)
        goto Cleanup;

    hr = THR(InitReadyState());
    if (hr)
        goto Cleanup;

    hr = THR(InitRender());
    if (hr)
        goto Cleanup;

    hr = THR(InitUI());
    if (hr)
        goto Cleanup;

    hr = THR(InitCategory());
    if (hr)
        goto Cleanup;

    hr = THR(InitCmdTarget());
    if (hr)
        goto Cleanup;

    SetFlag(NEEDDOCUMENTREADY);
    SetFlag(NEEDCONTENTREADY);
    SetFlag(AFTERINIT);

#ifndef PEER_NOTIFICATIONS_AFTER_INIT
    IGNORE_HR(EnsureNotificationsSent());
#endif

    //
    // finalize
    //

Cleanup:
    TraceTag((
        tagPeerAttach,
        "CPeerHolder::AttachPeer, peer holder: %ld, tag: %ls, tag id: %ls, SN: %ld, peer: %ls, hr: %x",
        CookieID(),
        _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN(),
        STRVAL(_pPeerFactoryUrl ? (LPTSTR)_pPeerFactoryUrl->_cstrUrl : NULL),
        hr));

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitUI
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitUI()
{
    _pPeerUI = NULL;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitCmdTarget
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitCmdTarget()
{
    HRESULT     hr;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IOleCommandTarget, (void **)&_pPeerCmdTarget));
    if (hr)
    {
        _pPeerCmdTarget = NULL; // don't trust peers to set it to NULL when they fail QI - it could be uninitialized
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitAttributes
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitAttributes()
{
    HRESULT                 hr;
    IPersistPropertyBag2 *  pPersistPropertyBag;
    CPropertyBag *          pPropertyBag;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IPersistPropertyBag2, (void **)&pPersistPropertyBag));
    if (hr || !pPersistPropertyBag)
        return S_OK;

    pPropertyBag = new CPropertyBag();
    if (!pPropertyBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pPropertyBag->_pElementExpandos = _pElement; // this transitions it to expandos loading mode

    hr = THR(_pElement->SaveAttributes(pPropertyBag));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }
    
    // ignore hr here so that if it does not implement Load method we still use the behavior
    IGNORE_HR(pPersistPropertyBag->Load(pPropertyBag, NULL));
    
Cleanup:
    ReleaseInterface(pPersistPropertyBag);
    if (pPropertyBag)
    {
        pPropertyBag->Release();
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitRender
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitRender()
{
    HRESULT                     hr;
    IElementBehaviorRender *    pPeerRender;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IElementBehaviorRender, (void**)&pPeerRender));
    if (hr || !pPeerRender)
        return S_OK;

    _pRenderBag = new CRenderBag();
    if (!_pRenderBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _pRenderBag->_pPeerRender = pPeerRender;

    hr = THR(_pRenderBag->_pPeerRender->GetRenderInfo(&_pRenderBag->_lRenderInfo));
    if (hr)
        goto Cleanup;

#if DBG == 1
    if (IsTagEnabled(tagPeerNoHitTesting))
    {
        _pRenderBag->_lRenderInfo &= ~BEHAVIORRENDERINFO_HITTESTING;
    }
#endif

    if (_pRenderBag->_lRenderInfo)
    {
        CLayout *       pLayout;

        pLayout = _pElement->GetCurLayout();
        if (pLayout)
        {
            // we must have missed the moment when layout was created and attached to the element
            // so simulate OnLayoutAvailable now
            IGNORE_HR(OnLayoutAvailable(pLayout));

            // and invalidate the layout to cause initial redraw
            pLayout->Invalidate();
        }
    }

Cleanup:
    // do not do ReleaseInterface(pPeerRender);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitReadyState
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitReadyState()
{
    HRESULT                         hr;
    HRESULT                         hr2;
    DWORD                           dwCookie;
    IConnectionPointContainer *     pCPC = NULL;
    IConnectionPoint *              pCP = NULL;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IConnectionPointContainer, (void**)&pCPC));
    if (hr || !pCPC)
        return S_OK;

    hr = THR_NOTRACE(pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR_NOTRACE(pCP->Advise((IPropertyNotifySink*)&_PeerSite, &dwCookie));
    if (hr)
        goto Cleanup;

    hr2 = THR_NOTRACE(UpdateReadyState());
    if (hr2)
    {
        _readyState = READYSTATE_UNINITIALIZED;
    }

Cleanup:
    ReleaseInterface(pCPC);
    ReleaseInterface(pCP);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::UpdateReadyState
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::UpdateReadyState()
{
    HRESULT     hr = S_OK;
    CInvoke     invoke;
    CVariant    varReadyState;

    EnsureDispatch();
    if (!_pDisp)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    if (TestFlag(DISPEX))
        hr = THR(invoke.Init((IDispatchEx*)_pDisp));
    else
        hr = THR(invoke.Init(_pDisp));
    if (hr)
        goto Cleanup;

    hr = THR(invoke.Invoke(DISPID_READYSTATE, DISPATCH_PROPERTYGET));
    if (hr)
        goto Cleanup;

    hr = THR(VariantChangeType(&varReadyState, invoke.Res(), 0, VT_I4));
    if (hr)
        goto Cleanup;

    _readyState = (READYSTATE) V_I4(&varReadyState);

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetReadyStateMulti
//
//-------------------------------------------------------------------------

READYSTATE
CPeerHolder::GetReadyStateMulti()
{
    READYSTATE          readyState;
    CPeerHolderIterator itr;

    // get min readyState of all peers that support readyState

    readyState = READYSTATE_COMPLETE;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (READYSTATE_UNINITIALIZED == itr.PH()->_readyState)
            continue;

        readyState = min(itr.PH()->_readyState, readyState);
    }

    return readyState;
}

//+------------------------------------------------------------------------
//
//  Helper:     NormalizeDoubleNullStr
//
//-------------------------------------------------------------------------

LPTSTR
NormalizeDoubleNullStr(LPTSTR pchSrc)
{
    LPTSTR      pch;
    LPTSTR      pch2;
    int         l;

    if (!pchSrc)
        return NULL;

    // count total length

    pch = pchSrc;
    while (pch[1]) // while second char is not null, indicating that there is at least one more string to go
    {
        pch = _tcschr(pch + 1, 0);
    }

    l = pch - pchSrc + 2;

    // allocate and copy string

    pch = new TCHAR [l];
    if (!pch)
        return NULL;

    memcpy (pch, pchSrc, sizeof(TCHAR) * l);

    // flip 0-s to 1-s so the string becomes usual null-terminated string

    pch2 = pch;
    while (pch2[1])
    {
        pch2 = _tcschr(pch + 1, 0);
        *pch2 = 1;
    }

    return pch;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InitCategory
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::InitCategory()
{
    HRESULT     hr;
    LPTSTR      pchCategory;
    LPTSTR      pchCategoryNorm;
    
    IElementBehaviorCategory * pPeerCategory;

    hr = THR_NOTRACE(QueryPeerInterface(IID_IElementBehaviorCategory, (void**)&pPeerCategory));
    if (hr || !pPeerCategory)
        return S_OK;

    hr = THR(pPeerCategory->GetCategory(&pchCategory));
    if (hr)
        goto Cleanup;

    // BUGBUG (alexz) this all should be optimized to use hash tables

    pchCategoryNorm = NormalizeDoubleNullStr(pchCategory);

    if (pchCategoryNorm)
    {
        hr = THR(_cstrCategory.Set(pchCategoryNorm));

        delete pchCategoryNorm;
    }

Cleanup:
    ReleaseInterface(pPeerCategory);

    if (pchCategory)
        CoTaskMemFree (pchCategory);


    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::ApplyStyleMulti
//
//  Synopsis:   Send down an applystyle to all behaviors.  Need to do this
//              in reverse order so that the behavior in the beginning
//              takes precedence.  
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::ApplyStyleMulti()
{
    HRESULT                 hr = S_OK;
    long                    c;
    CPeerHolderIterator     itr;
    BYTE                    fi[sizeof(CFormatInfo)];
    CFormatInfo *           pfi = (CFormatInfo*)&fi;
    CStyle *                pStyle = NULL;
    CTreeNode *             pNode;
    CVariant                varStyle;
    IHTMLCurrentStyle *     pCurrentStyle = NULL;

    CStackPtrAry <CPeerHolder *, 16> aryPeers(Mt(Mem));
    
    //
    // If not in markup, or any of the formats are not valid, bail out now.
    // We will compute the formats later so we will come back in here afterwards.  
    //

    Assert(_pElement);
    if (!_pElement->IsInMarkup())
        goto Cleanup;

    AssertSz(!_pElement->GetMarkup()->__fDbgLockTree,
             "CPeerHolder::ApplyStyleMulti appears to be called at an unsafe moment of time");

    //
    // If any of the formats are not valid, bail out now.
    // We will compute the formats later 
    // so we will come back in here afterwards.  
    //

    if (!_pElement->IsFormatCacheValid())
        goto Cleanup;
        
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->TestFlag(NEEDAPPLYSTYLE))
        {
            hr = THR(aryPeers.Append(itr.PH()));
            if (hr)
                break;
        }
    }
    
    if (!aryPeers.Size())
        goto Cleanup;
        
    //
    // Create and initialize write-able style object.
    //
    
    pStyle = new CStyle(
        _pElement, DISPID_UNKNOWN, 
        CStyle::STYLE_NOCLEARCACHES | CStyle::STYLE_SEPARATEFROMELEM);
    if (!pStyle)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pElement->get_currentStyle(&pCurrentStyle, NULL));
    if (hr)
        goto Cleanup;

    pStyle->_pStyleSource = pCurrentStyle;
    pStyle->_pStyleSource->AddRef();

    //
    // Iterate thru array backwards applying the style.
    //
    
    TraceTag((
        tagPeerApplyStyle,
        "CPeerHolder::ApplyStyleMulti, tag: %ls, id: %ls, sn: %ld; calling _pPeer->Notify",
        _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

    V_VT(&varStyle) = VT_DISPATCH;
    hr = THR(pStyle->QueryInterface(IID_IDispatch, (void **)&V_DISPATCH(&varStyle)));
    if (hr)
        goto Cleanup;
        
    for (c = aryPeers.Size() - 1; c >= 0; c--)
    {
        // reason to lock: case if the peer blows itself and the element away using outerHTML
        CLock lock(aryPeers[c]); 
        
        IGNORE_HR(aryPeers[c]->_pPeer->Notify(BEHAVIOREVENT_APPLYSTYLE, &varStyle));
    }

    //
    // Do a force compute on the element now with this style object,
    // but only if the caches are cleared.  This implies that
    // the peer did not change any props in the style.
    //

    pNode = _pElement->GetFirstBranch();
    if (!pNode)
        goto Cleanup;

    _pElement->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);

    pfi->_eExtraValues = ComputeFormatsType_ForceDefaultValue;
    pfi->_pStyleForce = pStyle;

    while (pNode)
    {
        TraceTag((
            tagPeerApplyStyle,
            "CPeerHolder::ApplyStyleMulti, tag: %ls, id: %ls, sn: %ld; calling _pElement->ComputeFormats",
            _pElement->TagName(), STRVAL(_pElement->GetAAid()), _pElement->SN()));

        _pElement->ComputeFormats(pfi, pNode);
        pNode = pNode->NextBranch();
    }

Cleanup:

    if (pStyle)
    {
        pStyle->PrivateRelease();
    }
    ReleaseInterface(pCurrentStyle);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureNotificationsSentMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::EnsureNotificationsSentMulti()
{
    HRESULT             hr = S_OK;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        IGNORE_HR(itr.PH()->EnsureNotificationsSent());
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureNotificationsSent
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::EnsureNotificationsSent()
{
    HRESULT     hr = S_OK;

    AssertSz(!_pElement->IsInMarkup() || !_pElement->GetMarkup()->__fDbgLockTree,
             "CPeerHolder::EnsureNotificationsSent appears to be called at an unsafe moment of time");

    if (_pPeer)
    {
        // if missed moment of parsing closing tag, or know that we won't get it
        if (TestFlag(NEEDCONTENTREADY) &&
            (_pElement->_fExplicitEndTag ||                         // (if we got the notification already)
             HpcFromEtag(_pElement->Tag())->_scope == SCOPE_EMPTY)) // (if we know that we are not going to get it)
        {
            ClearFlag(NEEDCONTENTREADY);
            IGNORE_HR(_pPeer->Notify(BEHAVIOREVENT_CONTENTREADY, NULL));
        }

        // if missed moment of quick done
        if (TestFlag(NEEDDOCUMENTREADY) &&
            _pElement->GetMarkup() && LOADSTATUS_QUICK_DONE <= _pElement->GetMarkup()->LoadStatus())
        {
            ClearFlag(NEEDDOCUMENTREADY);
            IGNORE_HR(_pPeer->Notify(BEHAVIOREVENT_DOCUMENTREADY, NULL));
        }
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::OnElementNotification
//
//-------------------------------------------------------------------------

void
CPeerHolder::OnElementNotification(CNotification * pNF)
{
    LONG                lEvent;

    switch (pNF->Type())
    {
    case NTYPE_END_PARSE:
        lEvent = BEHAVIOREVENT_CONTENTREADY;
        break;
        
    case NTYPE_DOC_END_PARSE:
        lEvent = BEHAVIOREVENT_DOCUMENTREADY;
        break;
        
    default:
        return;
    }

    NotifyMulti(lEvent);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::NotifyMulti
//
//-------------------------------------------------------------------------

void
CPeerHolder::NotifyMulti(LONG lEvent)
{
    CPeerHolderIterator itr;
    IElementBehavior *  pPeer;
    FLAGS               flag;

    AssertSz(!_pElement->IsInMarkup() || !_pElement->GetMarkup()->__fDbgLockTree,
             "CPeerHolder::NotifyMulti appears to be called at an unsafe moment of time");

    flag = FlagFromNotification(lEvent);

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        pPeer = itr.PH()->_pPeer;

        if (!pPeer || !itr.PH()->TestFlag(flag))
            continue;

        {
            CLock lock(itr.PH()); // reason to lock: case if the peer blows itself and the element away using outerHTML

            IGNORE_HR(pPeer->Notify(lEvent, NULL));
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::ExecMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::ExecMulti(
    const GUID *    pguidCmdGroup, 
    DWORD           nCmdID, 
    DWORD           nCmdExecOpt, 
    VARIANTARG *    pvaIn, 
    VARIANTARG *    pvaOut)
{
    HRESULT             hr = MSOCMDERR_E_NOTSUPPORTED;
    CPeerHolderIterator itr;

    AssertSz(!_pElement->IsInMarkup() || !_pElement->GetMarkup()->__fDbgLockTree,
             "CPeerHolder::ExecMulti appears to be called at an unsafe moment of time");

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->_pPeerCmdTarget)
        {
            hr = THR_NOTRACE(itr.PH()->_pPeerCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut));
            if (MSOCMDERR_E_NOTSUPPORTED != hr) // if (S_OK == hr || MSOCMDERR_E_NOTSUPPORTED != hr)
                break;
        }
    }

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::TestFlagMulti
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::TestFlagMulti(FLAGS flag)
{
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->TestFlag(flag))
            return TRUE;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Save
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL * pfAny)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    IPersistPropertyBag2 *  pPersistPropertyBag = NULL;
    CPropertyBag *          pPropertyBag = NULL;
    int                     i, c;
    PROPNAMEVALUE *         pPropPair;
    BSTR                    bstrPropName = NULL;
    BOOL                    fAny = FALSE;

    //
    // startup
    //

    if (!_pPeer)
        goto Cleanup;

    hr = _pPeer->QueryInterface(IID_IPersistPropertyBag2, (void**)&pPersistPropertyBag);
    if (hr)
        goto Cleanup;

    pPropertyBag = new CPropertyBag();
    if (!pPropertyBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // save
    //

    hr = THR(pPersistPropertyBag->Save(pPropertyBag, FALSE, FALSE));
    if (hr)
        goto Cleanup;

    //
    // process results
    //

    c = pPropertyBag->_aryProps.Size();

    if (pfAny)
        *pfAny = (0 < c);   // note that if c is 0, pfAny should not be touched (it can be TRUE already)
                            // (this is how it is used in the codepath)
    for (i = 0; i < c; i++)
    {
        fAny = TRUE;

        pPropPair = &pPropertyBag->_aryProps[i];

        if (pStreamWriteBuff)
        {
            CVariant    varStr;

            //
            // saving to stream
            //

            hr2 = THR(VariantChangeTypeSpecial(&varStr, &pPropPair->_varValue, VT_BSTR));
            if (hr2)
                continue;

            hr = THR(_pElement->SaveAttribute(
                    pStreamWriteBuff, pPropPair->_cstrName, V_BSTR(&varStr),
                    NULL, NULL, /* fEqualSpaces = */ TRUE, /* fAlwaysQuote = */ TRUE));
            if (hr)
                goto Cleanup;
        }
        else
        {
            //
            // saving to expandos
            // 

            hr = THR(FormsAllocString(pPropPair->_cstrName, &bstrPropName));
            if (hr)
                goto Cleanup;

            {
                // lock GetDispID method so that the attribute gets set as expando
                CLock lock (this, LOCKGETDISPID);

                hr = THR(_pElement->setAttribute(bstrPropName, pPropPair->_varValue, 0));
                if (hr)
                    goto Cleanup;
            }

            FormsFreeString(bstrPropName);
            bstrPropName = NULL;
        }
    }

Cleanup:
    delete pPropertyBag;

    ReleaseInterface(pPersistPropertyBag);

    if (bstrPropName)
    {
        FormsFreeString(bstrPropName);
        bstrPropName = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::SaveMulti
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::SaveMulti(CStreamWriteBuff * pStreamWriteBuff, BOOL * pfAny)
{
    HRESULT                 hr = S_OK;
    CPeerHolderIterator     itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        IGNORE_HR(itr.PH()->Save(pStreamWriteBuff, pfAny));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IllegalSiteCall
//
//  Synopsis:   returns TRUE if call is illegal
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IllegalSiteCall()
{
    if (!_pElement)
        return TRUE;

    if (_pElement->Doc()->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "peer object called MSHTML across apartment thread boundary (not from thread it was created on) (not an MSHTML bug)");
        return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::EnsureDispatch
//
//  Synposis:   ensures either IDispatch or IDispatchEx pointer cached
//
//-----------------------------------------------------------------------------------

void
CPeerHolder::EnsureDispatch()
{
#if DBG == 1
    if (IsTagEnabled(tagPeerNoOM))
        return;
#endif

    if (TestFlag(DISPCACHED) || !_pPeer)
        return;

    SetFlag (DISPCACHED);

    Assert (!_pDisp);

    IGNORE_HR(_pPeer->QueryInterface(IID_IDispatchEx, (void**)&_pDisp));
    if (_pDisp)
    {
        SetFlag (DISPEX);
    }
    else
    {
        IGNORE_HR(_pPeer->QueryInterface(IID_IDispatch, (void**)&_pDisp));
        Assert (!TestFlag(DISPEX));
    }
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetDispIDMulti
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::GetDispIDMulti(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    DISPID              dispid;
    CPeerHolderIterator itr;

    if (!TestFlag(LOCKGETDISPID))
    {
        CLock lock (this, LOCKGETDISPID);

        STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

        //
        // check if the requested name is name of a peer in the list
        //

        dispid = DISPID_PEER_NAME_BASE;

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (itr.PH()->_cstrName.Length() && bstrName &&
                0 == pfnStrCmp(itr.PH()->_cstrName, bstrName))
                break;

            dispid++;
        }
        if (!itr.IsEnd())
        {
            if (!pid)
            {
                RRETURN (E_POINTER);
            }

            (*pid) = dispid;
            RRETURN (S_OK);
        }

        //
        // check if the name is exposed by a peer
        //

        grfdex &= (~fdexNameEnsure);  // disallow peer to ensure the name

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            hr = THR_NOTRACE(itr.PH()->GetDispIDSingle(bstrName, grfdex, pid));
            if (S_OK == hr)
                break;
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvokeExMulti
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::InvokeExMulti(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    if (DISPID_PEER_NAME_BASE <= dispid && dispid <= DISPID_PEER_NAME_MAX)
    {
        if (0 == (wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        //
        // return IDispatch of a peer holder
        //

        // find corresponding peer holder
        dispid -= DISPID_PEER_NAME_BASE;
        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (0 == dispid)
                break;

            dispid--;
        }
        if (itr.IsEnd())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (!pvarResult)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        V_VT(pvarResult) = VT_DISPATCH;
        hr = THR_NOTRACE(itr.PH()->QueryPeerInterface(IID_IDispatch, (void**)&V_DISPATCH(pvarResult)));
        if (hr)
            goto Cleanup;
    }
    else if (DISPID_PEER_HOLDER_FIRST_RANGE_BASE <= dispid)
    {
        //
        // delegate to the corresponding peer holder
        //

        for (itr.Start(this); !itr.IsEnd(); itr.Step())
        {
            if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
                break;
        }

        if (itr.IsEnd())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        hr = THR_NOTRACE(itr.PH()->InvokeExSingle(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR_NOTRACE(InvokeExSingle(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetEventDispidMulti, helper
//
//  called from CScriptElement::CommitFunctionPointersCode
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetEventDispidMulti(LPOLESTR pchEvent, DISPID * pdispid)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    hr = DISP_E_UNKNOWNNAME;
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        hr = THR_NOTRACE(itr.PH()->_PeerSite.GetEventDispid(pchEvent, pdispid));
        if (S_OK == hr)
            break;
    }

    RRETURN (hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetDispIDSingle
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::GetDispIDSingle(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;

    if (IllegalSiteCall())
        RRETURN (E_UNEXPECTED);

    if (!pid)
        RRETURN (E_POINTER);

    //
    // check to see if the name is a registered event
    //

    if (0 < CustomEventsCount())
    {
        hr = THR_NOTRACE(_PeerSite.GetEventDispid(bstrName, pid));
        if (S_OK == hr)     // if found
            goto Cleanup;   // nothing more todo
    }

    //
    // delegate to the peer object
    //

    EnsureDispatch();

    if (_pDisp)
    {
        if (TestFlag(DISPEX))
        {
            hr = THR_NOTRACE(((IDispatchEx*)_pDisp)->GetDispID(
                bstrName,
                grfdex,
                pid));
        }
        else
        {
            hr = THR_NOTRACE(_pDisp->GetIDsOfNames(IID_NULL, &bstrName, 1, 0, pid));
        }

        if (S_OK == hr)
        {
            if (DISPID_PEER_BASE <= (*pid) && (*pid) <= DISPID_PEER_MAX)
            {
                (*pid) += _dispidBase;
            }
            else if (DISPID_PEER_HOLDER_BASE <= (*pid))
            {
                hr = DISP_E_UNKNOWNNAME;
            }
        }
    }
    else
    {
        hr = DISP_E_UNKNOWNNAME;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CPeerHolder::InvokeExSingle
//
//-----------------------------------------------------------------------------------

HRESULT
CPeerHolder::InvokeExSingle(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT         hr;

    if (IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    //
    // OM surfacing of custom peer events
    //

    if (IsCustomEventDispid(dispid))
    {
        hr = THR(_pElement->InvokeAA(dispid, CAttrValue::AA_Internal, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
        goto Cleanup;   // nothing more todo
    }

    //
    // delegate to the peer object
    //

    if (DISPID_PEER_HOLDER_FIRST_RANGE_BASE <= dispid)
    {
        dispid = DispidFromExternalRange(dispid);
    }

    EnsureDispatch();

    if (_pDisp)
    {
        if (TestFlag(DISPEX))
        {
            hr = THR_NOTRACE(((IDispatchEx*)_pDisp)->InvokeEx(
                dispid,
                lcid,
                wFlags,
                pdispparams,
                pvarResult,
                pexcepinfo,
                pSrvProvider));
        }
        else
        {
            hr = THR_NOTRACE(InvokeDispatchWithNoThis (
                _pDisp,
                dispid,
                lcid,
                wFlags,
                pdispparams,
                pvarResult,
                pexcepinfo));
        }
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetNextDispIDMulti, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetNextDispIDMulti(
    DWORD       grfdex,
    DISPID      dispid,
    DISPID *    pdispid)
{
    HRESULT             hr;
    CPeerHolderIterator itr;

    itr.Start(this);

    //
    // make a step within peer holder the current dispid belongs to
    //

    if (IsPeerDispid(dispid))
    {
        // find peer holder this dispid belongs to

        for (;;)
        {
            if (itr.PH()->TestFlag(DISPCACHED))
            {
                // if dispid belongs to the itr.PH() range
                if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
                    break;
            }

            itr.Step();
            if (itr.IsEnd())
            {
                hr = S_FALSE;
                goto Cleanup;
            }
        }

        Assert (itr.PH()->TestFlag(DISPEX));

        hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetNextDispID(
            grfdex, itr.PH()->DispidFromExternalRange(dispid), pdispid));
        switch (hr)
        {
        case S_OK:
            *pdispid = itr.PH()->DispidToExternalRange(*pdispid);
            goto Cleanup;   // done;
        case S_FALSE:
            break;          // keep searching
        default:
            goto Cleanup;   // fatal error
        }

        Assert (S_FALSE == hr);

        itr.Step();
        if (itr.IsEnd())
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    //
    // find first peer holder that:
    // - supports IDispatchEx, and
    // - returns next dispid
    //

    for (;;)
    {
        // find first peer holder that supports IDispatchEx
        for (;;)
        {
            itr.PH()->EnsureDispatch();

            if (itr.PH()->TestFlag(DISPEX))
                break;

            itr.Step();
            if (itr.IsEnd())
            {
                hr = S_FALSE;
                goto Cleanup;
            }
        }

        // check that it returns next dispid
        hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetNextDispID(grfdex, -1, pdispid));
        switch (hr)
        {
        case S_OK:
            *pdispid = itr.PH()->DispidToExternalRange(*pdispid);
            goto Cleanup;   // done;
        case S_FALSE:
            break;          // keep searching
        default:
            goto Cleanup;   // fatal error
        }

        itr.Step();
        if (itr.IsEnd())
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetMemberNameMulti, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetMemberNameMulti(DISPID dispid, BSTR * pbstrName)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->_dispidBase <= dispid && dispid < itr.PH()->_dispidBase + DISPID_PEER_HOLDER_RANGE_SIZE)
        {
            if (itr.PH()->TestFlag(DISPEX))
            {
                hr = THR_NOTRACE(((IDispatchEx*)itr.PH()->_pDisp)->GetMemberName(
                    itr.PH()->DispidFromExternalRange(dispid), pbstrName));
            }
            break;
        }
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CustomEventName, helper
//
//----------------------------------------------------------------------------

LPTSTR CPeerHolder::CustomEventName(int cookie)
{
    DISPID  dispid = CustomEventDispid(cookie);
    LPTSTR  pchName = NULL;

    if (IsStandardDispid(dispid))
    {
        PROPERTYDESC * pPropDesc;

        // CONSIDER (alexz): Because of the search below, we have a slight perf hit
        // when building typeinfo: we make a search in prop descs for every registered event.
        // To solve that, we could store prop desc ptr in the array instead of dispid;
        // but then all the rest of code will be slightly penalized for that by having to do 
        // more complex access to dispid, especially in normal case of non-standard events.
        _pElement->FindPropDescFromDispID (dispid, &pPropDesc, NULL, NULL);

        Assert(pPropDesc);

        pchName = (LPTSTR)pPropDesc->pstrName;
    }
    else
    {
        CAtomTable * pAtomTable = NULL;

        Assert (IsCustomEventDispid(dispid));

        pAtomTable = _pElement->GetAtomTable();

        Assert(pAtomTable && "missing atom table ");

        pAtomTable->GetNameFromAtom(AtomFromEventDispid(dispid), (LPCTSTR*)&pchName);
    }

    Assert(pchName && "(bad atom table entries?)");

    return pchName;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::HasCustomEventMulti, helper
//
//----------------------------------------------------------------------------

BOOL
CPeerHolder::HasCustomEventMulti(DISPID dispid)
{
    CPeerHolderIterator         itr;
    int                         i, c;
    CEventsBag *                pEventsBag;
    CEventsBag::CEventsArray *  pEventsArray;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        pEventsBag = itr.PH()->_pEventsBag;
        if (!pEventsBag)
            continue;

        pEventsArray = &pEventsBag->_aryEvents;
        if (!pEventsArray)
            continue;

        for (i = 0, c = CustomEventsCount(); i < c; i++)
        {
            if (dispid == CustomEventDispid(i))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetCustomEventsTypeInfoCount, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::GetCustomEventsTypeInfoCount(ULONG * pCount)
{
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (0 < itr.PH()->CustomEventsCount())
        {
            itr.PH()->_pEventsBag->_ulTypeInfoIdx = (*pCount);
            (*pCount)++;
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CreateCustomEventsTypeInfo, helper
//
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CreateCustomEventsTypeInfo(ULONG iTI, ITypeInfo ** ppTICoClass)
{
    HRESULT             hr = S_FALSE;
    CPeerHolderIterator itr;

    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (0 < itr.PH()->CustomEventsCount() && iTI == itr.PH()->_pEventsBag->_ulTypeInfoIdx)
        {
            hr = THR(itr.PH()->CreateCustomEventsTypeInfo(ppTICoClass));
            break;
        }
    }

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CreateCustomEventsTypeInfo, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CreateCustomEventsTypeInfo(ITypeInfo ** ppTICoClass)
{
    HRESULT     hr;
    int         i, c;
    FUNCDESC    funcdesc;
    LPTSTR      pchName;

    CCreateTypeInfoHelper Helper;

    if (!ppTICoClass)
        RRETURN (E_POINTER);

    //
    // start creating the typeinfo
    //

    hr = THR(Helper.Start(DIID_HTMLElementEvents));
    if (hr)
        goto Cleanup;

    //
    // set up the funcdesc we'll be using
    //

    memset (&funcdesc, 0, sizeof (funcdesc));
    funcdesc.funckind = FUNC_DISPATCH;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.cScodes = -1;

    //
    // add all registered events to the typeinfo
    //

    for (i = 0, c = CustomEventsCount(); i < c; i++)
    {
        funcdesc.memid = CustomEventDispid(i);

        hr = THR(Helper.pTypInfoCreate->AddFuncDesc(i, &funcdesc));
        if (hr)
            goto Cleanup;

        pchName = CustomEventName(i);
        hr = THR(Helper.pTypInfoCreate->SetFuncAndParamNames(i, &pchName, 1));
        if (hr)
            goto Cleanup;
    }

    //
    // finalize creating the typeinfo
    //

    hr = THR(Helper.Finalize(IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE));
    if (hr)
        goto Cleanup;

    ReplaceInterface (ppTICoClass, Helper.pTICoClassOut);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::OnLayoutAvailable
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::OnLayoutAvailable(CLayout * pLayout)
{
    HRESULT     hr = S_OK;

    if (_pRenderBag)
    {
        hr = THR(UpdateSurfaceFlags());
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::UpdateSurfaceFlags
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::UpdateSurfaceFlags()
{
    HRESULT     hr = S_OK;
    CLayout *   pLayout;

    if (!_pElement->HasLayout())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pLayout = _pElement->GetLayoutPtr();

    Assert (_pRenderBag);

    // if SURFACE or SURFACE3D bits are set
    if (_pRenderBag->_lRenderInfo & (BEHAVIORRENDERINFO_SURFACE | BEHAVIORRENDERINFO_3DSURFACE))
    {
        // set the bits on layout
        pLayout->SetSurfaceFlags(0 != (_pRenderBag->_lRenderInfo & BEHAVIORRENDERINFO_SURFACE),
                                 0 != (_pRenderBag->_lRenderInfo & BEHAVIORRENDERINFO_3DSURFACE));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::Draw, helper for IElementBehaviorRender
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::Draw(CFormDrawInfo * pDI, LONG lRenderInfo)
{
    HRESULT         hr = S_OK;
    HDC             hdc = pDI->GetDC(/* fPhysicallyClip = */ TRUE);
    CPropertyBag *  pBag = NULL;

#if DBG == 1
    if (IsTagEnabled(tagPeerNoRendering))
        goto Cleanup;
#endif

    Assert (_pRenderBag && _pRenderBag->_pPeerRender &&
            0 != (lRenderInfo & _pRenderBag->_lRenderInfo));

    //
    // prepare printing
    //

    if (_pElement->Doc()->IsPrintDoc())
    {
        CVariant    Var;
        
        //
        // Doing printing, so create up a propbag through which to pass additional
        // information through.  
        //

        pBag = new CPropertyBag;
        if (!pBag)
            RRETURN(E_OUTOFMEMORY);

        V_VT(&Var) = VT_HANDLE;
        V_I4(&Var) = HandleToLong(pDI->_hic);
        hr = THR(pBag->Write(_T("hic"), &Var));
        if (hr)
            goto Cleanup;

        V_VT(&Var) = VT_UINT_PTR;
        V_BYREF(&Var) = pDI->_ptd;
        hr = THR(pBag->Write(_T("Target Device"), &Var));
        if (hr)
            goto Cleanup;
    }
    
    //
    // do the actual draw
    //

    SaveDC(hdc);

    hr = THR(_pRenderBag->_pPeerRender->Draw(hdc, lRenderInfo, &pDI->_rc, (IPropertyBag *)pBag));

    RestoreDC(hdc, -1);

Cleanup:
    if (pBag)
    {
        pBag->Release();
    }
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::HitTestPoint, helper for IElementBehaviorRender
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::HitTestPoint(POINT * pPoint, BOOL * pfHit)
{
    HRESULT     hr;

    Assert (_pRenderBag && _pRenderBag->_pPeerRender &&
            0 != (BEHAVIORRENDERINFO_HITTESTING & _pRenderBag->_lRenderInfo));

    *pfHit = FALSE;

    hr = THR(_pRenderBag->_pPeerRender->HitTestPoint(pPoint, NULL, pfHit));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IsRelated, helper
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IsRelated(LPTSTR pchCategory)
{
    return _cstrCategory.IsNull() ?
        FALSE :
        NULL != StrStrI(_cstrCategory, pchCategory);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::IsRelatedMulti, helper
//
//-------------------------------------------------------------------------

BOOL
CPeerHolder::IsRelatedMulti(LPTSTR pchCategory, CPeerHolder ** ppPeerHolder)
{
    CPeerHolder::CPeerHolderIterator    itr;
    
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsRelated(pchCategory))
        {
            if (ppPeerHolder)
            {
                *ppPeerHolder = itr.PH();
            }
            return TRUE;
        }
    }

    if (ppPeerHolder)
    {
        *ppPeerHolder = NULL;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::AssertCorrectIdentityPeerHolder, DEBUG ONLY helper
//
//-------------------------------------------------------------------------

#if DBG == 1
void
CPeerHolder::AssertCorrectIdentityPeerHolder()
{
    int                                 c;
    CPeerHolder::CPeerHolderIterator    itr;
    
    c = 0;
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsIdentityPeer())
        {
            Assert (itr.PH() == this);

            c++;
        }
    }

    Assert (c <= 1);
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::GetRenderPeerHolder
//
//-------------------------------------------------------------------------

CPeerHolder *
CPeerHolder::GetRenderPeerHolder()
{
    CPeerHolder::CPeerHolderIterator    itr;
    
    for (itr.Start(this); !itr.IsEnd(); itr.Step())
    {
        if (itr.PH()->IsRenderPeer())
            return itr.PH();
    }
    return NULL;
}
