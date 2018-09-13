//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       substm.hxx
//
//  Contents:   Substream implementation
//
//  History:    04-22-1997   DBau (David Bau)    Created
//
//-------------------------------------------------------------------------

#ifdef UNIX
#  ifdef MW_MSCOMPATIBLE_LI
#    undef MW_MSCOMPATIBLE_LI
#  endif
#endif

#include "headers.hxx"

#ifndef X_SUBSTM_HXX_
#define X_SUBSTM_HXX_
#include "substm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

MtDefine(CSubstream, Dwn, "CSubstream")

//+------------------------------------------------------------------------
//
//  Function:   CreateReadOnlySubstream
//
//  Synopsis:   Constructs and intializes a read-only substream.
//
//              The substream begins at the current seek pointer in
//              the source stream, and stores its data in the next
//              cb bytes. The source stream's seek pointer is not
//              affected by a read only substream.
//
//-------------------------------------------------------------------------
HRESULT CreateReadOnlySubstream(CSubstream **ppStreamOut, IStream *pStreamSource, ULARGE_INTEGER cb)
{
    HRESULT hr;
    CSubstream *pSubstream = NULL;

    pSubstream = new CSubstream();
    if (!pSubstream)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pSubstream->InitRead(pStreamSource, cb));
    if (hr)
        goto Cleanup;
    
    *ppStreamOut = pSubstream;
    pSubstream = NULL;
    
Cleanup:
    ReleaseInterface(pSubstream);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CreateWritableSubstream
//
//  Synopsis:   Constructs and intializes a writable substream.
//
//              The substream begins at the current seek pointer in
//              the source stream, and grows as data is written. The
//              source stream's seek pointer is used directly by
//              the substream, so no operations should be done directly
//              on the source while the substream is in use.
//
//-------------------------------------------------------------------------
HRESULT CreateWritableSubstream(CSubstream **ppStreamOut, IStream *pStreamSource)
{
    HRESULT hr;
    CSubstream *pSubstream = NULL;

    pSubstream = new CSubstream();
    if (!pSubstream)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pSubstream->InitWrite(pStreamSource));
    if (hr)
        goto Cleanup;

    *ppStreamOut = pSubstream;
    pSubstream = NULL;
    
Cleanup:
    ReleaseInterface(pSubstream);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CSubstream::ctor
