//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       misc.cxx
//
//  Contents:   misc helpers for scriptoid handlers
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

//+------------------------------------------------------------------------
//
//  Function:     GetProperty_Dispatch
//
//-------------------------------------------------------------------------

HRESULT
GetProperty_Dispatch(IDispatch * pDisp, LPTSTR pchName, IDispatch ** ppDispRes)
{
    HRESULT     hr;
    DISPID      dispid;
    DISPPARAMS  dispparams = {NULL, NULL, 0, 0};
    VARIANT     varRes;
    EXCEPINFO   excepinfo;
    UINT        nArgErr;

    *ppDispRes = NULL;

    hr = pDisp->GetIDsOfNames(IID_NULL, &pchName, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (hr)
        goto Cleanup;

    hr = pDisp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_SYSTEM_DEFAULT,
	    DISPATCH_PROPERTYGET,
        &dispparams,
        &varRes,
        &excepinfo,
        &nArgErr);
    if (hr)
        goto Cleanup;

    if (VT_DISPATCH != V_VT(&varRes))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    (*ppDispRes) = V_DISPATCH(&varRes);

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:     PutProperty_Dispatch
//
//-------------------------------------------------------------------------

HRESULT
PutProperty_Dispatch(IDispatch * pDisp, LPTSTR pchName, IDispatch * pDispArg)
{
    HRESULT     hr;
    DISPID      dispid;
    VARIANT     varArg;
    VARIANT     varRes;
    EXCEPINFO   excepinfo;
    UINT        nArgErr;
    DISPPARAMS  dispparams = {&varArg, NULL, 1, 0};

    VariantInit(&varArg);
    V_VT(&varArg) = VT_DISPATCH;
    V_DISPATCH(&varArg) = pDispArg;

    hr = pDisp->GetIDsOfNames(IID_NULL, &pchName, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (hr)
        goto Cleanup;

    hr = pDisp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_SYSTEM_DEFAULT,
	    DISPATCH_PROPERTYPUT,
        &dispparams,
        &varRes,
        &excepinfo,
        &nArgErr);

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:     GetDocument
//
//-------------------------------------------------------------------------

HRESULT
GetDocument(IDispatch * pElement, IDispatch ** ppDispDocument)
{
    return GetProperty_Dispatch(pElement, _T("document"), ppDispDocument);
}

//+------------------------------------------------------------------------
//
//  Function:     GetWindow
//
//-------------------------------------------------------------------------

HRESULT
GetWindow(IDispatch * pElement, IDispatchEx ** ppDispWindow)
{
    HRESULT     hr;
    IDispatch * pDispDocument = NULL;
    IDispatch * pDispWindow = NULL;

    *ppDispWindow = NULL;

    hr = GetDocument(pElement, &pDispDocument);
    if (hr)
        goto Cleanup;

    hr = GetProperty_Dispatch (pDispDocument, _T("parentWindow"), &pDispWindow);
    if (hr)
        goto Cleanup;

    hr = pDispWindow->QueryInterface (IID_IDispatchEx, (void**) ppDispWindow);

Cleanup:
    ReleaseInterface(pDispDocument);
    ReleaseInterface(pDispWindow);

    return S_OK;
}
