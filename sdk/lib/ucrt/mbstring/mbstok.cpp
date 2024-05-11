/***
*mbstok.c - Break string into tokens (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Break string into tokens (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

/***
* _mbstok - Break string into tokens (MBCS)
*
*Purpose:
*       strtok considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into string immediately
*       following the returned token. subsequent calls with zero for the first
*       argument (string) will work thru the string until no tokens remain. the
*       control string may be different from call to call. when no tokens remain
*       in string a nullptr pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*       MBCS chars supported correctly.
*
*Entry:
*       char *string = string to break into tokens.
*       char *sepset = set of characters to use as seperators
*
*Exit:
*       returns pointer to token, or nullptr if no more tokens
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbstok_l(
    unsigned char*       const string,
    unsigned char const* const sepset,
    _locale_t            const locale
    )
{
    return _mbstok_s_l(string, sepset, &__acrt_getptd()->_mbstok_token, locale);
}

extern "C" unsigned char * __cdecl _mbstok(
        unsigned char * string,
        const unsigned char * sepset
        )
{
    /* We call the deprecated _mbstok_l (and not _mbstok_s_l) so that we keep one
     * single nextoken in the single thread case, i.e. the nextoken declared as static
     * inside _mbstok_l
     */
    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    return _mbstok_l(string, sepset, nullptr);
    _END_SECURE_CRT_DEPRECATION_DISABLE
}