//
//  Synopsis:   Maintains a secondary object count for the object
//
//-------------------------------------------------------------------------
CSubstream::CSubstream()
{
    IncrementSecondaryObjectCount(10);
    _ulRefs = 1;
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::dtor
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------
CSubstream::~CSubstream()
{
    Detach();
    DecrementSecondaryObjectCount(10);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Detach
//
//  Synopsis:   Detaches a substream from the original stream.
//              Useful for ensuring no further writes to the
//              real stream after a substream is leaked.
//
//-------------------------------------------------------------------------
void CSubstream::Detach()
{
    if (_pOrig && _pOrig != this)
    {
        _pOrig->Release();
        _pOrig = this;
    }
    
    ClearInterface(&_pStream);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::InitRead
//
//  Synopsis:   Initializes a fixed-size, read-only substream
//              object at the current seek pointer in the
//              source stream.
//
//-------------------------------------------------------------------------
HRESULT CSubstream::InitRead(IStream *pStream, ULARGE_INTEGER cb)
{
    IStream *pClone = NULL;
    HRESULT hr;
    
    Assert(!_pStream && !_pOrig); // Init can only be called once

    _pOrig = this;
    _fWritable = FALSE;
    
    hr = THR(pStream->Clone(&pClone));
    if (hr)
        goto Cleanup;
    
    hr = THR(pClone->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &_ibStart));
    if (hr)
        goto Cleanup;
        
    _ibEnd.QuadPart = _ibStart.QuadPart + cb.QuadPart;

    _pStream = pClone;
    pClone = NULL;

Cleanup:
    ReleaseInterface(pClone);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::InitWrite
//
//  Synopsis:   Initializes a zero-size, writable substream
//              object at the current seek pointer in the
//              source stream.
//
//-------------------------------------------------------------------------
HRESULT CSubstream::InitWrite(IStream *pStream)
{
    HRESULT hr;
    
    Assert(!_pStream && !_pOrig); // Init can only be called once

    _pOrig = this;
    _fWritable = TRUE;
    
    pStream->AddRef();
    _pStream = pStream;

    hr = THR(pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &_ibStart));
    if (hr)
        goto Cleanup;
        
    _ibEnd.QuadPart = _ibStart.QuadPart; // begin as zero-sized substream
    
Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::InitClone
//
//  Synopsis:   Initializes a clone of an existing substream.
//
//-------------------------------------------------------------------------
HRESULT CSubstream::InitClone(CSubstream *pOrig)
{
    HRESULT hr;
    
    Assert(!_pStream && !_pOrig); // Init can only be called once
    
    Assert(pOrig->_pOrig);
    
    hr = THR(pOrig->_pStream->Clone(&_pStream));
    if (hr)
        goto Cleanup;
        
    pOrig->_pOrig->AddRef();
    _pOrig = pOrig->_pOrig;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::QueryInterface
//
//  Synopsis:   QI implementation
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IStream)
        QI_INHERITS(this, IStream)
    default:
        return(E_NOINTERFACE);
    }
    
    ((IUnknown *) *ppv)->AddRef();

    DbgTrackItf(iid, "CSubstream", FALSE, ppv);
    
    return(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Read, IStream
//
//  Synopsis:   Read wrapper, guards against reading beyond
//              the boundaries of the substream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Read(void HUGEP *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr;
    ULARGE_INTEGER ibCur;
    
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ibCur));
    if (hr)
        goto Cleanup;
        
    Assert(_pOrig->_ibEnd.QuadPart >= ibCur.QuadPart);

    if (cb > _pOrig->_ibEnd.QuadPart - ibCur.QuadPart)
        cb = _pOrig->_ibEnd.QuadPart - ibCur.QuadPart;
        
    hr = THR(_pStream->Read(pv, cb, pcbRead));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Write, IStream
//
//  Synopsis:   Write wrapper, expands stream if needed when
//              writing beyond the end.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Write(const void HUGEP *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr;
    ULARGE_INTEGER ibCur;
    
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    if (!_pOrig->_fWritable)
        return(STG_E_ACCESSDENIED);
        
    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ibCur));
    if (hr)
        goto Cleanup;
        
    if (cb + ibCur.QuadPart > _pOrig->_ibEnd.QuadPart)
        _pOrig->_ibEnd.QuadPart = ibCur.QuadPart + cb;
        
    hr = THR(_pStream->Write(pv, cb, pcbRead));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Seek, IStream
//
//  Synopsis:   Seek wrapper, expands stream if needed when
//              seeking beyond the end.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Seek(LARGE_INTEGER cbMove, DWORD dwOrigin, ULARGE_INTEGER *pibNewPosition)
{
    LARGE_INTEGER cbSeek;
    ULARGE_INTEGER ibCur;
    
    HRESULT hr;

    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    switch (dwOrigin)
    {
    default:
    case STREAM_SEEK_SET:
        cbSeek.QuadPart = _pOrig->_ibStart.QuadPart + cbMove.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ibCur));
        if (hr)
            goto Cleanup;
        cbSeek.QuadPart = ibCur.QuadPart + cbMove.QuadPart;
        break;
    case STREAM_SEEK_END:
        cbSeek.QuadPart = _pOrig->_ibEnd.QuadPart + cbMove.QuadPart;
        break;
    }

    if ((unsigned)cbSeek.QuadPart < _pOrig->_ibStart.QuadPart)
        return(STG_E_ACCESSDENIED);
        
    if ((unsigned)cbSeek.QuadPart > _pOrig->_ibEnd.QuadPart)
    {
        if (!_pOrig->_fWritable)
            return(STG_E_ACCESSDENIED);
        _pOrig->_ibEnd.QuadPart = cbSeek.QuadPart;
    }
 
    hr = THR(_pStream->Seek(cbSeek, STREAM_SEEK_SET, &ibCur));
    if (hr)
        goto Cleanup;

    Assert((unsigned)cbSeek.QuadPart == ibCur.QuadPart);

    if (pibNewPosition)
        pibNewPosition->QuadPart = ibCur.QuadPart - _pOrig->_ibStart.QuadPart;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::SetSize, IStream
