/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "_gitpointer.hxx"

__gitpointer::__gitpointer(const IID * riid) 
: _riid(riid)
{
#ifdef USE_COMARSHAL_FOR_GIT
    _hGlobal = NULL;
    _fAddRefOnMainThread = FALSE;
#else
    HRESULT hr;
    
    _pGIT = NULL;
    _dwCookie = 0;
#endif
    _ptr = NULL;
    _dwMainThread = 0;
    
#ifndef USE_COMARSHAL_FOR_GIT
    hr = CoCreateInstance(
            CLSID_StdGlobalInterfaceTable,
            0,
            CLSCTX_INPROC_SERVER, 
            IID_IGlobalInterfaceTable, 
            (void**)&_pGIT);
            
    if (FAILED(hr))
        Exception::throwE(hr);
#endif
}

__gitpointer::~__gitpointer()
{
    reset();
#ifdef USE_COMARSHAL_FOR_GIT
#else
    if (_pGIT)
        _pGIT->Release();
#endif
}


void __gitpointer::reset()
{
#ifdef USE_COMARSHAL_FOR_GIT

    if (!_ptr)
        return;

    // this release matches the addref we did in _setPointer.
    if (_fAddRefOnMainThread)
    {
        // BUGBUG If we're not on the thread that _ptr came from, we can't
        // call Release.  So we just leak it.  Better than crashing.  (sambent)
        
        // We (sambent + istvanc) believe this will only arise in a multi-
        // threaded context (i.e. server) while using the CoMarshal implementation
        // (i.e. Unix) talking to a thunking object (i.e. CPeerHolder::CPeerSite).
        // We (sambent + a-rogerw) believe there are no such scenarios.  And if
        // there are, the right fix is to get the Unix folks to port
        // IGlobalInterfaceTable, and use the GIT implementation (just change
        // the ifdef at the top of _gitpointer.hxx).
        
        if (_dwMainThread == GetCurrentThreadId())
        {
            _ptr->Release();
        }
    }

    if (_hGlobal)
    {
        HRESULT hr;
        IStream * pStm = NULL;
        
        hr = CreateStreamOnHGlobal(_hGlobal, FALSE, &pStm);

        if (SUCCEEDED(hr))
        {
            CoReleaseMarshalData(pStm);

            pStm->Release();
        }

        _hGlobal = NULL;
    }
#else
    if (_pGIT && _dwCookie)
    {
        _pGIT->RevokeInterfaceFromGlobal(_dwCookie);
        _dwCookie = 0;
    }
#endif

    _ptr = NULL;
}

void __gitpointer::_assign(IUnknown* ptr)
{
    HRESULT hr = _setPointer(ptr);
    if (FAILED(hr)) 
        Exception::throwE(hr);
}

HRESULT __gitpointer::_setPointer(IUnknown* ptr)
{
    HRESULT hr = S_OK;

    // null out the current pointer
    reset();

    // replace it with the new one
    if (ptr != NULL)
    {
        _ptr = ptr;
        _dwMainThread = GetCurrentThreadId();
        
#ifdef USE_COMARSHAL_FOR_GIT
        IStream * pStm = NULL;

        _fAddRefOnMainThread = FALSE;

        // Create a stream
        hr = CreateStreamOnHGlobal(NULL, FALSE, &pStm);

        if (!hr)
        {
            hr = CoMarshalInterface(
                        pStm,
                        *_riid,
                        ptr,
                        MSHCTX_INPROC,
                        NULL,
                        MSHLFLAGS_TABLESTRONG);

            if (!hr)
            {
                // CoMarshal does ptr->QI(IUnknown), then addref's the
                // result.  If ptr is a tearoff thunk, the addref happens
                // on the base object, not on the thunk.  If so, we should also
                // addref the thunk so that it stays alive while we
                // hold onto it.

                // To decide if ptr is a thunk, let's see if ptr->QI(_riid)
                // returns the same physical address as ptr.  This doesn't
                // work 100% of the time, but it works often enough to be
                // useful (e.g. for Trident's tearoffs) and there isn't a
                // better idea.

                IUnknown *punkTemp = NULL;

                _fAddRefOnMainThread = TRUE;
                
                if (S_OK == ptr->QueryInterface(*_riid, (void**)&punkTemp))
                {
                    if (punkTemp == ptr)
                    {
                        _fAddRefOnMainThread = FALSE;   // ptr is not a thunk (probably)
                    }

                    punkTemp->Release();
                }
            }
        }

        // Save a handle to the stream
        if (!hr)
            hr = GetHGlobalFromStream(pStm, &_hGlobal);

        if (hr)
        {
            // hr != 0 here means either the stream or the CoMarshal failed.
            // That's pretty bad, but let's keep the direct pointer
            // alive and see if we can continue to work on the main thread.
            _fAddRefOnMainThread = TRUE;
            hr = S_OK;
        }

        if (pStm)
            pStm->Release();

        if (_fAddRefOnMainThread)
        {
            ptr->AddRef();
        }
        
#else
        hr = _pGIT->RegisterInterfaceInGlobal(
                            ptr,
                            *_riid,
                            &_dwCookie);
#endif

        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}


IUnknown*
__gitpointer::_getPointer()
{ 
    IUnknown* ptr = _ptr;

    if (ptr && _dwMainThread != GetCurrentThreadId())
    {
        HRESULT hr;
        
#ifdef USE_COMARSHAL_FOR_GIT

        if (_hGlobal)
        {
            IStream * pStm = NULL;

            hr = CreateStreamOnHGlobal(_hGlobal, FALSE, &pStm);

            if (SUCCEEDED(hr))
            {
                hr = CoUnmarshalInterface(
                        pStm,
                        *_riid,
                        (void**)&ptr);

                pStm->Release();
            }
        }
#else
        if (_dwCookie)
        {
            Assert(_pGIT);
            hr = _pGIT->GetInterfaceFromGlobal(
                                    _dwCookie,
                                    *_riid,
                                    (void**)&ptr);
        }
#endif
        else
        {
            hr = E_UNEXPECTED;
        }

        if (FAILED(hr))
            Exception::throwE(hr);
    }

    return ptr;
}

