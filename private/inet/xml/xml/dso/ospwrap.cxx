/*
 * @(#)ospwrap.cxx 1.0 8/11/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "ospwrap.hxx"


// We override the _comexport implementation so that QI(IID_IUnknown) returns
// a pointer to the same wrapper, rather than delegating to getWrapped() and
// creating a new one.  This is so that calls to GetVariant on a rowset-valued
// cell will return the same answer.

HRESULT STDMETHODCALLTYPE
OSPWrapper::QueryInterface(REFIID iid, void ** ppvObject)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    // null out ptr in case caller has not done so
    if (ppvObject != null)
    {
        *ppvObject = null;
    }
    else
    {
        // no one should ever pass us a null ptr-to-ptr
        Assert(FALSE);
        return E_POINTER;
    }

    if (iid == IID_IUnknown || iid == IID_OLEDBSimpleProvider)
    {
        *ppvObject = this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = super::QueryInterface(iid, ppvObject);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////
// OLEDBSimpleProvider implementation.
// All these functions merely check that the arguments are valid, delegate
// the real work to the wrapped object, catch any errors, and return.


HRESULT STDMETHODCALLTYPE
OSPWrapper::getRowCount( 
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRows)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pcRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *pcRows = getWrapped()->getRowCount();
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
OSPWrapper::getColumnCount( 
    /* [retval][out] */ DB_LORDINAL __RPC_FAR *pcColumns)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pcColumns)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *pcColumns = getWrapped()->getColumnCount();
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
OSPWrapper::getRWStatus( 
    /* [in] */ DBROWCOUNT iRow,
    /* [in] */ DB_LORDINAL iColumn,
    /* [retval][out] */ OSPRW __RPC_FAR *prwStatus)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!prwStatus)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *prwStatus = getWrapped()->getRWStatus((LONG) iRow, (LONG) iColumn);
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
OSPWrapper::getVariant( 
    /* [in] */ DBROWCOUNT iRow,
    /* [in] */ DB_LORDINAL iColumn,
    /* [in] */ OSPFORMAT format,
    /* [retval][out] */ VARIANT __RPC_FAR *pVar)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pVar)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        getWrapped()->getVariant((LONG) iRow, (LONG) iColumn, format, pVar);
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
OSPWrapper::setVariant( 
    /* [in] */ DBROWCOUNT iRow,
    /* [in] */ DB_LORDINAL iColumn,
    /* [in] */ OSPFORMAT format,
    /* [in] */ VARIANT var)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setVariant((LONG) iRow, (LONG) iColumn, format, var);
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
OSPWrapper::getLocale( 
    /* [retval][out] */ BSTR __RPC_FAR *pbstrLocale)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
OSPWrapper::deleteRows( 
    /* [in] */ DBROWCOUNT iRow,
    /* [in] */ DBROWCOUNT cRows,
    /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsDeleted)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pcRowsDeleted)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *pcRowsDeleted = getWrapped()->deleteRows((LONG) iRow, (LONG) cRows);
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
OSPWrapper::insertRows( 
    /* [in] */ DBROWCOUNT iRow,
    /* [in] */ DBROWCOUNT cRows,
    /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsInserted)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pcRowsInserted)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *pcRowsInserted = getWrapped()->insertRows((LONG) iRow, (LONG) cRows);
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
OSPWrapper::find( 
    /* [in] */ DBROWCOUNT iRowStart,
    /* [in] */ DB_LORDINAL iColumn,
    /* [in] */ VARIANT val,
    /* [in] */ OSPFIND findFlags,
    /* [in] */ OSPCOMP compType,
    /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRowFound)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE
OSPWrapper::addOLEDBSimpleProviderListener( 
    /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;
    
    TRY
    {
        getWrapped()->addOLEDBSimpleProviderListener(pospIListener);
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
OSPWrapper::removeOLEDBSimpleProviderListener( 
    /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;
    
    TRY
    {
        getWrapped()->removeOLEDBSimpleProviderListener(pospIListener);
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
OSPWrapper::isAsync( 
    /* [retval][out] */ BOOL __RPC_FAR *pbAsynch)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pbAsynch)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *pbAsynch = getWrapped()->isAsync();
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
OSPWrapper::getEstimatedRows( 
    /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRows)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!piRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    TRY
    {
        *piRows = getWrapped()->getEstimatedRows();
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
OSPWrapper::stopTransfer( void)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;
    
    TRY
    {
        getWrapped()->stopTransfer();
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    return hr;
}

