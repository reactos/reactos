//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.cxx
//
//  Contents:   IUnknown & IClassFactory for all OLE objects
//
//  History:    13-Dec-95    BruceFo     Created
//
//  Note:  There are three types of IUnknown implementations here. The first
//  is for the "Shared Folders" COM objects. Each of these interfaces can be
//  QueryInterface'd from the others, and all return the same IUnknown. There
//  is a single shared object reference count (not interface reference count).
//  These interfaces include: IShellFolder, IPersistFolder, IRemoteComputer.
//
//  The second type is standard, separate interfaces that get interface-specific
//  reference counts. This includes: IShellDetails, IEnumIDList,
//  IExtractIcon, IExtractIconA.
//
//  The third type is the IUnknown implementation for the "Shared Folders"
//  COM object class factory. This object is a global static object, so it
//  never gets destructed.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <initguid.h>
#include "guids.h"

#include "ole.hxx"
#include "shares.h"
#include "shares.hxx"
#include "sdetails.hxx"
#include "sfolder.hxx"
#include "pfolder.hxx"
#include "rcomp.hxx"
#include "menu.hxx"
#include "menubg.hxx"
#include "enum.hxx"
#include "xicon.hxx"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

ULONG g_ulcInstances = 0;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CShares::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        AddRef();
        *ppvObj = (IUnknown*) this;
        hr = S_OK;
    }
    else if (IsEqualIID(IID_IShellFolder, riid))
    {
        hr = m_ShellFolder.QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(IID_IPersistFolder, riid))
    {
        hr = m_PersistFolder.QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(IID_IRemoteComputer, riid))
    {
        hr = m_RemoteComputer.QueryInterface(riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

STDMETHODIMP_(ULONG)
CShares::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CShares::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesSF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;

    if (IsEqualIID(IID_IShellFolder, riid))
    {
        AddRef();
        *ppvObj = (IShellFolder*) this;
        hr = S_OK;
    }
    else
    {
        CShares* This = IMPL(CShares,m_ShellFolder,this);
        hr = This->QueryInterface(riid, ppvObj);
    }

    return hr;
}

STDMETHODIMP_(ULONG)
CSharesSF::AddRef()
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);
    return This->AddRef();
}

STDMETHODIMP_(ULONG)
CSharesSF::Release()
{
    CShares* This = IMPL(CShares,m_ShellFolder,this);
    return This->Release();
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesPF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;

    if (IsEqualIID(IID_IPersistFolder, riid))
    {
        AddRef();
        *ppvObj = (IPersistFolder*) this;
        hr = S_OK;
    }
    else
    {
        CShares* This = IMPL(CShares,m_PersistFolder,this);
        hr = This->QueryInterface(riid, ppvObj);
    }

    return hr;
}

STDMETHODIMP_(ULONG)
CSharesPF::AddRef()
{
    CShares* This = IMPL(CShares,m_PersistFolder,this);
    return This->AddRef();
}

STDMETHODIMP_(ULONG)
CSharesPF::Release()
{
    CShares* This = IMPL(CShares,m_PersistFolder,this);
    return This->Release();
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesRC::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;

    if (IsEqualIID(IID_IRemoteComputer, riid))
    {
        AddRef();
        *ppvObj = (IRemoteComputer*) this;
        hr = S_OK;
    }
    else
    {
        CShares* This = IMPL(CShares,m_RemoteComputer,this);
        hr = This->QueryInterface(riid, ppvObj);
    }

    return hr;
}

STDMETHODIMP_(ULONG)
CSharesRC::AddRef()
{
    CShares* This = IMPL(CShares,m_RemoteComputer,this);
    return This->AddRef();
}

STDMETHODIMP_(ULONG)
CSharesRC::Release()
{
    CShares* This = IMPL(CShares,m_RemoteComputer,this);
    return This->Release();
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesSD::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IShellDetails*) this;
    }
    else if (IsEqualIID(IID_IShellDetails, riid))
    {
        pUnkTemp = (IUnknown*)(IShellDetails*) this;
    }
    else
    {
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
CSharesSD::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesSD::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesCM::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IContextMenu*) this;
    }
    else if (IsEqualIID(IID_IContextMenu, riid))
    {
        pUnkTemp = (IUnknown*)(IContextMenu*) this;
    }
    else
    {
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
CSharesCM::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesCM::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesCMBG::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IContextMenu*) this;
    }
    else if (IsEqualIID(IID_IContextMenu, riid))
    {
        pUnkTemp = (IUnknown*)(IContextMenu*) this;
    }
    else
    {
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
CSharesCMBG::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesCMBG::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesEnum::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IEnumIDList*) this;
    }
    else if (IsEqualIID(IID_IEnumIDList, riid))
    {
        pUnkTemp = (IUnknown*)(IEnumIDList*) this;
    }
    else
    {
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
CSharesEnum::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesEnum::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesEI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IExtractIcon*) this;
    }
    else if (IsEqualIID(IID_IExtractIcon, riid))
    {
        pUnkTemp = (IUnknown*)(IExtractIcon*) this;
    }
    else
    {
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
CSharesEI::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesEI::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

//////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

STDMETHODIMP
CSharesEIA::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IExtractIconA*) this;
    }
    else if (IsEqualIID(IID_IExtractIcon, riid))
    {
        pUnkTemp = (IUnknown*)(IExtractIconA*) this;
    }
    else
    {
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
CSharesEIA::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    InterlockedIncrement((LONG*)&m_ulRefs);
    return m_ulRefs;
}

STDMETHODIMP_(ULONG)
CSharesEIA::Release()
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == cRef)
    {
        delete this;
    }
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return cRef;
}

#endif // UNICODE

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    IUnknown* pUnkTemp = NULL;
    HRESULT   hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        pUnkTemp = (IUnknown*)(IClassFactory*) this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        pUnkTemp = (IUnknown*)(IClassFactory*) this;
    }
    else
    {
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
CSharesCF::AddRef()
{
    InterlockedIncrement((LONG*)&g_ulcInstances);
    return g_ulcInstances;
}

STDMETHODIMP_(ULONG)
CSharesCF::Release()
{
    InterlockedDecrement((LONG*)&g_ulcInstances);
    return g_ulcInstances;
}

STDMETHODIMP
CSharesCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    if (pUnkOuter != NULL)
    {
        // don't support aggregation
        return E_NOTIMPL;
    }

    CShares* pShare = new CShares();
    if (NULL == pShare)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pShare->QueryInterface(riid, ppvObj);
    pShare->Release();
    return hr;
}

STDMETHODIMP
CSharesCF::LockServer(BOOL fLock)
{
    return S_OK; // BUGBUG: Whats supposed to happen here?
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

STDAPI
DllCanUnloadNow(
    VOID
    )
{
    if (0 == g_ulcInstances
        && 0 == g_NonOLEDLLRefs)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

CSharesCF cfCShares;

STDAPI
DllGetClassObject(
    REFCLSID cid,
    REFIID iid,
    LPVOID* ppvObj
    )
{
    InterlockedIncrement((LONG*)&g_ulcInstances); // don't nuke me now!

    HRESULT hr = E_NOINTERFACE;

    if (IsEqualCLSID(cid, CLSID_CShares))
    {
        hr = cfCShares.QueryInterface(iid, ppvObj);
    }

    InterlockedDecrement((LONG*)&g_ulcInstances);
    return hr;
}
