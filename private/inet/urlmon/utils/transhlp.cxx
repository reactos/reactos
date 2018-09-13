//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transhlp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-05-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>

HMODULE g_hLibMlang = NULL;


//+---------------------------------------------------------------------------
//
//  Function:   SzW2ADynamic
//
//  Synopsis:   Convert Unicode string wzFrom to Ansi, and return the Ansi string.
//              The Ansi string will be written into szTo (whose size is cchTo bytes) if
//              szTo is nonNull AND it is large enough to hold the Ansi string.
//              If this is not the case, the Ansi string is dynamically allocated.
//              If 'fTaskMalloc', allocation is done thru IMalloc, otherwise it is done
//              thru new.
//
//  Arguments:  [wzFrom] --
//              [szTo] --
//              [cchTo] --
//              [fTaskMalloc] --
//
//  Returns:    If error returns NULL, otherwise returns
//              pointer to Ansi string (if different from szTo, it must be freed via
//              delete or IMalloc, as appropriate).
//
//  History:    x-xx-xx   Clarg                    Created
//              2-25-96   JohannP (Johann Posch)   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR SzW2ADynamic(LPCWSTR wzFrom, LPSTR szTo, int cchTo, BOOL fTaskMalloc)
{
    int cchRequired;
    char *pszT = NULL;

    cchRequired = WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, NULL, 0, NULL, NULL);
    cchRequired++;
    TransAssert((cchRequired > 0));

    if (szTo && cchTo && (cchTo >= cchRequired))
    {
        // szTo has enough space
        pszT = szTo;
    }
    else
    {
        // szTo is not large enough; dynamically allocate the buffer
        if (fTaskMalloc)
        {
            pszT = (char*)CoTaskMemAlloc(sizeof(char) * cchRequired);
        }
        else
        {
            pszT = new char[cchRequired];
        }
        if (!pszT)
        {
            return NULL;
        }
    }

    if (!WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, pszT, cchRequired, NULL, NULL))
    {
        //TransAssert((0));
        if (pszT != szTo)
        {
            (fTaskMalloc ? CoTaskMemFree(pszT) : delete pszT);
        }
        pszT = NULL;
    }

    return pszT;
}

//+---------------------------------------------------------------------------
//
//  Function:   WzA2WDynamic
//
//  Synopsis:   Convert Ansi string szFrom to Unicode, and return the Unicode string.
//              The Unicode string will be written into wzTo (whose size is cwchTo bytes)
//              if wzTo is nonNull AND it is large enough to hold the Unicode string.
//              If this is not the case, the Unicode string is dynamically allocated.
//              If 'fTaskMalloc', allocation is done thru IMalloc, otherwise it is done
//              thru new.
//
//  Arguments:  [szFrom] --
//              [wzTo] --
//              [cwchTo] --
//              [fTaskMalloc] --
//
//  Returns:    If error returns NULL, otherwise returns
//              pointer to Unicode string (if different from wzTo, it must be freed via
//              delete or IMalloc, as appropriate).
//
//  History:    x-xx-xx   Clarg                    Created
//              2-25-96   JohannP (Johann Posch)   modified
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR WzA2WDynamic(LPCSTR szFrom, LPWSTR wzTo, int cwchTo, BOOL fTaskMalloc)
{
    int cwchRequired;
    WCHAR *pwzT = NULL;

    cwchRequired = MultiByteToWideChar(CP_ACP, 0, szFrom, -1, NULL, 0);
    cwchRequired++;
    TransAssert((cwchRequired > 0));

    if (wzTo && cwchTo && (cwchTo >= cwchRequired))
    {
        // wzTo has enough space
        pwzT = wzTo;
    }
    else
    {
        // wzTo is not large enough; dynamically allocate the buffer
        if (fTaskMalloc)
        {
            pwzT = (WCHAR*)CoTaskMemAlloc(sizeof(WCHAR) * cwchRequired);
        }
        else
        {
            pwzT = new WCHAR[cwchRequired];
        }

        if (!pwzT)
        {
            return NULL;
        }
    }

    if (!MultiByteToWideChar(CP_ACP, 0, szFrom, -1, pwzT, cwchRequired))
    {
        //Assert(0);
        if (pwzT != wzTo)
        {
            (fTaskMalloc ? CoTaskMemFree(pwzT) : delete pwzT);
        }

        pwzT = NULL;
    }

    return pwzT;
}