//
//  Synopsis:   SetSize wrapper, delegates to source stream's
//              SetSize implementation.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::SetSize(ULARGE_INTEGER cbNewSize)
{
    ULARGE_INTEGER cbSourceNewSize;
    HRESULT hr;
   
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    if (!_pOrig->_fWritable)
        return(STG_E_ACCESSDENIED);

    cbSourceNewSize.QuadPart = _pOrig->_ibStart.QuadPart + cbNewSize.QuadPart;
        
    hr = THR(_pStream->SetSize(cbSourceNewSize));
    if (hr)
        goto Cleanup;
        
    _pOrig->_ibEnd.QuadPart = cbSourceNewSize.QuadPart;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::CopyTo, IStream
//
//  Synopsis:   CopyTo wrapper, guards against reading beyond
//              the boundaries of the substream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::CopyTo(IStream *pStream, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ULARGE_INTEGER ibCur;
    HRESULT hr;

    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ibCur));
    if (hr)
        goto Cleanup;
        
    Assert(_pOrig->_ibEnd.QuadPart >= ibCur.QuadPart);

    if (cb.QuadPart > _pOrig->_ibEnd.QuadPart - ibCur.QuadPart)
        cb.QuadPart = _pOrig->_ibEnd.QuadPart - ibCur.QuadPart;

    hr = THR(_pStream->CopyTo(pStream, cb, pcbRead, pcbWritten));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Commit, IStream
//
//  Synopsis:   Commit wrapper.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Commit(DWORD dwFlags)
{
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    if (!_pOrig->_fWritable)
        return(STG_E_ACCESSDENIED);

    RRETURN(THR(_pStream->Commit(dwFlags)));
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Revert, IStream
//
//  Synopsis:   Revert wrapper.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Revert()
{
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    if (!_pOrig->_fWritable)
        return(STG_E_ACCESSDENIED);

    RRETURN(THR(_pStream->Revert()));
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::LockRegion, IStream
//
//  Synopsis:   LockRegion wrapper, guards against locking
//              beyond the boundary of the substream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::LockRegion(ULARGE_INTEGER ibOffset, ULARGE_INTEGER cb, DWORD dwFlags)
{
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    ibOffset.QuadPart += _pOrig->_ibStart.QuadPart;
    
    if (ibOffset.QuadPart < _pOrig->_ibStart.QuadPart || ibOffset.QuadPart + cb.QuadPart > _pOrig->_ibEnd.QuadPart)
        return STG_E_ACCESSDENIED;

    RRETURN(_pStream->LockRegion(ibOffset, cb, dwFlags));
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::UnlockRegion, IStream
//
//  Synopsis:   UnlockRegion wrapper, guards against unlocking
//              beyond the boundary of the substream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::UnlockRegion(ULARGE_INTEGER ibOffset, ULARGE_INTEGER cb, DWORD dwFlags)
{
    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    ibOffset.QuadPart += _pOrig->_ibStart.QuadPart;
    
    if (ibOffset.QuadPart < _pOrig->_ibStart.QuadPart || ibOffset.QuadPart + cb.QuadPart > _pOrig->_ibEnd.QuadPart)
        return STG_E_ACCESSDENIED;

    RRETURN(_pStream->LockRegion(ibOffset, cb, dwFlags));
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Stat, IStream
//
//  Synopsis:   Stat wrapper, reports the size of the substream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Stat(STATSTG *pstatstg, DWORD dwFlags)
{
    HRESULT hr;

    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    hr = THR(_pStream->Stat(pstatstg, dwFlags));
    if (hr)
        goto Cleanup;

    pstatstg->cbSize.QuadPart = _pOrig->_ibEnd.QuadPart - _pOrig->_ibStart.QuadPart;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSubstream::Clone, IStream
//
//  Synopsis:   Clone wrapper, creates a new instance of
//              CSubstream pointing to the same IStream.
//
//-------------------------------------------------------------------------
STDMETHODIMP CSubstream::Clone(IStream **ppStream)
{
    CSubstream *pSubstream;
    HRESULT hr;

    if (!_pOrig->_pStream)
        return(E_FAIL); // Detached

    pSubstream = new CSubstream();
    if (!pSubstream)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR(pSubstream->InitClone(this));
    if (hr)
        goto Cleanup;

    *ppStream = pSubstream;
    pSubstream = NULL;

Cleanup:
    ReleaseInterface(pSubstream);
    RRETURN(hr);
}

