//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       svrom.cxx
//
//  History:    19-May-1998     AnandRa     Created
//
//  Contents:   Server object model implementation
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

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_SCROID_H_
#define X_SCROID_H_
#include "scroid.h"         // For SID_ScriptletDispatch
#endif

#ifndef X_HANDLER_H_
#define X_HANDLER_H_
#include "handler.h"
#endif

#ifndef X_SVROM_HXX_
#define X_SVROM_HXX_
#include "svrom.hxx"
#endif

MtDefine(CSvrOM, Scriptlet, "CSvrOM")


///////////////////////////////////////////////////////////////////////////
//
// tearoff tables
//
///////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE(CSvrOM, IScriptletHandler)
	TEAROFF_METHOD(CSvrOM, GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk))
	TEAROFF_METHOD(CSvrOM, SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CSvrOM, IScriptletHandlerConstructor)
    TEAROFF_METHOD(CSvrOM, Load, load, (WORD wFlags, IScriptletXML *pxmlElement))
	TEAROFF_METHOD(CSvrOM, Create, create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler))
	TEAROFF_METHOD(CSvrOM, Register, register, (LPCOLESTR pstrPath))
	TEAROFF_METHOD(CSvrOM, Unregister, unregister, ())
	TEAROFF_METHOD(CSvrOM, AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass))
END_TEAROFF_TABLE()


///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

