/*
 * @(#)URL.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "url.hxx"
#include <shlwapip.h>
#ifdef UNIX
// Not needed under UNIX
#else
#ifndef _WIN64
#include <w95wraps.h>
#endif // _WIN64
#endif /* UNIX */

#define MAX_URL_LENGTH 4096    // URL parameters, etc.

TCHAR* Reclaim(TCHAR* s)
{
    // reclaim unused memory.
    TCHAR* result = ::StringDup(s);
    if (result != NULL)
    {
        delete [] s;
        return result;
    }
    return s;
}

URL::URL() 
{ 
    _pszUrl = _pszBase = _pszResolved = NULL; 
    _pszSecureBase = NULL; 
    _pBaseMoniker = NULL;
}

URL::~URL()
{
    clear();
    delete _pszSecureBase;
}

void
URL::clear()
{
    delete _pszUrl;
    delete _pszBase;
    delete _pszResolved;
    _pszUrl = _pszBase = _pszResolved = NULL;
    _pBaseMoniker = NULL;
}

URL::URL(const URL& other)
{
    _pszUrl = ::StringDup(other._pszUrl);
    _pszBase = ::StringDup(other._pszBase);
    _pszResolved = ::StringDup(other._pszResolved);
    _pszSecureBase = ::StringDup(other._pszSecureBase);
    _pBaseMoniker = other._pBaseMoniker;
}

HRESULT URL::set(const TCHAR * pszURL, const TCHAR* pszBase)
{
    TCHAR* s = NULL;
    HRESULT hr = S_OK;
    DWORD d;
    IMoniker * pMoniker = NULL;
    LPOLESTR pOleStr = NULL;

    Assert(pszURL != NULL);
    clear();

    // Save away the relative URL.
    _pszUrl = ::StringDupHR(pszURL, &hr);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pszBase == NULL || 0 == *pszBase)
    {
        if (PathIsURL(pszURL))
        {
            // Resolved is the same then.
            _pszResolved = ::StringDupHR(pszURL, &hr);
            goto Cleanup;
        }

        _pszResolved = new_ne TCHAR[MAX_URL_LENGTH];
        if (_pszResolved == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // We are about to call UrlCreateFromPath which escapes all special
        // characters.  For example, space character becomes %20.  But pszURL
        // may already be escaped, so we need to un-escape it first otherwise
        // it will become double escaped and %20 will become %2520.
        d = MAX_URL_LENGTH;
        hr = UrlUnescape((WCHAR*)pszURL, _pszResolved, &d, 0);

        s = new_ne TCHAR[MAX_URL_LENGTH];
        if (s == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (PathSearchAndQualify(_pszResolved, s, MAX_URL_LENGTH))
        {
            d = MAX_URL_LENGTH;
            hr = UrlCreateFromPath(s, _pszResolved, &d, 0);
            if (SUCCEEDED(hr))
            {
                _pszResolved = Reclaim(_pszResolved);
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        delete s;
        goto Cleanup;
    }
    else
    {
        // Save the base for later.
        _pszBase = ::StringDupHR(pszBase, &hr);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        hr = CreateURLMoniker(NULL, _pszBase, (IMoniker **)&pMoniker);
        if (FAILED(hr))
            goto Cleanup;
        _pBaseMoniker = pMoniker;
        pMoniker->Release();

        hr = CreateURLMoniker(_pBaseMoniker, pszURL, (IMoniker **)&pMoniker);
        if (SUCCEEDED(hr))
        {
            hr = pMoniker->GetDisplayName(NULL, pMoniker, &pOleStr);
            if (SUCCEEDED(hr))
            {
                _pszResolved = ::StringDupHR(pOleStr, &hr);
                CoTaskMemFree(pOleStr);
            }
            pMoniker->Release();
        }
    }

Cleanup:
    return hr;
}

HRESULT URL::set(const TCHAR * pszURL, const TCHAR* pszBase1, const TCHAR* pszBase2)
{
    HRESULT hr;
    WCHAR * pszBase = NULL;

    if (pszBase1)
    {
        hr = set(pszBase1, pszBase2);
        if (SUCCEEDED(hr))
        {
            pszBase = ::StringDup(getResolved());
            if (pszBase == NULL)
                hr = E_OUTOFMEMORY;
            else
            {
                hr = set(pszURL, pszBase);
                delete pszBase;
            }
        }
    }
    else
    {
        hr = set(pszURL, pszBase2);
    }

    return hr;
}

HRESULT URL::setSecureBase(const TCHAR* pszSecureBase)
{
    delete _pszSecureBase;
    _pszSecureBase = NULL;

    if (pszSecureBase != NULL)
    {
        _pszSecureBase = ::StringDup(pszSecureBase);
        if (_pszSecureBase == NULL)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

/**
 * Whether the URL is a File
 */
bool URL::isFile()
{
    return 0 != ::UrlIs(_pszResolved, URLIS_FILEURL);
}

/**
 * Get the URL File, the caller is responsible to free the string returned by
 * using <code> delete [] url </code>
 */
TCHAR * URL::getFile()
{
    DWORD d = MAX_URL_LENGTH;
    TCHAR *s = new_ne TCHAR[MAX_URL_LENGTH];
    if (s == NULL)
        return NULL;

    HRESULT hr = ::PathCreateFromUrl(_pszResolved, s, &d, 0);
    if (SUCCEEDED(hr))
    {
        return Reclaim(s);
    }
    else
    {
        delete [] s;
        return NULL;
    }
}


INTERNET_SCHEME
URL::getScheme(const WCHAR * pwszUrl)
{
	URL_COMPONENTSW		urlComp;

    ZeroMemory(&urlComp, sizeof(URL_COMPONENTSW));
    urlComp.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComp.dwSchemeLength = 1;
    urlComp.dwHostNameLength = 1;
    urlComp.dwUrlPathLength = 1;
    if (InternetCrackUrl(pwszUrl, 0, 0, &urlComp))
        return urlComp.nScheme;
    else
        return INTERNET_SCHEME_UNKNOWN;
}
