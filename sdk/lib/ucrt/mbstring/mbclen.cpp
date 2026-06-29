/***
*mbclen.c - Find length of MBCS character
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Find length of MBCS character
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbclen - Find length of MBCS character
*
*Purpose:
*       Find the length of the MBCS character (in bytes).
*
*Entry:
*       unsigned char *c = MBCS character
*
*Exit:
*       Returns the number of bytes in the MBCS character
*
*Exceptions:
*
*******************************************************************************/

extern "C" size_t __cdecl _mbclen_l(unsigned char const* c, _locale_t locale)
{
    /*  Don't return two if we have leadbyte, EOS.
        Don't assert here; too low level
    */
    return ((_ismbblead_l)(*c, locale) && c[1] != '\0') ? 2 : 1;
}

extern "C" size_t __cdecl _mbclen(unsigned char const* c)
{
    /*  Don't return two if we have leadbyte, EOS.
        Don't assert here; too low level
    */
    return (_ismbblead(*c) && c[1]!='\0')  ? 2 : 1;
}
