/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: Shell String Class Implementation

    shstr.cpp

Author:

    Zeke Lucas (zekel)  27-Oct-96

Environment:

    User Mode - Win32

Revision History:


 Abstract:

    this allows automatic resizing and stuff.

  NOTE: this class is specifically designed to be used as a stack variable


--*/

#include "proj.h"
#include <shstr.h>


#define MALLOC(c)       LocalAlloc(LPTR, (c))
#define FREE(p)         LocalFree(p)

//
//  ShStr Public Methods
//

//
//  Constructors
//

ShStr :: ShStr () 
{
    _szDefaultBuffer[0] = '\0';
    _pszStr = _szDefaultBuffer;
    _cchSize = ARRAYSIZE(_szDefaultBuffer);
}


/**************
    StrStr SetStr() methods

  Return:   
    Success - a pointer to the object
    Failure - NULL
**************/

HRESULT 
ShStr :: SetStr (LPCSTR pszStr)
{
    Reset();
    
    return _SetStr(pszStr);

}

HRESULT 
ShStr :: SetStr (LPCSTR pszStr, DWORD cchStr)
{
    Reset();
    
    return _SetStr(pszStr, cchStr);

}

HRESULT 
ShStr :: SetStr (LPCWSTR pwszStr, DWORD cchStr)
{
    Reset();
    
    return _SetStr(pwszStr, cchStr);

}


