//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.cxx
//
//  Contents:   Class factory, etc, for all OLE objects
//
//  History:    25-Sep-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <initguid.h>
#include "guids.h"

#include "ole.hxx"
#include "ext.hxx"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

ULONG g_ulcInstancesNetObj = 0;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CNetObj::QueryInterface(
    IN REFIID riid,
    OUT LPVOID* ppvObj
    )
{
    appDebugOut((DEB_ITRACE, "CNetObj::QueryInterface..."));

    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IUnknown\n"));
        pUnkTemp = (IUnknown*)(IShellExtInit*) this;    // doesn't matter which
    }
    else
    if (IsEqualIID(IID_IShellExtInit, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IShellExtInit\n"));
        pUnkTemp = (IShellExtInit*) this;
    }
    else
    if (IsEqualIID(IID_IShellPropSheetExt, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IShellPropSheetExt\n"));
        pUnkTemp = (IShellPropSheetExt*) this;
    }
    else
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "unknown interface\n"));
        hr = E_NOINTERFACE;
    }

    if (pUnkTemp != NULL)
    {
        pUnkTemp->AddRef();
    }

    *ppvObj = pUnkTemp;

    return hr;
}

STDMETHODIMP_(ULONG)
CNetObj::AddRef(
    VOID
    )
{
    InterlockedIncrement((LONG*)&g_ulcInstancesNetObj);
    InterlockedIncrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CNetObj::AddRef, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesNetObj));

    return _uRefs;
}

STDMETHODIMP_(ULONG)
CNetObj::Release(
    VOID
    )
{
    InterlockedDecrement((LONG*)&g_ulcInstancesNetObj);
    ULONG cRef = InterlockedDecrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CNetObj::Release, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesNetObj));

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CNetObjCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CNetObjCF::QueryInterface..."));

    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IUnknown\n"));
        pUnkTemp = (IUnknown*) this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IClassFactory\n"));
        pUnkTemp = (IClassFactory*) this;
    }
    else
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "unknown interface\n"));
        hr = E_NOINTERFACE;
    }

    if (pUnkTemp != NULL)
    {
        pUnkTemp->AddRef();
    }

    *ppvObj = pUnkTemp;

    return hr;
}


STDMETHODIMP_(ULONG)
CNetObjCF::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstancesNetObj);

    appDebugOut((DEB_ITRACE,
        "CNetObjCF::AddRef, DLL: %d\n",
        g_ulcInstancesNetObj));

    return g_ulcInstancesNetObj;
}

STDMETHODIMP_(ULONG)
CNetObjCF::Release()
{
    InterlockedDecrement((LONG*)&g_ulcInstancesNetObj);

    appDebugOut((DEB_ITRACE,
        "CNetObjCF::Release, DLL: %d\n",
        g_ulcInstancesNetObj));

    return g_ulcInstancesNetObj;
}

STDMETHODIMP
CNetObjCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CNetObjCF::CreateInstance\n"));

    if (pUnkOuter != NULL)
    {
        // don't support aggregation
        return E_NOTIMPL;
    }

    CNetObj* pNetObj = new CNetObj();
    if (NULL == pNetObj)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pNetObj->QueryInterface(riid, ppvObj);
    pNetObj->Release();

    if (FAILED(hr))
    {
        hr = E_NOINTERFACE; // BUGBUG: Whats the error code?
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP
CNetObjCF::LockServer(BOOL fLock)
{
    //
    // BUGBUG: Whats supposed to happen here?
    //
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

STDAPI
DllCanUnloadNow(
    VOID
    )
{
    if (0 == g_ulcInstancesNetObj
        && 0 == g_NonOLEDLLRefs
        )
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

CNetObjCF cfNetObj;

STDAPI
DllGetClassObject(
    REFCLSID cid,
    REFIID iid,
    LPVOID* ppvObj
    )
{
    appDebugOut((DEB_TRACE, "DllGetClassObject\n"));

    HRESULT hr = E_NOINTERFACE;

    if (IsEqualCLSID(cid, CLSID_CNetObj))
    {
        hr = cfNetObj.QueryInterface(iid, ppvObj);
    }

    return hr;
}
