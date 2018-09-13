/*
 * @(#)IPersistStream.cxx 1.0 11/17/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "core/io/ipersiststream.hxx"


HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::GetClassID( 
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

HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::Load( 
            /* [unique][in] */ IStream *pStm)
{
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pStm)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    TRY
    {
        getWrapped()->Load(pStm);
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

HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::Save( 
            /* [unique][in] */ IStream *pStm,
            /* [in] */ BOOL fClearDirty)
{
    // BUGBUG -- fClearDirty ignored (was ignored in old MSXML).
    STACK_ENTRY_WRAPPED;
    MutexReadLock lock(_pMutex);
    HRESULT hr = S_OK;

    if (!pStm)
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    TRY
    {
        getWrapped()->Save(pStm);
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

HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::IsDirty( void)
{
    // BUGBUG - not implemented (not implemented in old MSXML)
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::GetSizeMax( 
            /* [out] */ ULARGE_INTEGER *pcbSize)
{
    // BUGBUG - not implemented (not implemented in old MSXML)
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IPersistStreamWrapper::InitNew( void) 
{
    return S_OK;
}
