//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: stream.c
//
//  This file contains some of the stream support code that is used by
// the shell.  It also contains the shells implementation of a memory
// stream that is used by the cabinet to allow views to be serialized.
//
// History:
//  08-20-93 KurtE      Added header block and memory stream.
//
//---------------------------------------------------------------------------

#include "priv.h"
//#include "shlobj.h"     // For IToClass
#include <new.h>
#include "nullstm.h"

// This code was stolen from shell32.  This is the BETTER_STRONGER_FASTER
// version (smaller and half the allocs), added after Win95 shipped.
#include "stream.h"

EXTERN_C HKEY SHRegDuplicateHKey(HKEY hkey);

// The Win95/NT4/IE4 code did not enforce the grfMode.  Turn this on to enforce:
//#define ENFORCE_GRFMODE // Note: I haven't tested compat issues with this turned on yet... [mikesh]


STDMETHODIMP CMemStream::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IStream) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj=this;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CMemStream::AddRef()
{
    this->cRef++;
    return this->cRef;
}

BOOL CMemStream::WriteToReg()
{
    if (this->cbData)
    {
        return ERROR_SUCCESS == RegSetValueEx(this->hkey, 
            this->szValue[0] ? this->szValue : NULL, 0, REG_BINARY, 
            this->cbData ? this->pBuf : (LPBYTE)"", this->cbData);
    }
    else
    {
        DWORD dwRet = SHDeleteValue(this->hkey, NULL, this->szValue);

        // If the Stream is being stored in the default key, then
        // we should clean up the key. Otherwise, the caller
        // passed us the key, and they need it. It would be rude for us
        // to delete it. Fixes a Start Menu bug (NT#361333) where we would delete the
        // programs key where start menu stores it's stuff on a load, so we 
        // never persist anything. - lamadio (6.25.99)
        if (this->szValue[0] == TEXT('\0'))
        {
            SHDeleteEmptyKey(this->hkey, NULL);
        }

        return ERROR_SUCCESS == dwRet;
    }
}

STDMETHODIMP_(ULONG) CMemStream::Release()
{
    this->cRef--;
    if (this->cRef > 0)
        return this->cRef;

    // If this is backed up by the registry serialize the data
    if (this->hkey)
    {
        // Backed by the registry.
        // Write and cleanup.
        WriteToReg();
        RegCloseKey(this->hkey);
    }

    // Free the data buffer that is allocated to the stream
    if (this->pBuf)
        LocalFree(this->pBuf);

    LocalFree((HLOCAL)this);

    return 0;
}


STDMETHODIMP CMemStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
#ifdef ENFORCE_GRFMODE
    if ((this->grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_WRITE)
    {
        if (pcbRead != NULL)
            *pcbRead = 0;
        return STG_E_ACCESSDENIED;
    }
#endif

    ASSERT(pv);

    // I guess a null read is ok.
    if (!cb)
    {
        if (pcbRead != NULL)
            *pcbRead = 0;
        return S_OK;
    }

    if (this->iSeek >= this->cbData)
    {
        if (pcbRead != NULL)
            *pcbRead = 0;   // nothing read
    }

    else
    {
        if ((this->iSeek + cb) > this->cbData)
            cb = this->cbData - this->iSeek;

        // Now Copy the memory
        ASSERT(this->pBuf);
        CopyMemory(pv, this->pBuf + this->iSeek, cb);
        this->iSeek += (UINT)cb;

        if (pcbRead != NULL)
            *pcbRead = cb;
    }

    return S_OK;
}

