/****************************************************************************/
/*                                                                          */
/*  enpenv.c -                                                              */
/*                                                                          */
/*      Routines for expanding environment strings                          */
/*                                                                          */
/****************************************************************************/

#include "shellprv.h"

//-------------------------------------------------------------------------
// The given string is parsed and all environment variables
// are expanded. If the expansion doesn't over fill the buffer
// then the length of the new string will be returned in the
// hiword and TRUE in the low word.  If the expansion would over
// fill the buffer then the original string is left unexpanded,
// the original length in the high word and FALSE in the low word.
// The length of the string is in bytes and excludes the terminating
// NULL.
//
// NOTE 1: This function must now handle environment variables in Quotes
//
// NOTE 2: There is no need for this API since NT has the equivalent APIs such
//       as ExpandEnvironmentStrings. But must keep it since it is a public
//       API in Win3.1.
//       Instead of doing all the work here, just call ExpandEnvironmentStrings.
//-------------------------------------------------------------------------

DWORD  APIENTRY DoEnvironmentSubstA(
   LPSTR pszSrc,    // The input string.
   UINT cchSrc)  // The limit of characters in the input string inc null.
{
    LPSTR pszExp;
    DWORD cchExp;
    BOOL fRet = FALSE;
        
    pszExp = (LPSTR)LocalAlloc(LPTR, cchSrc);
    if (pszExp)
    {
        cchExp = SHExpandEnvironmentStringsA(pszSrc, pszExp, cchSrc);
        if (cchExp)
        {
            StrCpyA(pszSrc, pszExp);
            fRet = TRUE;
        }
        LocalFree(pszExp);
    }

    if (fRet)
        return MAKELONG(cchExp,TRUE);
    else
        return MAKELONG(cchSrc,FALSE);
}

#ifdef UNICODE // on Win9x platform, shlunimp.c provides the implementation
DWORD  APIENTRY DoEnvironmentSubstW(
   LPWSTR pszSrc,    // The input string.
   UINT cchSrc)    // The limit of characters in the input string inc null.
{
    LPWSTR pszExp;
    DWORD cchExp;
    BOOL fRet = FALSE;
        
    pszExp = (LPWSTR)LocalAlloc(LPTR, cchSrc * sizeof(WCHAR));
    if (pszExp)
    {
        cchExp = SHExpandEnvironmentStringsW(pszSrc, pszExp, cchSrc);
        if (cchExp)
        {
            StrCpyW(pszSrc, pszExp);
            fRet = TRUE;
        }
        LocalFree(pszExp);
    }

    if (fRet)
        return MAKELONG(cchExp,TRUE);
    else
        return MAKELONG(cchSrc,FALSE);
}
#endif

