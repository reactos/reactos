//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       peerhand.cxx
//
//  History:    30-Jan-1998     terrylu     Created
//
//  Contents:   IElementBehavior implementation
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_HANDIMPL_HXX_
#define X_HANDIMPL_HXX_
#include "handimpl.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_FORMSARY_HXX_
#define X_FORMSARY_HXX_
#include <formsary.hxx>
#endif

#ifndef X_SCROID_H_
#define X_SCROID_H_
#include "scroid.h"         // For SID_ScriptletDispatch
#endif

#ifndef X_HANDLER_H_
#define X_HANDLER_H_
#include "handler.h"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#ifdef _MAC
#include "misc2.hxx"
#else
#include "misc.hxx"
#endif
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_PEERHAND_HXX_
#define X_PEERHAND_HXX_
#include "peerhand.hxx"
#endif

#ifndef X_PEERDISP_HXX_
#define X_PEERDISP_HXX_
#include "peerdisp.hxx"
#endif

MtDefine(CPeerHandlerConstructor, Scriptlet, "CPeerHandlerConstructor")
MtDefine(CPeerHandlerConstructor_aryEventFlags, Scriptlet, "CPeerHandlerConstructor_aryEventFlags")
MtDefine(CPeerHandler, Scriptlet, "CPeerHandler")
MtDefine(CPeerHandlerLoad_ppropbag, Locals, "CPeerHandler::Load ppropbag")
MtDefine(CPeerHandlerLoad_pVar, Locals, "CPeerHandler::Load ppropVar")

///////////////////////////////////////////////////////////////////////////
//
// tearoff tables
//
///////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE(CPeerHandlerConstructor, IScriptletHandlerConstructor)
    TEAROFF_METHOD(CPeerHandler, Load, load, (WORD wFlags, PNODE *pnode))
	TEAROFF_METHOD(CPeerHandler, Create, create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler))
	TEAROFF_METHOD(CPeerHandler, Register, register, (LPCOLESTR pstrPath, REFCLSID rclisid, LPCOLESTR pstrProgId))
	TEAROFF_METHOD(CPeerHandler, Unregister, unregister, (REFCLSID rclsid, LPCOLESTR pstrProgId))
	TEAROFF_METHOD(CPeerHandler, AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass, UINT *puiImplIndex))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CPeerHandler, IScriptletHandler)
	TEAROFF_METHOD(CPeerHandler, GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk))
	TEAROFF_METHOD(CPeerHandler, SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CPeerHandler, IElementBehavior)
    TEAROFF_METHOD(CPeerHandler, Init, init, (IElementBehaviorSite *pPeerSite))
    TEAROFF_METHOD(CPeerHandler, Notify, notify, (DWORD dwEvent, VARIANT *pVar))
    TEAROFF_METHOD(CPeerHandler, Detach, detach, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CPeerHandler, IElementBehaviorUI)
    TEAROFF_METHOD(CPeerHandler, OnReceiveFocus, onreceivefocus, (BOOL fFocus, long lSubDivision))
	TEAROFF_METHOD(CPeerHandler, GetSubDivisionProvider, getsubdivisionprovider, (ISubDivisionProvider **pp))
	TEAROFF_METHOD_(CPeerHandler, CanTakeFocus, cantakefocus, BOOL, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CPeerHandler, IPersistPropertyBag2)
    TEAROFF_METHOD(CPeerHandler, GetClassID, getclassid, (CLSID *pclsID))
    TEAROFF_METHOD(CPeerHandler, InitNew, initnew, (void))
    TEAROFF_METHOD(CPeerHandler, Load, load, (IPropertyBag2  *pPropBag, IErrorLog *pErrLog))
    TEAROFF_METHOD(CPeerHandler, Save, save, (IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties))
    TEAROFF_METHOD(CPeerHandler, IsDirty, isdirty, (void))
END_TEAROFF_TABLE()

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

const CBase::CLASSDESC CPeerHandlerConstructor::s_classdesc =
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