LPBYTE CMemStream::GrowBuffer(ULONG cbNew)
{
    if (this->pBuf == NULL)
    {
        this->pBuf = (LPBYTE)LocalAlloc(LPTR, cbNew);
    }
    else
    {
        LPBYTE pTemp = (LPBYTE)LocalReAlloc(this->pBuf, cbNew, LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (pTemp)
        {
            this->pBuf = pTemp;
        }
        else
        {
            TraceMsg(TF_ERROR, "Stream buffer realloc failed");
            return NULL;
        }
    }
    if (this->pBuf)
        this->cbAlloc = cbNew;

    return this->pBuf;
}

#define SIZEINCR    0x1000


STDMETHODIMP CMemStream::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
#ifdef ENFORCE_GRFMODE
    if ((this->grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_READ)
    {
        if (pcbWritten != NULL)
            *pcbWritten = 0;
        return STG_E_ACCESSDENIED;
    }
#endif

    // I guess a null write is ok.
    if (!cb)
    {
        if (pcbWritten != NULL)
            *pcbWritten = 0;
        return S_OK;
    }

    // See if the data will fit into our current buffer
    if ((this->iSeek + cb) > this->cbAlloc)
    {
        // enlarge the buffer
        // Give it a little slop to avoid a lot of reallocs.
        if (GrowBuffer(this->iSeek + (UINT)cb + SIZEINCR) == NULL)
            return STG_E_INSUFFICIENTMEMORY;
    }

    ASSERT(this->pBuf);

    // See if we need to fill the area between the data size and
    // the seek position
    if (this->iSeek > this->cbData)
    {
        ZeroMemory(this->pBuf + this->cbData, this->iSeek - this->cbData);
    }

    CopyMemory(this->pBuf + this->iSeek, pv, cb);
    this->iSeek += (UINT)cb;
    if (this->iSeek > this->cbData)
        this->cbData = this->iSeek;

    if (pcbWritten != NULL)
        *pcbWritten = cb;

    return S_OK;
}

STDMETHODIMP CMemStream::Seek(LARGE_INTEGER dlibMove,
               DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    LONG lNewSeek;

    // Note: curently not testing for error conditions for number wrap...
    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        lNewSeek = (LONG)dlibMove.LowPart;
        break;
    case STREAM_SEEK_CUR:
        lNewSeek = (LONG)this->iSeek + (LONG)dlibMove.LowPart;
        break;
    case STREAM_SEEK_END:
        lNewSeek = (LONG)this->cbData + (LONG)dlibMove.LowPart;
        break;
    default:
        return STG_E_INVALIDPARAMETER;
    }

    if (lNewSeek < 0)
        return STG_E_INVALIDFUNCTION;

    this->iSeek = (UINT)lNewSeek;

    if (plibNewPosition != NULL)
    {
        plibNewPosition->LowPart = (DWORD)lNewSeek;
        plibNewPosition->HighPart = 0;
    }
    return S_OK;
}

