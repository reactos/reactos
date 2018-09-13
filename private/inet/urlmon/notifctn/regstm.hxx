//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       regstm.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _REGSTM_HXX_
#define _REGSTM_HXX_

#define MAX_REG_KEY     512

//
// CMemStream class
//
class CMemStream
{
public:
    CMemStream(BOOL fNewStream=TRUE);
    ~CMemStream();

    BOOL        IsError() { return _fError;}

    HRESULT     Read(void *pv, ULONG cb, ULONG *cbRead);
    HRESULT     Write(void *pv, ULONG cb, ULONG *cbWritten);
    HRESULT     Seek(long lMove, DWORD dwOrigin, DWORD *dwNewPos);
    HRESULT     SaveToStream(IUnknown *punk);
    HRESULT     LoadFromStream(IUnknown **ppunk);
    HRESULT     CopyToStream(IStream *pStm);

    HRESULT     GetStream(IStream **ppStm)
    {
        HRESULT hr = NOERROR;

        if (!ppStm)
        {
            hr = E_OUTOFMEMORY;
        }
        else if (_pstm)
        {
            *ppStm = _pstm;
            _pstm->AddRef();
        }
        else
        {
            hr = E_FAIL;
        }

        return hr;
    }

    void SetDirty(BOOL fDirty = TRUE)
    {
        _fDirty = fDirty;
    }
    BOOL GetDirty()
    {
        return _fDirty;
    }


public:
    BOOL        _fNewStream;

protected:
    IStream *   _pstm;
    BOOL        _fDirty;
    BOOL        _fError;

};

//
// CRegStream class
//
// Set fForceNewStream to TRUE if you dont' want it to init from the registry
//
class CRegStream : public CMemStream
{
public:
    CRegStream(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey, BOOL fForceNewStream=FALSE);
    ~CRegStream();

protected:
    CHAR        _pszSubKey[MAX_REG_KEY];
    HKEY        _hKey;
};

HRESULT RegIsPersistedKey(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey);
HRESULT RegIsPersistedValue(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey);

#endif // _REGSTM_HXX_

