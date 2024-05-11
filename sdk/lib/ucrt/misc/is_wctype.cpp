/***
*iswctype.c - support isw* wctype functions/macros for wide characters
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines iswctype - support isw* wctype functions/macros for
*       wide characters (esp. > 255).
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>

/***
*is_wctype - support obsolete name
*
*Purpose:
*       Name changed from is_wctype to iswctype. is_wctype must be supported.
*
*Entry:
*       wchar_t c    - the wide character whose type is to be tested
*       wchar_t mask - the mask used by the isw* functions/macros
*                       corresponding to each character class property
*
*Exit:
*       Returns non-zero if c is of the character class.
*       Returns 0 if c is not of the character class.
*
*Exceptions:
*       Returns 0 on any error.
*
*******************************************************************************/
extern "C" int __cdecl is_wctype (
        wint_t c,
        wctype_t mask
        )
{
    return iswctype(c, mask);
}