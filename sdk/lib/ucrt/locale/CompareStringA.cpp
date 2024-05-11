/***
*a_cmp.c - A version of CompareString.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Use either CompareStringA or CompareStringW depending on which is
*       available
*
*******************************************************************************/
#include <corecrt_internal.h>



/***
*int __cdecl __acrt_CompareStringA - Get type information about an ANSI string.
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call CompareStringA if available and uses CompareStringW
*       if it must. If neither are available it fails and returns 0.
*
*Entry:
*       LPCWSTR LocaleName  - locale context for the comparison.
*       DWORD   dwCmpFlags  - see NT\Chicago docs
*       LPCSTR  lpStringn   - multibyte string to be compared
*       int     cchCountn   - char (byte) count (NOT including nullptr)
*                             (-1 if nullptr terminated)
*       int     code_page   - for MB/WC conversion. If 0, use __lc_codepage
*
*Exit:
*       Success: 1 - if lpString1 <  lpString2
*                2 - if lpString1 == lpString2
*                3 - if lpString1 >  lpString2
*       Failure: 0
*
*Exceptions:
*
*******************************************************************************/

static int __cdecl InternalCompareStringA(
    _locale_t plocinfo,
    LPCWSTR   LocaleName,
    DWORD     dwCmpFlags,
    PCCH      lpString1,
    int       cchCount1,
    PCCH      lpString2,
    int       cchCount2,
    int       code_page
    ) throw()
{
    /*
     * CompareString will compare past nullptr. Must find nullptr if in string
     * before cchCountn chars.
     */

    if (cchCount1 > 0)
        cchCount1 = static_cast<int>(__strncnt(lpString1, cchCount1));
    else if (cchCount1 < -1)
        return FALSE;

    if (cchCount2 > 0)
        cchCount2 = static_cast<int>(__strncnt(lpString2, cchCount2));
    else if (cchCount2 < -1)
        return FALSE;


    int buff_size1;
    int buff_size2;

    /*
     * Use __lc_codepage for conversion if code_page not specified
     */

    if (0 == code_page)
        code_page = plocinfo->locinfo->_public._locale_lc_codepage;

    /*
     * Special case: at least one count is zero
     */

    if (!cchCount1 || !cchCount2)
    {
        unsigned char *cp;  // char pointer
        CPINFO cpInfo;      // struct for use with GetCPInfo

        /* both strings zero */
        if (cchCount1 == cchCount2)
            return 2;

        /* string 1 greater */
        if (cchCount2 > 1)
            return 1;

        /* string 2 greater */
        if (cchCount1 > 1)
            return 3;

        /*
         * one has zero count, the other has a count of one
         * - if the one count is a naked lead byte, the strings are equal
         * - otherwise it is a single character and they are unequal
         */

        if (GetCPInfo(code_page, &cpInfo) == FALSE)
            return 0;

        _ASSERTE(cchCount1==0 && cchCount2==1 || cchCount1==1 && cchCount2==0);

        /* string 1 has count of 1 */
        if (cchCount1 > 0)
        {
            if (cpInfo.MaxCharSize < 2)
                return 3;

            for ( cp = (unsigned char *)cpInfo.LeadByte ;
                  cp[0] && cp[1] ;
                  cp += 2 )
                if ( (*(unsigned char *)lpString1 >= cp[0]) &&
                     (*(unsigned char *)lpString1 <= cp[1]) )
                    return 2;

            return 3;
        }

        /* string 2 has count of 1 */
        if (cchCount2 > 0)
        {
            if (cpInfo.MaxCharSize < 2)
            return 1;

            for ( cp = (unsigned char *)cpInfo.LeadByte ;
                  cp[0] && cp[1] ;
                  cp += 2 )
                if ( (*(unsigned char *)lpString2 >= cp[0]) &&
                     (*(unsigned char *)lpString2 <= cp[1]) )
                    return 2;

            return 1;
        }
    }

    /*
     * Convert strings and return the requested information.
     */

    /* find out how big a buffer we need (includes nullptr if any) */
    if ( 0 == (buff_size1 = __acrt_MultiByteToWideChar( code_page,
                                                 MB_PRECOMPOSED |
                                                    MB_ERR_INVALID_CHARS,
                                                 lpString1,
                                                 cchCount1,
                                                 nullptr,
                                                 0 )) )
        return 0;

    /* allocate enough space for chars */
    __crt_scoped_stack_ptr<wchar_t> wbuffer1(_malloca_crt_t(wchar_t, buff_size1));
    if (wbuffer1.get() == nullptr)
        return 0;

    /* do the conversion */
    if ( 0 == __acrt_MultiByteToWideChar( code_page,
                                   MB_PRECOMPOSED,
                                   lpString1,
                                   cchCount1,
                                   wbuffer1.get(),
                                   buff_size1 ) )
        return 0;

    /* find out how big a buffer we need (includes nullptr if any) */
    if ( 0 == (buff_size2 = __acrt_MultiByteToWideChar( code_page,
                                                 MB_PRECOMPOSED |
                                                    MB_ERR_INVALID_CHARS,
                                                 lpString2,
                                                 cchCount2,
                                                 nullptr,
                                                 0 )) )
        return 0;

    /* allocate enough space for chars */
    __crt_scoped_stack_ptr<wchar_t> const wbuffer2(_malloca_crt_t(wchar_t, buff_size2));
    if (wbuffer2.get() == nullptr)
        return 0;

    int const actual_size = __acrt_MultiByteToWideChar(
        code_page,
        MB_PRECOMPOSED,
        lpString2,
        cchCount2,
        wbuffer2.get(),
        buff_size2);

    if (actual_size == 0)
        return 0;

    return __acrt_CompareStringEx(
        LocaleName,
        dwCmpFlags,
        wbuffer1.get(),
        buff_size1,
        wbuffer2.get(),
        buff_size2,
        nullptr,
        nullptr,
        0);
}

extern "C" int __cdecl __acrt_CompareStringA(
    _locale_t const locale,
    LPCWSTR   const locale_name,
    DWORD     const compare_flags,
    PCCH      const string1,
    int       const string1_count,
    PCCH      const string2,
    int       const string2_count,
    int       const code_page
    )
{
    _LocaleUpdate locale_update(locale);

    return InternalCompareStringA(
        locale_update.GetLocaleT(),
        locale_name,
        compare_flags,
        string1,
        string1_count,
        string2,
        string2_count,
        code_page
        );
}
