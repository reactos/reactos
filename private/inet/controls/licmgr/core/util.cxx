//+----------------------------------------------------------------------------
//  File:       util.cxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <core.hxx>


// Globals --------------------------------------------------------------------
const UCHAR SZ_NEWLINE[] = "\n\r";


//+----------------------------------------------------------------------------
//  Function:   CopyStream
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CopyStream(
    IStream *           pstmDest,
    IStream *           pstmSrc,
    ULARGE_INTEGER      cbCopy,
    ULARGE_INTEGER *    pcbRead,
    ULARGE_INTEGER *    pcbWritten)
{
    DWORD   cb     = cbCopy.LowPart;
    DWORD   cbStep = min(cb, 0xFFFF);
    DWORD   cbRead, cbWritten;
    DWORD   cbTotalRead = 0;
    DWORD   cbTotalWritten = 0;
    void *  pv = NULL;
    HRESULT hr = S_OK;

    if (cbCopy.HighPart)
        return E_INVALIDARG;

    pv = new BYTE[cbStep];
    if (!pv)
        return E_OUTOFMEMORY;

    while (cb)
    {
        cbRead = min(cbStep, cb);

        hr = pstmSrc->Read(pv, cbRead, &cbRead);
        if (hr || !cbRead)
            break;

        cbTotalRead += cbRead;

        hr = pstmDest->Write(pv, cbRead, &cbWritten);
        if (hr)
            break;

        cbTotalWritten += cbWritten;

        if (cbWritten != cbRead)
        {
            hr = E_UNEXPECTED;
            break;
        }

        cb -= cbRead;
    }

    if (pcbRead)
    {
        pcbRead->HighPart = 0;
        pcbRead->LowPart  = cbTotalRead;
    }

    if (pcbWritten)
    {
        pcbWritten->HighPart = 0;
        pcbWritten->LowPart  = cbTotalWritten;
    }

    delete [] pv;
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Read
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMemoryStream::Read(
    void *  pv,
    ULONG   cb,
    ULONG * pcbRead)
{
    Assert(_pbData);
    Assert(_ibPos <= _cbSize);

    cb = min(cb, _cbSize - _ibPos);

    ::memcpy(pv, _pbData + _ibPos, cb);
    _ibPos += cb;

    if (pcbRead)
    {
        *pcbRead = cb;
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Member:     Write
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMemoryStream::Write(
    const void *    pv,
    ULONG           cb,
    ULONG *         pcbWritten)
{
    HRESULT hr = S_OK;

    Assert(_pbData);
    Assert(_ibPos <= _cbSize);

    if ((_ibPos + cb) > _cbSize)
    {
        hr = STG_E_MEDIUMFULL;
        goto Cleanup;
    }

    ::memcpy(_pbData + _ibPos, pv, cb);
    _ibPos += cb;

Cleanup:
    if (pcbWritten)
    {
        *pcbWritten = (!hr
                            ? cb
                            : 0);
    }
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Seek
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMemoryStream::Seek(
    LARGE_INTEGER       dlibMove,
    DWORD               dwOrigin,
    ULARGE_INTEGER *    plibNewPosition)
{
    LONG    ibOffset = (LONG)dlibMove.LowPart;

    Assert(_pbData);

    // Ensure only 32-bits is in-use
    if (!(dlibMove.HighPart ==  0 && ibOffset >= 0) &&
        !(dlibMove.HighPart == -1 && ibOffset < 0))
        return E_INVALIDARG;

    switch (dwOrigin)
    {
        case STREAM_SEEK_SET:
            break;

        case STREAM_SEEK_CUR:
            ibOffset = (LONG)_ibPos + ibOffset;
            break;

        case STREAM_SEEK_END:
            ibOffset = (LONG)_cbSize + ibOffset;
            break;

        default:
            return E_INVALIDARG;
    }

    // Ensure the new offset is within the correct range
    if ((ULONG)ibOffset > _cbSize)
        return E_INVALIDARG;

    // Store the new offset and return it
    _ibPos = (ULONG)ibOffset;

    if (plibNewPosition)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart = _ibPos;
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//  Member:     SetSize
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMemoryStream::SetSize(
    ULARGE_INTEGER  libNewSize)
{
    if (libNewSize.HighPart)
        return STG_E_MEDIUMFULL;

    if (libNewSize.LowPart <= _cbSize)
    {
        _cbSize = libNewSize.LowPart;
    }
    else
    {
        BYTE *  pbData = new BYTE[libNewSize.LowPart];
        
        if (!pbData)
            return STG_E_MEDIUMFULL;

        if (_pbData && _cbSize)
        {
            ::memcpy(pbData, _pbData, _cbSize);
        }

        delete [] _pbData;
        _cbSize = libNewSize.LowPart;
        _pbData = pbData;
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Member:     CopyTo
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMemoryStream::CopyTo(
    IStream *           pstm,
    ULARGE_INTEGER      cb,
    ULARGE_INTEGER *    pcbRead,
    ULARGE_INTEGER *    pcbWritten)
{
    if (!pstm)
        return STG_E_INVALIDPOINTER;

    if (cb.HighPart || ((_ibPos + cb.LowPart) > _cbSize))
        return E_INVALIDARG;

    Assert(_pbData);
    return ::CopyStream(pstm, this, cb, pcbRead, pcbWritten);
}


//+----------------------------------------------------------------------------
//  Member:     CBufferedStream
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
CBufferedStream::CBufferedStream(
    IStream *   pstm,
    ULONG       cbNewLine,
    BOOL        fRead)
{
    Assert(_pstm);

    _fRead = fRead;
    _pb = NULL;
    _cb = 0;
    _ib = 0;
    _cbLine    = 0;
    _cbNewLine = cbNewLine;
    _cbTotal   = 0;
    _pstm = ::SAddRef(pstm);
}


//+----------------------------------------------------------------------------
//  Member:     ~CBufferedStream
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
CBufferedStream::~CBufferedStream()
{
    Verify(SUCCEEDED(Flush()));
    ::SRelease(_pstm);
}


//+----------------------------------------------------------------------------
//  Member:     Flush
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CBufferedStream::Flush(
    ULONG * pcbWritten)
{
    ULONG   cbWritten;
    HRESULT hr = S_OK;

    Assert(_pstm);
    Implies(_cbNewLine, _ib <= (_cb + CB_NEWLINE));
    Implies(!_cbNewLine, _ib <= _cb);

    if (!pcbWritten)
    {
        pcbWritten = &cbWritten;
    }
    *pcbWritten = 0;

    // For read-only streams, "read" the rest of the buffer by setting the buffer index
    // (This will force a re-load during the next read)
    if (_fRead)
    {
        _ib = _cb;
    }

    // For write-only streams, write the buffer to the stream
    else if (_ib)
    {
        Assert(!_fRead);
        hr = _pstm->Write(_pb, _ib, pcbWritten);
        _cbTotal += *pcbWritten;
        if (!hr)
        {
            Assert(*pcbWritten == _ib);
            _ib = 0;
        }
    }

    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Load
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CBufferedStream::Load()
{
    HRESULT hr;

    Assert(_fRead);

    hr = _pstm->Read(_pb, _cb, &_cbTotal);
    if (hr)
        goto Cleanup;
    _ib = 0;

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     SetBufferSize
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CBufferedStream::SetBufferSize(
    ULONG   cb)
{
    // The buffer size cannot be changed once it has been set
    // Also, it is illegal to use a zero-sized buffer
    if (_pb || !cb)
        return E_FAIL;

    // Allocate a new buffer of the requested size
    // (If the caller requested automatic interjection of NEWLINEs, slightly increase
    //  allocated buffer; the remembered size will continue to be that which they
    //  requested)
    _pb = new BYTE[cb + (_cbNewLine ? CB_NEWLINE : 0)];
    if (!_pb)
        return E_OUTOFMEMORY;
    _cb = cb;
    _ib = (_fRead ? _cb : 0);

    return S_OK;
}


//+----------------------------------------------------------------------------
//  Member:     Read
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CBufferedStream::Read(
    void *  pv,
    ULONG   cb,
    ULONG * pcbRead)
{
    ULONG   cbTotalRead;
    ULONG   cbRead;
    HRESULT hr = S_OK;

    Assert(_pb);
    Assert(_cb);
    Assert(_ib <= _cb);

    if (!pv)
        return E_INVALIDARG;

    if (!pcbRead)
    {
        pcbRead = &cbTotalRead;
    }
    *pcbRead = 0;

    // If bytes remain in the buffer, "read" those first
    if (_ib < _cbTotal)
    {
        cbRead = min(cb, _cbTotal-_ib);
        ::memcpy(pv, _pb+_ib, cbRead);
        _ib += cbRead;
        cb  -= cbRead;
        pv   = (void *)(((const BYTE *)pv) + cbRead);
        *pcbRead += cbRead;
    }

    // If bytes remain to be read, fetch them now
    if (cb)
    {
        Assert(_ib >= _cbTotal);
        Assert(_cbTotal <= _cb);

        // If the request fits within half of the buffer, then load a buffer full
        if (cb < (_cb/2))
        {
            hr = Load();
            if (hr)
                goto Cleanup;

            cbRead = min(cb, _cbTotal);
            ::memcpy(pv, _pb, cbRead);

            _ib = cbRead;
            *pcbRead += cbRead;
        }

        // Otherwise, read directly into the callers buffer
        else
        {
            hr = _pstm->Read(pv, cb, pcbRead);
        }
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Write
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CBufferedStream::Write(
    const void *    pv,
    ULONG           cb,
    ULONG *         pcbWritten)
{
    const BYTE *    pbSrc = (const BYTE *)pv;
    ULONG           ibSrc = 0;
    ULONG           cbWritten;
    HRESULT         hr = S_OK;

    Assert(_pb);
    Assert(_cb);
    Assert(_ib <= _cb);

    if (!pv)
        return E_INVALIDARG;

    if (pcbWritten)
    {
        *pcbWritten = 0;
    }

    // Treat calls to write on a read-only stream as if the stream is full
    if (_fRead)
    {
        hr = STG_E_MEDIUMFULL;
        goto Cleanup;
    }

    // Write the bytes to the stream
    while (cb)
    {
        // Determine the number of bytes to write this time through
        cbWritten = min(cb, _cb-_ib);
        if (_cbNewLine)
        {
            cbWritten = min(cbWritten, _cbNewLine-_cbLine);
        }

        // Write the bytes to the local buffer
        ::memcpy(_pb + _ib, pbSrc + ibSrc, cbWritten);

        // Update the counters reflecting what has been written
        // (Adding a newline if necessary)
        cb    -= cbWritten;
        ibSrc += cbWritten;
        _ib   += cbWritten;
        if (_cbNewLine)
        {
            _cbLine += cbWritten;
            if (_cbLine >= _cbNewLine)
            {
                ::memcpy(_pb + _ib, SZ_NEWLINE, CB_NEWLINE);
                _cbLine = 0;
                _ib    += CB_NEWLINE;
            }
        }

        // If the buffer is full, write it to the stream
        if (_ib >= _cb)
        {
            hr = Flush(&cbWritten);
            if (pcbWritten)
            {
                *pcbWritten += cbWritten;
            }
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     CFileStream
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
CFileStream::CFileStream()
{
    _hFile = NULL;
}


//+----------------------------------------------------------------------------
//  Member:     ~CFileStream
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
CFileStream::~CFileStream()
{
    if (_hFile)
    {
        ::CloseHandle(_hFile);
    }
}


//+----------------------------------------------------------------------------
//  Member:     Init
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CFileStream::Init(
    LPCWSTR                 wszFileName,
    DWORD                   dwDesiredAccess,
    DWORD                   dwShareMode,
    LPSECURITY_ATTRIBUTES   pSecurityAttributes,
    DWORD                   dwCreationDistribution,
    DWORD                   dwFlagsAndAttributes,
    HANDLE                  hTemplateFile)
{
	int len = WideCharToMultiByte(CP_ACP, 0, wszFileName, -1, 0, 0, NULL, NULL);
	LPSTR psz = new CHAR[len]; 
    ::WideCharToMultiByte(CP_ACP, 0, wszFileName, 
		-1, (LPSTR)psz, len, NULL, NULL);
	_hFile = ::CreateFileA(psz, dwDesiredAccess, dwShareMode, pSecurityAttributes,	
                        dwCreationDistribution,	dwFlagsAndAttributes, hTemplateFile);
	delete psz;

	// CreateFile succeeded if we didn't get back INVALID_HANDLE_VALUE
    return ((_hFile != INVALID_HANDLE_VALUE)
                ? S_OK
                : GetWin32Hresult());
}


//+----------------------------------------------------------------------------
//  Member:     GetFileSize
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CFileStream::GetFileSize(
    ULONG * pcbSize)
{
    Assert(_hFile);
    Assert(pcbSize);
    *pcbSize = ::GetFileSize(_hFile, NULL);
    return (*pcbSize == 0xFFFFFFFF
                ? GetWin32Hresult()
                : S_OK);
}


//+----------------------------------------------------------------------------
//  Member:     Read
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFileStream::Read(
    void *  pv,
    ULONG   cb,
    ULONG * pcbRead)
{
    ULONG   cbRead;
    HRESULT hr = S_OK;

    if (!pv)
        return E_INVALIDARG;

    if (!pcbRead)
    {
        pcbRead = &cbRead;
    }

    hr = (::ReadFile(_hFile, pv, cb, pcbRead, NULL)
                ? S_OK
                : S_FALSE);

    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Write
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFileStream::Write(
    const void *    pv,
    ULONG           cb,
    ULONG *         pcbWritten)
{
    ULONG   cbWritten;
    HRESULT hr;

    if (!pv)
        return E_INVALIDARG;

    if (!pcbWritten)
    {
        pcbWritten = &cbWritten;
    }

    hr = (::WriteFile(_hFile, pv, cb, pcbWritten, NULL)
                ? S_OK
                : STG_E_MEDIUMFULL);
    return hr;
}


//+----------------------------------------------------------------------------
//  Member:     Seek
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFileStream::Seek(
    LARGE_INTEGER       dlibMove,
    DWORD               dwOrigin,
    ULARGE_INTEGER *    plibNewPosition)
{
    DWORD           dwMoveMethod;
    ULARGE_INTEGER  libNewPosition;
    HRESULT         hr;

    if (plibNewPosition)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart  = 0;
    }

    switch(dwOrigin)
    {
        case STREAM_SEEK_SET: dwMoveMethod = FILE_BEGIN; break;
        case STREAM_SEEK_CUR: dwMoveMethod = FILE_CURRENT; break;
        case STREAM_SEEK_END: dwMoveMethod = FILE_END; break;
        default:
            return E_INVALIDARG;
    }

    libNewPosition.LowPart = ::SetFilePointer(_hFile, dlibMove.LowPart, &dlibMove.HighPart, dwMoveMethod);
    libNewPosition.HighPart = dlibMove.HighPart;

    if (libNewPosition.LowPart == 0xFFFFFFFF &&
        ::GetLastError() != NO_ERROR)
    {
        hr = STG_E_INVALIDPOINTER;
    }
    else
    {
        if (plibNewPosition)
        {
            *plibNewPosition = libNewPosition;
        }
        hr = S_OK;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetSize
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFileStream::SetSize(
    ULARGE_INTEGER  libNewSize)
{
    LONG    curLow, curHigh;
    BOOL    fEOFSet;

    curHigh = 0;
    curLow = ::SetFilePointer(_hFile, 0, &curHigh, FILE_CURRENT);
    if (0xFFFFFFFF == ::SetFilePointer(_hFile, libNewSize.LowPart,
                                        (LONG *)&libNewSize.HighPart, FILE_BEGIN))
    {
        return STG_E_INVALIDFUNCTION;
    }

    fEOFSet = ::SetEndOfFile(_hFile);
#ifdef _DEBUG
    Verify(0xFFFFFFFF != ::SetFilePointer(_hFile, curLow, &curHigh, FILE_BEGIN));
#else
    ::SetFilePointer(_hFile, curLow, &curHigh, FILE_BEGIN);
#endif

    return (fEOFSet
                ? S_OK
                : STG_E_MEDIUMFULL);
}


//+----------------------------------------------------------------------------
//
//  Member:     CopyTo
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFileStream::CopyTo(
    IStream *           pstm,
    ULARGE_INTEGER      cb,
    ULARGE_INTEGER *    pcbRead,
    ULARGE_INTEGER *    pcbWritten)
{
    if (!pstm)
        return STG_E_INVALIDPOINTER;

    return ::CopyStream(pstm, this, cb, pcbRead, pcbWritten);
}


//+----------------------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CFileStream::PrivateQueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    if (riid == IID_IStream)
        *ppvObj = (IStream *)this;

    if (*ppvObj)
        return S_OK;
    else
        return parent::PrivateQueryInterface(riid, ppvObj);
}


//+----------------------------------------------------------------------------
//  Function:   CoAlloc
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
void *
CoAlloc(
    ULONG   cb)
{
    Assert(TLS(dll.pmalloc));
    return TLS(dll.pmalloc)->Alloc(cb);
}


//+----------------------------------------------------------------------------
//  Function:   CoFree
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
void
CoFree(
    void *  pv)
{
    if (!pv)
        return;

    Assert(TLS(dll.pmalloc));
    Assert(CoDidAlloc(pv));
    TLS(dll.pmalloc)->Free(pv);
}


//+----------------------------------------------------------------------------
//  Function:   CoGetSize
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
ULONG
CoGetSize(
    void *  pv)
{
    Assert(TLS(dll.pmalloc));
    Assert(CoDidAlloc(pv));
    return (ULONG)(TLS(dll.pmalloc)->GetSize(pv));
}


//+----------------------------------------------------------------------------
//  Function:   CoDidAlloc
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
BOOL
CoDidAlloc(
    void *  pv)
{
    Assert(TLS(dll.pmalloc));
    return (!pv || TLS(dll.pmalloc)->DidAlloc(pv) == 1
                ? TRUE
                : FALSE);
}


#ifdef _NOCRT
//+----------------------------------------------------------------------------
//  Function:   purecall
//
//  Synopsis:   _SHIP build replacement for CRT vtable routine
//
//-----------------------------------------------------------------------------
int __cdecl
_purecall()
{
    return 0;
}


//+----------------------------------------------------------------------------
//  Function:   _tcslen
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" size_t __cdecl
_tcslen(
    const TCHAR *   psz)
{
    for (size_t i=0; *psz; psz++, i++);
    return i;
}


//+----------------------------------------------------------------------------
//  Function:   memcmp
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" int __cdecl
memcmp(
    const void *    pv1,
    const void *    pv2,
    size_t          cb)
{
    size_t  i;
    int     d;
    for (i=0, d=0; i < cb && !d; i++)
        d = (*(const BYTE *)pv1) - (*(const BYTE *)pv2);
    return d;
}


//+----------------------------------------------------------------------------
//  Function:   memcpy
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" void * __cdecl
memcpy(
    void *          pvDest,
    const void *    pvSrc,
    size_t          cb)
{
    for (size_t i=0; i < cb; i++)
        ((BYTE *)pvDest)[i] = ((const BYTE *)pvSrc)[i];
    return pvDest;
}


//+----------------------------------------------------------------------------
//  Function:   memset
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" void * __cdecl
memset(
    void *  pv,
    int     c,
    size_t  cb)
{
    for (size_t i=0; i < cb; i++)
        ((BYTE *)pv)[i] = (BYTE)c;
    return pv;
}


//+----------------------------------------------------------------------------
//  Function:   memmove
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" void * __cdecl
memmove(
    void *          pvDest,
    const void *    pvSrc,
    size_t          cb)
{
    BYTE *  pb1;
    BYTE *  pb2;

    if (pvSrc < pvDest)
    {
        pb1 = (BYTE *)pvDest + cb;
        pb2 = (BYTE *)pvSrc  + cb;
        for (; cb; cb--)
        {
            *pb1-- = *pb2--;
        }
    }
    else if (pvSrc > pvDest)
    {
        pb1 = (BYTE *)pvDest;
        pb2 = (BYTE *)pvSrc;
        for (; cb; cb--)
        {
            *pb1++ = *pb2++;
        }
    }
    return pvDest;
}
#endif // _NOCRT
