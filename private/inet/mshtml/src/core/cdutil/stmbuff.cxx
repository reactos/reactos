//+---------------------------------------------------------------------
//
//  File:       stmbuff.cxx
//
//  Contents:   Useful OLE helper functions
//
//----------------------------------------------------------------------

#ifdef UNIX
#  ifdef MW_MSCOMPATIBLE_LI
#    undef MW_MSCOMPATIBLE_LI
#  endif
#endif

#include "headers.hxx"

const LARGE_INTEGER LINULL = {0, 0};

//+---------------------------------------------------------------------------
//
//  Class:      CStreamBuffered 
//
//  Purpose:    IStream wrapper with a small internal cache for buffering read
//              operations
//
//  Interface:  CStreamBuffered             -- Constructor
//
//----------------------------------------------------------------------------

MtDefine(CStreamBuffered, Utilities, "CStreamBuffered")

class CStreamBuffered : public IStream
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStreamBuffered))
    CStreamBuffered(IStream * pStream);
    ~CStreamBuffered();

    // IUnknown methods
    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void) ;
    STDMETHOD_(ULONG, Release) (void);

    // IStream methods
    STDMETHOD(Read)(
         void * pv,
         ULONG cb,
         ULONG * pcbRead);

    STDMETHOD(Write)(
         const void * pv,
         ULONG cb,
         ULONG * pcbWritten);

    STDMETHOD(Seek)(
         LARGE_INTEGER dlibMove,
         DWORD dwOrigin,
         ULARGE_INTEGER * plibNewPosition);

    STDMETHOD(SetSize)(
         ULARGE_INTEGER libNewSize);

    STDMETHOD(CopyTo)(
         IStream * pstm,
         ULARGE_INTEGER cb,
         ULARGE_INTEGER * pcbRead,
         ULARGE_INTEGER * pcbWritten);

    STDMETHOD(Commit)(
         DWORD grfCommitFlags);

    STDMETHOD(Revert)( void);

    STDMETHOD(LockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType);

    STDMETHOD(UnlockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType);

    STDMETHOD(Stat)(
         STATSTG * pstatstg,
         DWORD grfStatFlag);

    STDMETHOD(Clone)(
         IStream ** ppstm);

#define STREAMBUFSIZE 4096

    ULONG       _ulRefs;
    IStream *   _pStm;
    BYTE    _ab[STREAMBUFSIZE];
    ULONG   _ulPos;                 // position in buffer
    ULONG   _ulSize;                // size filled in buffer
    ULARGE_INTEGER _uliSeekPos;     // seek position
};

//+------------------------------------------------------------------------
//
//  Return buffered stream if created or the original stream
//
//-------------------------------------------------------------------------

IStream *
BufferStream(IStream * pStm)
{
    CStreamBuffered * pStream = new CStreamBuffered(pStm);
    return pStream ? pStream : pStm;
}

//+------------------------------------------------------------------------
//
//  CStreamBuffered implementation.
//
//-------------------------------------------------------------------------

CStreamBuffered::CStreamBuffered(IStream * pStm)
{
    _pStm = pStm;
    _ulRefs = 1;
    _ulPos = 0;
    _ulSize = 0;
#ifndef _MAC
    _uliSeekPos.QuadPart = 0;
#else
    _uliSeekPos.LowPart = 0;
    _uliSeekPos.HighPart = 0;
#endif
    IncrementSecondaryObjectCount( 6 );
}

CStreamBuffered::~CStreamBuffered()
{
    DecrementSecondaryObjectCount( 6 );
}

