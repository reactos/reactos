/*----------------------------------------------------------------------------
/ Title;
/   unknown.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Helper functions for handling IUnknown
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "cfdefs.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ CUnknown
/   Helper functions to aid the implementation of IUnknown within objects,
/   handles not only AddRef and Release, but also QueryInterface.
/----------------------------------------------------------------------------*/

LONG g_cRefCount = 0;          // global reference count

CUnknown::CUnknown() :
    m_cRefCount(1)
{
    InterlockedIncrement(&g_cRefCount);
}

CUnknown::~CUnknown()
{
    InterlockedDecrement(&g_cRefCount);
}

STDMETHODIMP CUnknown::HandleQueryInterface(REFIID riid, LPVOID* ppvObject, LPINTERFACES aInterfaces, int cif)
{
    HRESULT hr = S_OK;
    int i;

    TraceAssert(ppvObject);
    TraceAssert(aInterfaces);
    TraceAssert(cif);

    if ( !ppvObject )
        ExitGracefully(hr, E_INVALIDARG, "Bailing because ppvObject == NULL");

    *ppvObject = NULL;                                              // no interface yet

    for ( i = 0; i != cif; i++ )
    {
        if ( IsEqualIID(riid, *aInterfaces[i].piid) || IsEqualIID(riid, IID_IUnknown) )
        {
            *ppvObject = aInterfaces[i].pvObject;
            goto exit_gracefully;
        }
    }

    hr = E_NOINTERFACE;         // failed.

exit_gracefully:

    if ( SUCCEEDED(hr) )
        ((LPUNKNOWN)*ppvObject)->AddRef();

    return hr;
}

STDMETHODIMP_(ULONG) CUnknown::HandleAddRef()
{
    return InterlockedIncrement(&m_cRefCount);

}

STDMETHODIMP_(ULONG) CUnknown::HandleRelease()
{
    ULONG cRefCount; 

    cRefCount = InterlockedDecrement(&m_cRefCount);
        
    if ( cRefCount )
        return cRefCount;

    delete this;    
    return 0;
}


/*-----------------------------------------------------------------------------
/ Static class factory - from browseui
/----------------------------------------------------------------------------*/

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        InterlockedIncrement(&g_cRefCount);
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    InterlockedIncrement(&g_cRefCount);
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    InterlockedDecrement(&g_cRefCount);
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;

        if ( punkOuter )
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cRefCount);
    else
        InterlockedDecrement(&g_cRefCount);

    return S_OK;
}