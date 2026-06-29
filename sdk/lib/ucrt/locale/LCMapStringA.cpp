/***
*a_map.c - A version of LCMapString.
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Use either LCMapStringA or LCMapStringW depending on which is available
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <locale.h>

/***
*int __cdecl __acrt_LCMapStringA - Get type information about an ANSI string.
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call LCMapStringA if available and uses LCMapStringW
*       if it must. If neither are available it fails and returns 0.
*
*Entry:
*       LPCWSTR  LocaleName  - locale context for the comparison.
*       DWORD    dwMapFlags  - see NT\Chicago docs
*       PCCH     cchSrc      - wide char (word) count of input string
*                              (including nullptr if any)
*                              (-1 if nullptr terminated)
*       PCH    lpDestStr     - pointer to memory to store mapping
*       int      cchDest     - char (byte) count of buffer (including nullptr)
*       int      code_page   - for MB/WC conversion. If 0, use __lc_codepage
*       BOOL     bError      - TRUE if MB_ERR_INVALID_CHARS set on call to
*                              MultiByteToWideChar when GetStringTypeW used.
*
*Exit:
*       Success: number of chars written to lpDestStr (including nullptr)
*       Failure: 0
*
*Exceptions:
*
*******************************************************************************/

static _Success_(return != 0) int __cdecl __acrt_LCMapStringA_stat(
        _In_opt_                    _locale_t plocinfo,
        _In_                        LPCWSTR   LocaleName,
        _In_                        DWORD     dwMapFlags,
        _In_CRT_NLS_string_(cchSrc) PCCH      lpSrcStr,
        _In_                        int       cchSrc,
        _Out_writes_opt_(cchDest)   PCH       lpDestStr,
        _In_                        int       cchDest,
        _In_                        int       code_page,
        _In_                        BOOL      bError
        )
{
    // LCMapString will map past the null terminator.  We must find the null
    // terminator if it occurs in the string before cchSrc characters
    // and cap the number of characters to be considered.
    if (cchSrc > 0)
    {
        int cchSrcCnt = static_cast<int>(__strncnt(lpSrcStr, cchSrc));

        // Include the null terminator if the source string is terminated within
        // the buffer.
        if (cchSrcCnt < cchSrc)
        {
            cchSrc = cchSrcCnt + 1;
        }
        else
        {
            cchSrc = cchSrcCnt;
        }
    }

    int retval = 0;
    int inbuff_size;
    int outbuff_size;

    /*
     * Convert string and return the requested information. Note that
     * we are converting to a wide string so there is not a
     * one-to-one correspondence between number of wide chars in the
     * input string and the number of *bytes* in the buffer. However,
     * there had *better be* a one-to-one correspondence between the
     * number of wide characters and the number of multibyte characters
     * or the resulting mapped string will be worthless to the user.
     */

    /*
     * Use __lc_codepage for conversion if code_page not specified
     */

    if (0 == code_page)
        code_page = plocinfo->locinfo->_public._locale_lc_codepage;

    /* find out how big a buffer we need (includes nullptr if any) */
    if ( 0 == (inbuff_size =
               __acrt_MultiByteToWideChar( code_page,
                                           bError ? MB_PRECOMPOSED |
                                               MB_ERR_INVALID_CHARS :
                                               MB_PRECOMPOSED,
                                           lpSrcStr,
                                           cchSrc,
                                           nullptr,
                                           0 )) )
        return 0;

    /* allocate enough space for wide chars */
    __crt_scoped_stack_ptr<wchar_t> const inwbuffer(_malloca_crt_t(wchar_t, inbuff_size));
    if (!inwbuffer)
        return 0;

    /* do the conversion */
    if ( 0 == __acrt_MultiByteToWideChar( code_page,
                                          MB_PRECOMPOSED,
                                          lpSrcStr,
                                          cchSrc,
                                          inwbuffer.get(),
                                          inbuff_size) )
        return retval;

    /* get size required for string mapping */
    if ( 0 == (retval = __acrt_LCMapStringEx( LocaleName,
                                              dwMapFlags,
                                              inwbuffer.get(),
                                              inbuff_size,
                                              nullptr,
                                              0,
                                              nullptr,
                                              nullptr,
                                              0)) )
        return retval;

    if (dwMapFlags & LCMAP_SORTKEY) {
        /* retval is size in BYTES */

        if (0 != cchDest) {

            if (retval > cchDest)
                return 0;

            /* do string mapping */
            // The buffer overflow warning here is due to an inadequate annotation
            // on __acrt_LCMapStringEx.  When the map flags include LCMAP_SORTKEY,
            // the destination buffer is actually required to be an array of bytes,
            // despite the type of the buffer being a wchar_t*.
            __pragma(warning(suppress: __WARNING_BUFFER_OVERFLOW))
            if ( 0 == (retval = __acrt_LCMapStringEx( LocaleName,
                                    dwMapFlags,
                                    inwbuffer.get(),
                                    inbuff_size,
                                    reinterpret_cast<PWCH>(lpDestStr),
                                    cchDest,
                                    nullptr,
                                    nullptr,
                                    0)) )
                return retval;
        }
    }
    else {
        /* retval is size in wide chars */

        outbuff_size = retval;

        /* allocate enough space for wide chars (includes nullptr if any) */
        __crt_scoped_stack_ptr<wchar_t> const outwbuffer(_malloca_crt_t(wchar_t, outbuff_size));
        if (!outwbuffer)
            return 0;

        /* do string mapping */
        if ( 0 == (retval = __acrt_LCMapStringEx( LocaleName,
                                dwMapFlags,
                                inwbuffer.get(),
                                inbuff_size,
                                outwbuffer.get(),
                                outbuff_size,
                                nullptr,
                                nullptr,
                                0)) )
            return retval;

        if (0 == cchDest) {
            /* get size required */
            if ( 0 == (retval =
                       __acrt_WideCharToMultiByte( code_page,
                                                   0,
                                                   outwbuffer.get(),
                                                   outbuff_size,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   nullptr )) )
                return retval;
        }
        else {
            /* convert mapping */
            if ( 0 == (retval =
                       __acrt_WideCharToMultiByte( code_page,
                                                   0,
                                                   outwbuffer.get(),
                                                   outbuff_size,
                                                   lpDestStr,
                                                   cchDest,
                                                   nullptr,
                                                   nullptr )) )
                return retval;
        }
    }

    return retval;
}

extern "C" int __cdecl __acrt_LCMapStringA(
        _locale_t const plocinfo,
        PCWSTR    const LocaleName,
        DWORD     const dwMapFlags,
        PCCH      const lpSrcStr,
        int       const cchSrc,
        PCH       const lpDestStr,
        int       const cchDest,
        int       const code_page,
        BOOL      const bError
        )
{
    _LocaleUpdate _loc_update(plocinfo);

    return __acrt_LCMapStringA_stat(
            _loc_update.GetLocaleT(),
            LocaleName,
            dwMapFlags,
            lpSrcStr,
            cchSrc,
            lpDestStr,
            cchDest,
            code_page,
            bError
            );
}