STDMETHODIMP
CStreamBuffered::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream))
    {
        *ppvObj = (IStream *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CStreamBuffered::AddRef(void)
{
    ++_ulRefs;
    return _pStm->AddRef();
}

STDMETHODIMP_(ULONG)
CStreamBuffered::Release(void)
{
    ULONG   ul;

    ul = _pStm->Release();
    if (--_ulRefs == 0)
    {
        delete this;
    }

    return ul;
}

#if DBG == 1
static BOOL s_fThru = FALSE;
#endif

STDMETHODIMP
CStreamBuffered::Read(
     void * pv,
     ULONG cb,
     ULONG * pcbRead)
{
    HRESULT hr;
    ULONG ul;
    ULONG ulRead;

#if DBG == 1
    if (s_fThru)
    {
        return _pStm->Read(pv, cb, pcbRead);
    }
#endif
    hr = S_OK;
    ulRead = 0;

    while (cb > 0 && !hr)
    {
        if (_ulPos == _ulSize)
        {
            //  If we need to read more bytes than will fit in our
            //    buffer, then read directly to the stream rather
            //    than trying to buffer.

            if (cb > sizeof(_ab))
            {
                hr = THR(_pStm->Read(pv, cb, &ul));
                ulRead += ul;

                goto Cleanup;
            }

            hr = THR(_pStm->Read(_ab, sizeof(_ab), &ul));

            // get current seek position
            hr = _pStm->Seek(LINULL, STREAM_SEEK_CUR, &_uliSeekPos);

            _ulPos = 0;
            _ulSize = ul;

            if (hr)
            {
                //  We may have attempted to read more bytes from the
                //    stream than actually exist.  If this happens, but
                //    we still read enough bytes to satisfy the user
                //    request, then report success from this call.

                if (ul >= cb)
                {
                    hr = S_OK;
                }
            }
            else
            {
                //  We may hit the end of the stream, and read less than
                //    the necessary number of bytes without getting an
                //    error code back.  In this case, we force an exit
                //    from the loop, but return S_OK as per docfiles.

                if (ul < cb)
                {
                    cb = ul;
                }
            }
        }

        ul = _ulSize - _ulPos;
        if (ul > cb)
        {
            ul = cb;
        }

        memcpy(pv, &_ab[_ulPos], ul);
        pv = (BYTE *)pv + ul;
        cb -= ul;

        _ulPos += ul;
        ulRead += ul;
    }

Cleanup:
    if (pcbRead)
    {
        *pcbRead = ulRead;
    }

    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CStreamBuffered::Write(
     const void * pv,
     ULONG cb,
     ULONG * pcbWritten)
{
    if (pcbWritten)
    {
        *pcbWritten = 0;
    }
    return E_FAIL;
}

STDMETHODIMP
CStreamBuffered::Seek(
     LARGE_INTEGER dlibMove,
     DWORD dwOrigin,
     ULARGE_INTEGER * plibNewPosition)
{

#if 0

    // BUGBUG this is the simple solution, costly seek (istvanc)

    // if buffer is not empty use our position
    if (dwOrigin == STREAM_SEEK_CUR && _ulPos != _ulSize)
    {
        dlibMove.QuadPart += (LONG)_ulPos - (LONG)_ulSize;
    }

    // mark buffer empty
    _ulPos = _ulSize;
    RRETURN(THR(_pStm->Seek(dlibMove, dwOrigin, plibNewPosition)));

#else

    // BUGBUG this is the fast solution but cost a call to get seek pos
    // at read (istvanc)
#ifndef _MAC
    // if buffer is not empty use our position
    if (_ulPos != _ulSize && dwOrigin != STREAM_SEEK_END)
    {
        if (dwOrigin == STREAM_SEEK_CUR)
        {
            dlibMove.QuadPart += (LONG)_ulPos - (LONG)_ulSize;
        }
        else   // dwOrigin == STREAM_SEEK_SET
        {
            dlibMove.QuadPart -= (LONGLONG)_uliSeekPos.QuadPart;
            dwOrigin = STREAM_SEEK_CUR;
        }
        if (dlibMove.QuadPart < 0 && dlibMove.QuadPart >= -(LONG)_ulSize)
        {
            _ulPos = (LONG)_ulSize + (LONG)dlibMove.QuadPart;
            if (plibNewPosition)
            {
                plibNewPosition->QuadPart = _uliSeekPos.QuadPart + dlibMove.QuadPart;
            }
            return S_OK;
        }
    }
#else
// Mac BUGBUG - Someone please check this block! - is it right?

    // if buffer is not empty use our position
    if (_ulPos != _ulSize && dwOrigin != STREAM_SEEK_END)
    {
        if (dwOrigin == STREAM_SEEK_CUR)
        {
            dlibMove.LowPart += (LONG)_ulPos - (LONG)_ulSize;
            if((LONG) dlibMove.LowPart < 0)
                dlibMove.HighPart = -1;
            else
                dlibMove.HighPart = 0;
        }
        else   // dwOrigin == STREAM_SEEK_SET
        {
            dlibMove.LowPart -= _uliSeekPos.LowPart;
            if((LONG) dlibMove.LowPart < 0)
                dlibMove.HighPart = -1;
            else
                dlibMove.HighPart = 0;
            dwOrigin = STREAM_SEEK_CUR;
        }
        if (dlibMove.HighPart < 0 && (LONG)dlibMove.LowPart >= -(LONG)_ulSize)
        {
            _ulPos = (LONG)_ulSize + (LONG)dlibMove.LowPart;
            if (plibNewPosition)
            {
                plibNewPosition->LowPart = _uliSeekPos.LowPart + (LONG)dlibMove.LowPart;
                plibNewPosition->HighPart = 0;
            }
            return S_OK;
        }
    }
#endif
    // mark buffer empty
    _ulPos = _ulSize;
    RRETURN(THR(_pStm->Seek(dlibMove, dwOrigin, plibNewPosition)));

#endif

}

STDMETHODIMP
CStreamBuffered::SetSize(
     ULARGE_INTEGER libNewSize)
{
    return E_FAIL;
}

STDMETHODIMP
CStreamBuffered::CopyTo(
     IStream * pstm,
     ULARGE_INTEGER cb,
     ULARGE_INTEGER * pcbRead,
     ULARGE_INTEGER * pcbWritten)
{
    if (pcbWritten)
    {
#ifndef _MAC
        pcbWritten->QuadPart = 0;
#else
        pcbWritten->LowPart = 0;
        pcbWritten->HighPart = 0;
#endif
    }
    return E_FAIL;
}

STDMETHODIMP
CStreamBuffered::Commit(
     DWORD grfCommitFlags)
{
    return E_FAIL;
}

STDMETHODIMP
CStreamBuffered::Revert( void)
{
    // mark buffer empty
    _ulPos = _ulSize;
    RRETURN(THR(_pStm->Revert()));
}

STDMETHODIMP
CStreamBuffered::LockRegion(
     ULARGE_INTEGER libOffset,
     ULARGE_INTEGER cb,
     DWORD dwLockType)
{
    RRETURN(THR(_pStm->LockRegion(libOffset, cb, dwLockType)));
}

STDMETHODIMP
CStreamBuffered::UnlockRegion(
     ULARGE_INTEGER libOffset,
     ULARGE_INTEGER cb,
     DWORD dwLockType)
{
    RRETURN(THR(_pStm->UnlockRegion(libOffset, cb, dwLockType)));
}

STDMETHODIMP
CStreamBuffered::Stat(
     STATSTG * pstatstg,
     DWORD grfStatFlag)
{
    RRETURN(THR(_pStm->Stat(pstatstg, grfStatFlag)));
}

STDMETHODIMP
CStreamBuffered::Clone(
     IStream ** ppstm)
{
#if 1
    // BUGBUG fix later, fail for now (istvanc)
    *ppstm = NULL;
    return E_FAIL;
#else
    HRESULT hr;

    //  BUGBUG the underlying stream's current pos may not match the
    //    buffering layer's position (chrisz)

    hr = THR(_pStm->Clone(ppstm));
    if (hr)
        goto Cleanup;

    //
    // Don't wrap if already wrapped. We can determine
    // if it is wrapped by looking at the vtable pointer.
    //

    if (**(DWORD**)ppstm == *(DWORD *)this)
        goto Cleanup;

    *ppstm = BufferStream(*ppstm);

Cleanup:
    RRETURN(hr);
#endif
}


