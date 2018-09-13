//+-----------------------------------------------------------------------
//
//  File:       peersite.cxx
//
//  Contents:   peer site
//
//  Classes:    CPeerHolder::CPeerSite
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#define PH MyCPeerHolder

///////////////////////////////////////////////////////////////////////////
//
// tearoff tables
//
///////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteOM)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterEvent,      registerevent,     (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetEventCookie,     geteventcookie,    (LPOLESTR pchEvent, LONG* plCookie))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, FireEvent,          fireevent,         (LONG lCookie, IHTMLEventObj * pEventObject))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, CreateEventObject,  createeventobject, (IHTMLEventObj ** ppEventObject))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterName,       registername,      (LPOLESTR pchName))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterUrn,        registerurn,       (LPOLESTR pchUrn))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteRender)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, Invalidate,              invalidate,             (LPRECT pRect))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRenderInfo,    invalidaterenderinfo,   ())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateStyle,         invalidatestyle,        ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteCategory)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetRelatedBehaviors, getrelatedbehaviors, (LONG lDirection, LPOLESTR pchCategory, IEnumUnknown ** ppEnumerator))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IServiceProvider)
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IBindHost)
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, CreateMoniker,        createmoniker,       (LPOLESTR pchName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved))
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, MonikerBindToStorage, monikerbindtostorage,(IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, MonikerBindToObject,  monikerbindtoobject, (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IPropertyNotifySink)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, OnChanged,     onchanged,     (DISPID dispid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, OnRequestEdit, onrequestedit, (DISPID dispid))
END_TEAROFF_TABLE()
            
BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IOleCommandTarget)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, QueryStatus,   querystatus,   (const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, Exec,          exec,          (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut))
END_TEAROFF_TABLE()
            
///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CPeerHolder::CPeerSite, CPeerHolder, _PeerSite)

// defined in site\ole\OleBindh.cxx:
extern HRESULT
MonikerBind(
    CDoc *                  pDoc,
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppv,
    BOOL                    fObject,
    DWORD                   dwCompatFlags);

DeclareTag(tagPeerFireEvent, "Peer", "trace CPeerHolder::CPeerSite::FireEvent")