//+---------------------------------------------------------------------------
//
//  Function:   OLESTRDuplicate
//
//  Synopsis:
//
//  Arguments:  [ws] --
//
//  Returns:
//
//  History:    2-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR OLESTRDuplicate(LPCWSTR ws)
{
    DWORD cLen;
    LPWSTR wsNew = NULL;

    if (ws)
    {
        wsNew = (LPWSTR) new  WCHAR [wcslen(ws) + 1];
        if (wsNew)
        {
            wcscpy(wsNew, ws);
        }
    }

    return wsNew;
}


//+-------------------------------------------------------------------------
//
//  Function:   WideCharToMultiByteWithMlang
//
//  Synopsis:   Convert Unicode string wzFrom to Ansi by uring MLANG, 
//
//  History:    5-29-97   DanpoZ (Danpo Zhang)     Created
//
//  Notes:
//
//--------------------------------------------------------------------------
void WideCharToMultiByteWithMlang(
    LPCWSTR lpwszWide,
    LPSTR   lpszAnsi,
    int     cchAnsi,
    DWORD   dwCodePage)
{
    INT cchOut;
    INT cchIn = wcslen(lpwszWide);
    HRESULT hr = NOERROR;

    typedef HRESULT (WINAPI * pfnMLANGW2A)(
        LPDWORD, DWORD, LPCWSTR, LPINT, LPSTR, LPINT);

    static pfnMLANGW2A pfnConvertINetUnicodeToMultiByte = NULL;

    if(!g_hLibMlang)
    {
        g_hLibMlang = LoadLibraryA("mlang.dll");
        if(!g_hLibMlang)
        {
            goto End;
        }
    }

    if (!pfnConvertINetUnicodeToMultiByte)
    {
        pfnConvertINetUnicodeToMultiByte= (pfnMLANGW2A)GetProcAddress(
            g_hLibMlang, "ConvertINetUnicodeToMultiByte");
        if (!pfnConvertINetUnicodeToMultiByte)
        {
            goto End;
        }
    }
        

    // first call to get the lenth of the Multi-Byte string 
    hr = pfnConvertINetUnicodeToMultiByte(
        NULL, dwCodePage, lpwszWide, &cchIn, NULL, &cchOut);

    if( !FAILED(hr) && cchOut <= cchAnsi )
    {
        hr = pfnConvertINetUnicodeToMultiByte(
            NULL, dwCodePage, lpwszWide, &cchIn, lpszAnsi, &cchOut);
    }

End:;
    
}

DWORD StrLenMultiByteWithMlang(
    LPCWSTR lpwszWide,
    DWORD   dwCodePage)
{
    INT cchOut;
    INT cchIn = wcslen(lpwszWide);
    HRESULT hr = NOERROR;

    typedef HRESULT (WINAPI * pfnMLANGW2A)(
        LPDWORD, DWORD, LPCWSTR, LPINT, LPSTR, LPINT);

    static pfnMLANGW2A pfnConvertINetUnicodeToMultiByte = NULL;

    if(!g_hLibMlang)
    {
        g_hLibMlang = LoadLibraryA("mlang.dll");
        if(!g_hLibMlang)
        {
            goto Error;
        }
    }

    if (!pfnConvertINetUnicodeToMultiByte)
    {
        pfnConvertINetUnicodeToMultiByte= (pfnMLANGW2A)GetProcAddress(
            g_hLibMlang, "ConvertINetUnicodeToMultiByte");
        if (!pfnConvertINetUnicodeToMultiByte)
        {
            goto Error;
        }
    }
        

    // first call to get the lenth of the Multi-Byte string 
    hr = pfnConvertINetUnicodeToMultiByte(
        NULL, dwCodePage, lpwszWide, &cchIn, NULL, &cchOut);

    if( !FAILED(hr) )
    {
        return cchOut;
    }

Error:
    return 0;

}