const CBase::CLASSDESC CSvrOM::s_classdesc =
{
    &CLSID_CSvrOMUses,              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+------------------------------------------------------------------------
//
//  Function:   CreateSvrOMUses
//
//  Synopsis:   Creates a new instance of scriptoid server handler
//
//-------------------------------------------------------------------------

HRESULT
CreateSvrOMUses(IUnknown *pUnkContext, IUnknown * pUnkOuter, IUnknown ** ppUnk)
{
    HRESULT     hr = S_OK;
    CSvrOM  *   pSvrOM;

    *ppUnk = NULL;

    pSvrOM = new CSvrOM(pUnkContext, pUnkOuter);
    if (!pSvrOM)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppUnk) = (IUnknown*)pSvrOM;

Cleanup:    
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOM constructor
//
//-------------------------------------------------------------------------

CSvrOM::CSvrOM(IUnknown *pUnkContext, IUnknown * pUnkOuter) :
  _pContext(pUnkContext),
  _pUnkOuter(pUnkOuter)
{
    IServiceProvider *  pSP = NULL;

    if (_pContext)
    {
        if ((!_pContext->QueryInterface(IID_IServiceProvider, (void **)&pSP)) &&
            pSP)
        {
            pSP->QueryService(SID_SServerOM, IID_IDispatch, (void **)&_pDispSvr);
        }
    }
    ReleaseInterface(pSP);
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOM ::Passivate
//
//-------------------------------------------------------------------------

void
CSvrOM::Passivate()
{
    ClearInterface(&_pDispSvr);
    super::Passivate();
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOM ::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOM ::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IScriptletHandler, _pUnkOuter)
    QI_TEAROFF(this, IScriptletHandlerConstructor, _pUnkOuter)
    default:
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOM ::GetNameSpaceObject, per IScriptletHandler
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOM::GetNameSpaceObject(IUnknown **ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    return _SvrOMDisp.QueryInterface(IID_IUnknown, (void **)ppUnk);
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOM::Create, per IScriptletHandlerConstructor
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOM::Create(IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler)
{
    RRETURN(CreateSvrOMUses(punkContext, punkOuter, ppunkHandler));
}


IMPLEMENT_SUBOBJECT_IUNKNOWN(CSvrOMDisp, CSvrOM, SvrOM, _SvrOMDisp);

//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOMDisp::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || 
        riid==IID_IDispatch || 
        riid == IID_IDispatchEx)
    {
        *ppv = (IDispatchEx *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::GetIDsOfNames, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOMDisp::GetIDsOfNames(
    REFIID      /*riid*/,
    LPOLESTR *  rgszNames,
    UINT        cNames,
    LCID        /*lcid*/,
    DISPID *    pdispid)
{
    HRESULT hr;
    BSTR    bstrName;
    
    if (1 != cNames)
        return E_UNEXPECTED;

    if (!rgszNames || !pdispid)
        return E_POINTER;

    bstrName = SysAllocString(rgszNames[0]);
    if (bstrName)
    {
        hr = GetDispID(bstrName, 0, pdispid);
        SysFreeString(bstrName);
    }
    else 
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
    
//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOMDisp::Invoke(
    DISPID          dispid,
    REFIID          /*riid*/,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          /* puArgErr */)
{
    return InvokeEx(dispid, lcid, wFlags, pDispParams, pvarResult, pExcepInfo, NULL);
}


//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::GetDispID, helper
//
//-------------------------------------------------------------------------

HRESULT
CSvrOMDisp::GetDispID(
    BSTR        bstrName,
    DWORD       grfdex,
    DISPID *    pdispid)
{
    STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    LPTSTR *    ppchName;

    if (!pdispid)
        return E_POINTER;

    for (ppchName = GetNamesTable(); *ppchName; ppchName++)
    {
        if (0 == pfnStrCmp (*ppchName, bstrName))
        {
            *pdispid = ppchName - GetNamesTable();
            (*pdispid) += DISPID_SVROM_FIRST;
            return S_OK;
        }
    }

    if (SvrOM()->_pDispSvr)
    {
        RRETURN_NOTRACE(SvrOM()->_pDispSvr->GetIDsOfNames(
                IID_NULL,
                &bstrName,
                1,
                g_lcidUserDefault,
                pdispid));
    }
    
    *pdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::Invoke, helper
//
//-------------------------------------------------------------------------

HRESULT
CSvrOMDisp::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceCaller)
{
    HRESULT         hr = S_OK;
    TCHAR *         pch = NULL;
    
    switch (dispid)
    {
    case DISPID_SVROM_BEGINLOG:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (pDispParams->cArgs == 1 && VT_BSTR == V_VT(&pDispParams->rgvarg[0]))
        {
            pch = V_BSTR(&pDispParams->rgvarg[0]);
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (pvarRes)
        {
            V_VT(pvarRes) = VT_EMPTY;
        }

        CloseHandle(SvrOM()->_hFileLog);
        SvrOM()->_hFileLog = CreateFile(
            pch,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (SvrOM()->_hFileLog == INVALID_HANDLE_VALUE)
        {
            SvrOM()->_hFileLog = NULL;
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        SetFilePointer(SvrOM()->_hFileLog, GetFileSize(SvrOM()->_hFileLog, 0), 0, 0);
        break;
        
    case DISPID_SVROM_ENDLOG:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        
        if (pDispParams->cArgs) // no arguments expected
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        if (pvarRes)
        {
            V_VT(pvarRes) = VT_EMPTY;
        }

        CloseHandle(SvrOM()->_hFileLog);
        SvrOM()->_hFileLog = NULL;
        break;
        
    case DISPID_SVROM_TRACELOG:
        if (!(wFlags & DISPATCH_METHOD) || !SvrOM()->_hFileLog)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (pDispParams->cArgs == 1 && VT_BSTR == V_VT(&pDispParams->rgvarg[0]))
        {
            pch = V_BSTR(&pDispParams->rgvarg[0]);
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        if (pvarRes)
        {
            V_VT(pvarRes) = VT_EMPTY;
        }

        {
            char    buffer [1024];
            long    cchLen;
            DWORD   nbw;

            cchLen = WideCharToMultiByte(
                CP_ACP, 0, pch, _tcslen(pch),
                buffer, ARRAY_SIZE(buffer), NULL, NULL );

            WriteFile(SvrOM()->_hFileLog, buffer, cchLen, &nbw, NULL);
        }
        break;
        
    case DISPID_SVROM_ISAVAILABLE:

        //
        // Is the interface(s) all available from the host?
        //

        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (pDispParams->cArgs) // no arguments expected
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        if (!pvarRes)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = SvrOM()->_pDispSvr ? VB_TRUE : VB_FALSE;
        break;

    default:
        if (SvrOM()->_pDispSvr)
        {
            hr = THR(SvrOM()->_pDispSvr->Invoke(
                    dispid,
                    IID_NULL,
                    lcid,
                    wFlags,
                    pDispParams,
                    pvarRes,
                    pExcepInfo,
                    NULL));
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        break;
    }

Cleanup:
    return hr;
}


        
//+------------------------------------------------------------------------
//
//  Member:     CSvrOMDisp::GetMemberName, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSvrOMDisp::GetMemberName(DISPID dispid, BSTR * pbstrName)
{
    if (!pbstrName)
        return E_POINTER;
        
    if (dispid < 0 || DISPID_COUNT <= dispid)
        return DISP_E_MEMBERNOTFOUND;

    *pbstrName = SysAllocString(GetNamesTable()[dispid]);
    return (*pbstrName) ? S_OK : E_OUTOFMEMORY;
}
        
        
