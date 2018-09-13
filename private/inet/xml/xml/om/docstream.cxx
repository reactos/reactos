/*
 * @(#)simplestream.cxx 1.0 12/08/1998
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _XML_OM_DOCSTREAM
#include "xml/om/docstream.hxx"
#endif

#include <xmlparser.h>

#define NOT_IMPLEMENTED  Assert(FALSE); return E_NOTIMPL

#ifdef UNIX
typedef const unsigned char * PUSHCAST;
#else
typedef const char * PUSHCAST; 
#endif


#define RUN_THRESHOLD   4096
#define BUFFER_SIZE     4096

// This function is also used by the xmlhttp control
// It saves the given document and returns a block of encoded bytes.
HRESULT SaveDocument(IDispatch * pDispatch, BYTE** ppData, DWORD* pdwSize)
{
    HRESULT             hr = S_OK;
    BOOL                fRetCode = FALSE;
    BSTR                bstrBody = NULL;
    IPersistStreamInit *pPS = NULL;
    IStream  *          pStm = NULL;
    HGLOBAL             hGlobal = NULL;
    BYTE*               pBytes = NULL;
    DWORD               dwSize = 0;
    STATSTG             statStg;

    if (!pDispatch)
        goto ErrorInvalidArg;
        
    // Save document to a memory stream.
    hr = pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPS);

    if (FAILED(hr))
        goto Error;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);

    if (FAILED(hr))
        goto Error;

    hr = pPS->Save(pStm, TRUE);
    
    if (FAILED(hr))
        goto Error;

    // Copy out the raw encoded bytes.
    hr = GetHGlobalFromStream(pStm, &hGlobal);

    if (FAILED(hr))
        goto Error;

    ::memset(&statStg,0,sizeof(STATSTG));
    pStm->Stat(&statStg, STATFLAG_NONAME);
    if (statStg.cbSize.HighPart)
    {
        // BUGBUG: should handle this error better
        hr = E_FAIL;
        goto Cleanup;
    }
    dwSize = statStg.cbSize.LowPart;

    if (dwSize > 0)
    {
        pBytes = (BYTE*)new_ne char[dwSize];

        if (pBytes == NULL)
        goto ErrorOutOfMemory;

        ::memcpy(pBytes, (const char*)GlobalLock(hGlobal), dwSize);
        GlobalUnlock(hGlobal);
    }   
Cleanup:
    if (pPS)
        pPS->Release();
    if (pStm)
        pStm->Release();

    *ppData = pBytes;
    *pdwSize = dwSize;
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    goto Cleanup;
}

DocStream::DocStream(Document* pDoc) : 
    _pDoc(pDoc),
    _cbWritten(0),
    _cbRead(0),
    _pBytes(NULL),
    _psa(NULL),
    _dwState(NONE),
    _dwBuffered(0)
{
    Assert (pDoc && "NULL pDoc is invalid");
}

DocStream::DocStream(IResponse* pResponse) : 
    _pResponse(pResponse),
    _cbWritten(0),
    _cbRead(0),
    _pBytes(NULL),
    _psa(NULL),
    _dwState(NONE),
    _dwBuffered(0)
{
    Assert (pResponse && "NULL pResponse is invalid");
}

DocStream::DocStream(IRequest* pRequest) :
    _cbWritten(0),
    _cbRead(0),
    _pBytes(NULL),
    _psa(NULL),
    _dwState(NONE),
    _dwBuffered(0)
{
    Assert (pRequest && "NULL pRequest is invalid");
    LONG _lCount;
    HRESULT hr;
    if (SUCCEEDED(pRequest->get_TotalBytes(&_lCount)))
    {
        _cbWritten = (DWORD)_lCount;
        // BUGBUG - this would be more efficient if we read a buffer at a time
        // but this works for now - since we don't anticipate HUGE XML Requests.
        VARIANT vData;
        VariantInit(&vData);
        VARIANT vSize;
        VariantInit(&vSize);
        vSize.vt = VT_I4;
        V_I4(&vSize) = _cbWritten;
        hr = pRequest->BinaryRead(&vSize, &vData);
        if (SUCCEEDED(hr))
        {
            _psa = V_ARRAY(&vData);
        }
        // No VariantClear - because we want to hold the safearray until read.
    }
}

DocStream::DocStream(SAFEARRAY* psa) :
    _cbWritten(0),
    _cbRead(0),
    _pBytes(NULL),
    _psa(NULL),
    _dwState(NONE),
    _dwBuffered(0)
{
    // A null safearray means we have nothing to read - it is an empty stream.
    if (psa)
    {
        // Copy it because we destroy it and because if this ends up being an
        // async load then the caller may destroy 'psa' after returning from load.
        HRESULT hr = SafeArrayCopy(psa,&_psa);
        if (hr)
            Exception::throwE(hr);

        _cbWritten = _psa->rgsabound[0].cElements;
    }
}

DocStream::~DocStream()
{
    if (_dwState == WRITING)
    {
        if (_pParser) 
        {
            // must tell the parser we've finished.
            _pParser->PushData(NULL, 0, true);
            _pParser->Run(-1);
        }
        else if (_dwBuffered > 0)
        {
            FlushSafeArray();
        }
    }
    _pParser = null;
    _pDoc = null;
    _pResponse = null;
    delete[] _pBytes;
    if (_psa)
        SafeArrayDestroy(_psa);
}

//
// ISequentialStream
//

HRESULT 
STDMETHODCALLTYPE 
DocStream::Read( 
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    STACK_ENTRY;
    HRESULT hr = S_OK;
    BYTE * pB = _pBytes;

    if (_dwState == WRITING) // cannot write then read (not yet anyway).
        return E_FAIL;

    _dwState = READING;

    if (! _pDoc && ! _psa)   // cannot read if we have no data !
    {
        cb = 0; // we have nothing to return then !!
        goto Done;
    }

    if (! pv && cb > 0)
    {
        hr = STG_E_INVALIDPOINTER;
        goto CleanUp;
    }
    if (cb == 0)
    {
        goto Done;
    }

    if (_psa)
    {
        hr = SafeArrayAccessData(_psa, (void **)&pB);
        if (hr)
            goto CleanUp; 
    }
    else if (_pBytes == NULL)
    {
        hr = SaveDocument();
        if (FAILED(hr))
            goto CleanUp;
        pB = _pBytes;
    }

    if (cb + _cbRead > _cbWritten)
    {
        cb = _cbWritten - _cbRead;
    }
    ::memcpy(pv, &pB[_cbRead], cb);
    _cbRead += cb;

    if (_psa)
    {
        SafeArrayUnaccessData(_psa);
    }

Done:
    if (pcbRead)
        *pcbRead = cb;

CleanUp:
    return hr;

}

    
HRESULT 
STDMETHODCALLTYPE 
DocStream::Write( 
     const void *pv,
     ULONG cb,
     ULONG *pcbWritten)
{
    HRESULT hr;

    STACK_ENTRY;

    if (_dwState == READING) // cannot write after reading (not yet anyway)
        return E_FAIL;

    _dwState = WRITING;

    if (! _pDoc && ! _pResponse)
    {
        return E_FAIL;  // cannot support write if we don't have a document or response object to write to
    }

    // NOTE even if cb == 0 we still have to tell the _pDoc that we are doing
    // a load operation and get the parser.

    if (! pv)
    {
        hr = STG_E_INVALIDPOINTER;
        goto CleanUp;
    }
    
    if (_pResponse)
    {
        hr = WriteSafeArray(pv,cb);
        if (hr)
            goto CleanUp;
    
        _cbWritten += cb;
        goto Done;
    }
    else if (!_pParser)
    {
        TRY
        {
            // We are going to save to the document, so we have to first
            // abort any downloads and reset it (just as if a load method was called).
            // calling enterDOMLoadLock() will handle abort and locking
            _pDoc->enterDOMLoadLock();
            _pDoc->reset();
            _pDoc->clear();
            _pDoc->getParser(&_pParser);
        }
        CATCH
        {
            hr = ERESULTINFO;

            TRY
            {
                _pDoc->leaveDOMLoadLock(hr);
            }
            CATCH
            {
                // Do nothing
            }
            ENDTRY

            goto CleanUp;
        }
        ENDTRY

        // This effectively lets Document::HandleEndDocument() to handle leaveDOMLoadLock()
        _pDoc->setExitedLoad(true);
    }

    if (cb == 0)    // now we can safely skip the rest of cb is zero.
        goto Done;

    Assert (_pParser && "NULL _pParser");
    hr =  _pParser->PushData((PUSHCAST)pv, cb, false);

    _cbWritten += cb;
    _dwBuffered += cb;

    // Run the parser every 4k or so.  Parser will buffer up the data until then.
    if (_dwBuffered > RUN_THRESHOLD)
    {
        _pParser->Run(-1);
        _dwBuffered = 0;
    }
    
Done:
    if (pcbWritten)
        *pcbWritten = cb;

CleanUp:
    return hr;
}


//
// IStream
//

HRESULT 
STDMETHODCALLTYPE 
DocStream::Seek( 
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    if (_dwState == WRITING)
    {
        // Canot seek after we've started writing.
        // unless it is a relative 0 move.
        if (dlibMove.QuadPart != 0)
        {
            NOT_IMPLEMENTED;
        }
    }
    else
    {
        // BUGBUG - we don't currently support LARGE_INTEGERS
        Assert(0 == dlibMove.HighPart); 

        switch (dwOrigin)
        {
        case STREAM_SEEK_SET:
            _cbRead = dlibMove.LowPart;
            break;
        case STREAM_SEEK_CUR:
            _cbRead += dlibMove.LowPart;
            break;
        case STREAM_SEEK_END:
            _cbRead = _cbWritten + dlibMove.LowPart;
            break;
        }

        // Check boundary conditions.
        if (_cbRead < 0) _cbRead = 0;
        else if (_cbRead > _cbWritten) _cbRead = _cbWritten;
    }


    if (plibNewPosition)
    {
        if (_dwState == NONE)
            plibNewPosition->LowPart = 0;
        else if (_dwState == READING)
            plibNewPosition->LowPart = _cbRead;
        else
            plibNewPosition->LowPart = _cbWritten;

        plibNewPosition->HighPart = 0;
    }
    return S_OK;
}
    

HRESULT 
STDMETHODCALLTYPE 
DocStream::SetSize( 
    ULARGE_INTEGER libNewSize)
{
    NOT_IMPLEMENTED;
}
    

HRESULT 
STDMETHODCALLTYPE 
DocStream::CopyTo( 
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    NOT_IMPLEMENTED;
}
    

HRESULT 
STDMETHODCALLTYPE 
DocStream::Commit( 
    DWORD grfCommitFlags)
{
    return S_OK; // nothing special to do here.
}

HRESULT 
STDMETHODCALLTYPE 
DocStream::Revert()
{
    NOT_IMPLEMENTED;
}


HRESULT 
STDMETHODCALLTYPE 
DocStream::LockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    NOT_IMPLEMENTED;
}
    

HRESULT 
STDMETHODCALLTYPE 
DocStream::UnlockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    NOT_IMPLEMENTED;
}
    
HRESULT 
STDMETHODCALLTYPE 
DocStream::Stat( 
    STATSTG *pstatstg,
    DWORD grfStatFlag)
{
    STACK_ENTRY;

    HRESULT hr = S_OK;
    if (! pstatstg)
    {
        hr = STG_E_INVALIDPOINTER;
        goto CleanUp;
    }

    ::memset(pstatstg,0,sizeof(STATSTG));
    pstatstg->type = STGTY_STREAM;

    TRY
    {
        if (_pDoc && ! _pBytes)
        {
            // if _pDoc then prime the pump so we can return the stream size.
            hr = SaveDocument();
            if (FAILED(hr))
                goto CleanUp;
        }
        pstatstg->cbSize.LowPart = _cbWritten;

        if (! (grfStatFlag & STATFLAG_NONAME ))
        {

            if (_pDoc)
            {
                String* pURL = _pDoc->getURL();
                if (pURL)
                {
                    ULONG length = pURL->length();
                    pstatstg->pwcsName = (LPWSTR)CoTaskMemAlloc((length + 1) * sizeof(WCHAR));
                    if (pstatstg->pwcsName == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        ::memcpy(pstatstg->pwcsName, pURL->getWCHARPtr(), length * sizeof(WCHAR));
                        pstatstg->pwcsName[length] = '\0';
                    }
                }
            }
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

CleanUp:
    return hr;
}
    
HRESULT 
STDMETHODCALLTYPE 
DocStream::Clone( 
    IStream **ppstm)
{
    NOT_IMPLEMENTED;
}

HRESULT 
DocStream::WriteSafeArray(const void *pv, ULONG cb)
{
    ULONG pos = 0;            
    ULONG remaining = cb;
    BYTE* pSrcBytes = (BYTE*)pv;
    HRESULT hr = S_OK;

    // make sure our safearray is large enough to hold the data,
    // then copy data into the safearray.
    if (NULL == _psa)
    {
        _psa = SafeArrayCreateVector(VT_UI1, 0, BUFFER_SIZE);
        _dwBuffered = 0;
        if (!_psa)
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
    }

    // Now, if the 'pv' buffer being written is bigger than our 4k
    // safearray, then write it out in 4k chunks - so we have a good
    // working set.  If the 'pv' buffer is just a few bytes then buffer 
    // up the bytes until we fill the 4k boundary - and then write the buffer.
    while (remaining > 0)
    {
        BYTE * pB;
        ULONG chunk = remaining;

        if (chunk + _dwBuffered > BUFFER_SIZE)
        {
            chunk = BUFFER_SIZE - _dwBuffered;
        }

        hr = SafeArrayAccessData(_psa, (void **)&pB);
        if (hr)
            goto CleanUp;

        memcpy(&pB[_dwBuffered], &pSrcBytes[pos], chunk);

        _dwBuffered += chunk;
        _psa->rgsabound[0].cElements = _dwBuffered;

        // BUGBUG - I'm not sure it's kosher to keep the safearray locked
        // while we call BinaryWrite - which is why I unlock it here, otherwise
        // we might be able to move this outside the loop.
        SafeArrayUnaccessData(_psa);

        remaining -= chunk;
        pos += chunk;

        if (_dwBuffered >= BUFFER_SIZE)
        {
            FlushSafeArray();
        }
    }

CleanUp:
    return hr;
//        VariantClear(&vData); // NO -- we want to keep the safearray alive !!
}

HRESULT 
DocStream::FlushSafeArray()
{
    VARIANT vData;
    // do we need to flush the buffer ?
    // wrap this in a variant.
    VariantInit(&vData);
    V_ARRAY(&vData) = _psa;
    vData.vt = VT_ARRAY | VT_UI1;

    // And write it out !!
    HRESULT hr = _pResponse->BinaryWrite(vData);
    _dwBuffered = 0;

    return hr;
}

HRESULT  
DocStream::SaveDocument()
{
    // Save document to memory.
    IDispatch* pDispatch = NULL;
    HRESULT hr = _pDoc->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    if (FAILED(hr))
        goto CleanUp;

    hr = ::SaveDocument(pDispatch, &_pBytes, &_cbWritten);

CleanUp:
    if (pDispatch)
        pDispatch->Release();
    return hr;
}
