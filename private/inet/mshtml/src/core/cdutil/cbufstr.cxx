//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1997, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         CBUFSTR.CXX
//
// Contents     Class implementation for a buffered, appendable string class
//
// Classes      CBufferedStr
//
//
//  History:
//              7-10-97     t-chrisr     created
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#define BUFFEREDSTR_SIZE 1024

MtDefine(CBufferedStr, Utilities, "CBufferedStr")
MtDefine(CBufferedStr_pchBuf, CBufferedStr, "CBufferedStr::_pchBuf")

//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::Set
//
//  Synopsis:   Initilizes a CBufferedStr
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::Set (const TCHAR* pch)
{
    HRESULT hr = S_OK;

    Free();

    _cchIndex = pch ? _tcslen (pch) : 0;
    _cchBufSize = _cchIndex > BUFFEREDSTR_SIZE ? _cchIndex : BUFFEREDSTR_SIZE;
    _pchBuf = new(Mt(CBufferedStr_pchBuf)) TCHAR [ _cchBufSize ];
    if (!_pchBuf)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pch)
    {
        _tcsncpy (_pchBuf, pch, _cchIndex);
    }

    _pchBuf[_cchIndex] = '\0';

    IF_NOT_WIN16(MemSetName((_pchBuf, "CBufferedStr text")));

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::QuickAppend
//
//  Parameters: pchNewStr   string to be added to _pchBuf
//
//  Synopsis:   Appends pNewStr into _pchBuf starting at
//              _pchBuf[uIndex].  Increments index to reference
//              new end of string.  If _pchBuf is not large enough,
//              reallocs _pchBuf and updates _cchBufSize.
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::QuickAppend (const TCHAR* pchNewStr, ULONG newLen)
{
    Assert (pchNewStr);
    HRESULT hr = S_OK;

    if (!_pchBuf)
    {
        hr = Set();
        if (hr)
            goto Cleanup;
    }

    if (_cchIndex + newLen >= _cchBufSize)    // we can't fit the new string in the current buffer
    {                                         // so allocate a new buffer, and copy the old string
        _cchBufSize += (newLen > BUFFEREDSTR_SIZE) ? newLen : BUFFEREDSTR_SIZE;
        TCHAR * pchTemp = new(Mt(CBufferedStr_pchBuf)) TCHAR [ _cchBufSize ];
        if (!pchTemp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcsncpy (pchTemp, _pchBuf, _cchIndex);

        Free();
        _pchBuf = pchTemp;
    }

    // append the new string
    _tcsncpy (_pchBuf + _cchIndex, pchNewStr, newLen);
    _cchIndex += newLen;
    _pchBuf[_cchIndex] = '\0';

Cleanup:
    RRETURN(hr);
}
