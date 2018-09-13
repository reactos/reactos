//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       regstm.cxx
//
//  Contents:   stream on registry - and CMemStream implementation
//
//  Classes:
//
//  Functions:
//
//  History:    1-15-1997   JohannP (Johann Posch)   taken from webcheck
//                                                   and modified
//
//----------------------------------------------------------------------------
#include <notiftn.h>


CMemStream::CMemStream(BOOL fNewStream)
{
    // We don't start dirty
    _fDirty = FALSE;
    _pstm = NULL;
    _fNewStream = fNewStream;
    _fError = FALSE;

    if (fNewStream)
    {
        // Create new stream
        if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &_pstm)))
        {
            _pstm = NULL;
            _fError = TRUE;
        }
    }
}

CMemStream::~CMemStream()
{
    DWORD   dwSize = 0;

    // Free up stream
    if (_pstm)
    {
        _pstm->Release();
    }

#ifdef DEBUG
    if (_fDirty)
        NotfAssert(( FALSE &&"CMemStream destructor with dirty data");
    if (_fError)
        NotfAssert(( FALSE &&"CMemStream encountered read/write error");
#endif
}

HRESULT CMemStream::CopyToStream(IStream *pStm)
{
    char *  pData;
    DWORD   dwSize = 0;
    HGLOBAL hGlobal=NULL;

    if (_pstm)
    {
        // get global handle from stream
        GetHGlobalFromStream(_pstm, &hGlobal);

        // find how much data we have
        STATSTG stats;
        _pstm->Stat( &stats, STATFLAG_NONAME );
        dwSize = stats.cbSize.LowPart;

        // write out data
        ASSERT(hGlobal);

        pData = (char *)GlobalLock(hGlobal);
        if (pData)
            pStm->Write(pData, dwSize, NULL);

        GlobalUnlock(hGlobal);

        _fDirty = FALSE;
    }

    return NOERROR;
}


HRESULT CMemStream::Read(void *pv, ULONG cb, ULONG *cbRead)
{
    HRESULT hr;

    // make sure we have a stream
    if (NULL == _pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = _pstm->Read(pv, cb, cbRead);
    if (FAILED(hr) || *cbRead!=cb)
        _fError=TRUE;

    return hr;
}

HRESULT CMemStream::Write(void *pv, ULONG cb, ULONG *cbWritten)
{
    HRESULT hr;

    // make sure we have a stream
    if (NULL == _pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    _fDirty = TRUE;

    hr = _pstm->Write(pv, cb, cbWritten);
    if (FAILED(hr) || *cbWritten != cb)
        _fError=TRUE;

    return hr;
}

HRESULT CMemStream::SaveToStream(IUnknown *punk)
{
    IPersistStream *    pips;
    HRESULT             hr;

    // make sure we have a stream
    if (NULL == _pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    // Get IPersistStream interface and save!
    hr = punk->QueryInterface(IID_IPersistStream, (void **)&pips);
    if (SUCCEEDED(hr))
        hr = OleSaveToStream(pips, _pstm);

    if (pips)
        pips->Release();

    if (FAILED(hr))
        _fError=TRUE;

    return hr;
}

HRESULT CMemStream::LoadFromStream(IUnknown **ppunk)
{
    HRESULT hr;

    // make sure we have a stream
    if (NULL == _pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = OleLoadFromStream(_pstm, IID_IUnknown, (void **)ppunk);

    if (FAILED(hr))
        _fError=TRUE;

    return hr;
}

HRESULT CMemStream::Seek(long lMove, DWORD dwOrigin, DWORD *dwNewPos)
{
    LARGE_INTEGER   liDist;
    ULARGE_INTEGER  uliNew;
    HRESULT         hr;

    LISet32(liDist, lMove);
    hr = _pstm->Seek(liDist, dwOrigin, &uliNew);
    if (dwNewPos)
    {
        *dwNewPos = uliNew.LowPart;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// CRegStream implementation
//
//////////////////////////////////////////////////////////////////////////

CRegStream::CRegStream(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey, BOOL fForceNewStream)
                       : CMemStream(fForceNewStream)
{
    long    lRes;
    DWORD   dwDisposition, dwType, dwSize;
    char *  pData;
    HGLOBAL hGlobal;

    _hKey = NULL;

    lstrcpy(_pszSubKey, pszSubKey);

    lRes = RegCreateKeyEx(
                         hBaseKey,
                         pszRegKey,
                         0,
                         NULL,
                         0,
                         HKEY_READ_WRITE_ACCESS,
                         NULL,
                         &_hKey,
                         &dwDisposition
                         );

    if (lRes != ERROR_SUCCESS)
        return;

    if (!fForceNewStream)
    {
        // Initialize our stream from the registry
        _fError=TRUE;  // assume failure

        // Find reg data size
        lRes = RegQueryValueEx(_hKey, _pszSubKey, NULL, &dwType, NULL, &dwSize);

        if (lRes != ERROR_SUCCESS)
        {
            // nothing in there at the moment - create new stream
            //DBG("CRegStream creating new stream");
            _fNewStream = TRUE;
            if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &_pstm)))
            {
                _pstm = NULL;
            }
            else
                _fError=FALSE;
        }
        else
        {
            // Get global handle to data
            hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
            if (NULL == hGlobal)
            {
                return;
            }

            pData = (char *)GlobalLock(hGlobal);

            lRes = RegQueryValueEx(_hKey, _pszSubKey, NULL, &dwType,
                                   (BYTE *)pData, &dwSize);
            GlobalUnlock(hGlobal);

            if (lRes != ERROR_SUCCESS)
            {
                GlobalFree(hGlobal);
                return;
            }

            // make stream on global handle
            if (FAILED(CreateStreamOnHGlobal(hGlobal, TRUE, &_pstm)))
            {
                _pstm = NULL;
                GlobalFree(hGlobal);
            }
            else
                _fError=FALSE;
        }
    }
}

CRegStream::~CRegStream()
{
    char *  pData;
    DWORD   dwSize = 0;
    HGLOBAL hGlobal=NULL;

    // Save stream to registry if it's changed
    if (_pstm && _fDirty)
    {
        GetHGlobalFromStream(_pstm, &hGlobal);

        STATSTG stats;
        _pstm->Stat( &stats, STATFLAG_NONAME );
        dwSize = stats.cbSize.LowPart;

        pData = (char *)GlobalLock(hGlobal);
        if (pData)
        {
            RegSetValueEx(_hKey, _pszSubKey, NULL, REG_BINARY,(const BYTE *)pData, dwSize);
        }
        GlobalUnlock(hGlobal);

        _fDirty = FALSE;
    }

    // clean up
    if (_hKey)
    {
        RegCloseKey(_hKey);
    }
}



