/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/stdlib/mbtowc.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*********************************************************************
 *		_mbtowc_l(MSVCRT.@)
 */
int CDECL _mbtowc_l(wchar_t *dst, const char* str, size_t n, _locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    wchar_t tmpdst = '\0';

    if (!locale)
        locinfo = get_locinfo();
    else
        locinfo = (MSVCRT_pthreadlocinfo)(locale->locinfo);

    if (n <= 0 || !str)
        return 0;

    if (!*str) {
        if (dst) *dst = 0;
        return 0;
    }

    if (!locinfo->lc_codepage) {
        if (dst) *dst = (unsigned char)*str;
        return 1;
    }
    if (n >= 2 && _isleadbyte_l((unsigned char)*str, locale)) {
        if (!MultiByteToWideChar(locinfo->lc_codepage, 0, str, 2, &tmpdst, 1))
            return -1;
        if (dst) *dst = tmpdst;
        return 2;
    }
    if (!MultiByteToWideChar(locinfo->lc_codepage, 0, str, 1, &tmpdst, 1))
        return -1;
    if (dst) *dst = tmpdst;
    return 1;
}

/*********************************************************************
 *              mbtowc(MSVCRT.@)
 */
int CDECL mbtowc(wchar_t *dst, const char* str, size_t n)
{
    return _mbtowc_l(dst, str, n, NULL);
}