HRESULT
ShStr :: Append(LPCTSTR pszStr, DWORD cchStr)
{
    HRESULT hr = S_OK;

    if(pszStr)
    {
        DWORD cchLen = GetLen();

        if(cchStr == (DWORD) -1)
            cchStr = lstrlen(pszStr);

        //
        //  StrCpyN automagically appends the null term, 
        //  so we need to give room for it
        //
        cchStr++;

        if(SUCCEEDED(SetSize(cchStr + cchLen)))
            StrCpyN(_pszStr + cchLen, pszStr, cchStr);
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

ShStr *
ShStr :: Clone()
{
    ShStr *pshstr = new ShStr;

    if (pshstr)
    {
        pshstr->SetStr(_pszStr);
    
        if(pshstr->IsValid())
            return pshstr;
    }

    if(pshstr)
        delete pshstr;

    return NULL;
}

LPSTR 
ShStr :: CloneStrA()
#ifdef UNICODE
{
    LPSTR pszStr = NULL;

    if(_pszStr)
    {
        DWORD cchStr;
    
        cchStr = WideCharToMultiByte(CP_ACP, 0,
            _pszStr, -1,
            NULL, 0,
            NULL, NULL);

        ASSERT(cchStr);

        if(cchStr)
        {
            pszStr = (LPSTR) MALLOC (CbFromCch(cchStr +1));

            if(pszStr)
            {
                cchStr = WideCharToMultiByte(CP_ACP, 0,
                    _pszStr, -1,
                    pszStr, cchStr,
                    NULL, NULL);
                ASSERT (cchStr);
            }
        }
    }

    return pszStr;
}

#else //!UNICODE

    {return _pszStr ? StrDupA(_pszStr) : NULL;}
#endif //UNICODE


#ifdef UNICODE
  
#endif
LPWSTR 
ShStr :: CloneStrW()
#ifdef UNICODE
    {return _pszStr ? StrDupW(_pszStr) : NULL;}
#else //!UNICODE
{
    LPWSTR pwsz;
    DWORD cch = lstrlenA(_pszStr) +1;

    pwsz = (LPWSTR) MALLOC (sizeof(WCHAR) * cch);
    
    if(pwsz)
        MultiByteToWideChar(CP_ACP, 0,
            _pszStr, -1,
            pwsz, cch);

    return pwsz;
}
#endif //UNICODE


/**************
    ShStr Utility methods

**************/


/**************
    ShStr SetSize method

    Sets the size of the internal buffer if larger than default

  Return:   
    Success - a pointer to the object
    Failure - NULL
**************/
HRESULT
ShStr :: SetSize(DWORD cchSize)
{
    HRESULT hr = S_OK;
    DWORD cchNewSize = _cchSize;

    ASSERT(!(_cchSize % DEFAULT_SHSTR_LENGTH));

    // so that we always allocate in increments
    while (cchSize > cchNewSize)
        cchNewSize <<= 2;
    
    if(cchNewSize != _cchSize)
    {
        if(cchNewSize > DEFAULT_SHSTR_LENGTH)
        {
            LPTSTR psz;

            psz = (LPTSTR) LocalAlloc(LPTR, CbFromCch(cchNewSize));
    
            if(psz)
            {
                StrCpyN(psz, _pszStr, cchSize);
                Reset();
                _cchSize = cchNewSize;
                _pszStr = psz;
            }
            else 
                hr = E_OUTOFMEMORY;
        }
        else
        {
            if (_pszStr && _cchSize) 
                StrCpyN(_szDefaultBuffer, _pszStr, ARRAYSIZE(_szDefaultBuffer));

            Reset();

            _pszStr = _szDefaultBuffer;
        }
    }

    return hr;
}

#ifdef DEBUG
BOOL
ShStr :: IsValid()
{
    BOOL fRet = TRUE;

    if(!_pszStr)
        fRet = FALSE;

    ASSERT( ((_cchSize != ARRAYSIZE(_szDefaultBuffer)) && (_pszStr != _szDefaultBuffer)) ||
            ((_cchSize == ARRAYSIZE(_szDefaultBuffer)) && (_pszStr == _szDefaultBuffer)) );

    ASSERT(!(_cchSize % DEFAULT_SHSTR_LENGTH));

    return fRet;
}
#endif //DEBUG

VOID 
ShStr :: Reset()
{
    if (_pszStr && (_cchSize != ARRAYSIZE(_szDefaultBuffer))) 
        LocalFree(_pszStr);

    _szDefaultBuffer[0] = TEXT('\0');
    _pszStr = _szDefaultBuffer;
    _cchSize = ARRAYSIZE(_szDefaultBuffer);
}

#define IsWhite(c)      ((DWORD) (c) > 32 ? FALSE : TRUE)
VOID
ShStr :: Trim()
{

    if(_pszStr)
    {
        // BUGBUG - NETSCAPE compatibility - zekel 29-JAN-97
        //  we want to leave one space in the string
        TCHAR chFirst = *_pszStr;

        //  first trim the backside
        TCHAR *pchLastWhite = NULL;
        LPTSTR pch = _pszStr;
        
        // the front side
        while (*pch && IsWhite(*pch))
            pch = CharNext(pch);

        if (pch > _pszStr)
        {
            LPTSTR pchDst = _pszStr;

            while (*pchDst = *pch)
            {
                pch = CharNext(pch);
                pchDst = CharNext(pchDst);
            }
        }

        // then the backside
        for (pch = _pszStr; *pch; pch = CharNext(pch))
        {
            if(pchLastWhite && !IsWhite(*pch))
                pchLastWhite = NULL;
            else if(!pchLastWhite && IsWhite(*pch))
                pchLastWhite = pch;
        }

        if(pchLastWhite)
            *pchLastWhite = TEXT('\0');

        if(TEXT(' ') == chFirst && !*_pszStr)
        {
            _pszStr[0] = TEXT(' ');
            _pszStr[1] = TEXT('\0');
        }
    }
}

    


//
//  ShStr Private Methods
//


/**************
    StrStr Set* methods

  Return:   
    Success - a pointer to the object
    Failure - NULL
**************/
HRESULT 
ShStr :: _SetStr(LPCSTR pszStr)
{
    HRESULT hr = S_FALSE;

    if(pszStr)
    {
        DWORD cchStr;

        cchStr = lstrlenA(pszStr);
    
        if(cchStr)
        {
            hr = SetSize(cchStr +1);

            if (SUCCEEDED(hr))
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0,
                    pszStr, -1,
                    _pszStr, _cchSize);
#else //!UNICODE
                lstrcpyA(_pszStr, pszStr);
#endif //UNICODE
        }
    }

    return hr;
}

HRESULT 
ShStr :: _SetStr(LPCSTR pszStr, DWORD cchStr)
{
    HRESULT hr = S_FALSE;

    if(pszStr && cchStr)
    {
        if (cchStr == (DWORD) -1)
            cchStr = lstrlenA(pszStr);

        hr = SetSize(cchStr +1);

        if(SUCCEEDED(hr))
        {
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, 0,
                pszStr, cchStr,
                _pszStr, _cchSize);
            _pszStr[cchStr] = TEXT('\0');

#else //!UNICODE
            StrCpyN(_pszStr, pszStr, (++cchStr < _cchSize ? cchStr : _cchSize) );
#endif //UNICODE
        }
    }

    return hr;
}

