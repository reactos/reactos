/*
 * @(#)datasrc.cxx 1.0 8/13/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "datasrc.hxx"

const IID IID_DataSource = 
{ 0x7c0ffab3,0xcd84, 0x11d0, {0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed}
};


///////////////////////////////////////////////////////////////////
// DataSource implementation.
// All these functions merely check that the arguments are valid, delegate
// the real work to the wrapped object, catch any errors, and return.


HRESULT STDMETHODCALLTYPE
DataSourceWrapper::getDataMember( 
        /* [in] */ DataMember bstrDM,
        /* [in] */ REFIID riid,
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!ppunk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *ppunk = getWrapped()->getDataMember(bstrDM, riid);
    }
    CATCH
    {
        *ppunk = NULL;
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE
DataSourceWrapper::getDataMemberName( 
        /* [in] */ long lIndex,
        /* [retval][out] */ DataMember __RPC_FAR *pbstrDM)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
DataSourceWrapper::getDataMemberCount( 
        /* [retval][out] */ long __RPC_FAR *plCount)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
DataSourceWrapper::addDataSourceListener( 
        /* [in] */ DataSourceListener __RPC_FAR *pDSL)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;
    
    TRY
    {
        getWrapped()->addDataSourceListener(pDSL);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE
DataSourceWrapper::removeDataSourceListener( 
        /* [in] */ DataSourceListener __RPC_FAR *pDSL)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;
    
    TRY
    {
        getWrapped()->removeDataSourceListener(pDSL);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    return hr;
}

