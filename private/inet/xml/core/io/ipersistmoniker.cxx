/*
 * @(#)IPersistMoniker.cxx 1.0 11/21/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "core/io/ipersistmoniker.hxx"


HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::GetClassID( 
            /* [out] */ CLSID *pClassID)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pClassID)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    TRY
    {
        *pClassID = getWrapped()->getClassID();
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::IsDirty( void)
{
    // BUGBUG - not implemented (not implemented in old MSXML)
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::Load( 
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pimkName)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    TRY
    {
        getWrapped()->load(fFullyAvailable == TRUE, pimkName, pibc, grfMode);
        hr = getWrapped()->GetLastError();     
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

CleanUp:
    return hr;
}

HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::Save( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::SaveCompleted( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IPersistMonikerWrapper::GetCurMoniker( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!ppimkName)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    TRY
    {
        getWrapped()->getCurMoniker(ppimkName);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

CleanUp:
    return hr;
}
