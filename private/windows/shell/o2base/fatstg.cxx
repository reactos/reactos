//+---------------------------------------------------------------------
//
//  File:       fatstg.hxx
//
//  Contents:   IStream on top of a DOS (non-docfile) file
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define _hread _lread
#define _hwrite _lwrite


//REVIEW: this file is substantially incomplete!
//REVIEW: this will either be completed or (hopefully) replaced!

//+---------------------------------------------------------------
//
//  Class:      FatStream
//
//  Purpose:    Provide an IStream interface to a DOS file
//
//---------------------------------------------------------------

class FatStream: public IStream
{
    friend HRESULT CreateStreamOnFile(LPCSTR, DWORD, LPSTREAM FAR* ppstrm);

public:
    DECLARE_STANDARD_IUNKNOWN(FatStream);

    // *** IStream methods ***
    STDMETHOD(Read) (VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead);
    STDMETHOD(Write) (VOID const HUGEP *pv, ULONG cb, ULONG FAR *pcbWritten);
    STDMETHOD(Seek) (LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                    ULARGE_INTEGER FAR *plibNewPosition);
    STDMETHOD(SetSize) (ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (IStream FAR *pstm, ULARGE_INTEGER cb,
                ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten);
    STDMETHOD(Commit) (DWORD grfCommitFlags);
    STDMETHOD(Revert) (void);
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                                                            DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                                                            DWORD dwLockType);
    STDMETHOD(Stat) (STATSTG FAR *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream FAR * FAR *ppstm);

private:
    FatStream(HFILE hfile)
      { _hfile = hfile; }

    ~FatStream()   // destroying the stream closes the file.
      { _lclose(_hfile); }

    HFILE _hfile;
};

//+---------------------------------------------------------------
//
//  Function:   CreateStreamOnFile, public
//
//  Synopsis:   Provides an IStream interface to a DOS file
//
//  Arguments:  [lpstrFile] -- the DOS file
//              [stgm] -- the storage modes for opening
//              [ppstrm] -- where the opened stream is returned
//
//  Returns:    Success iff the stream interface could be provided
//
//----------------------------------------------------------------

HRESULT
CreateStreamOnFile(LPCSTR lpstrFile, DWORD stgm, LPSTREAM FAR* ppstrm)
{
    HRESULT r;
    HFILE hfile;
    OFSTRUCT ofstruct;

    FatStream *pStrm;

    if ((stgm & 0x0000FFFF) != stgm)
    {
#if DBG
    DOUT(L"o2base/fatstg/CreateStreamOnFile E_INVALIDARG\r\n");
#endif
        r = E_INVALIDARG;
    }
    else
    {

        hfile = OpenFile(lpstrFile, &ofstruct, (UINT)(stgm & 0x0000FFFF));
        if (hfile != HFILE_ERROR)
        {

            //pStrm =  new (NullOnFail) FatStream(hfile);
            pStrm =  new FatStream(hfile);
            if (pStrm!=NULL)
            {
                *ppstrm = (IStream *)pStrm;
                r = NOERROR;
            }
            else
            {
                DOUT(L"o2base/fatstg/CreateStreamOnFile failed\r\n");
                r = E_OUTOFMEMORY;
            }

        }
        else
        {
#if DBG
    DOUT(L"o2base/fatstg/CreateStreamOnFile E_FAIL\r\n");
#endif
           r = E_FAIL;
        }
    }
    return(r);
}

IMPLEMENT_STANDARD_IUNKNOWN(FatStream)

