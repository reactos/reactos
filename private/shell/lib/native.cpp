//
//  native.cpp in shell\lib
//  
//  common Utility functions that need to be compiled for 
//  both UNICODE and ANSI
//

#include "proj.h"
#include <vdate.h>

// get the name and flags of an absolute IDlist
// in:
//      dwFlags     SHGDN_ flags as hints to the name space GetDisplayNameOf() function
//
// in/out:
//      *pdwAttribs (optional) return flags

STDAPI SHGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD *pdwAttribs)
{
    if (pszName)
    {
        VDATEINPUTBUF(pszName, TCHAR, cchName);
        *pszName = 0;
    }

    HRESULT hrInit = SHCoInitialize();

    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hres = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        if (pszName)
        {
            STRRET str;
            hres = psf->GetDisplayNameOf(pidlLast, dwFlags, &str);
            if (SUCCEEDED(hres))
                hres = StrRetToBuf(&str, pidlLast, pszName, cchName);
        }

        if (SUCCEEDED(hres) && pdwAttribs)
        {
            RIP(*pdwAttribs);    // this is an in-out param
            hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, pdwAttribs);
        }

        psf->Release();
    }

    SHCoUninitialize(hrInit);
    return hres;
}

STDAPI_(DWORD) GetUrlScheme(LPCTSTR pszUrl)
{
    if(pszUrl)
    {
        PARSEDURL pu;
        pu.cbSize = SIZEOF(pu);
        if(SUCCEEDED(ParseURL(pszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}

