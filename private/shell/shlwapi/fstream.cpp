//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1996
//
// File: fstream.cpp
//
//  implements a IStream on a DOS file.
//
//---------------------------------------------------------------------------

#include "priv.h"
#include "native.h"
#include "fstream.h"

//
// Class definition
//

CFileStream::CFileStream(HANDLE hf, DWORD grfMode)
{
    this->cRef = 1;
    this->hFile = hf;
    this->fWrite = (grfMode & STGM_WRITE);

    if (this->fWrite)
    {
        this->ib = 0;
    }
    else
    {
        this->cbBufLen = 0;
        this->ib = 0;
    }
}

CFileStream::~CFileStream()
{
    if (this->fWrite)
        Commit(0);

    ASSERT(this->hFile != INVALID_HANDLE_VALUE);
    CloseHandle(hFile);
}

STDMETHODIMP CFileStream::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IStream) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    this->cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CFileStream::AddRef()
{
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CFileStream::Release()
{
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    delete this;
    return 0;
}

//
// Member: CFileStream::Read
//
STDMETHODIMP CFileStream::Read(VOID *pv, ULONG cb, ULONG *pcbRead)
{
    ULONG cbReadRequestSize = cb;
    ULONG cbT, cbRead;
    HRESULT hres = NOERROR;

    while (cb > 0)
    {
        // Assert if we are beyond the bufferlen and Not sizeof(this->ab) which
        // would imply a seek happened...
        ASSERT((this->ib <= this->cbBufLen) || (this->ib == sizeof(this->ab)));

        if (this->ib < this->cbBufLen)
        {
            cbT = this->cbBufLen - this->ib;

            if (cbT > cb)
                cbT = cb;

            memcpy(pv, &this->ab[this->ib], cbT);
            this->ib += cbT;
            cb -= cbT;

            if (cb == 0)
                break;

            (BYTE *&)pv += cbT;
        }

        // Buffer's empty.  Handle rest of large reads directly...
        //
        if (cb > sizeof(this->ab))
        {
            cbT = cb - cb % sizeof(this->ab);
            if (!ReadFile(this->hFile, pv, cbT, &cbRead, NULL))
            {
                DebugMsg(DM_TRACE, TEXT("Stream read IO error %d"), GetLastError());
                hres = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            cb -= cbRead;
            (BYTE *&)pv += cbRead;

            if (cbT != cbRead)
                break;          // end of file
        }

        if (cb == 0)
            break;

        // was the last read a partial read?  if so we are done
        //
        if (this->cbBufLen > 0 && this->cbBufLen < sizeof(this->ab))
        {
            // DebugMsg(DM_TRACE, "Stream is empty");
            break;
        }

        // Read an entire buffer's worth.  We may try to read past EOF,
        // so we must only check for != 0...
        //
        if (!ReadFile(this->hFile, this->ab, sizeof(this->ab), &cbRead, NULL))
        {
            DebugMsg(DM_TRACE, TEXT("Stream read IO error 2 %d"), GetLastError());
            hres = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if (cbRead == 0)
            break;

        this->ib = 0;
        this->cbBufLen = cbRead;
    }

    if (pcbRead)
        *pcbRead = cbReadRequestSize - cb;

    if (cb != 0)
    {
        // DebugMsg(DM_TRACE, "CFileStream::Read() incomplete read");
        hres = S_FALSE; // still success! but not completely
    }

    return hres;
}

//
// Member: CFileStream::Write
//
STDMETHODIMP CFileStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    ULONG cbRequestedWrite = cb;
    ULONG cbT;
    HRESULT hres = NOERROR;

//  DebugMsg(DM_TRACE, "CFileStream::Write(%d bytes)", cb);

    while (cb > 0)
    {
        if (this->ib < sizeof(this->ab))
        {
            cbT = min((ULONG)(sizeof(this->ab) - this->ib), cb);

            memcpy(&this->ab[this->ib], pv, cbT);
            this->ib += cbT;
            cb -= cbT;

            if (cb == 0)
                break;

            (BYTE *&)pv += cbT;
        }

        hres = Commit(0);
        if (FAILED(hres))
            break;

        if (cb > sizeof(this->ab))
        {
            ULONG cbWrite;

            cbT = cb - cb % sizeof(this->ab);

            if (!WriteFile(this->hFile, pv, cbT, &cbWrite, NULL))
            {
                DebugMsg(DM_TRACE, TEXT("Stream write IO error 2, %d"), GetLastError());
                hres = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            cb -= cbWrite;
            (BYTE *&)pv += cbWrite;

            if (cbWrite != cbT)
                break;          // media full, we are done
        }
    }

    if (pcbWritten)
        *pcbWritten = cbRequestedWrite - cb;

    if ((cb != 0) && (hres == NOERROR))
    {
        DebugMsg(DM_TRACE, TEXT("CFileStream::Write() incomplete"));
        hres = S_FALSE; // still success! but not completely
    }

    return hres;
}

//
// Member: CFileStream::Seek
//
STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    if (fWrite)
        Commit(0);
    else
    {
        this->ib = sizeof(this->ab);
        this->cbBufLen = 0;     // Say we have not read it yet.
    }

    // HighPart should be 0, unless its been set to 0xFFFFFFFF to
    // indicate "read everything"

    ASSERT(dlibMove.HighPart == 0 || dlibMove.HighPart == (ULONG) -1);

    COMPILETIME_ASSERT(FILE_BEGIN   == STREAM_SEEK_SET);
    COMPILETIME_ASSERT(FILE_CURRENT == STREAM_SEEK_CUR);
    COMPILETIME_ASSERT(FILE_END     == STREAM_SEEK_END);

    DWORD dwCurrent = SetFilePointer(hFile, dlibMove.LowPart, NULL, dwOrigin);

    if (plibNewPosition)
    {
        plibNewPosition->LowPart = dwCurrent;
        plibNewPosition->HighPart = 0;
    }

    return NOERROR;
}

//
// Member: CFileStream::SetSize
//
STDMETHODIMP CFileStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

//
// REVIEW: this could use the internal buffer in the stream to avoid
// extra buffer copies.
//
STDMETHODIMP CFileStream::CopyTo(IStream *pstmTo, ULARGE_INTEGER cb,
             ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    BYTE buf[512];
    ULONG cbRead;
    HRESULT hres = NOERROR;

    if (pcbRead)
    {
        pcbRead->LowPart = 0;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten)
    {
        pcbWritten->LowPart = 0;
        pcbWritten->HighPart = 0;
    }

    ASSERT(cb.HighPart == 0);

    while (cb.LowPart)
    {
        hres = this->Read(buf, min(cb.LowPart, SIZEOF(buf)), &cbRead);

        if (pcbRead)
            pcbRead->LowPart += cbRead;

        if (FAILED(hres) || (cbRead == 0))
            break;

        cb.LowPart -= cbRead;

        hres = pstmTo->Write(buf, cbRead, &cbRead);

        if (pcbWritten)
            pcbWritten->LowPart += cbRead;

        if (FAILED(hres) || (cbRead == 0))
            break;
    }

    return hres;
}

//
// Member: CFileStream::Commit
//
STDMETHODIMP CFileStream::Commit(DWORD grfCommitFlags)
{
    if (this->fWrite)
    {
        if (this->ib > 0)
        {
            DWORD cbWrite;
            WriteFile(hFile, this->ab, this->ib, &cbWrite, NULL);
            if (cbWrite != this->ib)
            {
                DebugMsg(DM_TRACE, TEXT("CFileStream::Commit() incompleate write %d"), GetLastError());
                return STG_E_MEDIUMFULL;
            }
            this->ib = 0;
        }
    }
    return NOERROR;
}

STDMETHODIMP CFileStream::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Clone(IStream * *ppstm)
{
    return E_NOTIMPL;
}

// create an IStream from a Win32 file name.
// in:
//      pszFile     file name to open
//      grfMode     STGM_ flags
//

// We export a W version of this function
//
LWSTDAPI SHCreateStreamOnFileW(LPCWSTR pwszFile, DWORD grfMode, IStream** ppstm)
{
    if ( g_bRunningOnNT )
    {
        return _CreateStreamOnFileW(pwszFile, grfMode, ppstm);
    }
    else
    {
        CHAR szFile[MAX_PATH];

        SHUnicodeToAnsi(pwszFile, szFile, ARRAYSIZE(szFile));
        return _CreateStreamOnFileA(szFile, grfMode, ppstm);
    }
}

// We export an A version of this function
//
LWSTDAPI SHCreateStreamOnFileA(LPCSTR pszFile, DWORD grfMode, IStream** ppstm)
{
    return _CreateStreamOnFileA(pszFile, grfMode, ppstm);
}