HRESULT 
ShStr :: _SetStr (LPCWSTR pwszStr, DWORD cchStrIn)
{
    DWORD cchStr = cchStrIn;
    HRESULT hr = S_FALSE;

    if(pwszStr && cchStr)
    {
        if (cchStr == (DWORD) -1)
#ifdef UNICODE
            cchStr = lstrlen(pwszStr);
#else //!UNICODE
        cchStr = WideCharToMultiByte(CP_ACP, 0,
            pwszStr, cchStrIn,
            NULL, 0,
            NULL, NULL);
#endif //UNICODE

        if(cchStr)
        {
            hr = SetSize(cchStr +1);

            if(SUCCEEDED(hr))
            {
#ifdef UNICODE 
                StrCpyN(_pszStr, pwszStr, (cchStr + 1< _cchSize ? cchStr + 1: _cchSize));
#else //!UNICODE
                cchStr = WideCharToMultiByte(CP_ACP, 0,
                    pwszStr, cchStrIn,
                    _pszStr, _cchSize,
                    NULL, NULL);
                _pszStr[cchStr < _cchSize ? cchStr : _cchSize] = TEXT('\0');
                ASSERT (cchStr);
#endif //UNICODE
            }
        }
#ifdef DEBUG
        else
        {
            DWORD dw;
            dw = GetLastError();
        }
#endif //DEBUG

    }
#ifdef DEBUG
    else
    {
        DWORD dw;
        dw = GetLastError();
    }
#endif //DEBUG

    return hr;
}

#if 0  //DISABLED until i have written the SHUrl* functions - zekel 7-Nov-96
//
//  UrlStr Methods
//
  
  UrlStr &
UrlStr::SetUrl(LPCSTR pszUrl)
{
    return SetUrl(pszUrl, (DWORD) -1);
}

  UrlStr &
UrlStr::SetUrl(LPCWSTR pwszUrl)
{
    return SetUrl(pwszUrl, (DWORD) -1);
}

  UrlStr &
UrlStr::SetUrl(LPCSTR pszUrl, DWORD cchUrl)
{
    _strUrl.SetStr(pszUrl, cchUrl);
    return *this;
}

  UrlStr &
UrlStr::SetUrl(LPCWSTR pwszUrl, DWORD cchUrl)
{
    _strUrl.SetStr(pwszUrl, cchUrl);
    return *this;
}

 
UrlStr::operator LPCTSTR()
{
    return _strUrl.GetStr();
}

  
UrlStr::operator SHSTR()
{
    return _strUrl;
}




  HRESULT
UrlStr::Combine(LPCTSTR pszUrl, DWORD dwFlags)
{
    SHSTR strRel;
    SHSTR strOut;
    HRESULT hr;

    strRel.SetStr(pszUrl);

    hr = UrlCombine(_strUrl.GetStr(), 
    hr = SHUrlParse(&_strUrl, &strRel, &strOut, NULL, URL_PARSE_CREATE);

    if(SUCCEEDED(hr))
        _strUrl = strOut;

    return hr;
}

/*
    ShStr &GetLocation();
    ShStr &GetAnchor();
    ShStr &GetQuery();

    HRESULT Canonicalize(DWORD dwFlags);
    HRESULT Combine(LPCTSTR pszUrl, DWORD dwFlags);
    HRESULT Encode(DWORD dwFlags);
    HRESULT EncodeSpaces()
        {return Encode(URL_ENCODE_SPACES_ONLY)}
    HRESULT Decode(DWORD dwFlags)
*/
#endif  //DISABLED
