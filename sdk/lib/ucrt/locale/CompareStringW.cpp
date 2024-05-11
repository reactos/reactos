/***
*w_cmp.c - W versions of CompareString.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Wrapper for CompareStringW.
*
*******************************************************************************/
#include <corecrt_internal.h>

/***
*int __cdecl __acrt_CompareStringW - Get type information about a wide string.
*
*Purpose:
*  Internal support function. Assumes info in wide string format.
*
*Entry:
*  LPCWSTR  LocaleName  - locale context for the comparison.
*  DWORD    dwCmpFlags  - see NT\Chicago docs
*  LPCWSTR  lpStringn   - wide string to be compared
*  int      cchCountn   - wide char (word) count (NOT including nullptr)
*                       (-1 if nullptr terminated)
*
*Exit:
*  Success: 1 - if lpString1 <  lpString2
*           2 - if lpString1 == lpString2
*           3 - if lpString1 >  lpString2
*  Failure: 0
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl __acrt_CompareStringW(
        LPCWSTR  LocaleName,
        DWORD    dwCmpFlags,
        PCWCH    lpString1,
        int      cchCount1,
        PCWCH    lpString2,
        int      cchCount2
        )
{
    /*
     * CompareString will compare past nullptr. Must find nullptr if in string
     * before cchCountn wide characters.
     */

    if (cchCount1 > 0)
        cchCount1= (int) wcsnlen(lpString1, cchCount1);
    if (cchCount2 > 0)
        cchCount2= (int) wcsnlen(lpString2, cchCount2);

    if (!cchCount1 || !cchCount2)
        return (cchCount1 - cchCount2 == 0) ? 2 :
               (cchCount1 - cchCount2 < 0) ? 1 : 3;

    return __acrt_CompareStringEx( LocaleName,
                           dwCmpFlags,
                           lpString1,
                           cchCount1,
                           lpString2,
                           cchCount2,
                           nullptr,
                           nullptr,
                           0);
}