STDMETHODIMP CMemStream::SetSize(ULARGE_INTEGER libNewSize)
{
#ifdef ENFORCE_GRFMODE
    if ((this->grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_READ)
    {
        return STG_E_ACCESSDENIED;
    }
#endif

    UINT cbNew = (UINT)libNewSize.LowPart;

    // See if the data will fit into our current buffer
    if (cbNew > this->cbData)
    {
        // See if we have to Enlarge the buffer.
        if (cbNew > this->cbAlloc)
        {
            // enlarge the buffer - Does not check wrap...
            // Give it a little slop to avoid a lot of reallocs.
            if (GrowBuffer(cbNew) == NULL)
                return STG_E_INSUFFICIENTMEMORY;
        }

        // Now fill some memory
        ZeroMemory(this->pBuf + this->cbData, cbNew - this->cbData);
    }

    // Save away the new size.
    this->cbData = cbNew;
    return S_OK;
}

STDMETHODIMP CMemStream::CopyTo(IStream *pstmTo,
             ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
#ifdef ENFORCE_GRFMODE
    if ((this->grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_WRITE)
    {
        if (pcbRead != NULL)
            ZeroMemory(pcbRead, SIZEOF(pcbRead));
        if (pcbWritten != NULL)
            ZeroMemory(pcbWritten, SIZEOF(pcbWritten));
        return STG_E_ACCESSDENIED;
    }
#endif

    HRESULT hres = S_OK;
    UINT cbRead = this->cbData - this->iSeek;
    ULONG cbWritten = 0;

    if (cb.HighPart == 0 && cb.LowPart < cbRead)
    {
        cbRead = cb.LowPart;
    }

    if (cbRead > 0)
    {
        hres = pstmTo->Write(this->pBuf + this->iSeek, cbRead, &cbWritten);
        this->iSeek += cbRead;
    }

    if (pcbRead)
    {
        pcbRead->LowPart = cbRead;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten)
    {
        pcbWritten->LowPart = cbWritten;
        pcbWritten->HighPart = 0;
    }

    return hres;
}

STDMETHODIMP CMemStream::Commit(DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMemStream::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CMemStream::LockRegion(ULARGE_INTEGER libOffset,
                 ULARGE_INTEGER cb, DWORD dwLockType)

{
    return E_NOTIMPL;
}

STDMETHODIMP CMemStream::UnlockRegion(ULARGE_INTEGER libOffset,
                 ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

// Trident calls this to determine the size of the structure.
// No reason to not support this one.
STDMETHODIMP CMemStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    ZeroMemory(pstatstg, SIZEOF(*pstatstg));

    // we have no name
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.LowPart = this->cbData;
    // blow off modify, create, access times (we don't track anyway)
    pstatstg->grfMode = this->grfMode;
    // we're not transacting, so we have no lock modes
    // we're the null clsid already
    // we're not based on storage, so we have no state or storage bits
    
    return S_OK;
}

STDMETHODIMP CMemStream::Clone(IStream **ppstm)
{
    *ppstm = NULL;
    return E_NOTIMPL;
}

CMemStream *
CreateMemStreamEx(
    LPBYTE  pInit, 
    UINT    cbInit, 
    LPCTSTR pszValue)       OPTIONAL
{
    UINT l_cbAlloc = SIZEOF(CMemStream) + (pszValue ? lstrlen(pszValue) * SIZEOF(TCHAR) : 0);
    CMemStream *localthis = (CMemStream *)LocalAlloc(LPTR, l_cbAlloc);
    if (localthis) 
    {
        new (localthis) CMemStream;

        localthis->cRef = 1;

        // See if there is some initial data we should map in here.
        if ((pInit != NULL) && (cbInit > 0))
        {
            if (localthis->GrowBuffer(cbInit) == NULL)
            {
                // Could not allocate buffer!
                LocalFree((HLOCAL)localthis);
                return NULL;
            }

            localthis->cbData = cbInit;
            CopyMemory(localthis->pBuf, pInit, cbInit);
        }

        if (pszValue)
            lstrcpy(localthis->szValue, pszValue);

        // We have no other value to set this to
        localthis->grfMode = STGM_READWRITE;

        return localthis;
    }
    return NULL;
}


STDAPI_(IStream *)
SHCreateMemStream(
    LPBYTE  pInit, 
    UINT    cbInit)
{
    CMemStream *localthis = CreateMemStreamEx(pInit, cbInit, NULL);
    if (localthis) 
        return localthis;
    return NULL;
}


//----------------------------------------------------------------------------
// Open a stream to the reg file given an open key.
// NB pszValue can be NULL.
//
// Win9x exported OpenRegStream which *always* returned a stream, even for read,
// even when there was no data there.  IE4 shell32 delegated to shlwapi's SHOpenRegStream
// which needs to support this sub-optimal behavior.  See NT5 bug 190878 (shell32 fault).
//
STDAPI_(IStream *)
SHOpenRegStreamW(
    HKEY    hkey, 
    LPCWSTR  pszSubkey, 
    LPCWSTR  pszValue,       OPTIONAL
    DWORD   grfMode)
{
    IStream * pstm = SHOpenRegStream2W(hkey, pszSubkey, pszValue, grfMode);
#ifndef UNIX
    if (!pstm)
        pstm = SHConstNullStream();
#endif
    return pstm;
}

STDAPI_(IStream *)
SHOpenRegStreamA(
    HKEY    hkey, 
    LPCSTR  pszSubkey, 
    LPCSTR  pszValue,       OPTIONAL
    DWORD   grfMode)
{
    IStream * pstm = SHOpenRegStream2A(hkey, pszSubkey, pszValue, grfMode);
#ifndef UNIX
    if (!pstm)
        pstm = SHConstNullStream();
#endif
    return pstm;
}

// We should add STGM_CREATE support to the shlwapi streams.  When saving out 
// streams, we currently create the stream with STGM_WRITE (but not STGM_CREATE) 
// so shlwapi goes to all the wasted trouble of reading the old stream data into 
// memory, only to throw it away when we write over it.
// 
// STGM_CREATE means "I don't care about the old values because I'm going to 
// overwrite them anyway."  (It really should be named STGM_TRUNCATEONOPEN.)
// 
STDAPI_(IStream *)
SHOpenRegStream2(
    HKEY    hkey, 
    LPCTSTR pszSubkey, 
    LPCTSTR pszValue,       OPTIONAL
    DWORD   grfMode)
{
    CMemStream *localthis;    // In bed with class...

    ASSERT(IS_VALID_HANDLE(hkey, KEY));
    ASSERT(IS_VALID_STRING_PTR(pszSubkey, -1));

    // Null keys are illegal.
    if (!hkey)
    {
        TraceMsg(TF_ERROR, "OpenRegStream: Invalid key.");
        return NULL;
    }

    localthis = CreateMemStreamEx(NULL, 0, pszValue);
    if (!localthis)
        return NULL;       // Failed to allocate space

    localthis->grfMode = grfMode;

    // Get the hkey we're going to deal with
    //
    // Did the caller pass us a subkey, and does it contain a string?
    if (pszSubkey && *pszSubkey)
    {
        // Yes; The try to bind to that key.

        // If this stream is one the user mentioned as wanting to write to
        // we need to save away the regkey and value.
        if ((grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_READ)
        {
            // Store away the key.
            if (RegCreateKey(hkey, pszSubkey, &localthis->hkey) != ERROR_SUCCESS)
            {
                TraceMsg(TF_ERROR, "SHOpenRegStream: Unable to create key.");
                localthis->hkey = NULL; // be paranoid
            }
        }
        else if (RegOpenKey(hkey, pszSubkey, &localthis->hkey) != ERROR_SUCCESS)
        {
            localthis->hkey = NULL; // be paranoid
        }
    }
    else
    {
        localthis->hkey = SHRegDuplicateHKey(hkey);
    }

    // we don't have an hkey, bail
    if (NULL == localthis->hkey)
    {
        localthis->Release();
        return NULL;
    }


    // Now see if we need to initialize the stream.
    if ((grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_WRITE)
    {
        DWORD dwType;
        DWORD cbData;

        if ((RegQueryValueEx(localthis->hkey, pszValue, NULL, &dwType, NULL, &cbData) == ERROR_SUCCESS) && cbData)
        {
            if (localthis->GrowBuffer(cbData) != NULL)
            {
                ASSERT(localthis->cbAlloc >= cbData);

                // Get the data.
                RegQueryValueEx(localthis->hkey, pszValue, NULL, &dwType, localthis->pBuf, &cbData);

                ASSERT(localthis->cbAlloc >= cbData);

                localthis->cbData = cbData;
            }
            else
            {
                TraceMsg(TF_ERROR, "OpenRegStream: Unable to initialize stream to registry.");
                localthis->Release();
                return NULL;
            }
        }
    }

    return localthis;
}

#ifdef UNICODE
STDAPI_(IStream *)
SHOpenRegStream2A(
    HKEY    hkey, 
    LPCSTR  pszSubkey, 
    LPCSTR  pszValue,       OPTIONAL
    DWORD   grfMode)
{
    IStream * pstm = NULL;

    ASSERT(IS_VALID_HANDLE(hkey, KEY));
    ASSERT(IS_VALID_STRING_PTRA(pszSubkey, -1));

    if (pszSubkey)
    {
        WCHAR wszSubkey[MAX_PATH];
        WCHAR wszValue[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, pszSubkey, -1, wszSubkey, SIZECHARS(wszSubkey));
        pszSubkey = (LPCSTR)wszSubkey;

        if (pszValue)
        {
            MultiByteToWideChar(CP_ACP, 0, pszValue, -1, wszValue, SIZECHARS(wszValue));
            pszValue = (LPCSTR)wszValue;
        }

        pstm = SHOpenRegStream2W(hkey, (LPCWSTR)pszSubkey, (LPCWSTR)pszValue, grfMode);
    }

    return pstm;
}
#else
STDAPI_(IStream *)
SHOpenRegStream2W(
    HKEY    hkey, 
    LPCWSTR pszSubkey, 
    LPCWSTR pszValue,       OPTIONAL
    DWORD   grfMode)
{
    IStream * pstm = NULL;

    ASSERT(IS_VALID_HANDLE(hkey, KEY));
    ASSERT(IS_VALID_STRING_PTRW(pszSubkey, -1));

    if (pszSubkey)
    {
        CHAR szSubkey[MAX_PATH];
        CHAR szValue[MAX_PATH];

        WideCharToMultiByte(CP_ACP, 0, pszSubkey, -1, szSubkey, SIZECHARS(szSubkey), NULL, NULL);
        pszSubkey = (LPCWSTR)szSubkey;

        if (pszValue)
        {
            WideCharToMultiByte(CP_ACP, 0, pszValue, -1, szValue, SIZECHARS(szValue), NULL, NULL);
            pszValue = (LPCWSTR)szValue;
        }

        pstm = SHOpenRegStream2A(hkey, (LPCSTR)pszSubkey, (LPCSTR)pszValue, grfMode);
    }

    return pstm;
}
#endif // UNICODE

