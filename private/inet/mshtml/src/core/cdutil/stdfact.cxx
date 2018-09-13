//+---------------------------------------------------------------------
//
//  File:       stdfact.cxx
//
//  Contents:   Standard IClassFactory implementation
//
//  Classes:    CClassFactory
//              CStaticCF
//              CDynamicCF
//
//----------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_CLSFAC3_HXX_
#define X_CLSFAC3_HXX_
#include "clsfac3.hxx"
#endif


//+---------------------------------------------------------------
//
//  Member:     CClassFactory::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
CClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CClassFactory::AddRef
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CClassFactory::AddRef()
{
    IncrementSecondaryObjectCount( 4 );
    return 1;
}


//+---------------------------------------------------------------
//
//  Member:     CClassFactory::Release
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CClassFactory::Release(void)
{
    DecrementSecondaryObjectCount( 4 );
    return 1;
}


//+---------------------------------------------------------------
//
//  Member:     CClassFactory::LockServer, public
//
//  Synopsis:   Method of IClassFactory interface.
//
//  Notes:      Since class factories based on this class are global static
//              objects, this method doesn't serve much purpose.
//
//----------------------------------------------------------------

STDMETHODIMP
CClassFactory::LockServer (BOOL fLock)
{
    if (fLock)
        IncrementSecondaryObjectCount(4);
    else
        DecrementSecondaryObjectCount(4);

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CStaticCF::CreateInstance, public
//
//  Synopsis:   Method of IClassFactory interface.
//
//----------------------------------------------------------------

STDMETHODIMP
CStaticCF::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID iid,
    void **ppv)
{
    HRESULT hr;
    IUnknown *pUnk = 0;

    *ppv = NULL;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        goto Cleanup;

    if (pUnkOuter && iid != IID_IUnknown)
        return E_INVALIDARG;

    if (!_pfnCreate)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR((*_pfnCreate)(pUnkOuter, &pUnk));
    if (hr)
        goto Cleanup;

    if (pUnkOuter)
    {
        *ppv = pUnk;
    }
    else 
    {
        hr = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynamicCF::CDynamicCF
//
//  Synopsis:   Constructor.
//
//----------------------------------------------------------------------------

CDynamicCF::CDynamicCF()
{
    _ulRefs = 1;
    IncrementSecondaryObjectCount( 5 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynamicCF::~CDynamicCF
//
//  Synopsis:   Destructor.
//
//----------------------------------------------------------------------------

CDynamicCF::~CDynamicCF()
{
    DecrementSecondaryObjectCount( 5 );
}


//+---------------------------------------------------------------
//
//  Member:     CDynamicCF::AddRef, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDynamicCF::AddRef()
{
    return ++_ulRefs;
}


//+---------------------------------------------------------------
//
//  Member:     CDynamicCF::Release, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDynamicCF::Release()
{
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _ulRefs;
}


BEGIN_TEAROFF_TABLE_(CStaticCF3, IClassFactoryEx)
    TEAROFF_METHOD(CStaticCF3, CreateInstance, createinstance, (IUnknown *punkOuter, REFIID riid, void **ppvObject))
    TEAROFF_METHOD(CStaticCF3, LockServer, lockserver, (BOOL fLock))
    TEAROFF_METHOD(CStaticCF3, CreateInstanceWithContext, createinstancewithcontext, (IUnknown *punkContext, IUnknown *punkOuter, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()


STDMETHODIMP
CStaticCF3::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IClassFactory)
    QI_TEAROFF(this, IClassFactoryEx, NULL)
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *) *ppv)->AddRef();

    return S_OK;
}


STDMETHODIMP
CStaticCF3::CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv)
{
    return CreateInstanceWithContext(NULL, pUnkOuter, iid, ppv);
}

//+---------------------------------------------------------------
//
//  Member:     CStaticCF3::CreateInstanceWithContext, public
//
//  Synopsis:   Method of IClassFactoryEx interface.
//
//----------------------------------------------------------------

STDMETHODIMP
CStaticCF3::CreateInstanceWithContext(
    IUnknown *punkContext,
    IUnknown *pUnkOuter,
    REFIID iid,
    void **ppv)
{
    HRESULT hr;
    IUnknown *pUnk = 0;

    *ppv = NULL;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        goto Cleanup;

    if (pUnkOuter && iid != IID_IUnknown)
        return E_INVALIDARG;

    hr = THR((*_pfnCreate)(punkContext, pUnkOuter, &pUnk));
    if (hr)
        goto Cleanup;

    if (pUnkOuter)
    {
        *ppv = pUnk;
    }
    else 
    {
        hr = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();
    }

Cleanup:
    RRETURN(hr);
}
