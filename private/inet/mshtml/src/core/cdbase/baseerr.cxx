//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       baseerr.cxx
//
//  Contents:   CBase error utilties implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfo
//
//  Synopsis:
//
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfo(HRESULT hr)
{
    PreSetErrorInfo();

    if (FAILED(hr))
    {
        ClearErrorInfo();
        CloseErrorInfo(hr);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfoPGet
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfoPGet(HRESULT hr, DISPID dispid)
{
    // No PreSetErrorInfo call needed on read-only operations.

    if (FAILED(hr))
    {
        ClearErrorInfo();
        CloseErrorInfoPGet(hr, dispid);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfoPSet
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfoPSet(HRESULT hr, DISPID dispid)
{
    PreSetErrorInfo();

    if (FAILED(hr))
    {
        if (hr == E_INVALIDARG)
        {
            hr = CTL_E_INVALIDPROPERTYVALUE;
        }
        ClearErrorInfo();
        CloseErrorInfoPSet(hr, dispid);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBase::CloseErrorInfo
//
//  Synopsis:
//
//
//----------------------------------------------------------------------------

HRESULT
CBase::CloseErrorInfo(HRESULT hr)
{
    if (FAILED(hr))
    {
        Assert(BaseDesc()->_pclsid );
        ::CloseErrorInfo(hr, (BaseDesc()->_pclsid ? *BaseDesc()->_pclsid : 
                                                    CLSID_NULL));
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBase::CloseErrorInfo
//
//  Specific method for automation calls
//
//----------------------------------------------------------------------------

HRESULT
CBase::CloseErrorInfo(HRESULT hr, DISPID dispid, INVOKEKIND invkind)
{
    CErrorInfo *pEI;

    if (OK(hr))
        return hr;

    if ((pEI = GetErrorInfo()) != NULL)
    {
        pEI->_invkind = invkind;
        pEI->_dispidInvoke = dispid;
        if ( BaseDesc()->_piidDispinterface )
        {
            pEI -> _iidInvoke = *BaseDesc()->_piidDispinterface;
        }
        else
        {
            // BUGBUG rgardner If we don't have an IID what do we
            // do????
            return S_FALSE;
        }
    }

    return CloseErrorInfo(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfo
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfo(HRESULT hr, DISPID dispid, INVOKEKIND invkind, UINT ids, ...)
{
    PreSetErrorInfo();

    va_list arg;
    CErrorInfo *pEI;

    ClearErrorInfo();

    if(ids && (pEI = GetErrorInfo()) != NULL)
    {
        va_start(arg, ids);
        pEI->SetTextV(EPART_SOLUTION, ids, &arg);
        va_end(arg);
    }

    return CloseErrorInfo(hr, dispid, invkind);
}

//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfoBadValue
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfoBadValue(DISPID dispid, UINT ids, ...)
{
    PreSetErrorInfo();

    va_list arg;
    CErrorInfo *pEI;

    ClearErrorInfo();

    if (ids && (pEI = GetErrorInfo()) != NULL)
    {
        va_start(arg, ids);
        pEI->SetTextV(EPART_SOLUTION, ids, &arg);
        va_end(arg);
    }

    return CloseErrorInfoPSet(E_INVALIDARG, dispid);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfoPBadValue
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfoPBadValue(DISPID dispid, UINT ids, ...)
{
    PreSetErrorInfo();

    va_list arg;
    CErrorInfo *pEI;

    ClearErrorInfo();

    if (ids && (pEI = GetErrorInfo()) != NULL)
    {
        va_start(arg, ids);
        pEI->SetTextV(EPART_SOLUTION, ids, &arg);
        va_end(arg);
    }

    return CloseErrorInfoPSet(CTL_E_INVALIDPROPERTYVALUE, dispid);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBase::SetErrorInfoInvalidArg
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetErrorInfoInvalidArg()
{
    PreSetErrorInfo();

    ClearErrorInfo();
    return CloseErrorInfo(E_INVALIDARG);
}