const CBase::CLASSDESC CPeerHandler::s_classdesc =
{
    &CLSID_CPeerHandler,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

///////////////////////////////////////////////////////////////////////////
//
// CPeerHandlerConstructor
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Function:   CreatePeerHandlerConstructor
//
//  Synopsis:   Creates a new instance of scriptoid peer handler constructor
//
//-------------------------------------------------------------------------

HRESULT
CreatePeerHandlerConstructor(IUnknown * pUnkContext, IUnknown * pUnkOuter, IUnknown ** ppUnk)
{
    HRESULT         hr;
    CPeerHandlerConstructor *  pPeer;

    *ppUnk = NULL;

    if (pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto Cleanup;
    }
    
    pPeer = new CPeerHandlerConstructor();
    hr = pPeer ? S_OK : E_OUTOFMEMORY;
    if (hr)
        goto Cleanup;

    (*ppUnk) = (IUnknown*) pPeer;

Cleanup:    
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandlerConstructor::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IScriptletHandlerConstructor, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor constructor
//
//-------------------------------------------------------------------------

CPeerHandlerConstructor::CPeerHandlerConstructor()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor destructor
//
//-------------------------------------------------------------------------

CPeerHandlerConstructor::~CPeerHandlerConstructor()
{
    _aryEvents.Free();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor::Load, per IScriptletHandlerConstructor
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandlerConstructor::Load(WORD wFlags, PNODE * pNode)
{
    return ProcessTree(pNode);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor::Create, per IScriptletHandlerConstructor
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandlerConstructor::Create(IUnknown * punkContext, IUnknown * punkOuter, IUnknown ** ppunkHandler)
{
    HRESULT         hr = S_OK;
    CPeerHandler *  pPeerHandler = NULL;
    IElementBehaviorSite * pPeerSite = NULL;

    if (!punkContext)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // construct

    pPeerHandler = new CPeerHandler(punkOuter);
    if (!pPeerHandler)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = punkContext->QueryInterface(IID_IElementBehaviorSite, (void**)&pPeerSite);
    if (hr)
        goto Cleanup;

    // initialize

    hr = pPeerHandler->Init(pPeerSite);
    if (hr)
        goto Cleanup;

    // register events

    if (_cstrName.Length())
    {
        hr = THR(pPeerHandler->_pPeerSiteOM->RegisterName(_cstrName));
        if (hr)
            goto Cleanup;
    }

    {
        int i, c;

        for (i = 0, c = _aryEvents.Size(); i < c; i++)
        {
            hr = pPeerHandler->_pPeerSiteOM->RegisterEvent (_aryEvents[i], _aryEventFlags[i], NULL);
            if (hr)
                goto Cleanup;
        }
    }

    // finalize

    (*ppunkHandler) = (IUnknown*)pPeerHandler;

Cleanup:
    ReleaseInterface(pPeerSite);

    if (hr)
        delete pPeerHandler;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Helper:         IsToken, inline helper for iterating
//
//-------------------------------------------------------------------------

inline BOOL IsToken(PNODE * pNode, PK pk, LPCOLESTR pchName)
{
    Assert (pchName);
    return pk == pNode->pk && 0 == StrCmpI(pNode->pstrToken, pchName);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandlerConstructor::ProcessTree, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerHandlerConstructor::ProcessTree(PNODE * pNode)
{
    HRESULT     hr = S_OK;
    PNODE *     pNodeAttr;
    DWORD       dwFlags;
    LPTSTR      pchName;

    // make sure the node passed in is "implements" node
    Assert (IsToken(pNode, pkELEMENT, _T("implements")));
    if (!IsToken(pNode, pkELEMENT, _T("implements")))
        return E_UNEXPECTED;

    //
    // scan for name of the peer handler
    //

    pNodeAttr = pNode->element.pnodeAttr;
    while (pNodeAttr)
    {
        if (IsToken(pNodeAttr, pkATTRIBUTE, _T("name")))
        {
            if (pNodeAttr->attribute.pnodeValue &&
                pNodeAttr->attribute.pnodeValue->pstrToken)
            {
                hr = THR(_cstrName.Set((LPTSTR)pNodeAttr->attribute.pnodeValue->pstrToken));
                if (hr)
                    goto Cleanup;
            }
        }
        pNodeAttr = pNodeAttr->pnodeNext;
    } // eo while (pNodeAttr)

    //
    // scan events
    //

    // step down to the children list
    pNode = pNode->element.pnodeData;

    // scan children for "event" tags
    while (pNode)
    {
        if (IsToken(pNode, pkELEMENT, _T("event")))
        {
            // scan for "name", "bubble" attributes

            dwFlags = 0;
            pchName = 0;
            pNodeAttr = pNode->element.pnodeAttr;
            while (pNodeAttr)
            {
                if (IsToken(pNodeAttr, pkATTRIBUTE, _T("name")))
                {
                    if (pNodeAttr->attribute.pnodeValue)
                    {
                        pchName = (LPTSTR) pNodeAttr->attribute.pnodeValue->pstrToken;
                    }
                }
                else if (IsToken(pNodeAttr, pkATTRIBUTE, _T("bubble")))
                {
                    dwFlags |= BEHAVIOREVENTFLAGS_BUBBLE;
                }
                pNodeAttr = pNodeAttr->pnodeNext;
            } // eo while (pNodeAttr)

            if (pchName)
            {
                LONG   idx;

                hr = _aryEvents.AddNameToAtomTable (pchName, &idx);
                if (hr)
                    goto Cleanup;

                if (_aryEventFlags.Size() <= idx)
                {
                    _aryEventFlags.Grow(idx + 1);
                }
                _aryEventFlags[idx] = dwFlags;

            }
        } // eo if (IsToken(pNode, pkELEMENT, _T("event")))

        pNode = pNode->pnodeNext;
    } // eo while (pNode)

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerHandler
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler constructor
//
//-------------------------------------------------------------------------

CPeerHandler::CPeerHandler(IUnknown * pUnkOuter)
{
    _pUnkOuter = pUnkOuter;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler destructor
//
//-------------------------------------------------------------------------

CPeerHandler::~CPeerHandler()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::Passivate
//
//-------------------------------------------------------------------------

void
CPeerHandler::Passivate()
{
    ClearInterface(&_pScript);
    ClearInterface(&_pDispNotification);

    ClearInterface(&_pPeerSite);
    ClearInterface(&_pPeerSiteOM);

    if (_pPeerDisp)
    {
        _pPeerDisp->Disconnect();
        ClearInterface(&_pPeerDisp);
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IScriptletHandler, _pUnkOuter)
    QI_TEAROFF(this, IElementBehavior, _pUnkOuter)
    QI_TEAROFF(this, IElementBehaviorUI, _pUnkOuter)
    QI_TEAROFF(this, IPersistPropertyBag2, _pUnkOuter)
    default:
        return E_NOINTERFACE;
    }

    // if iid.Data1 == Data1_Foo, where Foo is one of interfaces supported above, then *ppv maybe NULL here!
    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::GetNameSpaceObject, per IScriptletHandler
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::GetNameSpaceObject(IUnknown **ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    if (!_pPeerDisp)
    {
        // Keep weak-ref in _pPeerDisp, object life time is controlled by caller.
        _pPeerDisp = new CPeerDispatch(this);
        if (!_pPeerDisp)
            return E_OUTOFMEMORY;
    }

    _pPeerDisp->AddRef();

    *ppUnk = (IUnknown *)_pPeerDisp;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::SetScriptNameSpace, per IScriptletHandler
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::SetScriptNameSpace(IUnknown *punkNameSpace)
{
    RRETURN(punkNameSpace->QueryInterface(IID_IDispatch, (void **)&_pScript));
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::Init, per IElementBehavior
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::Init(IElementBehaviorSite * pPeerSite)
{
    HRESULT hr;

    if (_pPeerSite)     // if already initialized from an IElementBehaviorSite *
        return S_OK;    // nothing more to do

    if (!pPeerSite)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    _pPeerSite = pPeerSite;
    _pPeerSite->AddRef();

    hr = _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&_pPeerSiteOM);
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::Notify, per IElementBehavior
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::Notify(DWORD dwNotification, VARIANT *)
{
    HRESULT     hr = S_OK;

    if (_pDispNotification)
    {
        VARIANT     varArg;
        EXCEPINFO   excepinfo;
        UINT        nArgErr;
        DISPPARAMS  dispparams = {&varArg, NULL, 1, 0};
        LPTSTR      pchNotification = NULL;

        switch (dwNotification)
        {
        case BEHAVIOREVENT_CONTENTCHANGE:
            pchNotification = _T("contentChange");
            break;

        case BEHAVIOREVENT_DOCUMENTREADY:
            pchNotification = _T("documentReady");
            break;

        default:
            pchNotification = _T("[unknown]");
            break;

        }

        V_VT(&varArg) = VT_BSTR;
        V_BSTR(&varArg) = SysAllocString(pchNotification);
        if (!V_BSTR(&varArg))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        IGNORE_HR (_pDispNotification->Invoke (
            0,                      // dispid
            IID_NULL,
            LOCALE_SYSTEM_DEFAULT,
	        DISPATCH_METHOD,
            &dispparams,
            NULL,                   // varRet
            &excepinfo,
            &nArgErr));

        VariantClear (&varArg);
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::OnReceiveFocus, per IElementBehaviorUI
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::OnReceiveFocus(BOOL fFocus, long lSubDivision)
{
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::GetSubDivisionProvider, per IElementBehaviorUI
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerHandler::GetSubDivisionProvider(ISubDivisionProvider **ppProvider)
{
    return E_NOTIMPL;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::CanTakeFocus, per IElementBehaviorUI
//
//-------------------------------------------------------------------------

BOOL
CPeerHandler::CanTakeFocus()
{
    return !!_fCanTakeFocus;
}




//+------------------------------------------------------------------------
//
//  Member:     CPeerHandler::Load, per IPersistPropertyBag2
//
//-------------------------------------------------------------------------

HRESULT
CPeerHandler::Load(IPropertyBag2 *pBag2, IErrorLog *pErrLog)
{
    ULONG           cProp;
    HRESULT         hr = S_OK;
    HRESULT         hr2;
    PROPBAG2 *      ppropbag = NULL;
    ULONG           cPropActual;
    ULONG           i;
    VARIANT *       pVar = NULL;
    DISPID          dispid;
    IDispatchEx *   pDEX = NULL;
    BSTR            bstr = NULL;
    
    if (!_pScript)
        goto Cleanup;

    Assert (_pPeerDisp);

    hr2 = THR_NOTRACE(_pUnkOuter->QueryInterface(IID_IDispatchEx, (void **)&pDEX));
    if (hr2)
        goto Cleanup;
        
    hr = THR(pBag2->CountProperties(&cProp));
    if (hr)
        goto Cleanup;

    if (!cProp)
        goto Cleanup;

    //
    // Now get all the properties.
    //

    ppropbag = new (Mt(CPeerHandlerLoad_ppropbag)) PROPBAG2[cProp];
    if (!ppropbag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR(pBag2->GetPropertyInfo(0, cProp, ppropbag, &cPropActual));
    if (hr)
        goto Cleanup;

    pVar = new (Mt(CPeerHandlerLoad_pVar)) VARIANT[cPropActual];
    if (!pVar)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    memset(pVar, 0, cPropActual * sizeof(VARIANT));
    
    hr = THR(pBag2->Read(cPropActual, ppropbag, NULL, pVar, NULL));
    if (hr)
        goto Cleanup;

    for (i = 0; i < cPropActual; i++)
    {
        //
        // Go case insensitive because these are html attributes.
        //

        FormsFreeString(bstr);
        hr = THR(FormsAllocString(ppropbag[i].pstrName, &bstr));
        if (hr)
            goto Cleanup;

        if (!pDEX->GetDispID(bstr, fdexNameCaseInsensitive, &dispid))
        {
            // We found the name now turn the scoping rules back on again.

            THR_NOTRACE(SetDispProp(
                pDEX,
                dispid,
                LOCALE_SYSTEM_DEFAULT,
                pVar + i,
                NULL,
                DISPATCH_PROPERTYPUT));
        }

        // release string in ppropbag[i] while we are in loop, we don't need it anymore
        CoTaskMemFree(ppropbag[i].pstrName);
    }
    
Cleanup:
    FormsFreeString(bstr);
    ReleaseInterface(pDEX);
    delete [] ppropbag;
    delete [] pVar;
    RRETURN(hr);
}


