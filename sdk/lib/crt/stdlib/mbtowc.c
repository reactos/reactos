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

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = (MSVCRT_pthreadlocinfo)(locale->locinfo);

    if(n <= 0 || !str)
        return 0;
    if(!locinfo->lc_codepage)
        tmpdst = (unsigned char)*str;
    else if(!MultiByteToWideChar(locinfo->lc_codepage, 0, str, n, &tmpdst, 1))
        return -1;
    if(dst)
        *dst = tmpdst;
    /* return the number of bytes from src that have been used */
    if(!*str)
        return 0;
    if(n >= 2 && _isleadbyte_l((unsigned char)*str, locale) && str[1])
        return 2;
    return 1;
}

/*********************************************************************
 *              mbtowc(MSVCRT.@)
 */
int CDECL mbtowc(wchar_t *dst, const char* str, size_t n)
{
    return _mbtowc_l(dst, str, n, NULL);
}
