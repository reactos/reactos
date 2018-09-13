//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padconst.cxx
//
//  Contents:   CPadScriptSite
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

IMPLEMENT_SUBOBJECT_IUNKNOWN(CPadScriptConst, CPadDoc, PadDoc, _ScriptConst);

static HRESULT
LookupSymbol(ITypeComp *pTypeComp, TCHAR *pch, long *pl)
{
    HRESULT     hr;
    ITypeInfo * pTypeInfo = NULL;
    DESCKIND    desckind = DESCKIND_NONE;
    BINDPTR     bindptr;

    hr = THR(pTypeComp->Bind(
            pch, 
            0, 
            INVOKE_PROPERTYGET, 
            &pTypeInfo,
            &desckind,
            &bindptr));
    if (hr)
        goto Cleanup;

    if (desckind != DESCKIND_VARDESC || 
        bindptr.lpvardesc->varkind != VAR_CONST ||
        bindptr.lpvardesc->lpvarValue->vt != VT_I4)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    *pl = V_I4(bindptr.lpvardesc->lpvarValue);

Cleanup:
    switch (desckind)
    {
    case DESCKIND_FUNCDESC:
        pTypeInfo->ReleaseFuncDesc(bindptr.lpfuncdesc);
        break;

    case DESCKIND_VARDESC:
        pTypeInfo->ReleaseVarDesc(bindptr.lpvardesc);
        break;

    case DESCKIND_TYPECOMP:
        ReleaseInterface(bindptr.lptcomp);
        break;

    case DESCKIND_IMPLICITAPPOBJ:
        Assert(0 && "What is this?");
        break;
    }

    ReleaseInterface(pTypeInfo);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
// 
//  Member: CPadScriptConst::QueryInterface
//
//---------------------------------------------------------------------------

STDMETHODIMP
CPadScriptConst::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IDispatch ||
        iid == IID_IUnknown)
    {
        *ppv = (IDispatch*)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
// 
//  Member: CPadScriptConst::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptConst::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    Assert(FALSE);
    return E_FAIL;
}

//---------------------------------------------------------------------------
// 
//  Member: CPadScriptConst::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptConst::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 0;
    return S_OK;
}

//---------------------------------------------------------------------------
// 
//  Member: CPadScriptConst::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptConst::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    HRESULT     hr;
    int         i;

    hr = THR(PadDoc()->LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    for (i = 0; i < ARRAY_SIZE(PadDoc()->_apTypeComp); i++)
    {
        hr = THR_NOTRACE(LookupSymbol(PadDoc()->_apTypeComp[i], rgszNames[0], rgdispid));
        if (!hr)
            break;
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
// 
//  Member: CPadScriptConst::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptConst::Invoke(DISPID dispidMember, 
                        REFIID riid, 
                        LCID lcid, 
                        WORD wFlags,
                        DISPPARAMS * pdispparams, 
                        VARIANT * pvarResult,
                        EXCEPINFO * pexcepinfo, 
                        UINT * puArgErr)
{
    HRESULT hr = S_OK;
    
    if(!(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    pvarResult->vt = VT_I4;
    pvarResult->lVal = dispidMember;

    RRETURN(hr);
}

