//+---------------------------------------------------------------------
//
//  File:       htc.cxx
//
//  Classes:    CHtmlComponent, etc.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
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

#define _cxx_
#include "htc.hdl"

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

DeclareTag(tagHtcTags,                      "HTC", "trace HTC tags (CHtmlComponentBase::Notify, documentReady)")
DeclareTag(tagHtcOnLoadStatus,              "HTC", "trace CHtmlComponent::OnLoadStatus")
DeclareTag(tagHtcPropertyEnsureHtmlLoad,    "HTC", "trace CHtmlComponentProperty::EnsureHtmlLoad")

CLSID CLSID_CHtmlComponentConstructor =
    {0x3050f4f8, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

CLSID CLSID_CHtmlComponent =
    {0x3050f4fa, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

CLSID CLSID_CHtmlComponentBase =
    {0x3050f5f1, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};

MtDefine(CHtmlComponentConstructor, Behaviors, "CHtmlComponentConstructor")
MtDefine(CHtmlComponentBase,        Behaviors, "CHtmlComponentBase")
MtDefine(CHtmlComponent,            Behaviors, "CHtmlComponent")
MtDefine(CHtmlComponentDD,          Behaviors, "CHtmlComponentDD")
MtDefine(CHtmlComponentProperty,    Behaviors, "CHtmlComponentProperty")
MtDefine(CHtmlComponentMethod,      Behaviors, "CHtmlComponentMethod")
MtDefine(CHtmlComponentEvent,       Behaviors, "CHtmlComponentEvent")
MtDefine(CHtmlComponentAttach,      Behaviors, "CHtmlComponentAttach")
MtDefine(CHtmlComponentDesc,        Behaviors, "CHtmlComponentDesc")

MtDefine(CHtmlComponent_CDispatchItemsArray,       CHtmlComponent, "CHtmlComponent::CDispatchItemsArray")
MtDefine(CHtmlComponent_CEventsArray,              CHtmlComponent, "CHtmlComponent::CEventsArray")
MtDefine(CHtmlComponent_CAttachArray,              CHtmlComponent, "CHtmlComponent::CAttachArray")

MtDefine(CProfferService_CItemsArray, Behaviors, "CProfferService::CItemsArray")

class CHtmlComponentConstructor g_cfHtmlComponentConstructor;

BEGIN_TEAROFF_TABLE(CHtmlComponentBase, IElementBehavior)
	TEAROFF_METHOD(CHtmlComponentBase, Init,   init,   (IElementBehaviorSite * pBehaviorSite))
	TEAROFF_METHOD(CHtmlComponentBase, Notify, notify, (LONG lEvent, VARIANT * pVar))
	TEAROFF_METHOD(CHtmlComponentBase, Detach, detach, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CHtmlComponent, IServiceProvider)
    TEAROFF_METHOD(CHtmlComponent, QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CHtmlComponent, IPersistMoniker)
    TEAROFF_METHOD(CHtmlComponent, GetClassID, getclassid, (LPCLSID lpClassID))
    TEAROFF_METHOD(CHtmlComponent, IsDirty, isdirty, ())
    TEAROFF_METHOD(CHtmlComponent, Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode))
    TEAROFF_METHOD(CHtmlComponent, Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember))
    TEAROFF_METHOD(CHtmlComponent, SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc))
    TEAROFF_METHOD(CHtmlComponent, GetCurMoniker, getcurmoniker, (IMoniker  **ppimkName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CHtmlComponent, IPersistPropertyBag2)
    TEAROFF_METHOD(CHtmlComponent, GetClassID,   getclassid, (CLSID *pClassID))
    TEAROFF_METHOD(CHtmlComponent, InitNew,      initnew,    ())
    TEAROFF_METHOD(CHtmlComponent, Load,         load,       (IPropertyBag2 * pPropBag, IErrorLog * pErrLog))
    TEAROFF_METHOD(CHtmlComponent, Save,         save,       (IPropertyBag2 * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties))
    TEAROFF_METHOD(CHtmlComponent, IsDirty,      isdirty,    ())
END_TEAROFF_TABLE()

const CBase::CLASSDESC CHtmlComponentBase::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponent::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    CHtmlComponent::s_acpi,         // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CONNECTION_POINT_INFO CHtmlComponent::s_acpi[] = {
    CPI_ENTRY(IID_IPropertyNotifySink, DISPID_A_PROPNOTIFYSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC CHtmlComponentDD::s_classdesc =
{
    &CLSID_HTCDefaultDispatch,      // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCDefaultDispatch,       // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentProperty::s_classdesc =
{
    &CLSID_HTCPropertyBehavior,     // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCPropertyBehavior,      // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentMethod::s_classdesc =
{
    &CLSID_HTCMethodBehavior,       // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCMethodBehavior,        // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentEvent::s_classdesc =
{
    &CLSID_HTCEventBehavior,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCEventBehavior,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentAttach::s_classdesc =
{
    &CLSID_HTCAttachBehavior,       // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCAttachBehavior,        // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

const CBase::CLASSDESC CHtmlComponentDesc::s_classdesc =
{
    &CLSID_HTCDescBehavior,         // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTCDescBehavior,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

///////////////////////////////////////////////////////////////////////////
//
// misc functions
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Function:   HasExpando
//
//-------------------------------------------------------------------------

BOOL
HasExpando (CElement * pElement, LPTSTR pchName)
{
    HRESULT hr;
    DISPID  dispid;
    hr = THR_NOTRACE(pElement->GetExpandoDispID(pchName, &dispid, 0));
    return S_OK == hr ? TRUE : FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpandoString
//
//-------------------------------------------------------------------------

LPTSTR
GetExpandoString (CElement * pElement, LPTSTR pchName)
{
    HRESULT         hr;
    DISPID          dispid;
    LPTSTR          pchValue = NULL;
    CAttrArray *    pAA;

    hr = THR_NOTRACE(pElement->GetExpandoDispID(pchName, &dispid, 0));
    if (hr)
        goto Cleanup;

    pAA = *(pElement->GetAttrArray());
    pAA->FindString(dispid, (LPCTSTR*)&pchValue, CAttrValue::AA_Expando);

Cleanup:

    return pchValue;
}

//+------------------------------------------------------------------------
//
//  Functions:   notification mappings
//
// CONSIDER     (alexz) using hash tables for this
//
//-------------------------------------------------------------------------

class CNotifications
{
public:

    //
    // data definitions
    //

    class CItem
    {
    public:
        LPTSTR      _pchName;
        LONG        _lEvent;
        DISPID      _dispidInternal;
    };

    static CItem s_ary[];

    //
    // methods
    //

    static CItem * Find(LPTSTR pch)
    {
        CItem *     pItem;
        for (pItem = s_ary; pItem->_pchName; pItem++)
        {
            if (0 == StrCmpIC(pItem->_pchName, pch))
                return pItem;
        }
        return NULL;
    };
};

CNotifications::CItem CNotifications::s_ary[] =
{
//  {   _T("onContentChange"),      BEHAVIOREVENT_CONTENTREADY,     DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY     },
    {   _T("onContentReady"),       BEHAVIOREVENT_CONTENTREADY,     DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY     },
    {   _T("onDocumentReady"),      BEHAVIOREVENT_DOCUMENTREADY,    DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY    },
    {   _T("onDetach"),             -1,                             -1                                          },
    {   _T("onApplyStyle"),         BEHAVIOREVENT_APPLYSTYLE,       DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE       },
    {   NULL,                       -1,                             -1                                          }
};

//+------------------------------------------------------------------------
//
//  Function:   TagNameToHtcBehaviorType
//
// CONSIDER     (alexz) using hash tables for this
//
//-------------------------------------------------------------------------

HTC_BEHAVIOR_TYPE
TagNameToHtcBehaviorType(LPCTSTR pchTagName)
{
    if (0 == StrCmpIC(pchTagName, _T("property")))
                                                        return HTC_BEHAVIOR_PROPERTY;
    else if (0 == StrCmpIC(pchTagName, _T("method")))
                                                        return HTC_BEHAVIOR_METHOD;
    else if (0 == StrCmpIC(pchTagName, _T("event")))
                                                        return HTC_BEHAVIOR_EVENT;
    else if (0 == StrCmpIC(pchTagName, _T("attach")))
                                                        return HTC_BEHAVIOR_ATTACH;
    else if (0 == StrCmpIC(pchTagName, _T("htc")))
                                                        return HTC_BEHAVIOR_DESC;
    else if (0 == StrCmpIC(pchTagName, _T("component")))
                                                        return HTC_BEHAVIOR_DESC;
    else
                                                        return HTC_BEHAVIOR_NONE;
}

//+------------------------------------------------------------------------
//
//  Function:   HtcBehaviorFromElement
//
//-------------------------------------------------------------------------

CHtmlComponentBase *
GetHtcBehaviorFromElement(CElement * pElement)
{
    CHtmlComponentBase *    pBehavior;

    Verify (S_OK == pElement->GetPeerHolder()->QueryPeerInterfaceMulti(
                CLSID_CHtmlComponentBase, (void**)&pBehavior, /* fIdentityOnly = */FALSE));

    return pBehavior;
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentConstructor
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::QueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IElementBehaviorFactory)
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

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentConstructor::FindBehavior, per IElementBehaviorFactory
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentConstructor::FindBehavior(
    BSTR                    bstrName,
    BSTR                    bstrUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppBehavior)
{
    HRESULT             hr = S_OK;
    CHtmlComponent *    pComponent = NULL;

    //
    // create and init CHtmlComponent
    //

    pComponent = new CHtmlComponent();
    if (!pComponent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pComponent->Init(pSite));
    if (hr)
        goto Cleanup;

    if (pComponent->IsRecursiveUrl(bstrUrl))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // get IElementBehavior interface
    //

    hr = THR(pComponent->PrivateQueryInterface(IID_IElementBehavior, (void**)ppBehavior));

Cleanup:
    if (pComponent)
        pComponent->PrivateRelease();

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentBase
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentBase::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    EnsureComponent();

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IElementBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    if (IsEqualGUID(iid, CLSID_CHtmlComponentBase))
    {
        *ppv = this;    // weak ref
        return S_OK;
    }

    RRETURN (super::PrivateQueryInterface(iid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Passivate
//
//-------------------------------------------------------------------------

void
CHtmlComponentBase::Passivate()
{
    // do not do ClearInterface (&_pComponent)

    _pElement = NULL;   // Weak ref

    ClearInterface (&_pSite);

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Init(IElementBehaviorSite * pSite)
{
    HRESULT             hr;
    IHTMLElement *      pHtmlElement = NULL;

    // get site

    _pSite = pSite;
    _pSite->AddRef();

    // get element

    hr = THR(_pSite->GetElement(&pHtmlElement));
    if (hr)
        goto Cleanup;

    hr = THR(pHtmlElement->QueryInterface(CLSID_CElement, (void**)&_pElement));
    if (hr)
        goto Cleanup;

    Assert (_pElement);

    EnsureComponent();
    
Cleanup:
    ReleaseInterface (pHtmlElement);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Notify(LONG lEvent, VARIANT * pVar)
{
    EnsureComponent();

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::Detach
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::Detach()
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::ExternalName, helper
//
//-------------------------------------------------------------------------

inline LPTSTR
CHtmlComponentBase::ExternalName()
{
    return GetExpandoString(_pElement, _T("name"));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::InvokeEngines, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentBase::InvokeEngines(
    LPTSTR          pchName,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarRes,
    EXCEPINFO *     pExcepInfo,
    IServiceProvider * pServiceProvider)
{
    HRESULT     hr = DISP_E_UNKNOWNNAME;
    CDoc *      pDoc;

    EnsureComponent();

    if (!_pElement)
        RRETURN(hr);
        
    pDoc = _pElement->Doc();

    Assert (pDoc->_pScriptCollection);

    hr = THR_NOTRACE(pDoc->_pScriptCollection->InvokeName(
        _pElement->GetMarkup(), pchName, g_lcidUserDefault, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::EnsureComponent, helper
//
//-------------------------------------------------------------------------

void
CHtmlComponentBase::EnsureComponent()
{
    IServiceProvider *  pSP = NULL;
    HRESULT             hr;
    
    if (_fComponentEnsured || !_pElement || !_pElement->IsInMarkup())
        return;

    _fComponentEnsured = TRUE;
    
    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    IGNORE_HR(pSP->QueryService(CLSID_CHtmlComponent, CLSID_CHtmlComponent, (void**)&_pComponent));

    if (_pComponent)
    {
        OnComponentSet();
    }
    
Cleanup:
    ReleaseInterface(pSP);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::ChildInternalName, helper
//
//  CONSIDER:   optimizing this so to avoid children scan
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentBase::ChildInternalName(LPTSTR pchChild)
{
    CChildIterator  ci (_pElement);
    CTreeNode *     pNode;
    CElement *      pElement;

    while (NULL != (pNode = ci.NextChild()))
    {
        pElement = pNode->Element();
        if (0 == StrCmpIC (pElement->TagName(), pchChild))
        {
            return GetExpandoString (pElement, _T("internalName"));
        }
    }

    return NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentBase::InternalName, helper
//
//  Parameters: pfDifferent     indicates if external and internal names are different
//
//-------------------------------------------------------------------------

LPTSTR
CHtmlComponentBase::InternalName(BOOL * pfScriptsOnly, WORD * pwFlags, DISPPARAMS * pDispParams)
{
    LPTSTR  pchName;
    BOOL    fScriptsOnly;

    if (!_pElement)
        return NULL;

    if (!pfScriptsOnly)
        pfScriptsOnly = &fScriptsOnly;

    //
    // putters / getters
    //

    if (pwFlags)
    {
        if ((*pwFlags) & DISPATCH_PROPERTYGET)
        {
            pchName = GetExpandoString(_pElement, _T("GET"));
            if (!pchName)
            {
                pchName = ChildInternalName(_T("GET"));
            }
        }
        else if ((*pwFlags) & DISPATCH_PROPERTYPUT)
        {
            pchName = GetExpandoString(_pElement, _T("PUT"));
            if (!pchName)
            {
                pchName = ChildInternalName(_T("PUT"));
            }
        }
        else
        {
            pchName = NULL;
        }

        if (pchName)                            // if there is a putter or getter method for the property
        {
            *pwFlags = DISPATCH_METHOD;         // switch to METHOD call type
            if (pDispParams)
            {
                pDispParams->cNamedArgs = 0;    // remove any named args
            }
            *pfScriptsOnly = TRUE;
            return pchName;                     // and use the putter/getter
        }
    }

    //
    // internal name
    //

    pchName = GetExpandoString(_pElement, _T("internalName"));
    if (pchName)
    {
        *pfScriptsOnly = TRUE;
        return pchName;
    }

    //
    // name
    //

    *pfScriptsOnly = FALSE;

    return ExternalName();
}




///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDD
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateAddRef, per IPrivateUnknown
//
//-------------------------------------------------------------------------

ULONG
CHtmlComponentDD::PrivateAddRef()
{
    return Component()->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateRelease, per IPrivateUnknown
//
//-------------------------------------------------------------------------

ULONG
CHtmlComponentDD::PrivateRelease()
{
    return Component()->SubRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::PrivateQueryInterface, per IPrivateUnknown
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCDefaultDispatch, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

#if DBG == 1
//+------------------------------------------------------------------------
//
//  Helper:     DDAssertDispidRanges
//
//              DEBUG ONLY
//
//-------------------------------------------------------------------------

void
DDAssertDispidRanges(DISPID dispid)
{
    struct
    {
        DISPID  dispidMin;
        DISPID  dispidMax;
    }   aryRanges[] =
    {
        // (0)  script collection - cross languages glue
        {   DISPID_OMWINDOWMETHODS,       1000000 - 1                             }, //     10,000 ..    999,999

        // (1)  markup window all collection (elements within htc accessed by id)
        {   DISPID_COLLECTION_MIN,        DISPID_COLLECTION_MAX                   }, //  1,000,000 ..  2,999,999

        // ()   doc fragment DD
        // DISPID_A_DOCFRAGMENT                 document
        // (in range 3)

        // ()   HTC DD
        // DISPID_A_HTCDD_ELEMENT               element
        // DISPID_A_HTCDD_CREATEEVENTOBJECT     createEventObject
        // (in range 3)

        // (2)  element's namespace - standard properties
        {   -1,                           -30000                                  }, // 0xFFFFFFFF .. 0xFFFF0000 (~)

        // (3)  element's namespace - properties
        {   DISPID_XOBJ_MIN,              DISPID_XOBJ_MAX                         }, // 0x80010000 .. 0x8001FFFF

        // (4)  element's namespace - expandos
        {   DISPID_EXPANDO_BASE,          DISPID_EXPANDO_MAX                      }, //  3,000,000 .. 3,999,999

        // (5)  element's namespace - properties of other behaviors attached to the element
        { DISPID_PEER_HOLDER_BASE,      DISPID_PEER_HOLDER_BASE + INT_MAX / 2     }, //  5,000,000 .. + infinity

        // (6)  element's namespace - properties CElement-derived elements
        { DISPID_NORMAL_FIRST,          DISPID_NORMAL_FIRST + 1000 * 9            },  //      1000 .. 9000
    };

    int     i, j;
    BOOL    fRangeHit;

    // check that the dispid falls into an expected range

    fRangeHit = FALSE;
    for (i = 0; i < ARRAY_SIZE(aryRanges); i++)
    {
        if (aryRanges[i].dispidMin <= dispid && dispid <= aryRanges[i].dispidMax)
        {
            fRangeHit = TRUE;
        }
    }

    Assert (fRangeHit);

    // check that the ranges do not overlap

    for (i = 0; i < ARRAY_SIZE(aryRanges) - 1; i++)
    {
        if (4 == i)
            continue;

        for (j = i + 1; j < ARRAY_SIZE(aryRanges); j++)
        {
            Assert (aryRanges[j].dispidMin < aryRanges[i].dispidMin || aryRanges[i].dispidMax < aryRanges[j].dispidMin);
            Assert (aryRanges[j].dispidMax < aryRanges[i].dispidMin || aryRanges[i].dispidMax < aryRanges[j].dispidMax);
        }
    }
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT             hr;
    CDoc *              pDoc = Component()->_pDoc;
    
    if (!Component()->_pMarkup)
        RRETURN (E_UNEXPECTED);

    //
    // delegation to docfragment
    //

    hr = THR_NOTRACE(Component()->_pMarkup->_OmDoc.GetDispID(bstrName, grfdex & (~fdexNameEnsure), pdispid));
    if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK == hr || (hr other then DISP_E_UNKNOWNNAME))
        goto Cleanup;               // get out
    
    //
    // standard handling
    //

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pdispid));
    if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK == hr || (hr other then DISP_E_UNKNOWNNAME))
        goto Cleanup;               // get out

    //
    // gluing different script engines togather 
    //

    if (Component()->_pMarkup && pDoc->_pScriptCollection)
    {
        CScriptCollection * pScriptCollection = pDoc->_pScriptCollection;

        hr = THR_NOTRACE(pScriptCollection->GetDispID(Component()->_pMarkup, bstrName, grfdex, pdispid));
        if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK or error other then DISP_E_UNKNOWNNAME)
            goto Cleanup;
    }

    //
    // access to id'd elements 
    //

    if (Component()->_pMarkup)
    {
        CCollectionCache *  pWindowCollection;

        hr = THR(Component()->_pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pWindowCollection = Component()->_pMarkup->CollectionCache();

        hr = THR_NOTRACE(pWindowCollection->GetDispID(
                CMarkup::WINDOW_COLLECTION,
                bstrName,
                grfdex,
                pdispid));

        // the collectionCache GetDispID will return S_OK with DISPID_UNKNOWN
        // if the name isn't found.

        if (S_OK == hr && DISPID_UNKNOWN == *pdispid)
        {
            hr = DISP_E_UNKNOWNNAME;
            goto Cleanup;
        }
        if (DISP_E_UNKNOWNNAME != hr)
            goto Cleanup;
    }
    
    //
    // access to element's namespace
    //

    if (Component()->_pElement)
    {
        hr = THR_NOTRACE(Component()->_pElement->GetDispID(bstrName, grfdex, pdispid));
    }
    
Cleanup:

#if DBG == 1
    if (S_OK == hr)
    {
        DDAssertDispidRanges(*pdispid);
    }
#endif

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::InvokeEx(
    DISPID          dispid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarRes,
    EXCEPINFO *     pExcepInfo,
    IServiceProvider * pSrvProvider)
{
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    IDispatchEx *       pdispexElement = NULL;
    CDoc *              pDoc = Component()->_pDoc;
    
    if (!Component()->_pMarkup)
        RRETURN (E_UNEXPECTED);

#if DBG == 1
    DDAssertDispidRanges(dispid);
#endif

    //
    // gluing different script engines togather 
    //

    if (Component()->_pMarkup && pDoc->_pScriptCollection)
    {
        CScriptCollection * pScriptCollection = pDoc->_pScriptCollection;

        hr = THR_NOTRACE(pScriptCollection->InvokeEx(
            Component()->_pMarkup, dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
        if (DISP_E_MEMBERNOTFOUND != hr)   // if (S_OK or error other then DISP_E_UNKNOWNNAME)
            goto Cleanup;
    }

    //
    // access to id'd elements on the markup.
    //

    if (Component()->_pMarkup)
    {
        CCollectionCache *  pWindowCollection;

        hr = THR(Component()->_pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pWindowCollection = Component()->_pMarkup->CollectionCache();

        if (pWindowCollection->IsDISPIDInCollection(CMarkup::WINDOW_COLLECTION, dispid))
        {
            hr = THR_NOTRACE(pWindowCollection->Invoke(
                CMarkup::WINDOW_COLLECTION, dispid, IID_NULL, lcid, wFlags,
                pDispParams, pvarRes, pExcepInfo, NULL));
            
            goto Cleanup;
        }
    }
    
    if (IsStandardDispid(dispid))
    {
        //
        // access to doc frag
        //

        hr = THR_NOTRACE(Component()->_pMarkup->_OmDoc.InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
        if (DISP_E_MEMBERNOTFOUND != hr)    // if (S_OK == hr || (hr other then DISP_E_MEMBERNOTFOUND))
            goto Cleanup;                   // get out

        //
        // standard handling
        //

        hr = THR_NOTRACE(super::InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
        if (DISP_E_MEMBERNOTFOUND != hr)    // if (S_OK == hr || (hr other then DISP_E_MEMBERNOTFOUND))
            goto Cleanup;                   // get out
    }
    
    //
    // access to element's namespace
    //

    if (Component()->_pElement)
    {
        hr = THR(Component()->_pElement->QueryInterface(
            IID_IDispatchEx, (void **)&pdispexElement));
        if (hr)
            goto Cleanup;
            
        hr = THR_NOTRACE(pdispexElement->InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
    }
    
Cleanup:

    ReleaseInterface(pdispexElement);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::GetNameSpaceParent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!Component()->_pMarkup)
        RRETURN (E_UNEXPECTED);

    hr = THR(Component()->_pMarkup->_OmDoc.PrivateQueryInterface(IID_IDispatchEx, (void**)ppunk));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::get_element
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::get_element(IHTMLElement ** ppHTMLElement)
{
    HRESULT hr;
    
    if (Component()->_pElement)
    {
        hr = THR(Component()->_pElement->QueryInterface(IID_IHTMLElement, (void**) ppHTMLElement));
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDD::createEventObject
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDD::createEventObject(IHTMLEventObj ** ppEventObj)
{
    HRESULT hr;

    hr = THR(Component()->_pSiteOM->CreateEventObject(ppEventObj));

    RRETURN(SetErrorInfo(hr));
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent constructor
//
//-------------------------------------------------------------------------

CHtmlComponent::CHtmlComponent()
{
#if DBG == 1
    _DD._pComponentDbg = this;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent destructor
//
//-------------------------------------------------------------------------

CHtmlComponent::~CHtmlComponent()
{
    // force passivation of omdoc
    _DD.Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponent::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IPersistMoniker, NULL)
        QI_TEAROFF(this, IPersistPropertyBag2, NULL)
        QI_CASE(IConnectionPointContainer)
        {
            if (IID_IConnectionPointContainer == iid)
            {
                *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(this, NULL);
                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
            }
            break;
        }
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    if (IsEqualGUID(iid, CLSID_CHtmlComponent))
    {
        *ppv = this; // weak ref
        return S_OK;
    }

    if (_pProfferService)
    {
        HRESULT     hr;
        IUnknown *  pUnk;

        hr = THR_NOTRACE(_pProfferService->QueryService(iid, iid, (void**) &pUnk));
        if (S_OK == hr)
        {
            hr = THR(CreateTearOffThunk(
                    pUnk,
                    *(void **)pUnk,
                    NULL,
                    ppv,
                    this,
                    *(void **)(IUnknown*)(IPrivateUnknown*)this,
                    QI_MASK | ADDREF_MASK | RELEASE_MASK,
                    NULL));

            pUnk->Release();

            if (S_OK == hr)
            {
                ((IUnknown*)*ppv)->AddRef();
            }

            RRETURN (hr);
        }
    }

    RRETURN (super::PrivateQueryInterface(iid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::QueryService, per IServiceProvider
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    if (IsEqualGUID(rguidService, CLSID_CHtmlComponent))
    {
        RRETURN (PrivateQueryInterface(riid, ppvObject));
    }
    else if (IsEqualGUID(rguidService, SID_SProfferService))
    {
        if (!_pProfferService)
        {
            _pProfferService = new CProfferService();
            if (!_pProfferService)
            {
                RRETURN (E_OUTOFMEMORY);
            }
        }

        RRETURN (_pProfferService->QueryInterface(riid, ppvObject));
    }

    RRETURN(_pDoc->QueryService(rguidService, riid, ppvObject));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Load(
    BOOL        fFullyAvailable,
    IMoniker *  pMoniker,
    IBindCtx *  pBindContext,
    DWORD       grfMode)
{
    HRESULT                 hr;
    CDwnBindInfo *          pBSC = NULL;
    IBindCtx *              pBindCtx = NULL;

    if (!pMoniker)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // create tree and connect to it
    //

    hr = _pDoc->CreateMarkup(&_pMarkup);
    if (hr)
        goto Cleanup;

    hr = _pMarkup->EnsureBehaviorContext();
    if (hr)
        goto Cleanup;

    _pMarkup->BehaviorContext()->_pHtmlComponent = this; // this should happen before load

    //
    // launch download
    //

    pBSC = new CDwnBindInfo();
    if (!pBSC)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(CreateAsyncBindCtx(0, pBSC, NULL, &pBindCtx));
    if (hr)
        goto Cleanup;

    hr = THR(_pMarkup->Load(pMoniker, pBindCtx));
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

    // we could have passivated as a result of _pMarkup->Load causing an inline script to blow us away
    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (_pPropertyNotifySink)
    {
        _pPropertyNotifySink->OnChanged(DISPID_READYSTATE);
    }

Cleanup:
    ReleaseInterface (pBindCtx);
    if (pBSC)
        pBSC->Release();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Init(IElementBehaviorSite * pSite)
{
    if (_pSite)         // if already initialized
        return S_OK;

    HRESULT                 hr;
    IServiceProvider *      pSP = NULL;

    //
    // get site interface pointers
    //
    
    hr = THR(super::Init(pSite));
    if (hr)
        goto Cleanup;

    hr = THR(_pSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&_pSiteOM));
    if (hr)
        goto Cleanup;

    hr = THR(_pSite->QueryInterface(IID_IPropertyNotifySink,(void**)&_pPropertyNotifySink));
    if (hr)
        goto Cleanup;

    //
    // get the doc
    //

    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    hr = THR(pSP->QueryService(CLSID_HTMLDocument, CLSID_HTMLDocument, (void**)&_pDoc));
    if (hr)
        goto Cleanup;

    Assert (_pDoc);
    _pDoc->SubAddRef();

Cleanup:
    ReleaseInterface (pSP);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Passivate
//
//-------------------------------------------------------------------------

void
CHtmlComponent::Passivate()
{
    ClearInterface (&_pSiteOM);
    ClearInterface (&_pPropertyNotifySink);

    if (_pMarkup)
    {
        CMarkupBehaviorContext *    pMarkupBehaviorContext = _pMarkup->BehaviorContext();

        if (pMarkupBehaviorContext)
        {
            // remove back pointer - important if someone else keeps reference on the tree
            pMarkupBehaviorContext->_pHtmlComponent = NULL;
        }

        // Make sure the download stops.
        IGNORE_HR( _pMarkup->StopDownload() );
        _pMarkup->Release();
        _pMarkup = NULL;
    }
    
    _pDoc->SubRelease();

    if (_pProfferService)
        _pProfferService->Release();

    // super
    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::IsRecursiveUrl, helper
//
//-------------------------------------------------------------------------

BOOL
CHtmlComponent::IsRecursiveUrl(LPTSTR pchUrl)
{
    HRESULT                     hr;
    TCHAR                       achUrlExpanded1[pdlUrlLen];
    TCHAR                       achUrlExpanded2[pdlUrlLen];
    CMarkup *                   pMarkup;
    CMarkupScriptContext *      pMarkupScriptContext;
    CMarkupBehaviorContext *    pMarkupBehaviorContext;

    Assert (_pElement && pchUrl && pchUrl[0]);

    hr = THR(_pElement->Doc()->ExpandUrl(
        pchUrl, ARRAY_SIZE(achUrlExpanded1), achUrlExpanded1, _pElement));
    if (hr)
        goto Cleanup;

    pMarkup = _pElement->GetMarkup();

    // walk up parent markup chain
    while (pMarkup)
    {
        pMarkupScriptContext = pMarkup->ScriptContext();
        if (!pMarkupScriptContext)
            break;

        if (pMarkupScriptContext->_cstrUrl.Length())
        {
            hr = THR(_pElement->Doc()->ExpandUrl(
                pMarkupScriptContext->_cstrUrl, ARRAY_SIZE(achUrlExpanded2), achUrlExpanded2, _pElement));
            if (hr)
                goto Cleanup;

            if (0 == StrCmpIC(achUrlExpanded1, achUrlExpanded2))
            {
                TraceTag((tagError, "Detected recursion in HTC!"));
                return TRUE;
            }
        }

        pMarkupBehaviorContext = pMarkup->BehaviorContext();
        if (!pMarkupBehaviorContext ||
            !pMarkupBehaviorContext->_pHtmlComponent ||
            !pMarkupBehaviorContext->_pHtmlComponent->_pElement)
            break;

        pMarkup = pMarkupBehaviorContext->_pHtmlComponent->_pElement->GetMarkup();
    }

Cleanup:
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    hr = THR(FireNotification(lEvent, pVar));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Detach, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Detach(void)
{
    HRESULT     hr;
    LONG        idx;
    CElement *  pElement;

    Assert(_pMarkup);   // passivate better not have been called
    
    //
    // call detachEvent on all attach tags
    //

    for (idx = 0;; idx++)
    {
        hr = THR(GetHtcElement(&idx, HTC_BEHAVIOR_ATTACH, &pElement));
        if (hr)
            goto Cleanup;
        if (!pElement)
            break;

        {
            CInvoke invoke(pElement);

            hr = THR(invoke.Invoke(DISPID_CHtmlComponentAttach_detachEvent, DISPATCH_METHOD));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Load
//
//----------------------------------------------------------------------------

void
CHtmlComponent::OnLoadStatus(LOADSTATUS LoadStatus)
{
    TraceTag((
        tagHtcOnLoadStatus,
        "CHtmlComponent::OnLoadStatus, load status: %ld",
        LoadStatus));

    switch (LoadStatus)
    {
    case LOADSTATUS_QUICK_DONE:

        if (_fContentReadyPending)
        {
            IGNORE_HR(FireNotification (BEHAVIOREVENT_CONTENTREADY, NULL));
        }

        if (_fDocumentReadyPending)
        {
            IGNORE_HR(FireNotification (BEHAVIOREVENT_DOCUMENTREADY, NULL));
        }

        if (_pPropertyNotifySink)
        {
            _pPropertyNotifySink->OnChanged(DISPID_READYSTATE);
        }
        
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetHtcElement, helper
//
//  Returns:    hr == S_OK, pElement != NULL:       found the element
//              hr == S_OK, pElement == NULL:       reached the end of collection
//              hr != S_OK:                         fatal error
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetHtcElement(LONG * pIdx, HTC_BEHAVIOR_TYPE typeRequested, CElement ** ppElement)
{
    HRESULT             hr;
    CCollectionCache *  pCollection;
    LONG                idx = 0;
    HTC_BEHAVIOR_TYPE   typeElement;

    if (!_pMarkup)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (!pIdx)
        pIdx = &idx;

    hr = THR(_pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollection = _pMarkup->CollectionCache();

    for (;; (*pIdx)++)
    {
        hr = THR_NOTRACE(pCollection->GetIntoAry(
            CMarkup::ELEMENT_COLLECTION, *pIdx, ppElement));
        Assert (S_FALSE != hr);
        if (DISP_E_MEMBERNOTFOUND == hr)        // if reached end of collection
        {
            hr = S_OK;
            *ppElement = NULL;
            goto Cleanup;
        }
        if (hr)                                 // if fatal error
            goto Cleanup;

        if (ETAG_GENERIC_BUILTIN != (*ppElement)->Tag())
            continue;

        typeElement = TagNameToHtcBehaviorType((*ppElement)->TagName());

        if (typeElement & typeRequested)
        {
            Assert (S_OK == hr);
            break;
        }
    }

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::AttachNotification, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::AttachNotification(DISPID dispid, IDispatch * pdispHandler)
{
    HRESULT hr;
    hr = THR(AddDispatchObjectMultiple(dispid, pdispHandler, CAttrValue::AA_AttachEvent));
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::FireNotification, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::FireNotification(LONG lEvent, VARIANT *pVar)
{
    HRESULT     hr = S_OK;
    DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
    DISPID      dispidEvent = 0;

    if (!_pMarkup)
        goto Cleanup;
        
    // ! don't fire the notifications before the whole tree is downloaded -
    // some <attach> tags may still be downloaded later. In that case defer firing
    // using _fContentReadyPending and _fDocumentReadyPending bit
    //
    // CONSIDER: use generic type queue for deferring firing events
    //

    switch (lEvent)
    {
    case BEHAVIOREVENT_CONTENTREADY:

        if (_pMarkup->LoadStatus() < LOADSTATUS_QUICK_DONE)
        {
            _fContentReadyPending = TRUE;
            goto Cleanup;   // get out
        }

        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY;

        break;

    case BEHAVIOREVENT_DOCUMENTREADY:

        if (_pMarkup->LoadStatus() < LOADSTATUS_QUICK_DONE)
        {
            _fDocumentReadyPending = TRUE;
            goto Cleanup;   // get out
        }

        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY;
        break;

    case BEHAVIOREVENT_APPLYSTYLE:
        if (pVar)
        {
            dispParams.rgvarg = pVar;
            dispParams.cArgs = 1;
        }
        dispidEvent = DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE;
        break;
        
    default:
        Assert (0 && "a notification not implemented in HTC");
        goto Cleanup;
    }

    IGNORE_HR(FireAttachEvents(dispidEvent, &dispParams, NULL, _pDoc));

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT             hr = DISP_E_UNKNOWNNAME;
    STRINGCOMPAREFN     pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;
    CElement *          pElement;
    LPCTSTR             pchName;
    long                idx;

    if (!_pMarkup)
        goto Cleanup;
        
    //
    // Search markup for all property and method tags.
    //

    for (idx = 0;; idx++)
    {
        hr = THR(GetHtcElement(&idx, HTC_BEHAVIOR_PROPERTYORMETHOD, &pElement));
        if (hr)
            goto Cleanup;
        if (!pElement)
        {
            hr = DISP_E_UNKNOWNNAME;
            break;
        }

        pchName = GetExpandoString(pElement, _T("name"));
        if (!pchName)
            continue;
            
        if (0 == pfnStrCmp(pchName, bstrName))
        {
            *pdispid = DISPID_COMPONENTBASE + idx;
            break;
        }
    }
    
Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::InvokeEx(
    DISPID          dispid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarRes,
    EXCEPINFO *     pExcepInfo,
    IServiceProvider * pSrvProvider)
{
    CElement *          pElement;
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    IDispatchEx *       pdexElement = NULL;

    if (!_pMarkup)
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_READYSTATE:
        V_VT(pvarRes) = VT_I4;
        hr = THR(GetReadyState((READYSTATE*)&V_I4(pvarRes)));
        goto Cleanup; // done
    }

    //
    // find the item
    //

    hr = THR_NOTRACE(GetHtcElement(dispid - DISPID_COMPONENTBASE, HTC_BEHAVIOR_PROPERTYORMETHOD, &pElement));
    if (hr)
        goto Cleanup;
    if (!pElement)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    //
    // invoke the item
    //

    hr = THR(pElement->QueryInterface(IID_IDispatchEx, (void **)&pdexElement));
    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pdexElement->InvokeEx(DISPID_A_HTCDISPATCHITEM_VALUE,
        lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));

Cleanup:
    if (hr)
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }
    ReleaseInterface(pdexElement);
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetNextDispID, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetNextDispID(
    DWORD       grfdex,
    DISPID      dispid,
    DISPID *    pdispid)
{
    CElement *      pElement;
    HRESULT         hr;

    if (!_pMarkup)
    {
        hr = S_FALSE;
        *pdispid = DISPID_UNKNOWN;
        goto Cleanup;
    }
    
    // offset from dispid range to array idx range

    if (-1 != dispid)
    {
        dispid -= DISPID_COMPONENTBASE;
    }

    dispid++;

    // get the next index

    hr = THR(GetHtcElement(&dispid, HTC_BEHAVIOR_PROPERTYOREVENT, &pElement));
    if (hr)
        goto Cleanup;
    if (!pElement)
    {
        hr = S_FALSE;
        *pdispid = DISPID_UNKNOWN;
        goto Cleanup;
    }

    *pdispid = DISPID_COMPONENTBASE + dispid;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetMemberName, helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetMemberName(DISPID dispid, BSTR * pbstrName)
{
    HRESULT     hr;
    CElement *  pElement;

    hr = THR(GetHtcElement(dispid - DISPID_COMPONENTBASE, HTC_BEHAVIOR_PROPERTYORMETHODOREVENT, &pElement));
    if (hr)
        goto Cleanup;
    if (!pElement)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR(FormsAllocString(GetExpandoString(pElement, _T("name")), pbstrName));
    if (hr)
        goto Cleanup;

Cleanup:        
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::GetReadyState
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::GetReadyState(READYSTATE * pReadyState)
{
    HRESULT     hr = S_OK;

    Assert (pReadyState);

    *pReadyState = _pMarkup && _pMarkup->LoadStatus() < LOADSTATUS_QUICK_DONE ?
                        READYSTATE_LOADING :
                        READYSTATE_COMPLETE;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::Save
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::Save(IPropertyBag2 * pPropBag2, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    LONG                idx;
    LPCTSTR             pchName;
    IPropertyBag *      pPropBag = NULL;
    CElement *          pElement;

    if (!_pMarkup)
        goto Cleanup;
        
    hr = THR(pPropBag2->QueryInterface(IID_IPropertyBag, (void**)&pPropBag));
    if (hr)
        goto Cleanup;

    //
    // for every property tag in the markup...
    //

    for (idx = 0;; idx++)
    {
        hr = THR(GetHtcElement(&idx, HTC_BEHAVIOR_PROPERTY, &pElement));
        if (hr)
            goto Cleanup;
        if (!pElement)
            break;

        pchName = GetExpandoString(pElement, _T("name"));
        if (!pchName || !HasExpando(pElement, _T("persist")))
            continue;

        {
            CInvoke invoke(pElement);
        
            hr2 = THR_NOTRACE(invoke.Invoke(DISPID_A_HTCDISPATCHITEM_VALUE, DISPATCH_PROPERTYGET));

            if (S_OK == hr2 &&
                VT_NULL  != V_VT(invoke.Res()) &&
                VT_EMPTY != V_VT(invoke.Res()))
            {
                hr = THR(pPropBag->Write(pchName, invoke.Res()));
                if (hr)
                    goto Cleanup;
            }
        }
    }
    
Cleanup:
    ReleaseInterface (pPropBag);

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponent::FindBehavior, static helper
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponent::FindBehavior(HTC_BEHAVIOR_TYPE type, IElementBehaviorSite * pSite, IElementBehavior ** ppBehavior)
{
    HRESULT                 hr = E_FAIL;
    CHtmlComponentBase *    pBehaviorItem = NULL;

    switch (type)
    {
    case HTC_BEHAVIOR_DESC:
        pBehaviorItem = new CHtmlComponentDesc();
        break;

    case HTC_BEHAVIOR_PROPERTY:
        pBehaviorItem = new CHtmlComponentProperty();
        break;

    case HTC_BEHAVIOR_METHOD:
        pBehaviorItem = new CHtmlComponentMethod();
        break;

    case HTC_BEHAVIOR_EVENT:
        pBehaviorItem = new CHtmlComponentEvent();
        break;

    case HTC_BEHAVIOR_ATTACH:
        pBehaviorItem = new CHtmlComponentAttach();
        break;

    default:
        Assert (FAILED(hr));
        goto Cleanup;
    }

    if (!pBehaviorItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pBehaviorItem->PrivateQueryInterface(IID_IElementBehavior, (void**)ppBehavior));

Cleanup:
    if (pBehaviorItem)
        pBehaviorItem->PrivateRelease();

    RRETURN (hr);
}


///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentProperty
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentProperty::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    EnsureComponent();

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCPropertyBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::Notify
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));

    switch (lEvent)
    {
    case BEHAVIOREVENT_CONTENTREADY:
        EnsureHtmlLoad(/* fScriptsOnly =*/TRUE);
        break;

    case BEHAVIOREVENT_DOCUMENTREADY:
        EnsureHtmlLoad(/* fScriptsOnly =*/FALSE);
        break;
    }


    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::EnsureHtmlLoad
//
//-------------------------------------------------------------------------

void
CHtmlComponentProperty::EnsureHtmlLoad(BOOL fScriptsOnly)
{
    HRESULT     hr;

    if (_fHtmlLoadEnsured)
        return;

    EnsureComponent();
    if (!_pComponent || !_pElement)
        return;

    hr = THR_NOTRACE(HtmlLoad(fScriptsOnly));
    if (S_OK == hr || 
        !_pComponent->_pMarkup ||
        LOADSTATUS_QUICK_DONE <= _pComponent->_pMarkup->LoadStatus())
    {
        _fHtmlLoadEnsured = TRUE;
    }

    TraceTag((
        tagHtcPropertyEnsureHtmlLoad,
        "CHtmlComponentProperty::EnsureHtmlLoad, element SN: %ld, name: %ls, internal name: %ls, ensured: %ls",
        _pElement ? _pElement->SN() : 0,
        ExternalName(), InternalName(), _fHtmlLoadEnsured ? _T("TRUE") : _T("FALSE")));
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::HtmlLoad
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::HtmlLoad(BOOL fScriptsOnly)
{
    HRESULT     hr = S_OK;
    HRESULT     hr2;
    DISPID      dispid;

    //
    // load attribute from element
    //

    hr2 = THR_NOTRACE(_pComponent->_pElement->GetExpandoDispID(ExternalName(), &dispid, 0));
    if (S_OK == hr2)
    {
        CInvoke     invoke(this);
        DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
        EXCEPINFO   excepInfo;

        invoke.AddArg();

        hr = THR(_pComponent->_pElement->InvokeAA(
            dispid, CAttrValue::AA_Expando, LCID_SCRIPTING,
            DISPATCH_PROPERTYGET, &dispParams, invoke.Arg(0), &excepInfo, NULL));
        if (hr)
            goto Cleanup;

        invoke.AddNamedArg(DISPID_PROPERTYPUT);

        hr = THR(invoke.Invoke(
            fScriptsOnly ? 
                DISPID_A_HTCDISPATCHITEM_VALUE_SCRIPTSONLY :
                DISPID_A_HTCDISPATCHITEM_VALUE,
            DISPATCH_PROPERTYPUT));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider)
{
    HRESULT             hr;

    switch (dispid)
    {
    case DISPID_A_HTCDISPATCHITEM_VALUE:

        if (wFlags & DISPATCH_PROPERTYGET)
        {
            EnsureHtmlLoad(/* fScriptsOnly = */FALSE);
        }

        hr = THR_NOTRACE(InvokeItem(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider,
            /* fScriptsOnly = */ FALSE));
        break;

    case DISPID_A_HTCDISPATCHITEM_VALUE_SCRIPTSONLY:

        hr = THR_NOTRACE(InvokeItem(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider,
            /* fScriptsOnly = */ TRUE));

        break;

    default:
        hr = THR_NOTRACE(super::InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
        break;
    }

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::InvokeItem, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::InvokeItem(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceProvider,
    BOOL                fScriptsOnly)
{
    HRESULT     hr;
    BOOL        fNameImpliesScriptsOnly;
    LPTSTR      pchInternalName = InternalName(&fNameImpliesScriptsOnly, &wFlags, pDispParams);

    EnsureComponent();

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // first, invoke script engines
    //

    hr = THR_NOTRACE(InvokeEngines(pchInternalName, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));
    if (DISP_E_UNKNOWNNAME != hr)   // if (S_OK == hr || DISP_E_UNKNOWNNAME != hr)
        goto Cleanup;               // done

    //
    // now do our own stuff
    //

    if (!fNameImpliesScriptsOnly &&
        !fScriptsOnly)
    {
        if (DISPATCH_PROPERTYGET & wFlags)
        {
            // see if there is expando "value"
            hr = THR_NOTRACE(_pElement->GetExpandoDispID(_T("value"), &dispid, 0));
            if (hr)
            {
                // not found, return null
                hr = S_OK;
                V_VT(pvarRes) = VT_NULL;
                goto Cleanup;       // done
            }

            hr = THR(_pElement->InvokeAA(
                dispid, CAttrValue::AA_Expando, lcid,
                wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

        }
        else if (DISPATCH_PROPERTYPUT & wFlags)
        {
            // set expando "value"

            hr = THR_NOTRACE(_pElement->GetExpandoDispID(_T("value"), &dispid, fdexNameEnsure));
            if (hr)
                goto Cleanup;

            hr = THR(_pElement->InvokeAA(
                dispid, CAttrValue::AA_Expando, lcid, wFlags, pDispParams, NULL, pExcepInfo, pServiceProvider));
        }
    }

Cleanup:
    if (S_OK == hr && (DISPATCH_PROPERTYPUT & wFlags))
    {
        IGNORE_HR(fireChange());
    }

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::fireChange
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentProperty::fireChange()
{
    HRESULT     hr;

    EnsureComponent();

    if (!_pComponent ||         // if no component
        !_fHtmlLoadEnsured)     // or have not loaded yet from html
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(_pComponent->_pPropertyNotifySink->OnChanged(
            _pElement->GetSourceIndex() + DISPID_COMPONENTBASE));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentProperty::put_value, 
//              CHtmlComponentProperty::get_value
//
//-------------------------------------------------------------------------

HRESULT CHtmlComponentProperty::put_value(VARIANT ) { Assert (0 && "not implemented"); RRETURN (E_NOTIMPL); }
HRESULT CHtmlComponentProperty::get_value(VARIANT*) { Assert (0 && "not implemented"); RRETURN (E_NOTIMPL); }

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentMethod
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentMethod::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentMethod::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    EnsureComponent();
    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    //QI_TEAROFF(this, IHTCMethodBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentMethod::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentMethod::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT             hr;

    if (DISPID_A_HTCDISPATCHITEM_VALUE == dispid &&
        (DISPATCH_METHOD & wFlags))
    {
        EnsureComponent();

        hr = THR_NOTRACE(InvokeEngines(
            InternalName(), DISPATCH_METHOD, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
    }
    else
    {
        hr = THR_NOTRACE(super::InvokeEx(
            dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pSrvProvider));
    }

    RRETURN(SetErrorInfo(hr));
}


///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentEvent
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentEvent::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    EnsureComponent();
    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCEventBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::OnComponentSet
//
//-------------------------------------------------------------------------

void
CHtmlComponentEvent::OnComponentSet()
{
    LPTSTR  pchName;
    
    if (!_pElement || !_pComponent)
        return;
        
    pchName = ExternalName();
    if (pchName)
    {
        LONG    lFlags;
        lFlags = HasExpando(_pElement, _T("bubble")) ? BEHAVIOREVENTFLAGS_BUBBLE : 0;
        
        IGNORE_HR(_pComponent->_pSiteOM->RegisterEvent(pchName, lFlags, &_lCookie));
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentEvent::fire
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentEvent::fire(IHTMLEventObj * pEventObj)
{
    HRESULT     hr;

    EnsureComponent();
    if (!_pComponent)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(_pComponent->_pSiteOM->FireEvent(_lCookie, pEventObj));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentAttach
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHtmlComponentAttach::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    EnsureComponent();

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTCAttachBehavior, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Init(IElementBehaviorSite * pSite)
{
    HRESULT     hr;

    hr = THR(super::Init(pSite));
    if (hr)
        goto Cleanup;

    hr = THR(_pSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&_pSiteOM));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Passivate, virtual
//
//-------------------------------------------------------------------------

void
CHtmlComponentAttach::Passivate()
{
    ClearInterface (&_pSiteOM);
    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetEventName
//
//-------------------------------------------------------------------------

inline LPTSTR
CHtmlComponentAttach::GetEventName()
{
    return GetExpandoString(_pElement, _T("event"));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetEventSource
//
//-------------------------------------------------------------------------

CBase *
CHtmlComponentAttach::GetEventSource()
{
    HRESULT     hr;
    CBase *     pSource = NULL;
    LPTSTR      pchFor;
    LPTSTR      pchEvent;

    pchFor = GetExpandoString(_pElement, _T("for"));
    if (pchFor)
    {
        if (0 == StrCmpIC(_T("window"), pchFor))                    // window
        {
            CDoc * pDoc = _pElement->Doc();

            hr = THR(pDoc->EnsureOmWindow());
            if (hr)
                goto Cleanup;

            pSource = (CBase*)pDoc->_pOmWindow;
        }
        else if (0 == StrCmpIC(_T("document"), pchFor))             // document
        {
            pSource = _pElement->Doc();
        }
        else if (0 == StrCmpIC(_T("element"), pchFor))              // element
        {
            pSource = _pComponent->_pElement;
        }
    }
    else
    {
        pSource = _pComponent->_pElement;
    }

    /// reset pSource for any peer notifications if the source is specified to be the element
    if (pSource && pSource == _pComponent->_pElement)
    {
        pchEvent = GetExpandoString(_pElement, _T("EVENT"));
        if (pchEvent && CNotifications::Find(pchEvent))
        {
            pSource = NULL;
        }
    }

Cleanup:

    return pSource;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::GetEventHandler
//
//-------------------------------------------------------------------------

IDispatch *
CHtmlComponentAttach::GetEventHandler()
{
    HRESULT         hr;
    IDispatch *     pdispHandler = NULL;
    LPTSTR          pchHandler;

    if (!_pElement->Doc()->_pScriptCollection)
        goto Cleanup;

    pchHandler = GetExpandoString(_pElement, _T("handler"));
    if (!pchHandler)
        goto Cleanup;

    //
    // get IDispatch from script engines
    //

    {
        VARIANT     varRes;
        DISPPARAMS  dispParams = {NULL, NULL, 0, 0};
        EXCEPINFO   excepInfo;

        hr = THR_NOTRACE(InvokeEngines(
            pchHandler, DISPATCH_PROPERTYGET, &dispParams, &varRes, &excepInfo, NULL));
        if (hr)
            goto Cleanup;

        if (VT_DISPATCH != V_VT(&varRes))
            goto Cleanup;

        pdispHandler = V_DISPATCH(&varRes);
    }

Cleanup:

    return pdispHandler;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::CreateEventObject
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::CreateEventObject(IDispatch * pdispArg, IHTMLEventObj ** ppEventObject)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    IHTMLEventObj2 *    pEventObject2 = NULL;
    IHTMLStyle2 *       pStyle = NULL;

    Assert (ppEventObject);

    hr = THR(_pSiteOM->CreateEventObject(ppEventObject));
    if (hr)
        goto Cleanup;

    hr = THR((*ppEventObject)->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObject2));
    if (hr)
        goto Cleanup;

    if (pdispArg)
    {
        // if the argument is a style object
        hr2 = THR_NOTRACE(pdispArg->QueryInterface(IID_IHTMLStyle2, (void**)&pStyle));
        if (S_OK == hr2)
        {
            CVariant    varStyle;

            // set "style" attribute on the event object

            V_VT(&varStyle) = VT_DISPATCH;
            V_DISPATCH(&varStyle) = pdispArg;
            pdispArg->AddRef();

            hr = THR(pEventObject2->setAttribute(_T("style"), varStyle, 0));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface (pEventObject2);
    ReleaseInterface (pStyle);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::FireHandler
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::FireHandler(IHTMLEventObj * pEventObject)
{
    HRESULT     hr;

    // NOTE any change here might have to be mirrored in CPeerHolder::CPeerSite::FireEvent

    if (HTMLEVENTOBJECT_USESAME == pEventObject)
    {
        hr = THR(FireHandler2(NULL));
    }
    else if (pEventObject)
    {
        CEventObj::COnStackLock onStackLock(pEventObject);

        hr = THR(FireHandler2(pEventObject));
    }
    else
    {
        EVENTPARAM param(_pElement->Doc(), TRUE);

        hr = THR(FireHandler2(pEventObject));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::FireHandler2
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::FireHandler2(IHTMLEventObj * pEventObject)
{
    HRESULT     hr = S_OK;
    IDispatch * pdispHandler = GetEventHandler();

    if (pdispHandler)
    {
        CInvoke     invoke(pdispHandler);
        VARIANT *   pvar;

        if (pEventObject)
        {
            hr = THR(invoke.AddArg());
            if (hr)
                goto Cleanup;

            pvar = invoke.Arg(0);
            V_VT(pvar) = VT_DISPATCH;
            V_DISPATCH(pvar) = pEventObject;
            V_DISPATCH(pvar)->AddRef();
        }
        
        hr = THR(invoke.Invoke(DISPID_VALUE, DISPATCH_METHOD));
    }

Cleanup:
    ReleaseInterface(pdispHandler);
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::fireEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::fireEvent(IDispatch * pdispArg)
{
    HRESULT             hr = S_OK;
    LONG                lCookie;
    IHTMLEventObj *     pEventObject = NULL;

    if (_pComponent && _pComponent->_pElement)
    {
        CLock       lockComponent(_pComponent);
    
        //
        // setup parameters
        //

        if (_fEvent)
        {
            pEventObject = HTMLEVENTOBJECT_USESAME;
        }
        else
        {
            hr = THR(CreateEventObject(pdispArg, &pEventObject));
            if (hr)
                goto Cleanup;
        }

        //
        // fire
        //

        hr = THR(_pSiteOM->GetEventCookie(_T("onevent"), &lCookie));
        if (hr)
            goto Cleanup;

        hr = THR(_pSiteOM->FireEvent(lCookie, pEventObject));
        if (hr)
            goto Cleanup;

        IGNORE_HR(FireHandler(pEventObject));
    }

Cleanup:
    if (HTMLEVENTOBJECT_USESAME != pEventObject)
    {
        ReleaseInterface(pEventObject);
    }

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::SinkEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::SinkEvent(BOOL fAttach)
{
    HRESULT         hr = S_OK;
    CBase *         pSource;
    LPTSTR          pchName;
    BSTR            bstrName = NULL;
    IDispatchEx *   pdispexThis = NULL;

    pSource = GetEventSource();
    if (!pSource)
        goto Cleanup;

    pchName = GetEventName();
    if (!pchName)
        goto Cleanup;

    hr = THR(FormsAllocString(pchName, &bstrName));
    if (hr)
        goto Cleanup;

    hr = THR(PrivateQueryInterface(IID_IDispatchEx, (void**)&pdispexThis));
    if (hr)
        goto Cleanup;

    if (fAttach)
    {
        IGNORE_HR(pSource->attachEvent(bstrName, pdispexThis, NULL));
        _fEvent = TRUE;
    }
    else
    {
        IGNORE_HR(pSource->detachEvent(bstrName, pdispexThis));
    }

Cleanup:
    ReleaseInterface(pdispexThis);
    FormsFreeString(bstrName);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Attach1
//
//  Synopsis:   registers the event and attaches self to:
//              - event of element this htc is associated with,
//              - element.document or
//              - window
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Attach1()
{
    EnsureComponent();

    if (!_pComponent || !_pElement)
        RRETURN (E_UNEXPECTED);
        
    HRESULT                     hr;
    IElementBehaviorSiteOM *    pSiteOM = NULL;

    hr = THR(_pSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&pSiteOM));
    if (hr)
        goto Cleanup;

    hr = THR(pSiteOM->RegisterEvent(_T("onevent"), 0, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(SinkEvent(/* fAttach = */TRUE));

Cleanup:
    ReleaseInterface(pSiteOM);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Attach2
//
//  Synopsis:   attaches to internally fired notifications
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Attach2()
{
    HRESULT                     hr = S_OK;
    LPTSTR                      pchName;
    IDispatchEx *               pdispexThis = NULL;
    CNotifications::CItem *     pNotification;
    
    if (_fEvent)
        goto Cleanup;

    if (GetEventSource())
        goto Cleanup;

    pchName = GetEventName();
    if (!pchName)
        goto Cleanup;

    hr = THR(PrivateQueryInterface(IID_IDispatchEx, (void**)&pdispexThis));
    if (hr)
        goto Cleanup;

    pNotification = CNotifications::Find(pchName);
    if (pNotification)
    {
        if (-1 != pNotification->_dispidInternal)
        {
            hr = THR(_pComponent->AttachNotification(pNotification->_dispidInternal, pdispexThis));
            if (hr)
                goto Cleanup;
        }

        if (-1 != pNotification->_lEvent)
        {
            hr = THR(_pComponent->_pSite->RegisterNotification(pNotification->_lEvent));
        }
    }
    
Cleanup:
    ReleaseInterface(pdispexThis);
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::Notify
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    hr = THR(super::Notify(lEvent, pVar));
    if (hr || !_pComponent)
        goto Cleanup;

    switch (lEvent)
    {
    case BEHAVIOREVENT_CONTENTREADY:

        IGNORE_HR(Attach1());

        break;

    case BEHAVIOREVENT_DOCUMENTREADY:

        Assert (S_OK == TestProfferService());

        IGNORE_HR(Attach2());

        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::detachEvent
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::detachEvent()
{
    HRESULT     hr = S_OK;
    LPTSTR      pchName;

    pchName = GetEventName();
    if (!pchName)
        goto Cleanup;

    if (0 == StrCmpIC(_T("onDetach"), pchName))
    {
        fireEvent(NULL);
    }

    hr = THR(SinkEvent(/* fAttach = */FALSE));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CHtmlComponentDesc
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentDesc::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentDesc::Notify(LONG lEvent, VARIANT * pVar)
{
    HRESULT     hr;

    if (!_pElement)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    hr = THR(super::Notify(lEvent, pVar));
    if (hr)
        goto Cleanup;

    switch (lEvent)
    {
    case BEHAVIOREVENT_DOCUMENTREADY:

        {
            LPTSTR      pch;

            EnsureComponent();
            if (_pComponent)
            {
                pch = GetExpandoString(_pElement, _T("urn"));
                if (pch)
                {
                    IGNORE_HR(_pComponent->_pSiteOM->RegisterUrn(pch));
                }

                pch = GetExpandoString(_pElement, _T("name"));
                if (pch)
                {
                    IGNORE_HR(_pComponent->_pSiteOM->RegisterName(pch));
                }
            }
        }

        break;
    }
    
Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// Proffer service testing, DEBUG ONLY code
//
///////////////////////////////////////////////////////////////////////////

#if DBG == 1

const IID IID_IProfferTest = {0x3050f5d7, 0x98b5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B};

class CProfferTestObj : public IServiceProvider
{
public:
    CProfferTestObj() { _ulRefs = 1; }
    DECLARE_FORMS_STANDARD_IUNKNOWN(CProfferTestObj);
    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppv);
};

HRESULT
CProfferTestObj::QueryInterface(REFIID riid, void ** ppv)
{
    if (IsEqualGUID(riid, IID_IProfferTest))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }

    Assert (0 && "CProfferTestObj should never be QI-ed with anything but IID_IProfferTest");

    return E_NOTIMPL;
}

HRESULT
CProfferTestObj::QueryService(REFGUID rguidService, REFIID riid, void ** ppv)
{
    Assert (IsEqualGUID(rguidService, riid));
    RRETURN (QueryInterface(riid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlComponentAttach::TestProfferService, DEBUG ONLY code
//
//-------------------------------------------------------------------------

HRESULT
CHtmlComponentAttach::TestProfferService()
{
    HRESULT             hr;
    IServiceProvider *  pSP = NULL;
    IProfferService *   pProfferService = NULL;
    DWORD               dwServiceCookie;
    IUnknown *          pTestInterface = NULL;
    IUnknown *          pTestInterface2 = NULL;
    CProfferTestObj *   pProfferTest;

    pProfferTest = new CProfferTestObj();
    if (!pProfferTest)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // register the interface with HTC
    //

    hr = THR(_pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP));
    if (hr)
        goto Cleanup;

    hr = THR(pSP->QueryService(SID_SProfferService, IID_IProfferService, (void**)&pProfferService));
    if (hr)
        goto Cleanup;

    hr = THR(pProfferService->ProfferService(IID_IProfferTest, pProfferTest, &dwServiceCookie));
    if (hr)
        goto Cleanup;

    //
    // verify the interface is QI-able from HTC behavior and properly thunked
    //

    hr = THR(_pComponent->_pElement->GetPeerHolder()->QueryPeerInterfaceMulti(
        IID_IProfferTest, (void**)&pTestInterface, /* fIdentityOnly = */FALSE));
    if (hr)
        goto Cleanup;

    pTestInterface->AddRef();
    pTestInterface->Release();

    hr = THR(pTestInterface->QueryInterface(IID_IProfferTest, (void**)&pTestInterface2));
    if (hr)
        goto Cleanup;

    pTestInterface2->AddRef();
    pTestInterface2->Release();

Cleanup:
    ReleaseInterface (pSP);
    ReleaseInterface (pProfferService);
    ReleaseInterface (pTestInterface);
    ReleaseInterface (pTestInterface2);
    if (pProfferTest)
        pProfferTest->Release();

    RRETURN(hr);
}

#endif

///////////////////////////////////////////////////////////////////////////
//
// CProfferService
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CProfferService constructor
//
//-------------------------------------------------------------------------

CProfferService::CProfferService()
{
    _ulRefs = 1;
};

//+------------------------------------------------------------------------
//
//  Member:     CProfferService destructor
//
//-------------------------------------------------------------------------

CProfferService::~CProfferService()
{
    int i, c;

    for (i = 0, c = _aryItems.Size(); i < c; i++)
    {
        delete _aryItems[i];
    }

    _aryItems.DeleteAll();
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CProfferService::QueryInterface(REFIID iid, void **ppv)
{

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IProfferService)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN (E_NOTIMPL);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::ProfferService, per IProfferService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::ProfferService(REFGUID rguidService, IServiceProvider * pSP, DWORD * pdwCookie)
{
    HRESULT                 hr;
    CProfferServiceItem *   pItem;

    pItem = new CProfferServiceItem(rguidService, pSP);
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_aryItems.Append(pItem));
    if (hr)
        goto Cleanup;

    if (!pdwCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pdwCookie = _aryItems.Size() - 1;

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::RevokeService, per IProfferService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::RevokeService(DWORD dwCookie)
{
    if ((DWORD)_aryItems.Size() <= dwCookie)
    {
        RRETURN (E_INVALIDARG);
    }

    delete _aryItems[dwCookie];
    _aryItems[dwCookie] = NULL;

    RRETURN (S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CProfferService::QueryService
//
//-------------------------------------------------------------------------

HRESULT
CProfferService::QueryService(REFGUID rguidService, REFIID riid, void ** ppv)
{
    CProfferServiceItem *   pItem;
    int                     i, c;

    for (i = 0, c = _aryItems.Size(); i < c; i++)
    {
        pItem = _aryItems[i];
        if (pItem && IsEqualGUID(pItem->_guidService, rguidService))
        {
            RRETURN (pItem->_pSP->QueryService(rguidService, riid, ppv));
        }
    }

    RRETURN (E_NOTIMPL);
}
