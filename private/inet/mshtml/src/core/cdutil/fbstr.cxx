//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       fbstr.cxx
//
//  Contents:   Wrappers around BSTR api to account for wierdness with NULL
//
//  Functions:  FormsAllocString
//              FormsAllocStringLen
//              FormsReAllocString
//              FormsReAllocStringLen
//              FormsFreeString
//              FormsStringLen
//              FormsStringByteLen
//              FormsStringCmp
//              FormsStringNCmp
//              FormsStringICmp
//              FormsStringNICmp
//
//----------------------------------------------------------------------------

#include "headers.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   FormsAllocString
//
//  Synopsis:   Allocs a BSTR and initializes it from a string.  If the
//              initializer is NULL or the empty string, the resulting bstr is
//              NULL.
//
//  Arguments:  [pch]   -- String to initialize BSTR.
//              [pBSTR] -- The result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR]
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------
HRESULT
FormsAllocStringW(LPCWSTR pch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pch || !*pch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
    *pBSTR = SysAllocString(pch);
    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#if defined(WIN16)
HRESULT
FormsAllocStringA(LPCSTR pch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pch || !*pch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
    *pBSTR = SysAllocString(pch);
    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#ifndef WIN16
HRESULT
FormsAllocStringA(LPCWSTR pwch, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (!pwch || !*pwch)
    {
        *pBSTR = NULL;
        return S_OK;
    }
	CStr str;
    str.Set(pwch);
	*pBSTR = SysAllocString(str.GetAltStr());

    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}
#endif // !WIN16
#endif //_MAC
//+---------------------------------------------------------------------------
//
//  Function:   FormsAllocStringLen
//
//  Synopsis:   Allocs a BSTR of [uc] + 1 OLECHARS, and
//              initializes it from an optional string.  If [uc] == 0, the
//              resulting bstr is NULL.
//
//  Arguments:  [pch]   -- String to initialize.
//              [uc]    -- Count of characters of string.
//              [pBSTR] -- The result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
FormsAllocStringLenW(LPCWSTR pch, UINT uc, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (uc == 0)
    {
        *pBSTR = NULL;
        return S_OK;
    }

    *pBSTR = SysAllocStringLen(pch, uc);
    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#if defined(WIN16)
HRESULT
FormsAllocStringLenA(LPCSTR pch, UINT uc, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (uc == 0)
    {
        *pBSTR = NULL;
        return S_OK;
    }

    *pBSTR = SysAllocStringLen(pch, uc);

    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}

#ifndef WIN16
HRESULT
FormsAllocStringLenA(LPCWSTR pch, UINT uc, BSTR * pBSTR)
{
    HRESULT hr;

    Assert(pBSTR);
    if (uc == 0)
    {
        *pBSTR = NULL;
        return S_OK;
    }

    if(pch)
    {
        CStr str;
        str.Set(pch);
	    *pBSTR = SysAllocStringLen(str.GetAltStr(), uc);
    }
    else
    {
	    *pBSTR = SysAllocStringLen(NULL, uc);
    }

    hr = *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}
#endif // !WIN16
#endif // _MAC && WIN16


//+---------------------------------------------------------------------------
//
//  Function:   FormsReAllocString
//
//  Synopsis:   Allocates a BSTR initialized from a string; if successful,
//              frees the original string and replaces it.
//
//  Arguments:  [pBSTR] -- String to reallocate.
//              [pch]   -- Initializer.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
FormsReAllocStringW(BSTR * pBSTR, LPCWSTR pch)
{
    Assert(pBSTR);

#if DBG == 1
    HRESULT hr;
    BSTR    bstrTmp;

    hr = THR(FormsAllocStringW(pch, &bstrTmp));
    if (hr)
        RRETURN(hr);

    FormsFreeString(*pBSTR);
    *pBSTR = bstrTmp;
    return S_OK;
#else  //DBG == 1
#  ifndef _MAC
    return SysReAllocString(pBSTR, pch) ? S_OK : E_OUTOFMEMORY;
#  else
    // mac note: Richedit text requires BSTRs that contain unicode strings
    //          so we will occasionally need to call this wide char version 
    return SysReAllocStringLen(pBSTR, (OLECHAR *)pch, wcslen(pch) * sizeof(WCHAR)) ? S_OK : E_OUTOFMEMORY;
#  endif

#endif //DBG == 1
}

#if defined(WIN16)
HRESULT
FormsReAllocStringA(BSTR * pBSTR, LPCSTR pch)
{
    Assert(pBSTR);
#if DBG == 1
    HRESULT hr;
    BSTR    bstrTmp;

    hr = THR(FormsAllocStringA(pch, &bstrTmp));
    if (hr)
        RRETURN(hr);

    FormsFreeString(*pBSTR);
    *pBSTR = bstrTmp;
    return S_OK;
#else  //DBG == 1
    return SysReAllocString(pBSTR, pch) ? S_OK : E_OUTOFMEMORY;
#endif //DBG == 1
}
#endif // _MAC

//+---------------------------------------------------------------------------
//
//  Function:   FormsReAllocStringLen
//
//  Synopsis:   Allocates a BSTR of [uc] + 1 OLECHARs and optionally
//              initializes it from a string; if successful, frees the original
//              string and replaces it.
//
//  Arguments:  [pBSTR] -- String to reallocate.
//              [pch]   -- Initializer.
//              [uc]    -- Count of characters.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
FormsReAllocStringLenW(BSTR * pBSTR, LPCWSTR pch, UINT uc)
{
    Assert(pBSTR);

#if DBG == 1
    HRESULT hr;
    BSTR    bstrTmp;

    hr = THR(FormsAllocStringLen(pch, uc, &bstrTmp));
    if (hr)
        RRETURN(hr);

    FormsFreeString(*pBSTR);
    *pBSTR = bstrTmp;
    return S_OK;
#else
    return SysReAllocStringLen(pBSTR, pch, uc) ? S_OK : E_OUTOFMEMORY;
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   FormsStringLen
//
//  Synopsis:   Returns the length of the BSTR.
//
//  History:    5-06-94   adams   Created
//              6-30-95   andrewl Changed BSTR to const BSTR
//
//----------------------------------------------------------------------------

UINT
FormsStringLen(const BSTR bstr)
{
    return bstr ? SysStringLen((BSTR)bstr) : 0;
}



//+---------------------------------------------------------------------------
//
//  Function:   FormsStringByteLen
//
//  Synopsis:   Returns the length of a BSTR in bytes.
//
//  History:    5-06-94   adams   Created
//              6-30-95   andrewl Changed BSTR to const BSTR
//
//----------------------------------------------------------------------------

UINT
FormsStringByteLen(const BSTR bstr)
{
    return bstr ? SysStringByteLen((BSTR)bstr) : 0;
}



//+---------------------------------------------------------------------------
//
//  Function:   FormsStringCmp
//
//  Synopsis:   As per _tcscmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//
//----------------------------------------------------------------------------

int
FormsStringCmp(LPCTSTR bstr1, LPCTSTR bstr2)
{
    return _tcscmp(STRVAL(bstr1), STRVAL(bstr2));
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsStringNCmp
//
//  Synopsis:   As per _tcsncmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//
//----------------------------------------------------------------------------

int
FormsStringNCmp(LPCTSTR bstr1, int cch1, LPCTSTR bstr2, int cch2)
{
    return _tcsncmp(STRVAL(bstr1), cch1, STRVAL(bstr2), cch2);
}



//+---------------------------------------------------------------------------
//
//  Function:   FormsStringICmp
//
//  Synopsis:   As per wcsicmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//              15-Aug-94 doncl   changed from wcsicmp to _tcsicmp
//
//----------------------------------------------------------------------------

int
FormsStringICmp(LPCTSTR bstr1, LPCTSTR bstr2)
{
    return _tcsicmp(STRVAL(bstr1), STRVAL(bstr2));
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsStringNICmp
//
//  Synopsis:   As per wcsnicmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//              15-Aug-94 doncl   changed from wcsnicmp to _tcsnicmp
//
//----------------------------------------------------------------------------

int
FormsStringNICmp(LPCTSTR bstr1, int cch1, LPCTSTR bstr2, int cch2)
{
    return _tcsnicmp(STRVAL(bstr1), cch1, STRVAL(bstr2), cch2);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsStringCmpCase
//
//----------------------------------------------------------------------------

int
FormsStringCmpCase(LPCTSTR bstr1, LPCTSTR bstr2, BOOL fCaseSensitive)
{
    return (fCaseSensitive) ?
        _tcscmp (STRVAL(bstr1), STRVAL(bstr2)) :
        _tcsicmp(STRVAL(bstr1), STRVAL(bstr2));
}


//+-------------------------------------------------------------------------
// Function:    FormsSplitAtDelimiter
//
// Synopsis:    split a name into its head component (everything before the first
//              dot), and the tail component (the rest).
//
// Arguments:	bstrName    name to be split
//              pbstrHead   where to store head component
//              pbstrTail   where to store the rest
//              fFirst      TRUE - split at first delimiter, FALSE - at last
//              tchDelim    delimiter character (defaults to _T('.'))

void
FormsSplitAtDelimiter(LPCTSTR bstrName, BSTR *pbstrHead, BSTR *pbstrTail,
                            BOOL fFirst, TCHAR tchDelim)
{
    if (FormsIsEmptyString(bstrName))
    {
        *pbstrHead = NULL;
        *pbstrTail = NULL;
    }
    else
    {
        TCHAR *ptchDelim = fFirst ?  _tcschr(bstrName, tchDelim)
                                  : _tcsrchr(bstrName, tchDelim);

        if (ptchDelim)
        {
            FormsAllocStringLen(bstrName, ptchDelim - bstrName, pbstrHead);
            FormsAllocString(ptchDelim + 1, pbstrTail);
        }
        else if (fFirst)
        {
            FormsAllocString(bstrName, pbstrHead);
            *pbstrTail = NULL;
        }
        else
        {
            *pbstrHead = NULL;
            FormsAllocString(bstrName, pbstrTail);
        }
    }
}

