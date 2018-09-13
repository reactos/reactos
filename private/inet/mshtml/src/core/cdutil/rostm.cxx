//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       rostm.cxx
//
//  Contents:   CROStmOnBuffer
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

MtDefine(CROStmOnBuffer, Utilities, "CROStmOnBuffer")
MtDefine(CROStmOnBuffer_pbBuf, CROStmOnBuffer, "CROStmOnBuffer::_pbBuf")

//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::CROStmOnBuffer
//
//  Synopsis:   constructor
//
//----------------------------------------------------------------------------

CROStmOnBuffer::CROStmOnBuffer()
{
    _ulRefs = 1;
    _pbSeekPtr = NULL;
    _pbBuf = NULL;
    _cbBuf = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::~CROStmOnBuffer
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------

CROStmOnBuffer::~CROStmOnBuffer()
{
    if (_pbBuf)
    {
        delete _pbBuf;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::Init
//
//  Synopsis:   Initializer
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::Init(BYTE *pb, long cb)
{
    HRESULT hr = S_OK;
    
    _cbBuf = cb;
    if (cb)
    {
        _pbBuf = new(Mt(CROStmOnBuffer_pbBuf)) BYTE[cb];
        if (!_pbBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        memcpy((void *)_pbBuf, pb, cb);
        _pbSeekPtr = _pbBuf;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::QueryInterface
//
//  Synopsis:   per IUnknown
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IStream || riid == IID_IUnknown)
    {
        *ppvObj = (IStream *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::AddRef
//
//  Synopsis:   per IUnknown
//
//----------------------------------------------------------------------------

ULONG
CROStmOnBuffer::AddRef()
{
    return ++_ulRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::Release
//
//  Synopsis:   per IUnknown
//
//----------------------------------------------------------------------------

ULONG
CROStmOnBuffer::Release()
{
    ULONG   ulRefs = _ulRefs;
    
    if (--_ulRefs == 0)
    {
        delete this;
    }

    return ulRefs - 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::Read
//
//  Synopsis:   per IStream
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::Read(void HUGEP * pv, ULONG cb, ULONG * pcbRead)
{
    HRESULT hr = S_OK;
    
    if (!pv)
    {
        hr = STG_E_INVALIDPOINTER;
        goto Cleanup;
    }
    
    //
    // Catch the case where a read might go beyond the end
    // of the buffer
    //
    
    if (_pbBuf + _cbBuf < _pbSeekPtr + cb)
    {
        cb = _pbBuf + _cbBuf - _pbSeekPtr;
    }

    memcpy(pv, (void *)_pbSeekPtr, cb);
    _pbSeekPtr += cb;
    
    if (pcbRead)
    {
        *pcbRead = cb;
    }

    if (cb == 0)
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::Seek
//
//  Synopsis:   per IStream
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::Seek(
    LARGE_INTEGER dlibMove, 
    DWORD dwOrigin, 
    ULARGE_INTEGER * plibNewPosition)
{
    HRESULT     hr  = S_OK;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        if (dlibMove.HighPart != 0)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            _pbSeekPtr = _pbBuf + dlibMove.LowPart;
        }
        break;

    case STREAM_SEEK_CUR:
        if (dlibMove.HighPart != 0 && dlibMove.HighPart != -1)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            _pbSeekPtr += (int) dlibMove.LowPart;
        }
        break;

    case STREAM_SEEK_END:
        if (dlibMove.HighPart != -1 && dlibMove.HighPart != 0)
        {
            hr = STG_E_ACCESSDENIED;
        }
        else
        {
            _pbSeekPtr = _pbBuf + _cbBuf + (int) dlibMove.LowPart;
        }
        break;

    default:
        hr = STG_E_INVALIDFUNCTION;
        break;
    }

    if (_pbSeekPtr < _pbBuf)
    {
        _pbSeekPtr = _pbBuf;
        hr = E_INVALIDARG;
    }
    else if (_pbSeekPtr > _pbBuf + _cbBuf)
    {
        _pbSeekPtr = _pbBuf + _cbBuf;
        hr = STG_E_ACCESSDENIED;
    }

    if (plibNewPosition)
    {
        plibNewPosition->LowPart = _pbSeekPtr - _pbBuf;
        plibNewPosition->HighPart = 0;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::CopyTo
//
//  Synopsis:   per IStream
//
//      As far as I know this routine is only used during the
//      FullWindowEmbed IHistory saving scenario.  This scenario
//      copies only a view bytes so this routine is not very optimized.
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::CopyTo(
     IStream * pstm,
     ULARGE_INTEGER cb,
     ULARGE_INTEGER * pcbRead,
     ULARGE_INTEGER * pcbWritten)
{
    ULONG   ulWritten, ulToWrite, ulAvailable;
    HRESULT hr = S_OK;

    ulToWrite = cb.LowPart;
    if( cb.HighPart != 0 )
        return( E_INVALIDARG );  // sorry guy.

    ulAvailable = (_pbBuf + _cbBuf) - _pbSeekPtr;
    if( ulToWrite > ulAvailable )
        ulToWrite = ulAvailable;

    hr = pstm->Write( _pbSeekPtr, ulToWrite, &ulWritten );

    _pbSeekPtr += ulWritten;  // advance past what was written.
    
    if( pcbRead != NULL )
    {
        pcbRead->LowPart = ulWritten;
        pcbRead->HighPart = 0;
    }
    if( pcbWritten != NULL )
    {
        pcbWritten->LowPart = ulWritten;
        pcbWritten->HighPart = 0;
    }

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CROStmOnBuffer::Stat
//
//  Synopsis:   per IStream
//
//----------------------------------------------------------------------------

HRESULT
CROStmOnBuffer::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    if (pstatstg)
    {
        memset(pstatstg, 0, sizeof(STATSTG));
        pstatstg->type = STGTY_STREAM;
        ULISet32(pstatstg->cbSize, _cbBuf);
        pstatstg->grfMode = STGM_READ;
    }

    return S_OK;
}
