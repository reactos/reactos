#include "shellprv.h"
#pragma  hdrstop


// NOTE: the STRRET_WSTR pOleStr gets freed, so don't try to use it later

STDAPI_(BOOL) StrRetToStrN(LPTSTR pszOut, UINT cchOut, STRRET *pStrRet, LPCITEMIDLIST pidl)
{
    return SUCCEEDED(StrRetToBuf(pStrRet, pidl, pszOut, cchOut));
}

STDAPI_(int) OleStrToStrN(LPTSTR pszOut, int cchOut, LPCWSTR pwsz, int cchWideChar)
{
    int cchOutput;
#ifdef UNICODE
    VDATEINPUTBUF(pszOut, WCHAR, cchOut);

    if (cchOut > cchWideChar && -1 != cchWideChar)
        cchOut = cchWideChar;

    cchOutput = cchOut;

    while (cchOut)
    {
        if ((*pszOut++ = *pwsz++) == 0)
            return cchOutput - cchOut + 1;
        cchOut--;
    }

    if (-1 == cchWideChar)
        pszOut--;              // Make room for the null 

    *pszOut = 0;
    return cchOutput;
#else
    VDATEINPUTBUF(pszOut, CHAR, cchOut);
    cchOutput = WideCharToMultiByte(CP_ACP, 0, pwsz, cchWideChar, pszOut, cchOut, NULL, NULL);
    if (cchOutput && (cchOutput == cchOut))
        cchOutput--;
    pszOut[cchOutput] = 0;
    return cchOutput;
#endif // UNICODE
}


STDAPI_(int) StrToOleStrN(LPWSTR pwszOut, int cchOut, LPCTSTR psz, int cchIn)
{
    int cchOutput;
#ifdef UNICODE
    VDATEINPUTBUF(pwszOut, WCHAR, cchOut);

    if (cchOut > cchIn)
        cchOut = cchIn;

    cchOutput = cchOut;

    while (--cchOut)
    {
        if ((*pwszOut++ = *psz++) == 0)
            return cchOutput - cchOut + 1;
    }

    *pwszOut = 0;
    return cchOutput;
#else
    VDATEINPUTBUF(pwszOut, WCHAR, cchOut);
    cchOutput = MultiByteToWideChar(CP_ACP, 0, psz, cchIn, pwszOut, cchOut);
    if (cchOutput && (cchOutput == cchOut))
        cchOutput--;
    pwszOut[cchOutput] = 0;
    return cchOutput;
#endif
}

// bogus export, too scared to remove it
STDAPI_(int) StrToOleStr(LPWSTR pwszOut, LPCTSTR psz)
{
    VDATEINPUTBUF(pwszOut, WCHAR, MAX_PATH);
    return SHTCharToUnicode(psz, pwszOut, MAX_PATH);
}
