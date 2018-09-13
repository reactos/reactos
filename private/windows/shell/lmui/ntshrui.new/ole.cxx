//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.cxx
//
//  Contents:   Class factory, etc, for all OLE objects:
//              CShare and CShareCopyHook
//
//  History:    6-Apr-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <initguid.h>
#include "guids.h"

#include "ole.hxx"
#include "copyhook.hxx"
#include "share.hxx"
#include "dllmain.hxx"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

ULONG g_ulcInstancesShare = 0;
ULONG g_ulcInstancesShareCopyHook = 0;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CShare::QueryInterface(
    IN REFIID riid,
    OUT LPVOID* ppvObj
    )
{
    appDebugOut((DEB_ITRACE, "CShare::QueryInterface..."));

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
    if (IsEqualIID(IID_IContextMenu, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IContextMenu\n"));
        pUnkTemp = (IContextMenu*) this;
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
CShare::AddRef(
    VOID
    )
{
    InterlockedIncrement((LONG*)&g_ulcInstancesShare);
    InterlockedIncrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CShare::AddRef, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesShare));

    return _uRefs;
}

STDMETHODIMP_(ULONG)
CShare::Release(
    VOID
    )
{
    InterlockedDecrement((LONG*)&g_ulcInstancesShare);
    ULONG cRef = InterlockedDecrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CShare::Release, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesShare));

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CShareCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CShareCF::QueryInterface..."));

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
CShareCF::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstancesShare);

    appDebugOut((DEB_ITRACE,
        "CShareCF::AddRef, DLL: %d\n",
        g_ulcInstancesShare));

    return g_ulcInstancesShare;
}

STDMETHODIMP_(ULONG)
CShareCF::Release()
{
    InterlockedDecrement((LONG*)&g_ulcInstancesShare);

    appDebugOut((DEB_ITRACE,
        "CShareCF::Release, DLL: %d\n",
        g_ulcInstancesShare));

    return g_ulcInstancesShare;
}

STDMETHODIMP
CShareCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CShareCF::CreateInstance\n"));

    if (pUnkOuter != NULL)
    {
        // don't support aggregation
        return E_NOTIMPL;
    }

    CShare* pShare = new CShare();
    if (NULL == pShare)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pShare->QueryInterface(riid, ppvObj);
    pShare->Release();

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
CShareCF::LockServer(BOOL fLock)
{
    //
    // BUGBUG: Whats supposed to happen here?
    //
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CShareCopyHook::QueryInterface(
    IN REFIID riid,
    OUT LPVOID* ppvObj
    )
{
    appDebugOut((DEB_ITRACE, "CShareCopyHook::QueryInterface..."));

    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "IUnknown\n"));
        pUnkTemp = (IUnknown*) this;
    }
    else
    if (IsEqualIID(IID_IShellCopyHook, riid))
    {
        appDebugOut((DEB_ITRACE | DEB_NOCOMPNAME, "ICopyHook\n"));
        pUnkTemp = (ICopyHook*) this;
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
CShareCopyHook::AddRef(
    VOID
    )
{
    InterlockedIncrement((LONG*)&g_ulcInstancesShare);
    InterlockedIncrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CShareCopyHook::AddRef, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesShare));

    return _uRefs;
}

STDMETHODIMP_(ULONG)
CShareCopyHook::Release(
    VOID
    )
{
    InterlockedDecrement((LONG*)&g_ulcInstancesShare);
    ULONG cRef = InterlockedDecrement((LONG*)&_uRefs);

    appDebugOut((DEB_ITRACE,
        "CShareCopyHook::Release, local: %d, DLL: %d\n",
        _uRefs,
        g_ulcInstancesShare));

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CShareCopyHookCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CShareCopyHookCF::QueryInterface..."));

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
CShareCopyHookCF::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstancesShareCopyHook);
    return g_ulcInstancesShareCopyHook;
}

STDMETHODIMP_(ULONG)
CShareCopyHookCF::Release()
{
    InterlockedDecrement((LONG*)&g_ulcInstancesShareCopyHook);
    return g_ulcInstancesShareCopyHook;
}

STDMETHODIMP
CShareCopyHookCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    appDebugOut((DEB_ITRACE, "CShareCopyHookCF::CreateInstance\n"));

    if (pUnkOuter != NULL)
    {
        // don't support aggregation
        return E_NOTIMPL;
    }

    CShareCopyHook* pShareCopyHook = new CShareCopyHook();
    if (NULL == pShareCopyHook)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pShareCopyHook->QueryInterface(riid, ppvObj);
    pShareCopyHook->Release();

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
CShareCopyHookCF::LockServer(BOOL fLock)
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
    OneTimeInit();

    if (0 == g_ulcInstancesShare
        && 0 == g_ulcInstancesShareCopyHook
        && 0 == g_NonOLEDLLRefs)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

CShareCF cfShare;
CShareCopyHookCF cfShareCopyHook;

STDAPI
DllGetClassObject(
    REFCLSID cid,
    REFIID iid,
    LPVOID* ppvObj
    )
{
    OneTimeInit();

    appDebugOut((DEB_TRACE, "DllGetClassObject\n"));

    HRESULT hr = E_NOINTERFACE;

    if (IsEqualCLSID(cid, CLSID_CShare))
    {
        hr = cfShare.QueryInterface(iid, ppvObj);
    }
    else if (IsEqualCLSID(cid, CLSID_CShareCopyHook))
    {
        hr = cfShareCopyHook.QueryInterface(iid, ppvObj);
    }

    return hr;
}