//+---------------------------------------------------------------
//
//  Member:     FatStream::QueryInterface
//
//  Synopsis:   method from IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IStream))
    {
        *ppv = (LPVOID)(LPSTREAM)this;
    }
    else
    {
        *ppv = NULL;
#if DBG
    DOUT(L"FatStream::QueryInterface E_NOINTERFACE\r\n");
#endif
        return E_NOINTERFACE;
    }

    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     FatStream::Read
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Read(VOID HUGEP *pv,
        ULONG cb,
        ULONG FAR *pcbRead)
{
    ULONG cbRead = _hread(_hfile, pv, cb);
    if (pcbRead != NULL)
        *pcbRead = cbRead;

    if (cbRead == -1)
    {
#if DBG
    DOUT(L"FatStream::Read E_FAIL\r\n");
#endif
        return E_FAIL;
    }
    else
        return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Write
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Write(VOID const HUGEP *pv,
        ULONG cb,
        ULONG FAR *pcbWritten)
{
    // we have to cast to char ptr because Win32 uses LPCSTR as
    // 2nd argument to _hwrite.
    ULONG cbWritten = _hwrite(_hfile, (char const HUGEP *)pv, cb);
    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

    if (cbWritten == -1)
    {
#if DBG
    DOUT(L"FatStream::Write E_FAIL\r\n");
#endif
        return E_FAIL;
    }
    else
        return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Seek
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Seek(LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER FAR *plibNewPosition)
{
    if (dlibMove.HighPart != 0 && dlibMove.HighPart != -1)
    {
#if DBG
    DOUT(L"FatStream::Seek E_FAIL\r\n");
#endif
        return E_FAIL;
    }

    // cast below is bad under certain circumstances!
    LONG newpos = _llseek(_hfile, (LONG)dlibMove.LowPart, (int)dwOrigin);
    if (plibNewPosition != NULL)
        ULISet32(*plibNewPosition, newpos);

    if (newpos == HFILE_ERROR)
    {
#if DBG
    DOUT(L"FatStream::Seek E_FAIL(2)\r\n");
#endif
        return E_FAIL;
    }
    else
        return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::SetSize
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::SetSize(ULARGE_INTEGER libNewSize)
{
    if (libNewSize.HighPart != 0)
    {
#if DBG
    DOUT(L"FatStream::SetSize E_FAIL\r\n");
#endif
        return E_FAIL;
    }

    // below is a bad cast under certain circumstances!
    if (_chsize(_hfile, (long)libNewSize.LowPart) == 0)
    {
        return NOERROR;
    }
    else
    {
#if DBG
    DOUT(L"FatStream::SetSize E_FAIL(2)\r\n");
#endif
        return E_FAIL;
    }
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::CopyTo
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::CopyTo(IStream FAR *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER FAR *pcbRead,
        ULARGE_INTEGER FAR *pcbWritten)
{
#if DBG
    DOUT(L"FatStream::CopyTo E_NOTIMPL\r\n");
#endif
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Commit
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Commit(DWORD grfCommitFlags)
{
#if DBG
    DOUT(L"FatStream::Commit STG_E_INVALIDFUNCTION\r\n");
#endif
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Revert
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Revert(void)
{
#if DBG
    DOUT(L"FatStream::Revert STG_E_INVALIDFUNCTION\r\n");
#endif
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::LockRegion
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::LockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
#if DBG
    DOUT(L"FatStream::LockRegion STG_E_INVALIDFUNCTION\r\n");
#endif
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::UnlockRegion
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP FatStream::UnlockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
#if DBG
    DOUT(L"FatStream::UnLockRegion STG_E_INVALIDFUNCTION\r\n");
#endif
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Stat
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Stat(STATSTG FAR *pstatstg, DWORD grfStatFlag)
{

    if (pstatstg != NULL)
    {
        pstatstg->pwcsName = NULL;
        pstatstg->type = STGTY_STREAM;
        ULISet32(pstatstg->cbSize,_filelength(_hfile));
        //pstatstg->mtime = fstatus.m_mtime;
        //pstatstg->ctime = fstatus.m_ctime;
        //pstatstg->atime = fstatus.m_atime;
        //pstatstg->grfMode = ;
        pstatstg->grfLocksSupported = 0;     // no locking supported
    }

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Clone
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Clone(IStream FAR * FAR *ppstm)
{
#if DBG
    DOUT(L"FatStream::Clone E_NOTIMPL\r\n");
#endif
    return E_NOTIMPL;
}

#if 0
//REVIEW:  this could get integrated into CopyTo!
//+---------------------------------------------------------------
//
//  Function:   CopyFileIntoStream
//
//---------------------------------------------------------------
void
CopyFileIntoStream( LPSTREAM lpStream, int fh )
{
    int sChunk = 4096;
    //LPBYTE pbBuffer = new (NullOnFail) BYTE[sChunk];
    LPBYTE pbBuffer = new BYTE[sChunk];
    for(int i = 0; i < 8; i++)
    {
        if(pbBuffer != NULL)
            break;
        sChunk >>= 1;
        //pbBuffer = new (NullOnFail) BYTE[sChunk];
        pbBuffer = new BYTE[sChunk];
    }
    if(pbBuffer == NULL)
        return; // fatal error condition
    int sRead;
    ULONG ulWritten;
    for( ; (sRead = _lread( fh, pbBuffer, sChunk )) >= 0; )
    {
        lpStream->Write( (VOID FAR *)pbBuffer, (ULONG)sRead, &ulWritten );
        if(sRead < sChunk)
        {
            break;
        }
    }
    if(pbBuffer != NULL)
        delete pbBuffer;
}
#endif
