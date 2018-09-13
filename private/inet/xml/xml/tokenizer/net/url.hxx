/*
 * @(#)URL.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _URL_HXX
#define _URL_HXX

#include <wininet.h>

//-------------------------------------------------------------------------
// This is a little helper class that wraps the SHLWAPI URL routines.
class URL
{
public:

    URL();
    URL(const URL& other);
    ~URL();

    static INTERNET_SCHEME getScheme(const WCHAR * pwszUrl);

    // Initialize URL using given URL and optional base URL.  If
    // base URL is provided, it will be used to resolve the URL
    // and can also be used in security checking during download.
    HRESULT set(const TCHAR * pszURL, const TCHAR* pszBase = NULL);

    // Initialize URL using given URL and two base URLs. pszBase1 takes presidence when 
    // it comes to resolve relative pszURL if pszBase1 is not NULL.
    HRESULT set(const TCHAR * pszURL, const TCHAR* pszBase1, const TCHAR* pszBase2);

    // The secure base can be different from the base used to resolve
    // relative URL's -- thanks to the <BASE> tag.
    HRESULT setSecureBase(const TCHAR* pszSecureBase = NULL);

    TCHAR * getRelative() { return _pszUrl; }
    TCHAR * getBase() { return _pszBase; }
    TCHAR * getResolved() { return _pszResolved; }

    TCHAR* getSecureBase() { return _pszSecureBase; }

    // Return true if the URL is a local file
    bool isFile();

    // Get the null terminated file name if it is a local file
    TCHAR * getFile(); 

    IMoniker * getBaseMoniker() { return _pBaseMoniker; }
   
    void clear();
private:

    TCHAR * _pszUrl;      // the url string
    TCHAR * _pszBase;
    TCHAR * _pszResolved; // fully resolved URL
    TCHAR * _pszSecureBase;
    _reference<IMoniker> _pBaseMoniker;
};

#endif _URL_HXX