//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       window.cxx
//
//  Contents:   Implementation of COmWindow2, CScreen classes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"   // for a brief reference to cframesite.
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h" // idispatchex
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef WIN16
#ifndef X_HTMLHELP_H_
#define X_HTMLHELP_H_
#include "htmlhelp.h"
#endif
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTAINERPLUS_HXX_
#define X_DISPCONTAINERPLUS_HXX_
#include "dispcontainerplus.hxx"
#endif

#ifndef X_DISPSCROLLERPLUS_HXX_
#define X_DISPSCROLLERPLUS_HXX_
#include "dispscrollerplus.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_USER32_HXX_
#define X_USER32_HXX_
#include "user32.hxx"
#endif

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

BOOL GetCallerHTMLDlgTrust(CBase *pBase);

DYNLIB g_dynlibHTMLHELP = { NULL, NULL, "HHCTRL.OCX" };
typedef HWND (WINAPI * PHTMLHELPA)(HWND, LPCSTR, UINT, DWORD);

#define _cxx_
#include "window.hdl"

DeclareTag(tagOmWindow, "OmWindow", "OmWindow methods")
MtDefine(COmWindow2, ObjectModel, "COmWindow2")
MtDefine(COmWindow2FindNamesFromOtherScripts_aryScript_pv, Locals, "COmWindow2::FindNamesFromOtherScripts aryScript::_pv")
MtDefine(COmWindow2FindNamesFromOtherScripts_pdispid, Locals, "COmWindow2::FindNamesFromOtherScripts pdispid")
MtDefine(EVENTPARAM, Locals, "EVENTPARAM")
MtDefine(CDataTransfer, ObjectModel, "CDataTransfer")

