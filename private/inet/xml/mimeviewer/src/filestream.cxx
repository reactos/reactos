/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop
#include "utils.hxx"

//+---------------------------------------------------------------
//
//  Class:      FatStream
//
//  Purpose:    Provide an IStream interface to a DOS file
//
//---------------------------------------------------------------

class FatStream: public IStream
{
    friend HRESULT CreateStreamOnFile(LPCTSTR, DWORD, IStream **ppstrm);
private:
    long    _ulRefs;
    HANDLE  _hfile;
public:
    // ============= IUnknown ============================
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // ============= ISequentialStream ====================
    virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);

    // ============= IStream ==============================
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
    virtual HRESULT STDMETHODCALLTYPE Revert(void);
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);

private:
    FatStream() 
    {
        _ulRefs = 1;
        _hfile = INVALID_HANDLE_VALUE;
    }
    ~FatStream()
     {
        if (_hfile != INVALID_HANDLE_VALUE)
            CloseHandle(_hfile);
     }
    HRESULT Init(HANDLE hfile);
};

//+---------------------------------------------------------------
//
//  Function:   CreateStreamOnFile, public
//
//  Synopsis:   Provides an IStream interface to a DOS file
//
//  Arguments:  [pchFile] -- the DOS file
//              [dwDesiredAccess] -- see CreateFile
//              [dwShareMode] -- see CreateFile
//              [dwCreationDistribution] -- see CreateFile
//              [ppstrm] -- where the opened stream is returned
//
//  Returns:    Success iff the stream interface could be provided
//
//----------------------------------------------------------------

HRESULT
CreateStreamOnFile(LPCTSTR pchFile, DWORD dwSTGM, LPSTREAM * ppstrm)
{
    DWORD       dwDesiredAccess = 0;
    DWORD       dwShareMode = 0;
    DWORD       dwCreationDistribution = 0;
    HANDLE      hfile;
    FatStream * pStmFat = NULL;
    TCHAR       pchtFile[MAX_PATH];
    HRESULT     hr = S_OK;

    *ppstrm = NULL;

    // BUGBUG really should support this eventually for full implementation
    if (dwSTGM & STGM_DELETEONRELEASE)
        return E_NOTIMPL;

    if (dwSTGM & STGM_READWRITE)
        dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);
    else if (dwSTGM & STGM_WRITE)
        dwDesiredAccess |= GENERIC_WRITE;
    else
        dwDesiredAccess |= GENERIC_READ;

    if (dwSTGM & STGM_SHARE_DENY_NONE)
        dwShareMode |= (FILE_SHARE_READ|FILE_SHARE_WRITE);
    else if (dwSTGM & STGM_SHARE_DENY_WRITE)
        dwShareMode |= FILE_SHARE_READ;
    else if (dwSTGM & STGM_SHARE_DENY_READ)
        dwShareMode |= FILE_SHARE_WRITE;

    if (dwSTGM & STGM_CREATE)
        dwCreationDistribution |= CREATE_ALWAYS;
    else
        dwCreationDistribution |= OPEN_EXISTING;

    hfile = CreateFile(
            pchFile,
            dwDesiredAccess,
            dwShareMode,
            NULL,
            dwCreationDistribution,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hfile==INVALID_HANDLE_VALUE)
    {
        hr = ::GetLastError();
        goto CleanUp;
    }

    pStmFat = new_ne FatStream();
    if (!pStmFat)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    hr = pStmFat->Init(hfile);
    CHECKHR(hr);
    hr = pStmFat->QueryInterface(IID_IStream, (void **)ppstrm);
    CHECKHR(hr);

CleanUp:
    SafeRelease(pStmFat);
    return hr;
}


HRESULT STDMETHODCALLTYPE 
FatStream::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IUnknown || riid == IID_ISequentialStream || riid == IID_IStream)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE 
FatStream::AddRef(void)
{
    return InterlockedIncrement(&_ulRefs);
}

ULONG STDMETHODCALLTYPE 
FatStream::Release(void)
{
    if (InterlockedDecrement(&_ulRefs) == 0)
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}


//+---------------------------------------------------------------
//
//  Member:     FatStream::Init
//
//  Synopsis:   Initialize the class.
//
//----------------------------------------------------------------

HRESULT
FatStream::Init(HANDLE hfile)
{
    _hfile = hfile;
    return (hfile == INVALID_HANDLE_VALUE) ? E_FAIL : S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     FatStream::Read
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Read(void HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{
    ULONG   cbTemp;
    return (ReadFile(_hfile, pv, cb, pcbRead ? pcbRead : &cbTemp, NULL) ? S_OK : ::GetLastError());
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Write
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Write(void const HUGEP *pv,
        ULONG cb,
        ULONG FAR *pcbWritten)
{
    BOOL    fSuccess;
    DWORD   cbWritten;

    fSuccess = WriteFile(_hfile, pv, (DWORD) cb, &cbWritten, NULL);
    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;
    return (fSuccess ? S_OK : ::GetLastError());
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
    DWORD   newpos;

    if (dlibMove.HighPart != 0 && dlibMove.HighPart != -1)
        return E_FAIL;

    newpos = SetFilePointer(_hfile, (LONG) dlibMove.LowPart, NULL, (int) dwOrigin);

    if (plibNewPosition != NULL)
        ULISet32(*plibNewPosition, (LONG) newpos);

    return(newpos == -1 ? E_FAIL : S_OK);
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
    DWORD   newpos;

    if (libNewSize.HighPart != 0)
        return E_FAIL;

    newpos = SetFilePointer(_hfile, (LONG) libNewSize.LowPart, NULL, FILE_BEGIN);
    if (newpos == -1)
        return ::GetLastError();

    if (!SetEndOfFile(_hfile))
        return ::GetLastError();

    return S_OK;
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
    DWORDLONG   cbRequest = *(LONGLONG *) &cb;
    DWORDLONG   cbActual = 0;
    HRESULT     hr = S_OK;

    while (cbRequest)
    {
        BYTE    buf[4096];
        ULONG   cbAttempt = (ULONG)(min(cbRequest, (DWORDLONG) sizeof(buf)));
        ULONG   cbRead;

        hr = Read(buf, cbAttempt, &cbRead);
        CHECKHR(hr);
        hr = pstm->Write(buf, cbRead, NULL);
        CHECKHR(hr);

        cbActual += cbRead;
        cbRequest -= cbRead;

        if (cbAttempt > cbRead)
            break;
    }

    if (pcbRead)
        *pcbRead = *(ULARGE_INTEGER *) &cbActual;
    if (pcbWritten)
        *pcbWritten = *(ULARGE_INTEGER *) &cbActual;

CleanUp:
    return hr;
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
    HRESULT     hr = S_OK;

    if (pstatstg != NULL)
    {
        pstatstg->pwcsName = NULL;
        pstatstg->type = STGTY_STREAM;
        ULISet32(pstatstg->cbSize,(LONG) GetFileSize(_hfile, NULL));
        pstatstg->grfLocksSupported = 0;     // no locking supported
    }

Cleanup:
    return (hr);
}

//+---------------------------------------------------------------
//
//  Member:     FatStream::Clone
//
//  Synopsis:   method of IStream interface
//
//----------------------------------------------------------------

STDMETHODIMP
FatStream::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}