///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_POINTER);

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IElementBehaviorSite*)this, IUnknown)
    QI_INHERITS(this, IElementBehaviorSite)
    QI_TEAROFF(this, IElementBehaviorSiteOM, NULL)
    QI_TEAROFF(this, IElementBehaviorSiteRender, NULL)
    QI_TEAROFF(this, IElementBehaviorSiteCategory, NULL)
    QI_TEAROFF(this, IServiceProvider, NULL)
    QI_TEAROFF(this, IBindHost, NULL)
    QI_TEAROFF(this, IPropertyNotifySink, NULL)
    QI_TEAROFF(this, IOleCommandTarget, NULL)
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
//  Member:     CPeerHolder::CPeerSite::GetElement
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetElement(IHTMLElement ** ppElement)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN (THR(PH()->QueryInterface(IID_IHTMLElement, (void**)ppElement)));
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterNotification
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterNotification(LONG lEvent)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    PH()->SetFlag(PH()->FlagFromNotification(lEvent));

    // Only do this if the format caches are already valid.  
    if (BEHAVIOREVENT_APPLYSTYLE == lEvent && PH()->_pElement->IsFormatCacheValid())
    {
        if (PH()->TestFlag(AFTERINIT))
        {
            IGNORE_HR(PH()->_pElement->ProcessPeerTask(PEERTASK_APPLYSTYLE_UNSTABLE));
        }
        else
        {
            IGNORE_HR(PH()->_pElement->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ FALSE));
        }
    }
    
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::QueryService
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    HRESULT             hr;
    CMarkup *           pMarkup;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (IsEqualGUID(rguidService, SID_SBindHost))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
        goto Cleanup;
    }
    else if (IsEqualGUID(rguidService, SID_SElementBehaviorMisc))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
        goto Cleanup;
    }
    else if (IsEqualGUID(rguidService, SID_ScriptletSite) && PH()->_pPeerFactoryUrl)
    {
        hr = THR_NOTRACE(PH()->_pPeerFactoryUrl->_context.QueryService(rguidService, riid, ppvObject));
        goto Cleanup;
    }

    pMarkup = PH()->_pElement->GetMarkup();
    if (pMarkup)
    {
        CMarkupBehaviorContext * pBehaviorContext = pMarkup->BehaviorContext();

        if (pBehaviorContext && pBehaviorContext->_pHtmlComponent)
        {
            hr = THR(pBehaviorContext->_pHtmlComponent->QueryService(rguidService, riid, ppvObject));
            goto Cleanup;   // done
        }

        hr = THR_NOTRACE(pMarkup->QueryService(rguidService, riid, ppvObject));

        goto Cleanup; // done (markup will delegate to the doc)
    }

    hr = THR_NOTRACE(Doc()->QueryService(rguidService, riid, ppvObject));

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::CreateMoniker, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::CreateMoniker(
    LPOLESTR    pchUrl,
    IBindCtx *  pbc,
    IMoniker ** ppmk,
    DWORD dwReserved)
{
    HRESULT     hr;
    TCHAR       achExpandedUrl[pdlUrlLen];

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(Doc()->ExpandUrl(pchUrl, ARRAY_SIZE(achExpandedUrl), achExpandedUrl, PH()->_pElement));
    if (hr)
        goto Cleanup;

    hr = THR(CreateURLMoniker(NULL, achExpandedUrl, ppmk));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::MonikerBindToStorage, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::MonikerBindToStorage(
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppvObj)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN1(MonikerBind(
        Doc(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        FALSE, // fObject = FALSE
        COMPAT_SECURITYCHECKONREDIRECT), S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::MonikerBindToObject, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::MonikerBindToObject(
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppvObj)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN1(MonikerBind(
        Doc(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        TRUE, // fObject = TRUE
        COMPAT_SECURITYCHECKONREDIRECT), S_ASYNCHRONOUS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterEvent, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterEvent(BSTR bstrEvent, LONG lFlags, LONG * plCookie)
{
    HRESULT     hr;
    LONG        lCookie;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (!bstrEvent)
        RRETURN (E_POINTER);

    if (!plCookie)
        plCookie = &lCookie;

    hr = THR(GetEventCookieHelper(bstrEvent, lFlags, plCookie, /* fEnsureCookie = */ TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventCookie, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventCookie(BSTR bstrEvent, LONG * plCookie)
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (!plCookie || !bstrEvent)
        RRETURN (E_POINTER);

    hr = THR_NOTRACE(GetEventCookieHelper(bstrEvent, 0, plCookie, /* fEnsureCookie = */ FALSE));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventDispid, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventDispid(LPOLESTR pchEvent, DISPID * pdispid)
{
    HRESULT hr;
    LONG    lCookie;

    hr = THR_NOTRACE(GetEventCookieHelper(pchEvent, 0, &lCookie, /* fEnsureCookie = */ FALSE));
    if (hr)
        goto Cleanup;

    *pdispid = PH()->CustomEventDispid(lCookie);

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventCookieHelper, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventCookieHelper(
    LPOLESTR pchEvent,
    LONG     lFlags,
    LONG *   plCookie,
    BOOL     fEnsureCookie)
{
    HRESULT     hr = DISP_E_UNKNOWNNAME;
    LONG        idx, cnt;

    Assert (pchEvent && plCookie);

    //
    // ensure events bag
    //

    if (!PH()->_pEventsBag)
    {
        PH()->_pEventsBag = new CEventsBag();
        if (!PH()->_pEventsBag)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    //
    // do the lookup or registration
    //

    // make search in array of dispids
    for (idx = 0, cnt = PH()->CustomEventsCount(); idx < cnt; idx++)
    {
        if (0 == StrCmpIC(pchEvent, PH()->CustomEventName(idx)))
            break;
    }

    if (idx < cnt) // if found the event
    {
        *plCookie = idx;
        hr = S_OK;
    }
    else // if not found the event
    {
        // depending on action
        if (!fEnsureCookie)
        {
            Assert (DISP_E_UNKNOWNNAME == hr);
        }
        else
        {
            //
            // register the new event
            //

            BOOL        fConnectNow = FALSE;
            DISPID      dispidSrc;
            DISPID      dispidHandler;
            int         c;

            CEventsBag::CEventsArray *  pEventsArray = &PH()->_pEventsBag->_aryEvents;

            hr = PH()->_pElement->CBase::GetDispID(pchEvent, 0, &dispidSrc);
            switch (hr)
            {
            case S_OK:

                if (IsStandardDispid(dispidSrc))
                {
                    if (PH()->_pElement->GetBaseObjectFor(dispidSrc) != PH()->_pElement)
                    {
                        // we don't allow yet to override events fired by window
                        hr = E_NOTIMPL;
                    }

                    dispidHandler = dispidSrc;
                }
                else
                {
                    Assert (IsExpandoDispid(dispidSrc));

                    dispidHandler = PH()->AtomToEventDispid(dispidSrc - DISPID_EXPANDO_BASE);
                    fConnectNow = TRUE;

                    lFlags &= ~BEHAVIOREVENTFLAGS_BUBBLE; // disable bubbling for anything but standard events
                }

                break;

            case DISP_E_UNKNOWNNAME:
                {

                    CAtomTable * pAtomTable = PH()->_pElement->GetAtomTable();

                    Assert(pAtomTable && "missing atom table");

                    hr = THR(pAtomTable->AddNameToAtomTable (pchEvent, &dispidHandler));
                    if (hr)
                        goto Cleanup;

                    dispidHandler = PH()->AtomToEventDispid(dispidHandler);

                    lFlags &= ~BEHAVIOREVENTFLAGS_BUBBLE; // disable bubbling for anything but standard events
                }
                break;

            default:
                // fatal error
                goto Cleanup;
            }

            c = pEventsArray->Size();

            hr = THR(pEventsArray->EnsureSize(c + 1));
            if (hr)
                goto Cleanup;

            pEventsArray->SetSize(c + 1);

            // c is now index of the last (yet uninitialized) item of the array

            (*pEventsArray)[c].dispid  = dispidHandler;
            (*pEventsArray)[c].lFlags = lFlags;

            *plCookie = c;

            if (fConnectNow)
            {
                // case when the event handler is inlined: <A:B onFooEvent = "..." >

                // construct and connect code
                hr = THR(PH()->_pElement->ConnectInlineEventHandler(
                    dispidSrc,          // dispid of expando with text of code
                    dispidHandler,      // dispid of function pointer in attr array
                    FALSE,              // fStandard
                    0, 0));             // line/offset info - not known here - will be retrieved from attr array
                if (hr)
                    goto Cleanup;

            }
        }
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::CreateEventObject, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::CreateEventObject(IHTMLEventObj ** ppEventObject)
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(CEventObj::Create(ppEventObject, Doc(), /* fCreateAttached = */FALSE, PH()->_cstrUrn));

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::FireEvent, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::FireEvent(LONG lCookie, BOOL fSameEventObject)
{
    HRESULT         hr = S_OK;
    LONG            lFlags;
    DISPID          dispid;
    CDoc *          pDoc = Doc();
    CTreeNode *     pNode = PH()->_pElement->GetFirstBranch();

    if (PH()->CustomEventsCount() <= lCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!fSameEventObject)
    {
        pDoc->_pparam->SetNodeAndCalcCoordinates(pNode);

        if (!pDoc->_pparam->GetSrcUrn())
             pDoc->_pparam->SetSrcUrn(PH()->_cstrUrn);
        // (do not check here pDoc->_pparam->GetSrcUrn()[0] - allow the string to be 0-length)
    }

    dispid  = PH()->CustomEventDispid(lCookie);
    lFlags  = PH()->CustomEventFlags (lCookie);

    TraceTag((tagPeerFireEvent,
        "CPeerHolder::CPeerSite::FireEvent %08lX  Cookie: %d   DispID: %08lX  Flags: %08lX  Custom Name: %ls", 
        PH(), lCookie, dispid, lFlags, STRVAL(PH()->CustomEventName(lCookie))));

    //
    // now fire
    //

    if (BEHAVIOREVENTFLAGS_BUBBLE & lFlags)
    {
        IGNORE_HR(PH()->_pElement->BubbleEventHelper(
            pNode, 0, dispid, dispid, /* fRaisedByPeer = */ TRUE, NULL, (BYTE *) VTS_NONE));
    }
    else
    {
        IGNORE_HR(PH()->_pElement->FireEventHelper(dispid, dispid, (BYTE *) VTS_NONE));
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::FireEvent, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::FireEvent(LONG lCookie, IHTMLEventObj * pEventObject)
{
    HRESULT     hr = S_OK;

    if (PH()->IllegalSiteCall() || !PH()->_pEventsBag)
        RRETURN(E_UNEXPECTED);

    // NOTE any change here might have to be mirrored in CHtmlComponentAttach::FireHandler

    if (HTMLEVENTOBJECT_USESAME == pEventObject)
    {
        IGNORE_HR(FireEvent(lCookie, TRUE));
    }
    else if (pEventObject)
    {
        CEventObj::COnStackLock onStackLock(pEventObject);

        IGNORE_HR(FireEvent(lCookie));
    }
    else
    {
        EVENTPARAM param(Doc(), TRUE); // so to replace pEventObject

        IGNORE_HR(FireEvent(lCookie));
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterName, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterName(BSTR bstrName)
{
    HRESULT hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->_cstrName.Set(bstrName));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterUrn, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterUrn(BSTR bstrName)
{
    HRESULT hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->_cstrUrn.Set(bstrName));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::Invalidate, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::Invalidate(LPRECT pRect)
{
    CLayout * pLayout = PH()->_pElement->GetUpdatedLayout();

    if (PH()->IllegalSiteCall() || !pLayout)
        RRETURN(E_UNEXPECTED);

    pLayout->Invalidate(pRect);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRenderInfo, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRenderInfo()
{
    HRESULT     hr;

     if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->_pRenderBag->_pPeerRender->GetRenderInfo(&PH()->_pRenderBag->_lRenderInfo));
    if (hr)
        goto Cleanup;

    hr = THR(PH()->UpdateSurfaceFlags());

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateStyle, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateStyle()
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    PH()->_pElement->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// helper class: peers enumerator
//
///////////////////////////////////////////////////////////////////////////

class CPeerEnumerator : IEnumUnknown
{
public:

    // construction and destruction

    CPeerEnumerator (DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart);
    ~CPeerEnumerator();

    static HRESULT Create(DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart, IEnumUnknown ** ppEnumerator);

    // IUnknown

    DECLARE_FORMS_STANDARD_IUNKNOWN(CPeerEnumerator);

    // IEnumUnknown

    STDMETHOD(Next)(ULONG c, IUnknown ** ppUnkBehavior, ULONG * pcFetched);
    STDMETHOD(Skip)(ULONG c);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumUnknown ** ppEnumerator);

    // helpers

    BOOL Next();

    // data

    DWORD               _dwDir;
    CStr                _cstrCategory;
    CElement *          _pElementStart;
    CTreeNode *         _pNode;
    CElement *          _pElementCurrent;
    CPeerHolder *       _pPeerHolderCurrent;
};

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator constructor
//
//----------------------------------------------------------------------------

CPeerEnumerator::CPeerEnumerator (DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart)
{
    _ulRefs = 1;

    _dwDir = dwDir;
    _cstrCategory.Set(pchCategory);
    _pElementStart = pElementStart;

    _pNode = NULL;

    _pElementStart->AddRef(); // so that if someone blows this away from tree we won't not crash

    Reset();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator destructor
//
//----------------------------------------------------------------------------

CPeerEnumerator::~CPeerEnumerator()
{
    _pElementStart->Release();

    if (_pNode)
        _pNode->NodeRelease();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::QueryInterface, per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if (IsEqualGUID(IID_IUnknown, riid) || IsEqualGUID(IID_IEnumUnknown, riid))
    {
        *ppv = this;
        RRETURN(S_OK);
    }
    else
    {
        *ppv = NULL;
        RRETURN (E_NOINTERFACE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Reset, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Reset(void) 
{
    // cleanup previously set values

    if (_pNode)
    {
        _pNode->NodeRelease();
        _pNode = NULL;
    }

    // init

    _pNode = _pElementStart->GetFirstBranch();
    if (!_pNode)
        return E_UNEXPECTED;
    _pNode->NodeAddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Next, helper
//
//----------------------------------------------------------------------------

BOOL
CPeerEnumerator::Next() 
{
    if (!_pNode)
        return FALSE;

    for (;;)
    {
        // BUGBUG: what if _pNode goes away after this release?!?!
        _pNode->NodeRelease();

        switch (_dwDir)
        {
        case BEHAVIOR_PARENT:

            _pNode = _pNode->Parent();

            break;
        }

        if (!_pNode)
            return FALSE;

        _pNode->NodeAddRef();

        _pElementCurrent = _pNode->Element();

        _pPeerHolderCurrent = NULL;

        if (_pElementCurrent->HasPeerHolder())
        {
            if (_pElementCurrent->GetPeerHolder()->IsRelatedMulti(_cstrCategory, &_pPeerHolderCurrent))
                return TRUE;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Next, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Next(ULONG c, IUnknown ** ppUnkBehaviors, ULONG * pcFetched) 
{
    ULONG       i;
    HRESULT     hr = S_OK;
    IUnknown ** ppUnk;

    if (0 == c || !ppUnkBehaviors)
        RRETURN (E_INVALIDARG);

    for (i = 0, ppUnk = ppUnkBehaviors; i < c; i++, ppUnk++)
    {
        if (!Next())
        {
            hr = E_FAIL;
            break;
        }

        hr = THR(_pPeerHolderCurrent->QueryPeerInterface(IID_IUnknown, (void**)ppUnk));
        if (hr)
            break;
    }

    if (pcFetched)
        *pcFetched = i;

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Skip, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Skip(ULONG c) 
{
    ULONG i;

    for (i = 0; i < c; i++)
    {
        if (!Next())
            break;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Clone, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Clone(IEnumUnknown ** ppEnumerator)
{
    RRETURN (Create(_dwDir, _cstrCategory, _pElementStart, ppEnumerator));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Create, per IEnumUnknown
//
//----------------------------------------------------------------------------

HRESULT
CPeerEnumerator::Create(DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart, IEnumUnknown ** ppEnumerator)
{
    HRESULT             hr = S_OK;
    CPeerEnumerator *   pEnumerator = NULL;

    pEnumerator = new CPeerEnumerator (dwDir, pchCategory, pElementStart);
    if (!pEnumerator)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppEnumerator = pEnumerator;

Cleanup:
    // do not do pEnumerator->Release();

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetRelatedBehavior, per IElementBehaviorSiteCategory
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetRelatedBehaviors (LONG lDir, LPTSTR pchCategory, IEnumUnknown ** ppEnumerator)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (BEHAVIOR_PARENT != lDir)
        RRETURN (E_NOTIMPL);

    if (!pchCategory || !ppEnumerator ||
        lDir < BEHAVIOR_FIRSTRELATION || BEHAVIOR_LASTRELATION < lDir)
        RRETURN (E_INVALIDARG);

    RRETURN (CPeerEnumerator::Create(lDir, pchCategory, PH()->_pElement, ppEnumerator));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::OnChanged, per IPropertyNotifySink
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::OnChanged(DISPID dispid)
{
    HRESULT     hr = S_OK;
    CLock       lock(PH()); // reason to lock: whe might run onpropertychange and onreadystatechange scripts here

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    //
    // custom processing
    //

    switch (dispid)
    {
    case DISPID_READYSTATE:
        {
            HRESULT     hr;

            if (PH()->_readyState == READYSTATE_UNINITIALIZED)  // if the peer did not provide connection point
                goto Cleanup;                                   // don't support it's readyState changes

            hr = THR(PH()->UpdateReadyState());
            if (hr)
                goto Cleanup;

            CPeerMgr::UpdateReadyState(PH()->_pElement, PH()->_readyState);

            goto Cleanup; // done
        }
        break;
    }

    //
    // default processing
    //

    if (PH()->_pElement)
    {
        PH()->_pElement->OnPropertyChange(PH()->DispidToExternalRange(dispid), 0);
    }

Cleanup:
    RRETURN (hr);
}
//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::OnChanged, per IPropertyNotifySink
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANT *       pvarArgIn,
    VARIANT *       pvarArgOut)
{
    HRESULT     hr = OLECMDERR_E_UNKNOWNGROUP;

    if (!pguidCmdGroup)
        goto Cleanup;

    if (IsEqualGUID(CGID_ElementBehaviorMisc, *pguidCmdGroup))
    {
        hr = OLECMDERR_E_NOTSUPPORTED;

        switch (nCmdID)
        {
        case CMDID_ELEMENTBEHAVIORMISC_GETCONTENTS:

            if (!pvarArgOut)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            V_VT(pvarArgOut) = VT_BSTR;
            if (ETAG_GENERIC_LITERAL == PH()->_pElement->Tag())
            {
                hr = THR(FormsAllocString(
                    DYNCAST(CGenericElement, PH()->_pElement)->_cstrContents,
                    &V_BSTR(pvarArgOut)));
            }
            else
            {
                hr = THR(PH()->_pElement->get_innerHTML(&V_BSTR(pvarArgOut)));
            }

            break;

        case CMDID_ELEMENTBEHAVIORMISC_PUTCONTENTS:

            if (!pvarArgIn ||
                VT_BSTR != V_VT(pvarArgIn))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            if (ETAG_GENERIC_LITERAL == PH()->_pElement->Tag())
            {
                hr = THR(DYNCAST(CGenericElement, PH()->_pElement)->_cstrContents.Set(V_BSTR(pvarArgIn)));
            }
            else
            {
                hr = THR(PH()->_pElement->put_innerHTML(V_BSTR(pvarArgIn)));
            }
            
            break;
        }
    }

Cleanup:
    RRETURN (hr);
}