BEGIN_TEAROFF_TABLE(COmWindow2, IProvideMultipleClassInfo)
    TEAROFF_METHOD(COmWindow2, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(COmWindow2, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(COmWindow2, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(COmWindow2, GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmWindow2, IServiceProvider)
        TEAROFF_METHOD(COmWindow2, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

const CONNECTION_POINT_INFO COmWindow2::s_acpi[] =
{
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC COmWindow2::s_classdesc =
{
    &CLSID_HTMLWindow2,             // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLWindow2,              // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


class COptionsHolder;

HRESULT
InternalShowModalDialog(HWND        hwndParent,
                        IMoniker *  pMk,
                        VARIANT *   pvarArgIn,
                        TCHAR *     pchOptions,
                        VARIANT *   pvarArgOut,
                        IUnknown *  punk,
                        COptionsHolder *pOptionsHolder,
                        DWORD       dwFlags);
HRESULT
InternalModelessDialog( HWND            hwndParent,
                        IMoniker      * pMk,
                        VARIANT       * pvarArgIn,
                        VARIANT       * pvarOptions,
                        DWORD           dwFlags,
                        COptionsHolder *pOptionsHolder,
                        CDoc           *pParentDoc,
                        IHTMLWindow2  **ppDialogWindow);


//+----------------------------------------------------------------------------------
//
//  Helper:     VariantToPrintableString
//
//  Synopsis:   helper for alert and confirm methods of window, etc.
//
//              Converts VARIANT to strings that conform with the JavaScript model.
//
//-----------------------------------------------------------------------------------

HRESULT
VariantToPrintableString (VARIANT * pvar, CStr * pstr)
{
    HRESULT     hr = S_OK;
    TCHAR       szBuf[64];

    Assert (pstr);

    switch (V_VT(pvar))
    {
        case VT_EMPTY :
        case VT_ERROR :
            LoadString(GetResourceHInst(), IDS_VAR2STR_VTERROR, szBuf, ARRAY_SIZE(szBuf));
            hr =THR(pstr->Set(szBuf));
            break;
        case VT_NULL :
            LoadString(GetResourceHInst(), IDS_VAR2STR_VTNULL, szBuf, ARRAY_SIZE(szBuf));
            hr = THR(pstr->Set(szBuf));
            break;
        case VT_BOOL :
            if (VARIANT_TRUE == V_BOOL(pvar))
            {
                LoadString(GetResourceHInst(), IDS_VAR2STR_VTBOOL_TRUE, szBuf, ARRAY_SIZE(szBuf));
                hr = THR(pstr->Set(szBuf));
            }
            else
            {
                LoadString(GetResourceHInst(), IDS_VAR2STR_VTBOOL_FALSE, szBuf, ARRAY_SIZE(szBuf));
                hr = THR(pstr->Set(szBuf));
            }
            break;
        case VT_BYREF:
        case VT_VARIANT:
            pvar = V_VARIANTREF(pvar);
            // fall thru
        default:
        {
            VARIANT varNew;
            VariantInit(&varNew);
            hr = THR(VariantChangeTypeSpecial(&varNew, pvar,VT_BSTR));
            if (!hr)
            {
                hr = THR(pstr->Set(V_BSTR(&varNew)));
                VariantClear(&varNew);
            }
        }

    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::COmWindow2
//
//  Synopsis:   ctor
//
//--------------------------------------------------------------------------

COmWindow2::COmWindow2(CDoc *pDoc)
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
#endif
    _pDoc = pDoc;
    _pDoc->SubAddRef();
    Assert(_pDoc);
    _pHistory = NULL;
    _pLocation = NULL;
    _pNavigator = NULL;
#if 0
// BUGBUG: ***TLL*** quick VBS event hookup
    _pSinkupObject = NULL;
#endif
    VariantInit(&_varOpener);
    MemSetName((this, "OmWindow pDoc=%08x", pDoc));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::COmWindow2
//
//  Synopsis:   dtor
//
//--------------------------------------------------------------------------
COmWindow2::~COmWindow2()
{
    if (_pDoc)
        _pDoc->SubRelease();

    // It is safe to delete here as we will get here only when
    // all refs to these subobjects are gone.
    delete _pHistory;
    delete _pLocation;
    delete _pNavigator;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::Passivate
//
//  Synopsis:   1st phase destructor
//
//--------------------------------------------------------------------------

void
COmWindow2::Passivate()
{
    // Free the name string
    _cstrName.Free();
    VariantClear(&_varOpener);

    // passivate embedded objects
    _Screen.Passivate();

#if 0
// BUGBUG: ***TLL*** quick VBS event hookup
    ClearInterface(_pSinkupObject);
#endif

    super::Passivate();
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
COmWindow2::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
    // The IOmWindow * cast is required to distinguish between CBase vs/
    // IOmWindow IDispatch methods.
    QI_TEAROFF(this, IProvideMultipleClassInfo, NULL)
    QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
    QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
    QI_INHERITS(this, IHTMLWindow2)
    QI_INHERITS(this, IHTMLWindow3)
    QI_INHERITS(this, IDispatchEx)
    QI_INHERITS2(this, IDispatch, IHTMLWindow2)
    QI_INHERITS2(this, IUnknown, IHTMLWindow2)
    QI_INHERITS2(this, IHTMLFramesCollection2, IHTMLWindow2)
    QI_TEAROFF(this, IServiceProvider, NULL)
    QI_CASE(IConnectionPointContainer)
        if (iid == IID_IConnectionPointContainer)
        {
            *((IConnectionPointContainer **)ppv) =
                    new CConnectionPointContainer(this, NULL);

            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
        }
        break;

    default:
        if (iid == CLSID_HTMLWindow2)
        {
            *ppv = this;
            // do not do AddRef()
            return S_OK;
        }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    DbgTrackItf(iid, "COmWindow2", FALSE, ppv);

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method : COmWindow
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
COmWindow2::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    hr = THR_NOTRACE(_pDoc->QueryService(guidService,
                                         riid,
                                         ppvObject));

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagOmWindow, "GetTypeInfoCount"));

    RRETURN(super::GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagOmWindow, "GetTypeInfo"));

    RRETURN(super::GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    HRESULT     hr;

    hr = THR_NOTRACE(GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid));

    // If we get no error but a DISPID_UNKNOWN then we'll want to return error
    // this mechanism is used for JScript to return a null for undefine name
    // sort of a passive expando.
    if (!hr && rgdispid[0] == DISPID_UNKNOWN)
    {
        hr = DISP_E_UNKNOWNNAME;
    }

    RRETURN(hr);
}


HRESULT
COmWindow2::Invoke(DISPID dispid,
                   REFIID riid,
                   LCID lcid,
                   WORD wFlags,
                   DISPPARAMS *pdispparams,
                   VARIANT *pvarResult,
                   EXCEPINFO *pexcepinfo,
                   UINT *puArgErr)
{
    return InvokeEx(dispid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}
//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::InvokeEx
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::InvokeEx(DISPID dispidMember,
                     LCID lcid,
                     WORD wFlags,
                     DISPPARAMS *pdispparams,
                     VARIANT *pvarResult,
                     EXCEPINFO *pexcepinfo,
                     IServiceProvider *pSrvProvider)
{
    TraceTag((tagOmWindow, "Invoke dispid=0x%x", dispidMember));

    HRESULT             hr;
    RETCOLLECT_KIND     collectionCreation;
    IDispatch          *pDisp = NULL;
    IDispatchEx        *pDispEx = NULL;
    CCollectionCache   *pCollectionCache;

    // BUGBUG:  ***TLL*** Should be removed when IActiveScript is fixed to
    //          refcount the script site so we don't have the addref the
    //          _pScriptCollection.
    CDoc::CLock Lock(_pDoc);

    //
    // Handle magic dispids which are used to serve up certain objects to the
    // browser:
    //

    switch( dispidMember )
    {
    case DISPID_SECURITYCTX:
        //
        // Return the url of the document and it's domain (if set)
        // in an array.
        //

        if (!pvarResult)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        V_VT(pvarResult) = VT_BSTR;

        Assert(!!_pDoc->_cstrUrl);
        hr = THR(_pDoc->_cstrUrl.AllocBSTR(&V_BSTR(pvarResult)));
        goto Cleanup;

    case DISPID_SECURITYDOMAIN:
        //
        // If set, return the domain property of the document.
        // Fail otherwise.
        //

        if (_pDoc->_cstrSetDomain.Length())
        {
            if (!pvarResult)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            V_VT(pvarResult) = VT_BSTR;
            hr = THR(_pDoc->_cstrSetDomain.AllocBSTR(&V_BSTR(pvarResult)));
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        goto Cleanup;

    case DISPID_HISTORYOBJECT:
        if (!_pHistory)
        {
            _pHistory = new COmHistory(this);
            if (!_pHistory)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        hr = THR(_pHistory->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    case DISPID_LOCATIONOBJECT:
        if(!_pLocation)
        {
            _pLocation = new COmLocation(this);
            if (!_pLocation)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        hr = THR(_pLocation->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    case DISPID_NAVIGATOROBJECT:
        if(!_pNavigator)
        {
            _pNavigator = new COmNavigator(this);
            if (!_pNavigator)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        hr = THR(_pNavigator->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    default:
        break;

FinishVTAndReturn:
        if (hr)
            goto Cleanup;
        pvarResult->vt = VT_DISPATCH;
        goto Cleanup;   // successful termination.
    }

    hr = THR(_pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = _pDoc->PrimaryMarkup()->CollectionCache();

    if ( pCollectionCache->IsDISPIDInCollection ( CMarkup::FRAMES_COLLECTION, dispidMember ) )
    {

        if ( pCollectionCache->IsOrdinalCollectionMember (
            CMarkup::FRAMES_COLLECTION, dispidMember ) )
        {
            // Resolve ordinal access to Frames
            //
            // If this dispid is one that maps to the numerically indexed
            // items, then simply use the index value with our Item() mehtod.
            // This handles the frames[3] case.
            //

            // BUGWIN16:: the case below of (IHTMLWindow2**) could be potential badness.
            // need to investigate.
            hr = THR(_pDoc->GetOmWindow(dispidMember -
                pCollectionCache->GetOrdinalMemberMin ( CMarkup::FRAMES_COLLECTION ),
                (IHTMLWindow2**)&V_DISPATCH(pvarResult) ));

            if (!hr) // if successfull, nothing to do more
            {
                V_VT(pvarResult) = VT_DISPATCH;
                goto Cleanup;
            }
        }
        else
        {
            // Resolve named frame

            // dispid from FRAMES_COLLECTION GIN/GINEX
            CAtomTable *pAtomTable;
            LPCTSTR     pch = NULL;
            long        lOffset;
            BOOL        fCaseSensitive;

            lOffset = pCollectionCache->GetNamedMemberOffset(CMarkup::FRAMES_COLLECTION,
                                        dispidMember, &fCaseSensitive);

            pAtomTable = pCollectionCache->GetAtomTable(FALSE);
            if( !pAtomTable )
                goto Cleanup;

            hr = THR(pAtomTable->GetNameFromAtom(dispidMember - lOffset, &pch));

            if (!hr)
            {
                CElement *      pElem;
                IHTMLWindow2 *  pOmWindow;
                CFrameSite *    pFrameSite;

                hr = pCollectionCache->GetIntoAry( CMarkup::FRAMES_COLLECTION,
                                            pch, FALSE, &pElem, 0, fCaseSensitive);

                // The FRAMES_COLLECTION is marked to always return the last matching name in
                // the returned pElement - rather than the default first match - For Nav compat.
                // GetIntoAry return S_FALSE if more than one item found - like Nav
                // though we ignore this
                if ( hr && hr != S_FALSE )
                    goto Cleanup;

                pFrameSite = DYNCAST (CFrameSite, pElem );

                hr = pFrameSite->GetOmWindow( &pOmWindow );
                if (!hr)
                {
                    V_VT(pvarResult) = VT_DISPATCH;
                    V_DISPATCH(pvarResult) = pOmWindow;
                }
                goto Cleanup;
            }
            // else name not found in atom table, try the other routes
            // to resolve the Dispid.
        }
    }

    // If the DISPID came from the dynamic typelib collection, adjust the DISPID into
    // the WINDOW_COLLECTION, and record that the Invoke should only return a single item
    // Not a collection

    if ( dispidMember >= DISPID_COLLECTION_MIN &&
        dispidMember < pCollectionCache->GetMinDISPID( CMarkup::WINDOW_COLLECTION ) )
    {
        // DISPID Invoke from the dynamic typelib collection
        // Adjust it into the WINDOW collection
        dispidMember = dispidMember - DISPID_COLLECTION_MIN +
            pCollectionCache->GetMinDISPID( CMarkup::WINDOW_COLLECTION );
        // VBScript only wants a dipatch ptr to the first element that matches the name
        collectionCreation = RETCOLLECT_FIRSTITEM;
    }
    else if (dispidMember >= DISPID_EVENTHOOK_SENSITIVE_BASE &&
             dispidMember <= DISPID_EVENTHOOK_INSENSITIVE_MAX)
    {
        // Quick VBS event hookup always hooks to the last item in the collection.  This is for
        // IE4 VBS compatibility when foo_onclick is used if foo returns a collection of elements
        // with the name foo.  The only element to hookup for the onclick event would be the last
        // item in the collection of foo's.
        if (dispidMember <= DISPID_EVENTHOOK_SENSITIVE_MAX)
        {
            dispidMember -= DISPID_EVENTHOOK_SENSITIVE_BASE;
            dispidMember += pCollectionCache->GetSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
        }
        else
        {
            dispidMember -= DISPID_EVENTHOOK_INSENSITIVE_BASE;
            dispidMember += pCollectionCache->GetNotSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
        }

        collectionCreation = RETCOLLECT_LASTITEM; 
    }
    else
    {
        collectionCreation = RETCOLLECT_ALL;
    }

    // If we Invoke on the dynamic collection, only return 1 item
    hr = THR_NOTRACE(DispatchInvokeCollection(this,
                                              &super::InvokeEx,
                                              pCollectionCache,
                                              CMarkup::WINDOW_COLLECTION,
                                              dispidMember,
                                              IID_NULL,
                                              lcid,
                                              wFlags,
                                              pdispparams,
                                              pvarResult,
                                              pexcepinfo,
                                              NULL,
                                              pSrvProvider,
                                              collectionCreation ));
    if (!hr) // if successfull, nothing more to do.
        goto Cleanup;

    if ( dispidMember >= DISPID_OMWINDOWMETHODS)
    {
        if (_pDoc->_pScriptCollection)
        {
            hr = THR(_pDoc->_pScriptCollection->InvokeEx(
                _pDoc->PrimaryMarkup(), dispidMember, lcid, wFlags, pdispparams,pvarResult, pexcepinfo, pSrvProvider));
        }
        else
        {   // Handle this on retail builds - don't crash:
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }

    // Finally try the external object iff we're in a dialog.  Reason for
    // this is compat with existing pages from beta1/2 which are already
    // using old syntax.

    if (hr && _pDoc->_fInHTMLDlg)
    {
        if (OK(get_external(&pDisp)) &&
            OK(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx)))
        {
            hr = THR_NOTRACE(pDispEx->InvokeEx(
                    dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    pSrvProvider));
        }
    }

Cleanup:
    ReleaseInterface(pDisp);
    ReleaseInterface(pDispEx);
    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::GetDispID
//
//  Synopsis:   Per IDispatchEx.  This is called by script engines to lookup
//              dispids of expando properties and of named elements such as
//              <FRAME NAME="AFrameName">
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr = DISP_E_UNKNOWNNAME;
    IDispatchEx    *pDispEx = NULL;
    IDispatch      *pDisp = NULL;
    CMarkup        *pMarkup;
    BOOL            fNoDynamicProperties = !!(grfdex & fdexNameNoDynamicProperties);

    if (_pDoc->_pScriptCollection && _pDoc->_pScriptCollection->_fInEnginesGetDispID)
        goto Cleanup;

    pMarkup = _pDoc->PrimaryMarkup();

    // Quick event hookup for VBS.
    if (fNoDynamicProperties)
    {
        // Yes, if the name is window then let the script engine resolve everything
        // else better be a real object (frame, or all collection).
        STRINGCOMPAREFN pfnCompareString = (grfdex & fdexNameCaseSensitive)
                                                   ? StrCmpC : StrCmpIC;

        // If "window" or "document" is found then we'll turn fNoDynamicProperties off so that
        // the script engine is searched for "window" which is the global object
        // for the script engine.
        fNoDynamicProperties = !(pfnCompareString(s_propdescCOmWindow2window.a.pstrName, bstrName)   == 0 ||
                                 pfnCompareString(s_propdescCOmWindow2document.a.pstrName, bstrName) == 0);
   
    }

    // Does the script engine want names of object only (window, element ID).
    // If so then no functions or names in the script engine and no expandos.
    // This is for quick event hookup in VBS.
    if (!fNoDynamicProperties)
    {
        //
        // Try script collection namespace first.
        //

        if (_pDoc->_pScriptCollection)
        {
            hr = THR_NOTRACE(_pDoc->_pScriptCollection->GetDispID(pMarkup, bstrName, grfdex, pid));
            if (S_OK == hr)
                goto Cleanup;   // done;
        }

        // Next, try the external object iff we're in a dialog.  Reason for
        // this is compat with existing pages from beta1/2 which are already
        // using old syntax.

        if (hr && _pDoc->_fInHTMLDlg)
        {
            if (OK(get_external(&pDisp)) &&
                OK(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx)))
            {
                hr = THR_NOTRACE(pDispEx->GetDispID(bstrName, grfdex, pid));
            }
        }

        // Try property and expandos on the object.
        if (hr)
        {
            // Pass the special name on to CBase::GINEx.
            hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));
        }
    
        // Next try named frames in the FRAMES_COLLECTION
        if (hr)
        {
            hr = THR(pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
            if (hr)
                goto Cleanup;

            hr = THR_NOTRACE(pMarkup->CollectionCache()->GetDispID(CMarkup::FRAMES_COLLECTION,
                                                                 bstrName,
                                                                 grfdex,
                                                                 pid));

            // The collectionCache GetDispID will return S_OK w/ DISPID_UNKNOWN
            // if the name isn't found, catastrophic errors are of course returned.
            if (!hr && *pid == DISPID_UNKNOWN)
            {
                hr = DISP_E_UNKNOWNNAME;
            }
        }
    }

    // Next try WINDOW collection name space.
    if (hr)
    {
        CCollectionCache   *pCollectionCache;

        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pCollectionCache = pMarkup->CollectionCache();

        hr = THR_NOTRACE(pCollectionCache->GetDispID(CMarkup::WINDOW_COLLECTION,
                                                     bstrName,
                                                     grfdex,
                                                     pid));
        // The collectionCache GetDispID will return S_OK w/ DISPID_UNKNOWN
        // if the name isn't found, catastrophic errors are of course returned.
        if (!hr && *pid == DISPID_UNKNOWN)
        {
            hr = DISP_E_UNKNOWNNAME;
        }
        else if (fNoDynamicProperties && pCollectionCache->IsNamedCollectionMember(CMarkup::WINDOW_COLLECTION, *pid))
        {
            DISPID dBase;
            DISPID dMax;

            if (grfdex & fdexNameCaseSensitive)
            {
                *pid -= pCollectionCache->GetSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
                dBase = DISPID_EVENTHOOK_SENSITIVE_BASE;
                dMax =  DISPID_EVENTHOOK_SENSITIVE_MAX;
            }
            else
            {
                *pid -= pCollectionCache->GetNotSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
                dBase = DISPID_EVENTHOOK_INSENSITIVE_BASE;
                dMax =  DISPID_EVENTHOOK_INSENSITIVE_MAX;
            }

            *pid += dBase;
            if (*pid > dMax)
            {
                hr = DISP_E_UNKNOWNNAME;
                *pid = DISPID_UNKNOWN;
            }
        }
    }

Cleanup:
    ReleaseInterface(pDisp);
    ReleaseInterface(pDispEx);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------


STDMETHODIMP
COmWindow2::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

STDMETHODIMP
COmWindow2::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}

STDMETHODIMP
COmWindow2::GetNextDispID(DWORD grfdex,
                          DISPID id,
                          DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(_pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetNextDispIDCollection(this,
                                         &super::GetNextDispID,
                                         _pDoc->PrimaryMarkup()->CollectionCache(),
                                         CMarkup::FRAMES_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
COmWindow2::GetMemberName(DISPID id,
                          BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(_pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
                                         super::GetMemberName,
                                         _pDoc->PrimaryMarkup()->CollectionCache(),
                                         CMarkup::FRAMES_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
COmWindow2::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::get_document
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::get_document(IHTMLDocument2**p)
{
    TraceTag((tagOmWindow, "get_Document"));

    RRETURN(SetErrorInfo(_pDoc->_pPrimaryMarkup->QueryInterface(IID_IHTMLDocument2, (void **)p)));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::get_frames
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::get_frames(IHTMLFramesCollection2 ** ppOmWindow)
{
    HRESULT hr = E_INVALIDARG;

    if (!ppOmWindow)
        goto Cleanup;

    hr = THR_NOTRACE(QueryInterface(IID_IHTMLWindow2, (void**) ppOmWindow));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   object model implementation
//
//              we handle the following parameter cases:
//                  0 params:               returns IDispatch of this
//                  1 param as number N:    returns IDispatch of om window of
//                                          frame # N, or fails if doc is not with
//                                          frameset
//                  1 param as string "foo" returns the om window of the frame
//                                          element that has NAME="foo"
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT             hr = S_OK;
    IHTMLWindow2 *     pOmWindow;

    if (!pvarRes)
        RRETURN (E_POINTER);

    // Perform indirection if it is appropriate:
    if( V_VT(pvarArg1) == (VT_BYREF | VT_VARIANT) )
        pvarArg1 = V_VARIANTREF(pvarArg1);

    VariantInit (pvarRes);

    if (VT_EMPTY == V_VT(pvarArg1))
    {
        // this is on of the following cases if the call is from a script engine:
        //      window
        //      window.value
        //      window.frames
        //      window.frames.value

        V_VT(pvarRes) = VT_DISPATCH;
        hr = THR(QueryInterface(IID_IHTMLWindow2, (void**)&V_DISPATCH(pvarRes)));
    }
    else if( VT_BSTR == V_VT(pvarArg1) )
    {
        CElement *pElem;

        hr = THR(_pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        hr = _pDoc->PrimaryMarkup()->CollectionCache()->GetIntoAry( CMarkup::WINDOW_COLLECTION,
                                    V_BSTR(pvarArg1), FALSE, &pElem );
        if( hr )
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        CFrameSite *pFrameSite = DYNCAST ( CFrameSite, pElem );
        hr = pFrameSite->GetOmWindow( &pOmWindow );
        if (hr)
            goto Cleanup;

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = pOmWindow;
    }
    else
    {
        // this is one of the following cases if the call is from a script engine:
        //      window(<index>)
        //      window.frames(<index>)

        hr = THR(VariantChangeTypeSpecial(pvarArg1, pvarArg1, VT_I4));
        if (hr)
            goto Cleanup;

        hr = THR(_pDoc->GetOmWindow(V_I4(pvarArg1), &pOmWindow));
        if (hr)
            goto Cleanup;

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = pOmWindow;
    }

Cleanup:
    // don't release pOmWindow as it is copied to pvarRes without AddRef-ing

    RRETURN (SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = E_NOTIMPL;

    RRETURN(SetErrorInfo( hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::get_event
//
//  Synopsis:   Per IOmWindow. but if we are not in an event handler (there is
//              nothing on the "EVENTPARAM" stack) then just return NULL.
//
//--------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::get_event(IHTMLEventObj** ppEventObj)
{
    HRESULT     hr;

    // are we in an event?
    if (_pDoc->_pparam)
    {
        hr = THR(CEventObj::Create(ppEventObj, _pDoc));
    }
    else
    {
        // set hr = S_OK and just return NULL
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::setTimeout
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::setTimeout(
    BSTR strCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    VARIANT v;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = strCode;

    RRETURN(SetErrorInfo(_pDoc->SetTimeout(&v, lMSec, FALSE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::setTimeout (can accept IDispatch *)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::setTimeout(
    VARIANT *pCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    RRETURN(SetErrorInfo(_pDoc->SetTimeout(pCode, lMSec, FALSE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::clearTimeout
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::clearTimeout(LONG timerID)
{
    RRETURN(SetErrorInfo(_pDoc->ClearTimeout(timerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::setInterval
//
//  Synopsis:  Runs <Code> every <msec> milliseconds
//             Note: Nav 4 behavior of msec=0 is like setTimeout msec=0
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::setInterval(
    BSTR strCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    VARIANT v;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = strCode;

    RRETURN(SetErrorInfo(_pDoc->SetTimeout(&v, lMSec, TRUE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::setInterval (can accept IDispatch *)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::setInterval(
    VARIANT *pCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    RRETURN(SetErrorInfo(_pDoc->SetTimeout(pCode, lMSec, TRUE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::clearInterval
//
//  Synopsis:   deletes the period timer set by setInterval
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::clearInterval(LONG timerID)
{
    RRETURN(SetErrorInfo(_pDoc->ClearTimeout(timerID)));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::get_screen
//
//--------------------------------------------------------------------------

HRESULT
COmWindow2::get_screen(IHTMLScreen**p)
{
    HRESULT     hr;

    hr = THR(_Screen.QueryInterface(
            IID_IHTMLScreen,
            (void **)p));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::showModelessDialog
//
//  Synopsis - helper function for the modal/modeless dialogs
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::ShowHTMLDialogHelper(BSTR      bstrUrl, 
                                 VARIANT * pvarArgIn, 
                                 VARIANT * pvarOptions, 
                                 BOOL      fModeless,                                 
                                 IHTMLWindow2** ppDialog,
                                 VARIANT * pvarArgOut)
{
    IMoniker * pMK = NULL;
    HRESULT    hr = S_OK;
    TCHAR    * pchOptions;
    TCHAR      achBuf[pdlUrlLen];
    DWORD      cchBuf;
    BOOL       fBlockArguments;
    BSTR       bstrTempUrl = NULL;
    DWORD      dwTrustMe = (_pDoc->_fTrustedDoc || 
                            GetCallerHTMLDlgTrust(this)) ? 0 : HTMLDLG_DONTTRUST;
    CStr       cstrSpecialURL;

    //
    // is this a valid call to make at this time?
    if (!(_pDoc && _pDoc->_pInPlace))
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    //
    // First do some in-parameter checking
    if (pvarOptions &&
        VT_ERROR == V_VT(pvarOptions) &&
        DISP_E_PARAMNOTFOUND == V_ERROR(pvarOptions))
    {
        pvarOptions = NULL;
    }

    //
    // Can't load a blank page, or none bstr VarOptions
    if (!bstrUrl || !*bstrUrl) 
    {
        if (fModeless)
        {
            // use about:blank for the name and continue
            bstrTempUrl = SysAllocString(_T("about:blank"));
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else
    {
        Assert(!!_pDoc->_cstrUrl);

        if (WrapSpecialUrl(bstrUrl, &cstrSpecialURL, _pDoc->_cstrUrl, FALSE, FALSE) != S_OK)
            goto Cleanup;

        hr = cstrSpecialURL.AllocBSTR(&bstrTempUrl);
        if (hr)
            goto Cleanup;
    }
        
    if (pvarOptions && 
        VT_ERROR != V_VT(pvarOptions) && 
        VT_BSTR != V_VT(pvarOptions))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // dereference if needed
    if (pvarOptions && V_VT(pvarOptions) == (VT_BYREF | VT_VARIANT))
        pvarOptions = V_VARIANTREF(pvarOptions);

    pchOptions = (pvarOptions && VT_BSTR == V_VT(pvarOptions)) ?
        V_BSTR(pvarOptions) : NULL;

    if (pvarArgIn && V_VT(pvarArgIn)== (VT_BYREF | VT_VARIANT))
        pvarArgIn = V_VARIANTREF(pvarArgIn);

    //
    // Create a moniker from the combined url handed in and our own url
    // (to resolve relative paths).
    Assert(!!_pDoc->_cstrUrl);
    hr = CoInternetCombineUrl(_pDoc->_cstrUrl,
                               bstrTempUrl ? bstrTempUrl :  bstrUrl,
                               URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                               achBuf,
                               ARRAY_SIZE(achBuf),
                               &cchBuf,
                               0);
    if (hr)
        goto Cleanup;


    // SECURITY ALERT - now that we have a combined url we can do some security 
    // work.  see bug ie11361 - the retval args and the ArgsIn can contain 
    // scriptable objects.  Because they pass through directly from one doc to 
    // another it is possible to send something across domains and thus avoid the
    // security tests.  To stop this what we do is check for accessAllowed between
    // this parent doc, and the dialog url.  If access is allowed, we continue on 
    // without pause.  If, however, it is cross domain then we still allow the dialog
    // to come up, but we block the passing of arguments back and forth. 

    // we don't need to protect HTA, about:blank, or res: url
    fBlockArguments = (_pDoc->_fTrustedDoc || _pDoc->_fInTrustedHTMLDlg) ? FALSE : ( !_pDoc->AccessAllowed(achBuf) );

    hr = THR(CreateURLMoniker(NULL, achBuf, &pMK));
    if (hr)
        goto Cleanup;

    //
    // bring up the dialog
    //--------------------------------------------------------------------
    if (fModeless)
    {
        hr = THR(InternalModelessDialog(
                                        _pDoc->_pInPlace->_hwnd,
                                        pMK,
                                        (fBlockArguments ? NULL : pvarArgIn),
                                        pvarOptions,    // options string
                                        dwTrustMe,      // Trust flag
                                        NULL,           // options holder
                                        _pDoc,
                                        ppDialog));     // IHTMLWindow2 -- this is already
                                                        // a secure object, so we don't need
                                                        // the security block for arguments
        // else do nothing since we can't get the parentage correct
    }
    else
    {
        CDoEnableModeless   dem(_pDoc);

        if (dem._hwnd)
        {
            hr = THR(InternalShowModalDialog(
                                        _pDoc->_pInPlace->_hwnd,
                                        pMK,
                                        (fBlockArguments ? NULL : pvarArgIn),
                                        pchOptions,
                                        (fBlockArguments ? NULL : pvarArgOut),
                                        NULL,
                                        NULL,
                                        dwTrustMe));
        }
    }

    if (hr)
        goto Cleanup;

Cleanup:
    if (bstrTempUrl)
        SysFreeString(bstrTempUrl);
    ReleaseInterface(pMK);
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::showModelessDialog
//
//  Synopsis:   Interface method to bring up a modeless HTMLdialog given a url
//
//----------------------------------------------------------------------------
STDMETHODIMP
COmWindow2::showModelessDialog(/* in */ BSTR      bstrUrl, 
                               /* in */ VARIANT * pvarArgIn, 
                               /* in */ VARIANT * pvarOptions, 
                               /* ret */IHTMLWindow2** ppDialog)
{
    RRETURN(SetErrorInfo(ShowHTMLDialogHelper( bstrUrl, 
                    pvarArgIn, 
                    pvarOptions, 
                    TRUE, 
                    ppDialog, 
                    NULL)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindow2::showModalDialog
//
//  Synopsis:   Interface method to bring up a HTMLdialog given a url
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::showModalDialog(BSTR       bstrUrl,
                            VARIANT *  pvarArgIn,
                            VARIANT *  pvarOptions,
                            VARIANT *  pvarArgOut)
{

    RRETURN(SetErrorInfo(ShowHTMLDialogHelper(bstrUrl, 
                    pvarArgIn, 
                    pvarOptions, 
                    FALSE, 
                    NULL, 
                    pvarArgOut)));
}


//---------------------------------------------------------------------------
//
//  Member:     COmWindow2::GetMultiTypeInfoCount
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetMultiTypeInfoCount(ULONG *pc)
{
    TraceTag((tagOmWindow, "GetMultiTypeInfoCount"));

    *pc = _pDoc->LoadStatus() >= LOADSTATUS_QUICK_DONE ? 2 : 1;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     COmWindow2::GetInfoOfIndex
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::GetInfoOfIndex(
    ULONG       iTI,
    DWORD       dwFlags,
    ITypeInfo** ppTICoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    TraceTag((tagOmWindow, "GetInfoOfIndex"));

    HRESULT hr = S_OK;

    //
    // First try the main typeinfo
    //

    if (1 == iTI &&
        _pDoc->LoadStatus() >= LOADSTATUS_QUICK_DONE &&
        dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        //
        // Caller wanted info on the dynamic part of the window.
        //

        hr = THR(_pDoc->EnsureObjectTypeInfo());
        if (hr)
            goto Cleanup;

        *ppTICoClass = _pDoc->_pTypInfoCoClass;
        (*ppTICoClass)->AddRef();

        //
        // Clear out these values so that we can use the base impl.
        //

        dwFlags &= ~MULTICLASSINFO_GETTYPEINFO;
        iTI = 0;
        ppTICoClass = NULL;
    }

    hr = THR(super::GetInfoOfIndex(
        iTI,
        dwFlags,
        ppTICoClass,
        pdwTIFlags,
        pcdispidReserved,
        piidPrimary,
        piidSource));

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::FireEvent
//
//  Synopsis:   CBase doesn't allow an EVENTPARAM, which we need.
//
//--------------------------------------------------------------------------

HRESULT
COmWindow2::FireEvent(
    DISPID dispidEvent,
    DISPID dispidProp,
    LPCTSTR pchEventType,
    BYTE * pbTypes,
    ...)
{
    va_list         valParms;
    HRESULT         hr;
    EVENTPARAM      param(_pDoc, TRUE);
    IDispatch      *pEventObj = NULL;

    param.SetType(pchEventType);

    va_start(valParms, pbTypes);

    // Get the eventObject.
    IGNORE_HR(get_event((IHTMLEventObj **)&pEventObj));
    
    hr = THR(FireEventV(dispidEvent,
                        dispidProp,
                        pEventObj,
                        NULL,
                        pbTypes,
                        valParms));

    ReleaseInterface(pEventObj);
    
    va_end(valParms);
    RRETURN(hr);
}


extern BYTE byOnErrorParamTypes[4];

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::Fire_onerror
//
//  Synopsis:   Tries to fires the onerror event and returns S_FALSE if ther was no
//              error handling script
//
//--------------------------------------------------------------------------
void
COmWindow2::Fire_onerror(BSTR bstrDescr, BSTR bstrURL, long ulLine)
{
    FireEvent(DISPID_EVMETH_ONERROR, 0, _T("onerror"),
        byOnErrorParamTypes, bstrDescr, bstrURL, ulLine);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::Fire_onscroll
//
//  Synopsis:   Fires the onscroll events of the window
//
//--------------------------------------------------------------------------
void COmWindow2::Fire_onscroll()
{
    FireEvent(DISPID_EVMETH_ONSCROLL, 0, _T("scroll"), (BYTE *)VTS_NONE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::Fire_onload, Fire_onunload, Fire_onbeforeunload
//
//  Synopsis:   Fires the onload/onunload events of the window
//
//--------------------------------------------------------------------------

void COmWindow2::Fire_onload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONLOAD, 0, _T("load"), (BYTE *) VTS_NONE);
}

void COmWindow2::Fire_onunload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONUNLOAD, 0, _T("unload"), (BYTE *) VTS_NONE);
}

BOOL COmWindow2::Fire_onbeforeunload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONBEFOREUNLOAD, 0, _T("beforeunload"), (BYTE *) VTS_NONE);
    return TRUE;
}

//--------------------------------------------------------------------------
//  Member : Fire_onhelp
//
//  Synopsis: onhelp has special return value handling and so needs its own
//      implementation of the fireEvent code.
//--------------------------------------------------------------------------
BOOL
COmWindow2::Fire_onhelp(BYTE * pbTypes, ...)
{
    va_list         valParms;
    EVENTPARAM      param(_pDoc, TRUE);
    BOOL            fRet = TRUE;
    VARIANT_BOOL    vb;
    CVariant        Var;
    IDispatch      *pEventObj = NULL;

    param.SetType(_T("help"));

    va_start(valParms, pbTypes);

    // Get the eventObject.
    IGNORE_HR(get_event((IHTMLEventObj **)&pEventObj));
    
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    IGNORE_HR(FireEventV(
                DISPID_EVMETH_ONHELP,
                DISPID_EVPROP_ONHELP,
                pEventObj,
                &Var,
                pbTypes,
                valParms));

    ReleaseInterface(pEventObj);
    
    va_end(valParms);

    vb = (V_VT(&Var) == VT_BOOL) ? V_BOOL(&Var) : VB_TRUE;

    fRet = !param.IsCancelled() && VB_TRUE == vb;

    return fRet;
}

void
COmWindow2::Fire_onresize()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent( DISPID_EVMETH_ONRESIZE, 0, _T("resize"), (BYTE *) VTS_NONE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindow2::Fire_onfocus, Fire_onblur
//
//  Synopsis:   Fires the onfocus/onblur events of the window
//
//--------------------------------------------------------------------------

void COmWindow2::Fire_onfocus()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONFOCUS, 0, _T("focus"), (BYTE *) VTS_NONE);
}

void COmWindow2::Fire_onblur()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONBLUR, 0, _T("blur"), (BYTE *) VTS_NONE);
}

void COmWindow2::Fire_onbeforeprint()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONBEFOREPRINT, 0, _T("beforeprint"), (BYTE *)VTS_NONE);
}

void COmWindow2::Fire_onafterprint()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(DISPID_EVMETH_ONAFTERPRINT, 0, _T("afterprint"), (BYTE *)VTS_NONE);
}

//+----------------------------------------------------------------------------------
//
//  Member:     alert
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::alert(BSTR message)
{
    BOOL fRightToLeft;
    _pDoc->GetDocDirection(&fRightToLeft);

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS))
        RRETURN(S_OK);
    else
        RRETURN(SetErrorInfo(
            THR(_pDoc->ShowMessageEx(
                NULL,
                fRightToLeft ? MB_OK | MB_ICONEXCLAMATION | MB_RTLREADING | MB_RIGHT : MB_OK | MB_ICONEXCLAMATION,
                NULL,
                0,
                (TCHAR*)STRVAL(message)))));
}

//+----------------------------------------------------------------------------------
//
//  Member:     confirm
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::confirm(BSTR message, VARIANT_BOOL *pConfirmed)
{
    HRESULT     hr;
    int         nRes;

    if (!pConfirmed)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pConfirmed = VB_FALSE;

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS))
    {
        hr = S_OK;
        nRes = IDOK;
    }
    else
    {
        hr = THR(_pDoc->ShowMessageEx(
                &nRes,
                MB_OKCANCEL | MB_ICONQUESTION,
                NULL,
                0,
                (TCHAR*)STRVAL(message)));
        if (hr)
            goto Cleanup;
    }

    *pConfirmed = (IDOK == nRes) ? VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   object model implementation
//
//              returns number of frames in frameset of document;
//              fails if the doc does not contain frameset
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::get_length(long * pcFrames)
{
    HRESULT hr;

    if (!pcFrames)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pcFrames = 0;

    hr = THR(_pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR ( _pDoc->PrimaryMarkup()->CollectionCache()->GetLength(CMarkup::FRAMES_COLLECTION, pcFrames ) );

Cleanup:
    RRETURN (SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member: COmWindow2::showHelp
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::showHelp(BSTR helpURL, VARIANT helpArg, BSTR features)
{
    // BUGBUG5229: (jenlc) Several things need to be done later.
    //
    // * wait for Window open task to be done.
    // * The meaning of parameter "features". For example, in WinHelp,
    //   features can be "HELP_CONTEXT", "HELP_CONTEXTPOPUP", or others.
    //
    HRESULT   hr = E_FAIL;
    VARIANT * pHelpArg;
    long      lHelpId = 0;
    TCHAR   * pchExt;
    CVariant  cvArg;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR   * pchHelpFile = cBuf;
    BOOL    fPath;

    if (!helpURL)
        goto Cleanup;

    // we need a HelpId for WinHelp Files:
    // dereference if variable passed in
    pHelpArg = (V_VT(&helpArg) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&helpArg) : &helpArg;

    // if the extention is .hlp it is WinHelp, otherwise assume html/text
    pchExt = _tcsrchr(helpURL, _T('.'));
    if (pchExt && !FormsStringICmp(pchExt, _T(".hlp")) && _pDoc->_fInTrustedHTMLDlg)
    {
        UINT      uCommand = HELP_CONTEXT;
        if(V_VT(pHelpArg) == VT_BSTR)
        {
            hr = THR(ttol_with_error(V_BSTR(pHelpArg), &lHelpId));
            if (hr)
            {
                // BUGBUG (carled) if it is not an ID it might be a bookmark
                //  or empty, or just text, extra processing necessary here.
                goto Cleanup;
            }
        }
        else
        {
            // make sure the second argument is a I4
            hr = cvArg.CoerceVariantArg(pHelpArg, VT_I4);
            if(FAILED(hr))
                goto Cleanup;

            if(hr == S_OK)
            {
                lHelpId = V_I4(&cvArg);
            }
        }

        //  If features == "popup", we need to open the
        //  hwlp window as a popup.
        if (features && !FormsStringICmp(features, _T("popup")))
        {
            uCommand = HELP_CONTEXTPOPUP;
        }

         // if the path is absent try to use the ML stuff
        fPath =  (_tcsrchr(helpURL, _T('\\')) != NULL) || (_tcsrchr(helpURL, _T('/')) != NULL);

        if(!fPath)
        {
            hr =  THR(MLWinHelp(TLS(gwnd.hwndGlobalWindow),
                           helpURL,
                           uCommand,
                           lHelpId)) ? S_OK : E_FAIL;
        }
        // If the call failed try again without using ML. It could be that the 
        // help file was not found in the language directory, but it sits somewhere under
        // the path.
        if(fPath || hr == E_FAIL)
        {
            hr =  THR(WinHelp(TLS(gwnd.hwndGlobalWindow),
                           helpURL,
                           uCommand,
                           lHelpId)) ? S_OK : E_FAIL;
        }

   }
#ifndef WIN16
    else
    {
        // HTML help case
        HWND hwndCaller;
        TCHAR achFile[MAX_PATH];
        ULONG cchFile = ARRAY_SIZE(achFile);

        _pDoc->GetWindowForBinding(&hwndCaller);

        hr = _pDoc->ExpandUrl(helpURL, ARRAY_SIZE(cBuf), pchHelpFile, NULL);
        if (hr || !*pchHelpFile)
            goto Cleanup;

        if (GetUrlScheme(pchHelpFile) == URL_SCHEME_FILE)
        {
            hr = THR(PathCreateFromUrl(pchHelpFile, achFile, &cchFile, 0));
            if (hr)
                goto Cleanup;

            pchHelpFile = achFile;
        }

        // Show the help file
        hr = THR(CallHtmlHelp(hwndCaller, pchHelpFile, HH_DISPLAY_TOPIC, 0));
    }
#endif // ndef WIN16

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+---------------------------------------------------------------------------
//
//  Member: COmWindow2::CallHtmlHelp
//
//  Wrapper that dynamically loads, translates to ASCII and calls HtmlHelpA
//----------------------------------------------------------------------------

HRESULT
COmWindow2::CallHtmlHelp(HWND hwnd, BSTR pszFile, UINT uCommand, DWORD_PTR dwData, HWND *pRetVal /*=NULL*/)
{
    static DYNPROC s_dynprocHTMLHELPA = { NULL, &g_dynlibHTMLHELP, "HtmlHelpA" };
    char        szPathA[MAX_PATH];
    HRESULT     hr = S_OK;
    char       * pFileName = NULL;
    int         nResult;
    HWND        hwndRet = NULL;

    if(pszFile != NULL)
    {
        nResult = WideCharToMultiByte(CP_ACP, 0, pszFile, -1, szPathA,
                sizeof(szPathA), NULL, NULL);
        if (!nResult) {
            hr = GetLastWin32Error();
            goto Cleanup;
        }
        pFileName = szPathA;
    }

    hr = THR(LoadProcedure(&s_dynprocHTMLHELPA));
    if (hr)
       goto Cleanup;

//$ WIN64: HtmlHelpA needs to have fourth argumetn expanded to DWORD_PTR

    hwndRet = (* (PHTMLHELPA)s_dynprocHTMLHELPA.pfn)
                    (hwnd, pFileName, uCommand, (DWORD)dwData);

Cleanup:
    if(pRetVal)
        *pRetVal = hwndRet;

    RRETURN(SetErrorInfo( hr));
}



HRESULT
COmWindow2::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};

#define TITLEBAR_FEATURE_NAME   _T("titlebar")

//+---------------------------------------------------------------------------
//
//  Member: COmWindow2::FilterOutFeaturesString
//
//  Removes the unsafe features from the features string
//----------------------------------------------------------------------------

HRESULT 
COmWindow2::FilterOutFeaturesString(BSTR bstrFeatures, BSTR *pbstrOut)
{
    HRESULT         hr = S_OK;
    LPCTSTR         pchFound;
    LPCTSTR         pchInPtr = bstrFeatures;
    CBufferedStr    strOut;

    Assert(pbstrOut);

    *pbstrOut = NULL;

    if(!bstrFeatures)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    do
    {
        pchFound = _tcsistr(pchInPtr, TITLEBAR_FEATURE_NAME);
        if(!pchFound)
        {
            strOut.QuickAppend(pchInPtr);
            break;
        }

        strOut.QuickAppend(pchInPtr, pchFound - pchInPtr);
        
        //Make sure we are on the left side of the =
        if(pchFound > pchInPtr && *(pchFound - 1) != _T(',') && !isspace(*(pchFound - 1)))
        {
            LPTSTR pchComma, pchEqual;
            pchComma =  _tcschr(pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1), _T(','));
            pchEqual =  _tcschr(pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1), _T('='));
            // If there is no = or there is an = and there is a comma and comma is earlier we ignore
            if(!pchEqual || (pchComma && pchComma < pchEqual))
            {
                // This was a wrong titlebar setting, keep it
                strOut.QuickAppend(pchFound, (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1));
                pchInPtr = pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1);
                continue;
            }
        }

        pchInPtr = pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1);

        pchFound = _tcschr(pchInPtr, _T(','));
        if(!pchFound)
            break;

        pchInPtr = pchFound + 1;

    } while (*pchInPtr != 0);

    hr = THR(FormsAllocString(strOut, pbstrOut));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function: PromptDlgProc()
//
//----------------------------------------------------------------------------

struct SPromptDlgParams      // Communicates info the to dlg box routines.
{
    CStr    strUserMessage;     // Message to display.
    CStr    strEditFieldValue;  // IN has initial value, OUT has return value.
};

INT_PTR CALLBACK PromptDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
#ifdef WIN16        // on 16bit we only support one prompt dlg at a time
    static SPromptDlgParams *pdp = NULL;
#else
    SPromptDlgParams *pdp = NULL;
#endif

    switch (message)
    {
        case WM_INITDIALOG:
            Assert(lParam);
#ifndef WIN16
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
#endif
            {
                pdp = (SPromptDlgParams*)lParam;
                SetDlgItemText(hDlg,IDC_PROMPT_PROMPT,pdp->strUserMessage);
                SetDlgItemText(hDlg,IDC_PROMPT_EDIT,pdp->strEditFieldValue);
                // Select the entire default string value:
                SendDlgItemMessage(hDlg,IDC_PROMPT_EDIT,EM_SETSEL, 0, -1 );
            }
            return (TRUE);

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    {
                        HRESULT hr;
                        int linelength;
#ifndef WIN16
                        pdp = (SPromptDlgParams*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
#endif
                        Assert( pdp );
                        linelength = SendDlgItemMessage(hDlg,IDC_PROMPT_EDIT,EM_LINELENGTH,(WPARAM)0,(LPARAM)0);
                        // FYI: this routine allocates linelength+1 TCHARS, +1 for the NULL.
                        hr = pdp->strEditFieldValue.Set( NULL, linelength );
                        if( FAILED( hr ) )
                            goto ErrorDispatch;
                        GetDlgItemText(hDlg,IDC_PROMPT_EDIT,pdp->strEditFieldValue,linelength+1);
                    }
                    EndDialog(hDlg, TRUE);
                    break;
                case IDCANCEL:
                ErrorDispatch:;
                    EndDialog(hDlg, FALSE);
                    break;
            }
            return TRUE;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;

    }
    return FALSE;
}


STDMETHODIMP
COmWindow2::prompt(BSTR message, BSTR defstr, VARIANT *retval)
{
    HRESULT             hr = S_OK;
    int                 result = 0;
    SPromptDlgParams    pdparams;

    pdparams.strUserMessage.SetBSTR(message);
    pdparams.strEditFieldValue.SetBSTR(defstr);

    _pDoc->EnableModeless(FALSE);

    result = DialogBoxParam(GetResourceHInst(),             // handle to application instance
                            MAKEINTRESOURCE(IDD_PROMPT_MSHTML),    // identifies dialog box template name
                            _pDoc->InPlace() ? _pDoc->InPlace()->_hwnd : NULL,
                            PromptDlgProc,                  // pointer to dialog box procedure
                            (LPARAM)&pdparams);

    _pDoc->EnableModeless(TRUE);

    if ( retval )
    {
        if (!result)  // If the user hit cancel then allways return the empty string
        {
            // Nav compatability - return null
            V_VT(retval) = VT_NULL;
        }
        else
        {
            // Allocate the returned bstr:
            V_VT(retval) = VT_BSTR;
            hr = THR((pdparams.strEditFieldValue.AllocBSTR( &V_BSTR(retval) )));
        }
    }

    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------------------------------
//
//  Member:     focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::focus()
{
    HRESULT hr;
    hr = THR(_pDoc->WindowFocusHelper());
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     blur
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
COmWindow2::blur()
{
    HRESULT hr;
    hr = THR(_pDoc->WindowBlurHelper());
    RRETURN(SetErrorInfo(hr));
}

//=============================================================================
//=============================================================================
//
//          EVENTPARAM CLASS:
//
//=============================================================================
//=============================================================================

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::EVENTPARAM
//
//  Synopsis:   constructor
//
//  Parameters: pDoc            Doc to bind to
//              fInitState      If true, then initialize state type
//                              members.  (E.g. x, y, keystate, etc).
//
//---------------------------------------------------------------------------

EVENTPARAM::EVENTPARAM(CDoc * pNewDoc, BOOL fInitState, BOOL fPush /* = TRUE */)
{
    memset(this, 0, sizeof(*this));

    pDoc = pNewDoc;
    pDoc->SubAddRef(); // balanced in destructor

    _fOnStack = FALSE;

    if (fPush)
        Push();

    _lKeyCode = 0;

    if (fInitState)
    {
        POINT       pt;

        ::GetCursorPos(&pt);
        _screenX= pt.x;
        _screenY= pt.y;

        // CHROME
        // If we Chrome hosted then there is no valid HWND on
        // which to call ScreenToClient. Hence, if this event
        // is being fired in response to mouse interaction
        // the client supplied mouse coordinates which are
        // cached by Chrome are used. Otherwise the standard
        // code path is used - ScreenToClient with a NULL hwnd
        // which results in client coordinates being the
        // same as screen coordinates. This should be acceptable
        // for non-mouse generated events.
        if (!pDoc->IsChromeHosted())
        {
            if (pDoc->_pInPlace)
            {
                ::ScreenToClient(pDoc->_pInPlace->_hwnd, &pt);
            }
        }
        else
        {
            // If the event was not fired due to a mouse message
            // this function will not set pt resulting in screen
            // and client coordinates being identical. Otherwise
            // it will set the client coordinates to the cached
            // coordinates passed in by the container.
            pDoc->GetChromeCursorPos(&pt);
        }
        _clientX = pt.x;
        _clientY = pt.y;
        _sKeyState = VBShiftState();
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::~EVENTPARAM
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------------------

EVENTPARAM::~EVENTPARAM()
{
    Pop();

    ReleaseInterface( (IUnknown *)psrcFilter );

    pDoc->SubRelease(); // to balance SubAddRef in constructor
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::Push
//
//---------------------------------------------------------------------------

void
EVENTPARAM::Push()
{
    if (!_fOnStack)
    {
        _fOnStack = TRUE;
        pparamPrev = pDoc->_pparam;
        pDoc->_pparam = this;
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::Pop
//
//---------------------------------------------------------------------------

void
EVENTPARAM::Pop()
{
    if (_fOnStack)
    {
        _fOnStack = FALSE;
        pDoc->_pparam = pparamPrev;
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::IsCancelled
//
//---------------------------------------------------------------------------

BOOL
EVENTPARAM::IsCancelled()
{
    HRESULT     hr;

    if (VT_EMPTY != V_VT(&varReturnValue))
    {
        hr = THR(VariantChangeTypeSpecial (&varReturnValue, &varReturnValue,VT_BOOL));
        if (hr)
            return FALSE; // assume event was not cancelled if changing type to bool failed

        return (VT_BOOL == V_VT(&varReturnValue) && VB_FALSE == V_BOOL(&varReturnValue));
    }
    else
    {   // if the variant is empty, we consider it is not cancelled by default
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_defaultStatus
//
//  Synopsis :  Return string of the default status.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_defaultStatus(BSTR *pbstr)
{
    HRESULT hr = S_OK;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(_pDoc->_acstrStatus[STL_DEFSTATUS].AllocBSTR(pbstr));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_status
//
//  Synopsis :  Return string of the status.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_status(BSTR *pbstr)
{
    HRESULT hr = S_OK;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(_pDoc->_acstrStatus[STL_TOPSTATUS].AllocBSTR(pbstr));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::put_defaultStatus
//
//  Synopsis :  Set the default status.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::put_defaultStatus(BSTR bstr)
{
    HRESULT hr;

    hr = THR(_pDoc->SetStatusText(bstr, STL_DEFSTATUS));

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::put_status
//
//  Synopsis :  Set the status text.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::put_status(BSTR bstr)
{
    HRESULT hr;

    hr = THR(_pDoc->SetStatusText(bstr, STL_TOPSTATUS));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::scroll
//
//  Synopsis :  Scroll the document by this x & y.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::scroll(long x, long y)
{
    CLayout *   pLayout;
    CDispNode * pDispNode;
    CElement  * pElementClient = _pDoc->GetPrimaryElementClient();

    if (!pElementClient)
    {
        return S_OK;
    }

    pLayout   = pElementClient->GetCurLayout();

    if (!pLayout)
    {
        return S_OK;
    }

    // make sure that we are calced before trying anything fancy
    pElementClient->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

    pDispNode =  pLayout->GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        pLayout->ScrollTo(CSize(x, y));
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::scrollTo
//
//  Synopsis :  Scroll the document to this x & y.
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::scrollTo(long x, long y)
{
    RRETURN(scroll(x, y));
}

//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::scrollBy
//
//  Synopsis :  Scroll the document by this x & y.
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::scrollBy(long x, long y)
{
    CLayout *   pLayout;
    CDispNode * pDispNode;
    CElement  * pElementClient = _pDoc->GetPrimaryElementClient();

    if (!pElementClient)
        return S_OK;

    // make sure that we are calced before trying anything fancy
    pElementClient->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

    pLayout   = pElementClient->GetCurLayout();
    pDispNode = pLayout
                    ? pLayout->GetElementDispNode()
                    : NULL;

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        pLayout->ScrollBy(CSize(x, y));
    }

    return S_OK;
}


class CGetShDocvwWindow
{
public:
    CGetShDocvwWindow(COmWindow2 *pWindow);
    ~CGetShDocvwWindow();

    IHTMLWindow2 *  _pWindow;
};

CGetShDocvwWindow::CGetShDocvwWindow(COmWindow2 *pWindow)
{
    _pWindow = NULL;
    THR_NOTRACE(pWindow->_pDoc->QueryService(
            SID_SHTMLWindow2,
            IID_IHTMLWindow2,
            (void**)&_pWindow));
}

CGetShDocvwWindow::~CGetShDocvwWindow()
{
    ReleaseInterface(_pWindow);
}


class CGetShDocvwWindow3
{
public:
    CGetShDocvwWindow3(COmWindow2 *pWindow);
    ~CGetShDocvwWindow3();

    IHTMLWindow3 *  _pWindow;
};

CGetShDocvwWindow3::CGetShDocvwWindow3(COmWindow2 *pWindow)
{
    _pWindow = NULL;
    THR_NOTRACE(pWindow->_pDoc->QueryService(
            SID_SHTMLWindow2,
            IID_IHTMLWindow3,
            (void**)&_pWindow));
}

CGetShDocvwWindow3::~CGetShDocvwWindow3()
{
    ReleaseInterface(_pWindow);
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_location
//
//  Synopsis :  Retrieve the location object.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_location(IHTMLLocation **ppLocation)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_location(ppLocation));
    }
    else
    {
        if (!_pLocation)
        {
            _pLocation = new COmLocation(this);
            if (!_pLocation)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // will subaddref on us if QI succeeded
        hr = THR(_pLocation->QueryInterface(IID_IHTMLLocation, (VOID **)ppLocation));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_history
//
//  Synopsis :  Retrieve the history object.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_history(IOmHistory **ppHistory)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_history(ppHistory));
    }
    else
    {
        if (!_pHistory)
        {
            _pHistory = new COmHistory(this);
            if (!_pHistory)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // will subaddref on us if QI succeeded
        hr = THR(_pHistory->QueryInterface(IID_IOmHistory, (VOID **)ppHistory));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::close
//
//  Synopsis :  Close this window.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::close()
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->close());
    }
    else
    {
        // Try to Exec an OLECMDID_CLOSE command.
        CTExec(_pDoc->_pClientSite, NULL, OLECMDID_CLOSE, 0, NULL, NULL);
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::put_opener
//
//  Synopsis :  Retrieve the opener property.
//
//----------------------------------------------------------------------------

HRESULT COmWindow2::put_opener(VARIANT varOpener)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->put_opener(varOpener));
    }
    else
    {
        hr = THR(VariantCopy(&_varOpener, &varOpener));
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_opener
//
//  Synopsis :  Retrieve the opener object.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_opener(VARIANT *pvarOpener)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_opener(pvarOpener));
    }
    else
    {
        if (pvarOpener)
            hr = THR(VariantCopy(pvarOpener, &_varOpener));
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_navigator
//
//  Synopsis :  Retrieve the navigator object.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_navigator(IOmNavigator **ppNavigator)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_navigator(ppNavigator));
    }
    else
    {
        if (!_pNavigator)
        {
            _pNavigator = new COmNavigator(this);
            if (!_pNavigator)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // will subaddref on us if QI succeeded
        hr = THR(_pNavigator->QueryInterface(IID_IOmNavigator, (VOID **)ppNavigator));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_clientInformation
//
//  Synopsis :  Retrieve the navigator object though the client alias
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_clientInformation(IOmNavigator **ppNavigator)
{
    RRETURN(get_navigator(ppNavigator));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::put_name
//
//  Synopsis :  Set the name of this window.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::put_name(BSTR bstr)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->put_name(bstr));
    }
    else
    {
        hr = THR(_cstrName.Set(bstr));
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_name
//
//  Synopsis :  Retrieve the name of this window.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_name(BSTR *pbstr)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_name(pbstr));
    }
    else
    {
        hr = THR(_cstrName.AllocBSTR(pbstr));
    }

    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindow2::get_screenLeft
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT
COmWindow2::get_screenLeft(long *pVal)
{
    HRESULT             hr = S_OK;
    POINT               pt;

    pt.x = pt.y = 0;

    if(_pDoc->_pInPlace && !ClientToScreen(_pDoc->_pInPlace->_hwnd, &pt))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pVal = pt.x;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+--------------------------------------------------------------------------
//
//  Member:     COmWindow2::get_screenTop
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT
COmWindow2::get_screenTop(long *pVal)
{
    HRESULT             hr = S_OK;
    POINT               pt;

    pt.x = pt.y = 0;

    if(_pDoc->_pInPlace && !ClientToScreen(_pDoc->_pInPlace->_hwnd, &pt))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pVal = pt.y;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
COmWindow2::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    RRETURN(CBase::attachEvent(event, pDisp, pResult));
}

HRESULT
COmWindow2::detachEvent(BSTR event, IDispatch* pDisp)
{
    RRETURN(CBase::detachEvent(event, pDisp));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::execScript
//
//  Synopsis :  interface method to immediately execute a piece of script passed
//      in. this is added to support the multimedia efforts that need to execute
//      a script immediatedly, based on a high resolution timer. rather than
//      our setTimeOut mechanism which has to post execute-messages
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::execScript(BSTR bstrCode, BSTR bstrLanguage, VARIANT * pvarRet)
{
    HRESULT hr = E_INVALIDARG;

    if (SysStringLen(bstrCode) && pvarRet)
    {
        CExcepInfo       ExcepInfo;

        hr = CTL_E_METHODNOTAPPLICABLE;

        if (_pDoc->_pScriptCollection)
        {
            hr = THR(_pDoc->_pScriptCollection->ParseScriptText(
                bstrLanguage,           // pchLanguage
                NULL,                   // pMarkup
                NULL,                   // pchType
                bstrCode,               // pchCode
                NULL,                   // pchItemName
                _T("\""),               // pchDelimiter
                0,                      // ulOffset
                0,                      // ulStartingLine
                NULL,                   // pSourceObject
                SCRIPTTEXT_ISVISIBLE,   // dwFlags
                pvarRet,                // pvarResult
                &ExcepInfo));           // pExcepInfo
        }
    }

    RRETURN(SetErrorInfo( hr ));
}


//+--------------------------------------------------------------------------
//
//  Member : print
//
//  Synopsis : implementation of the IOmWindow3 method to expose print behavior
//      through the OM. to get the proper UI all we need to do is send this 
//      to the top level window.
//
//+--------------------------------------------------------------------------
HRESULT
COmWindow2::print() 
{
    HRESULT  hr = S_OK;
    CDoc   * pDocRoot = _pDoc->GetRootDoc();

    if (pDocRoot->IsPrintDoc() || _pDoc->_fPrintEvent)
        goto Cleanup;

    // turn this into the print ExecCommand.
    hr = THR(pDocRoot->Exec(const_cast < GUID * > ( & CGID_MSHTML ), 
                            IDM_EXECPRINT, 
                            0, 
                            NULL, 
                            NULL));

    // is the print canceled?
    if (hr == S_FALSE || hr == OLECMDERR_E_CANCELED)
        hr = S_OK;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::open
//
//  Synopsis :  Open a new window.
//
//----------------------------------------------------------------------------

#ifdef NO_MARSHALLING
extern "C" HRESULT CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk );
#endif

HRESULT
COmWindow2::open(
    BSTR url,
    BSTR name,
    BSTR features,
    VARIANT_BOOL replace,
    IHTMLWindow2** pomWindowResult)
{
    HRESULT             hr;
    CGetShDocvwWindow   cg(this);
    IWebBrowserApp *    pBrowser = NULL;
    IServiceProvider *  pSP = NULL;
    IHTMLWindow2 *      pWindow = NULL;
    BSTR                bstrFilteredFeatures = NULL;
    BSTR              * pbstrFeatures = &features;

    // If running on NT5, we must call AllowSetForegroundWindow
    // so that the new window will be shown in the foreground.
    // 
    if (VER_PLATFORM_WIN32_NT == g_dwPlatformID
       && g_dwPlatformVersion >= 0x00050000)
    {
        // BUGBUG: Pass -1 to AllowSetForegroundWindow
        // to specify that all processes can set the foreground
        // window. We should really pass ASFW_ANY but at this point
        // Trident is being built with _WIN32_WINNT set to version 4
        // so this constant doesn't get included. When we start using
        // NT5 headers, the -1 should be changed to ASFW_ANY
        //
        AllowSetForegroundWindow((DWORD)-1);
    }

    if(features && *features)
    {
        CDoc *  pDocRoot = _pDoc->GetRootDoc();
        if(!pDocRoot->_fTrustedDoc)
        {
            // We are not inside a trusted doc (e.g. an HTA), remove the unsafe items
            // from the features string
            hr = THR(FilterOutFeaturesString(features, &bstrFilteredFeatures));
            if(FAILED(hr))
                goto Cleanup;
            pbstrFeatures = &bstrFilteredFeatures;
        }
    }

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->open(url, name, *pbstrFeatures, replace, pomWindowResult));
        if (hr == S_FALSE)
            hr = S_OK;
    }
    else
    {
#ifdef NO_MARSHALLING
        hr = THR(CoCreateInternetExplorer( IID_IWebBrowserApp, CLSCTX_LOCAL_SERVER,
                                           (void **) &pBrowser ));
#else
        hr = THR(CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER,
                                  IID_IWebBrowserApp, (void **)&pBrowser));
#endif
        if (hr)
        {
            AssertSz(FALSE, "window.open : CoCreate failed");
            goto Cleanup;
        }

        hr = THR(pBrowser->QueryInterface(IID_IServiceProvider, (void **)&pSP));
        if (hr)
        {
            AssertSz(FALSE, "Window.open: QI for ISP failed");
            goto Cleanup;
        }

        hr = THR(pSP->QueryService(SID_SHTMLWindow2, IID_IHTMLWindow2, (void**)&pWindow));
        if (hr)
        {
            AssertSz(FALSE, "Window.open: QS for IHTMLWindow2 failed");
            goto Cleanup;
        }

        hr = THR(pWindow->open(url, name, *pbstrFeatures, replace, pomWindowResult));
        if (hr)
        {
            AssertSz(FALSE, "Window.open: failed");
            goto Cleanup;
        }
    }

Cleanup:
    FormsFreeString(bstrFilteredFeatures);
    ReleaseInterface(pWindow);
    ReleaseInterface(pSP);
    ReleaseInterface(pBrowser);

    RRETURN1 (SetErrorInfo(hr), S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::navigate
//
//  Synopsis :  Navigate this window elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::navigate(BSTR bstr)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->navigate(bstr));
    }
    else
    {
        hr = THR(_pDoc->FollowHyperlink(bstr));
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_closed
//
//  Synopsis :  Retrieve the closed property.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_closed(VARIANT_BOOL *p)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->get_closed(p));
    }
    else
    {
        // always return false since close() is a NOP
        *p = FALSE;
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_self
//
//  Synopsis :  Retrieve ourself.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_self(IHTMLWindow2 **ppSelf)
{
    HRESULT             hr = S_OK;

    if (_pDoc->_fDelegateWindowOM)
    {
        CGetShDocvwWindow   cg(this);
        if (cg._pWindow)
        {
            hr = THR(cg._pWindow->get_self(ppSelf));
            goto Cleanup;
        }
    }

    *ppSelf = (IHTMLWindow2 *)this;
    AddRef();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_window
//
//  Synopsis :  Retrieve self.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_window(IHTMLWindow2 **ppWindow)
{
    RRETURN(THR(get_self(ppWindow)));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_top
//
//  Synopsis :  Get the topmost window in this hierarchy.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_top(IHTMLWindow2 **ppTop)
{
    HRESULT hr = S_OK;
    CDoc *  pDocTop = NULL; 
    
    if (_pDoc->_fDelegateWindowOM)
    {
        CGetShDocvwWindow   cg(this);
        if (cg._pWindow)
        {
            hr = THR(cg._pWindow->get_top(ppTop));
            goto Cleanup;
        }
    }

    pDocTop = _pDoc->GetTopDoc();
    
    hr = THR(pDocTop->EnsureOmWindow());
    if (hr)
        goto Cleanup;

    *ppTop = (IHTMLWindow2 *)pDocTop->_pOmWindow->_pWindow;
    (*ppTop)->AddRef();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_parent
//
//  Synopsis :  Retrieve parent window in this hierarchy.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_parent(IHTMLWindow2 **ppParent)
{
    HRESULT hr = S_OK;
    CDoc *  pDocParent = NULL;

    
    if (_pDoc->_fDelegateWindowOM)
    {
        CGetShDocvwWindow   cg(this);
        if (cg._pWindow)
        {
            hr = THR(cg._pWindow->get_parent(ppParent));
            goto Cleanup;
        }
    }

    // If this document doesn't have a parent, or if not trusted with a trusted
    // parent, return self.
    if ((!_pDoc->_pDocParent) || (!_pDoc->_fTrustedDoc && _pDoc->_pDocParent->_fTrustedDoc))
        pDocParent = _pDoc;
    else
        pDocParent = _pDoc->_pDocParent;
        
    hr = THR(pDocParent->EnsureOmWindow());
    if (hr)
        goto Cleanup;

    *ppParent = (IHTMLWindow2 *)pDocParent->_pOmWindow->_pWindow;
    (*ppParent)->AddRef();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------
//
//  Members: moveTo, moveBy, resizeTo, resizeBy
//
// BUGBUG (carled) - eventually the then clause of the shdocvw if, should 
// be removed and the else clause should become the body of 
// this function.  To do this SHDOCVW will have to implement
// IHTMLOMWindowServices and this is part of the window split 
// work.  This will have to happen for almost every function and
// method in this file that delegates explicitly to shdocvw.  i.e.
// trident should have NO explicit knowledge about its host.
//
//----------------------------------------------------------------------

HRESULT
COmWindow2::moveTo(long x, long y)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->moveTo(x, y));
    }
    else if (_pDoc == _pDoc->GetRootDoc())
    {
        // don't do anything unless we are the top level document
        IHTMLOMWindowServices *pOMWinServices = NULL;

        // QueryService our container for IHTMLOMWindowServices
        hr = THR(_pDoc->QueryService(IID_IHTMLOMWindowServices, 
                                     IID_IHTMLOMWindowServices, 
                                     (void**)&pOMWinServices));
        if (hr)
            goto Cleanup;

        // delegate the call to our host.
        hr = THR(pOMWinServices->moveTo(x, y));

Cleanup:
        if (hr == E_NOINTERFACE)
            hr = S_OK;   // fail silently

        ReleaseInterface(pOMWinServices);
    }

    RRETURN(SetErrorInfo(hr));
}

HRESULT
COmWindow2::moveBy(long x, long y)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        hr = THR(cg._pWindow->moveBy(x, y));
    }
    else if (_pDoc == _pDoc->GetRootDoc())
    {
        // don't do anything unless we are the top level document
        IHTMLOMWindowServices *pOMWinServices = NULL;

        // QueryService our container for IHTMLOMWindowServices
        hr = THR(_pDoc->QueryService(IID_IHTMLOMWindowServices, 
                                     IID_IHTMLOMWindowServices, 
                                     (void**)&pOMWinServices));
        if (hr)
            goto Cleanup;

        // delegate the call to our host.
        hr = THR(pOMWinServices->moveBy(x, y));

Cleanup:
        if (hr == E_NOINTERFACE)
            hr = S_OK;   // fail silently

        ReleaseInterface(pOMWinServices);
    }

    RRETURN(SetErrorInfo(hr));
}

HRESULT
COmWindow2::resizeTo(long x, long y)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;

    if (cg._pWindow)
    {
        // now pass the (secure) size request up to shdocvw
        hr = THR(cg._pWindow->resizeTo(x, y));
    }
    else if (_pDoc == _pDoc->GetRootDoc())
    {
        // don't do anything unless we are the top level document
        IHTMLOMWindowServices *pOMWinServices = NULL;

        // QueryService our container for IHTMLOMWindowServices
        hr = THR(_pDoc->QueryService(IID_IHTMLOMWindowServices, 
                                     IID_IHTMLOMWindowServices, 
                                     (void**)&pOMWinServices));
        if (hr)
            goto Cleanup;

        // delegate the call to our host.
        hr = THR(pOMWinServices->resizeTo(x, y));

Cleanup:
        if (hr == E_NOINTERFACE)
            hr = S_OK;   // fail silently

        ReleaseInterface(pOMWinServices);
    }

    RRETURN(SetErrorInfo(hr));
}

HRESULT
COmWindow2::resizeBy(long x, long y)
{
    CGetShDocvwWindow   cg(this);
    HRESULT             hr = S_OK;


    if (cg._pWindow)
    {
        // now pass the (secure) size request up to shdocvw
        hr = THR(cg._pWindow->resizeBy(x, y));
    }
    else if (_pDoc == _pDoc->GetRootDoc())
    {
        // don't do anything unless we are the top level document
        IHTMLOMWindowServices *pOMWinServices = NULL;

        // QueryService our container for IHTMLOMWindowServices
        hr = THR(_pDoc->QueryService(IID_IHTMLOMWindowServices, 
                                     IID_IHTMLOMWindowServices, 
                                     (void**)&pOMWinServices));
        if (hr)
            goto Cleanup;

        // delegate the call to our host.
        hr = THR(pOMWinServices->resizeBy(x, y));

Cleanup:
        if (hr == E_NOINTERFACE)
            hr = S_OK;   // fail silently

        ReleaseInterface(pOMWinServices);
    }

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_Option
//
//  Synopsis :  Retrieve Option element factory.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_Option(IHTMLOptionElementFactory **ppDisp)
{
    HRESULT                 hr = S_OK;
    COptionElementFactory * pFactory;

    if (ppDisp == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppDisp = NULL;

    pFactory = new COptionElementFactory;
    if ( !pFactory )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pFactory->_pDoc = _pDoc;
    hr = pFactory->QueryInterface ( IID_IHTMLOptionElementFactory, (void **)ppDisp );
    pFactory->PrivateRelease();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_Image
//
//  Synopsis :  Retrieve Image element factory.
//
//----------------------------------------------------------------------------

HRESULT
COmWindow2::get_Image(IHTMLImageElementFactory**ppDisp)
{
    HRESULT                 hr = S_OK;
    CImageElementFactory *  pFactory;

    if (ppDisp == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppDisp = NULL;

    pFactory = new CImageElementFactory;
    if ( !pFactory )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pFactory->_pDoc = _pDoc;
    hr = pFactory->QueryInterface ( IID_IHTMLImageElementFactory, (void **)ppDisp );
    pFactory->PrivateRelease();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  CScreen - implementation for the window.screen object
//
//--------------------------------------------------------------------------

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CScreen, COmWindow2, _Screen)

const CBase::CLASSDESC CScreen::s_classdesc =
{
    &CLSID_HTMLScreen,              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLScreen,               // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+-------------------------------------------------------------------------
//
//  CScreen:: property members
//
//--------------------------------------------------------------------------

STDMETHODIMP CScreen::get_colorDepth(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    long d = MyCOmWindow2()->_pDoc->_bufferDepth;
    if (d > 0)
        *p = d;
    else
    {
        // BUGBUG: If code can run during printing, we should figure out
        // how to get to the color depth of the printer.
        // If not, we could add BITSPIXEL as a TLS cached item.
        *p = GetDeviceCaps(TLS(hdcDesktop), BITSPIXEL);
    }
    return S_OK;
}

STDMETHODIMP CScreen::put_bufferDepth(long v)
{
    switch (v)
    {
    case 0:                     // 0 means no explicit buffering requested
    case -1:                    // -1 means buffer at screen depth
    case 1:                     // other values are specific buffer depths...
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
        break;
    default:
        v = -1;                 // for unknown values, use screen depth
        break;
    }

    CDoc *pDoc = MyCOmWindow2()->_pDoc;
    if (pDoc->_bufferDepth != v)
    {
        pDoc->_bufferDepth = v;
        pDoc->Invalidate();
    }
    return S_OK;
}

STDMETHODIMP CScreen::get_bufferDepth(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    *p = MyCOmWindow2()->_pDoc->_bufferDepth;
    return S_OK;
}

STDMETHODIMP CScreen::get_width(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    // BUGBUG: Implement the printer case.
    *p = GetDeviceCaps(TLS(hdcDesktop), HORZRES);
    return S_OK;
}

STDMETHODIMP CScreen::get_height(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    // BUGBUG: Implement the printer case.
    *p = GetDeviceCaps(TLS(hdcDesktop), VERTRES);
    return S_OK;
}

//----------------------------------------------------------------------------
//  Member: put_updateInterval
//
//  Synopsis:   updateInterval specifies the interval between painting invalid
//              regions. This is used for throttling mutliple objects randomly
//              invalidating regions to a specific update time.
//              interval specifies milliseconds.
//----------------------------------------------------------------------------
STDMETHODIMP CScreen::put_updateInterval(long interval)
{
    MyCOmWindow2()->_pDoc->UpdateInterval( interval );
    return S_OK;
}

STDMETHODIMP CScreen::get_updateInterval(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    *p = MyCOmWindow2()->_pDoc->GetUpdateInterval();
    return S_OK;
}

STDMETHODIMP CScreen::get_availHeight(long*p)
{
    HRESULT hr = S_OK;
    RECT    Rect;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = 0;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    *p = Rect.bottom - Rect.top;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CScreen::get_availWidth(long*p)
{
    HRESULT hr = S_OK;
    RECT    Rect;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = 0;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    *p = Rect.right - Rect.left;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CScreen::get_fontSmoothingEnabled(VARIANT_BOOL*p)
{
    HRESULT hr = S_OK;
    BOOL    fSmoothing;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = VB_FALSE;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &fSmoothing, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    if(fSmoothing)
        *p = VB_TRUE;

Cleanup:
    RRETURN(hr);
}
//+-------------------------------------------------------------------------
//
//  Method:     CScreen::QueryInterface
//
//--------------------------------------------------------------------------

HRESULT
CScreen::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    switch (iid.Data1)
    {
     QI_INHERITS(this, IUnknown)
     QI_TEAROFF2(this, IDispatch, IHTMLScreen, NULL)
     QI_TEAROFF(this, IHTMLScreen, NULL)
     QI_TEAROFF(this, IObjectIdentity, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

HRESULT CScreen::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    return QueryInterface(iid, ppv);
}

//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::put_offscreenBuffering
//
//  Synopsis :  Set whether we paint using offscreen buffering
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::put_offscreenBuffering(VARIANT var)
{
    HRESULT     hr;
    CVariant    varTemp;

    hr = varTemp.CoerceVariantArg( &var, VT_BOOL );
    if ( DISP_E_TYPEMISMATCH == hr )
    {
        // handle Nav 4 compat where "" is the same as FALSE
        // and anything in a string is considered TRUE
        if ( VT_BSTR == V_VT(&var) )
        {
            V_VT(&varTemp) = VT_BOOL;
            hr = S_OK;
            if ( 0 == SysStringByteLen(V_BSTR(&var)) )
                V_BOOL(&varTemp) = VB_FALSE;
            else
                V_BOOL(&varTemp) = VB_TRUE;
        }
    }
    if ( SUCCEEDED(hr) )
    {
        _pDoc->SetOffscreenBuffering(BOOL_FROM_VARIANT_BOOL(V_BOOL(&varTemp)));
    }
    return SetErrorInfo(hr);
}


//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_offscreenBuffering
//
//  Synopsis :  Return string/bool of tristate: auto, true, or false
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::get_offscreenBuffering(VARIANT *pvar)
{
    HRESULT hr = S_OK;
    int buffering;      // -1 = auto, 0=false, 1=true

    if (!pvar)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    buffering = _pDoc->GetOffscreenBuffering();
    if ( buffering < 0 )
    {
        V_VT(pvar) = VT_BSTR;
        hr = THR(FormsAllocString ( _T("auto"), &V_BSTR(pvar)));
    }
    else
    {
        V_VT(pvar) = VT_BOOL;
        V_BOOL(pvar) = VARIANT_BOOL_FROM_BOOL(!!buffering);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindow2::get_external
//
//  Synopsis:   Get IDispatch object associated with the IDocHostUIHandler
//
//---------------------------------------------------------------------------
HRESULT
COmWindow2::get_external(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDisp = NULL;

    if (_pDoc->_pHostUIHandler)
    {
        hr = _pDoc->_pHostUIHandler->GetExternal(ppDisp);
            if (hr == E_NOINTERFACE)
            {
                hr = S_OK;
                Assert(*ppDisp == NULL);
            }
            else if (hr)
                goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CAtomTable *
COmWindow2::GetAtomTable (BOOL *pfExpando)
{
    if (pfExpando)
    {
        *pfExpando = _pDoc->_fExpando;
    }

    return &(_pDoc->_AtomTable);
}

//+---------------------------------------------------------------------------
//
//  member :    COmWindow2::get_clipboardData
//
//  Synopsis :  Return the data transfer object.
//
//----------------------------------------------------------------------------
HRESULT
COmWindow2::get_clipboardData(IHTMLDataTransfer **ppDataTransfer)
{
    HRESULT hr = S_OK;
    CDataTransfer * pDataTransfer;
    IDataObject * pDataObj = NULL;

    if (!ppDataTransfer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDataTransfer = NULL;

    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    pDataTransfer = new CDataTransfer(_pDoc, pDataObj, FALSE);  // fDragDrop = FALSE

    pDataObj->Release();    // extra addref from OleGetClipboard

    if (!pDataTransfer)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pDataTransfer->QueryInterface(
                IID_IHTMLDataTransfer,
                (void **) ppDataTransfer));
        pDataTransfer->Release();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//---------------------------------------------------------------------------
//
//  CDataTransfer ClassDesc
//
//---------------------------------------------------------------------------

const CBase::CLASSDESC CDataTransfer::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDataTransfer,             // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};



//---------------------------------------------------------------------------
//
//  Member:     CDataTransfer::CDataTransfer
//
//  Synopsis:   ctor
//
//---------------------------------------------------------------------------

CDataTransfer::CDataTransfer(CDoc * pDoc, IDataObject * pDataObj, BOOL fDragDrop)
{
    _pDoc = pDoc;
    _pDoc->SubAddRef();
    _pDataObj = pDataObj;
    _pDataObj->AddRef();
    _fDragDrop = fDragDrop;
}


//---------------------------------------------------------------------------
//
//  Member:     CDataTransfer::~CDataTransfer
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------------------

CDataTransfer::~CDataTransfer()
{
    _pDoc->SubRelease();
    _pDataObj->Release();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataTransfer::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
CDataTransfer::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTMLDataTransfer, NULL)
    QI_TEAROFF2(this, IUnknown, IHTMLDataTransfer, NULL)
    default:
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

static HRESULT copyBstrToHglobal(BSTR bstr, HGLOBAL *phglobal)
{
    HRESULT hr = S_OK;
    TCHAR *pchText;
    DWORD cbSize;

    Assert(phglobal);

    cbSize = (FormsStringLen(bstr) + 1) * sizeof(TCHAR);
    *phglobal = GlobalAlloc(GMEM_MOVEABLE, cbSize);
    if (!*phglobal)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pchText = (TCHAR *) GlobalLock(*phglobal);
    if (!pchText)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    memcpy(pchText, bstr, cbSize);
    GlobalUnlock(*phglobal);

Cleanup:
    RRETURN(hr);
Error:
    if (*phglobal)
        GlobalFree(*phglobal);
    *phglobal = NULL;
    goto Cleanup;
}

HRESULT
CDataTransfer::setData(BSTR format, VARIANT* data, VARIANT_BOOL* pret)
{
    HRESULT hr = S_OK;
    STGMEDIUM stgmed = {0, NULL};
    FORMATETC fmtc;
    TCHAR * pchData = NULL;
    VARIANT *pvarData;
    EVENTPARAM * pparam;
    IDataObject * pDataObj = NULL;
    BOOL fCutCopy;
    BOOL fAddRef = TRUE;
    CDragStartInfo * pDragStartInfo = _pDoc->_pDragStartInfo;
    IUniformResourceLocator * pUrlToDrag = NULL;

    if (!_fDragDrop)    // access to window.clipboardData.setData
    {
        BOOL fAllow;

        hr = THR(_pDoc->ProcessURLAction(URLACTION_SCRIPT_PASTE, &fAllow));
        if (hr || !fAllow)
            goto Error;       // Fail silently
    }

    pparam = _pDoc->_pparam;

    fCutCopy = !StrCmpIC(_T("cut"), pparam->GetType()) || !StrCmpIC(_T("copy"), pparam->GetType());

    if (_fDragDrop && pDragStartInfo)
        pDataObj = _pDataObj;
    else
    {
        fCutCopy = TRUE;
        if (TLS(pDataClip))
            pDataObj = TLS(pDataClip);
        else
        {
            pDataObj = new CGenDataObject(_pDoc);
            if (pDataObj == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
            fAddRef = FALSE;
        }
    }
    if (fAddRef)
        pDataObj->AddRef();

    if (!pret)
    {
        hr = E_POINTER;
        goto Error;
    }

    pvarData = (data && (V_VT(data) == (VT_BYREF | VT_VARIANT))) ? 
        V_VARIANTREF(data) : data;
    
    if (pvarData)
    {
        if (V_VT(pvarData) == VT_BSTR)
            pchData = V_BSTR(pvarData);
        else
        {
            hr = E_INVALIDARG;
            goto Error;
        }
    }

    if (!StrCmpIC(format, _T("Text")))
    {
        if (pchData)
        {
            hr = copyBstrToHglobal(pchData, &(stgmed.hGlobal));
            if (hr)
                goto Error;
        }
        else
            stgmed.hGlobal = NULL;

        stgmed.tymed = TYMED_HGLOBAL;
        fmtc.cfFormat = CF_UNICODETEXT;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        hr = pDataObj->SetData(&fmtc, &stgmed, TRUE);
        if (hr)
            goto Error;
    }
    else if (!StrCmpIC(format, _T("Url")))
    {
        IDataObject * pLinkDataObj;

        if (pDragStartInfo && pDragStartInfo->_pUrlToDrag)
            hr = pDragStartInfo->_pUrlToDrag->SetURL(pchData, 0);
        else if (pchData && (pDragStartInfo || fCutCopy))
        {
            hr = THR(CreateLinkDataObject(pchData, NULL, &pUrlToDrag));
            if (hr)
                goto Cleanup;

            hr = THR(pUrlToDrag->QueryInterface(IID_IDataObject, (void **) &pLinkDataObj));
            if (hr)
                goto Cleanup;

            ReplaceInterface(&(DYNCAST(CBaseBag, pDataObj)->_pLinkDataObj), pLinkDataObj);
            pLinkDataObj->Release();
        }
    }
    else if (!data)
    {
        if (!StrCmpIC(format, _T("File")))
            fmtc.cfFormat = CF_HDROP;
        else if (!StrCmpIC(format, _T("Html")))
            fmtc.cfFormat = cf_HTML;
        else if (!StrCmpIC(format, _T("Image")))
            fmtc.cfFormat = CF_DIB;
        else
        {
            hr = E_UNEXPECTED;
            goto Error;
        }

        stgmed.hGlobal = NULL;
        stgmed.tymed = TYMED_HGLOBAL;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        hr = pDataObj->SetData(&fmtc, &stgmed, TRUE);
        if (hr)
            goto Error;
    }
    else
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    if (fCutCopy)
        _pDoc->SetClipboard(pDataObj);

    *pret = VB_TRUE;
Cleanup:
    ReleaseInterface(pDataObj);
    ReleaseInterface(pUrlToDrag);
    RRETURN(SetErrorInfo(hr));
Error:
    if (stgmed.hGlobal)
        GlobalFree(stgmed.hGlobal);
    if (pret)
        *pret = VB_FALSE;
    goto Cleanup;
}

HRESULT
CDataTransfer::getData(BSTR format, VARIANT* pvarRet)
{
    HRESULT hr = S_OK;
    STGMEDIUM stgmed = {0, NULL};
    FORMATETC fmtc;
    HGLOBAL hGlobal = NULL;
    HGLOBAL hUnicode = NULL;
    TCHAR * pchText = NULL;
    IUniformResourceLocator * pUrlToDrag = NULL;

    if (!_fDragDrop)    // access to window.clipboardData.getData
    {
        BOOL fAllow;

        hr = THR(_pDoc->ProcessURLAction(URLACTION_SCRIPT_PASTE, &fAllow));
        if (hr || !fAllow)
            goto Cleanup;       // Fail silently
    }

    if (!pvarRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!StrCmpIC(format, _T("Text")))
    {
        fmtc.cfFormat = CF_UNICODETEXT;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        if (    (_pDataObj->QueryGetData(&fmtc) == NOERROR)
            &&  (_pDataObj->GetData(&fmtc, &stgmed) == S_OK))
        {
            hGlobal = stgmed.hGlobal;
            pchText = (TCHAR *) GlobalLock(hGlobal);
        }
        else
        {
            fmtc.cfFormat = CF_TEXT;
            if (    (_pDataObj->QueryGetData(&fmtc) == NOERROR)
                &&  (_pDataObj->GetData(&fmtc, &stgmed) == S_OK))
            {
                hGlobal = stgmed.hGlobal;
                hUnicode = TextHGlobalAtoW(hGlobal);
                pchText    = (TCHAR *) GlobalLock(hUnicode);
            }
            else
            {
                V_VT(pvarRet) = VT_NULL;
                hr = S_OK;
                goto Cleanup;
            }
        }

        if (!pchText)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(pchText);
    }
    else if (!StrCmpIC(format, _T("Url")))
    {
        hr = _pDataObj->QueryInterface(IID_IUniformResourceLocator, (void**)&pUrlToDrag);
        if (hr && DYNCAST(CBaseBag, _pDataObj)->_pLinkDataObj)
            hr = DYNCAST(CBaseBag, _pDataObj)->_pLinkDataObj->QueryInterface(IID_IUniformResourceLocator, (void**)&pUrlToDrag);
        if (hr)
        {
            V_VT(pvarRet) = VT_NULL;
            hr = S_OK;
            goto Cleanup;
        }

        hr = pUrlToDrag->GetURL(&pchText);
        if (hr)
            goto Cleanup;

        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(pchText);
        CoTaskMemFree(pchText);
    }
    else
        hr = E_UNEXPECTED;


Cleanup:
    if (hGlobal)
    {
        GlobalUnlock(hGlobal);
        ReleaseStgMedium(&stgmed);
    }
    if (hUnicode)
    {
        GlobalUnlock(hUnicode);
        GlobalFree(hUnicode);
    }
    ReleaseInterface(pUrlToDrag);

    RRETURN(_pDoc->SetErrorInfo(hr));
}

HRESULT
CDataTransfer::clearData(BSTR format, VARIANT_BOOL* pret)
{
    HRESULT hr = S_OK;

    if (!StrCmpIC(format, _T("null")))
    {
        IGNORE_HR(setData(_T("Text"), NULL, pret));
        IGNORE_HR(setData(_T("Url"), NULL, pret));
        IGNORE_HR(setData(_T("File"), NULL, pret));
        IGNORE_HR(setData(_T("Html"), NULL, pret));
        IGNORE_HR(setData(_T("Image"), NULL, pret));
    }
    else
    {
        hr = setData(format, NULL, pret);
    }

    RRETURN(hr);
}

HRESULT
CDataTransfer::get_dropEffect(BSTR *pbstrDropEffect)
{
    HRESULT hr;
    htmlDropEffect dropEffect = htmlDropEffectNone;
    EVENTPARAM * pparam;

    if (!_fDragDrop)
    {
        *pbstrDropEffect = NULL;
        hr = S_OK;
        goto Cleanup;
    }

    if (pbstrDropEffect == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pparam = _pDoc->_pparam;

    switch (pparam->dwDropEffect)
    {
    case DROPEFFECT_COPY:
        dropEffect = htmlDropEffectCopy;
        break;
    case DROPEFFECT_LINK:
        dropEffect = htmlDropEffectLink;
        break;
    case DROPEFFECT_MOVE:
        dropEffect = htmlDropEffectMove;
        break;
    case DROPEFFECT_NONE:
        dropEffect = htmlDropEffectNone;
        break;
    default:
        Assert(FALSE);
    }

    hr = THR(STRINGFROMENUM(htmlDropEffect, (long) dropEffect, pbstrDropEffect));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDataTransfer::put_dropEffect(BSTR bstrDropEffect)
{
    HRESULT hr;
    htmlDropEffect dropEffect;
    EVENTPARAM * pparam;

    if (!_fDragDrop)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlDropEffect, bstrDropEffect, (long *) &dropEffect));
    if (hr)
        goto Cleanup;

    pparam = _pDoc->_pparam;

    switch (dropEffect)
    {
    case htmlDropEffectCopy:
        pparam->dwDropEffect = DROPEFFECT_COPY;
        break;
    case htmlDropEffectLink:
        pparam->dwDropEffect = DROPEFFECT_LINK;
        break;
    case htmlDropEffectMove:
        pparam->dwDropEffect = DROPEFFECT_MOVE;
        break;
    case htmlDropEffectNone:
        pparam->dwDropEffect = DROPEFFECT_NONE;
        break;
    default:
        Assert(FALSE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDataTransfer::get_effectAllowed(BSTR *pbstrEffectAllowed)
{
    HRESULT hr;
    htmlEffectAllowed effectAllowed;
    CDragStartInfo * pDragStartInfo = _pDoc->_pDragStartInfo;

    if (!_fDragDrop)
    {
        *pbstrEffectAllowed = NULL;
        hr = S_OK;
        goto Cleanup;
    }

    if (pbstrEffectAllowed == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDragStartInfo)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    switch (pDragStartInfo->_dwEffectAllowed)
    {
    case DROPEFFECT_COPY:
        effectAllowed = htmlEffectAllowedCopy;
        break;
    case DROPEFFECT_LINK:
        effectAllowed = htmlEffectAllowedLink;
        break;
    case DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedMove;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_LINK:
        effectAllowed = htmlEffectAllowedCopyLink;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedCopyMove;
        break;
    case DROPEFFECT_LINK | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedLinkMove;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedAll;
        break;
    case DROPEFFECT_NONE:
        effectAllowed = htmlEffectAllowedNone;
        break;
    default:
        effectAllowed = htmlEffectAllowedUninitialized;
        break;
    }

    hr = THR(STRINGFROMENUM(htmlEffectAllowed, (long) effectAllowed, pbstrEffectAllowed));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDataTransfer::put_effectAllowed(BSTR bstrEffectAllowed)
{
    HRESULT hr;
    htmlEffectAllowed effectAllowed;
    CDragStartInfo * pDragStartInfo = _pDoc->_pDragStartInfo;

    if (!_fDragDrop)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (!pDragStartInfo)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlEffectAllowed, bstrEffectAllowed, (long *) &effectAllowed));
    if (hr)
        goto Cleanup;

    switch (effectAllowed)
    {
    case htmlEffectAllowedCopy:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY;
        break;
    case htmlEffectAllowedLink:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_LINK;
        break;
    case htmlEffectAllowedMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedCopyLink:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_LINK;
        break;
    case htmlEffectAllowedCopyMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedLinkMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_LINK | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedAll:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedNone:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_NONE;
        break;
    case htmlEffectAllowedUninitialized:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_UNINITIALIZED;
        break;
    default:
        Assert(FALSE);
    }

Cleanup:
    RRETURN(hr);
}
