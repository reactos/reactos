//
// iswctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Support functionality for the wide character classification functions and
// macros.
//
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>



// Ensure that the masks used by GetCharType() match the CRT masks, so that we
// can simply reinterpret the masks.
#if _UPPER   != C1_UPPER || \
    _LOWER   != C1_LOWER || \
    _DIGIT   != C1_DIGIT || \
    _SPACE   != C1_SPACE || \
    _PUNCT   != C1_PUNCT || \
    _CONTROL != C1_CNTRL
    #error Character type masks do not agree in ctype and winnls
#endif



// The iswctype functions are called by the wide character classification
// functions and macros (e.g. iswalpha) when their argument is a wide
// character greater than 255.  It is also a standard function and can be
// called by the user, even for characters less than 256.  The function
// returns nonzero if the argument c satisfies the character class property
// encoded by the mask.  Returns zero otherwise, or for WEOF.  These functions
// are neither locale nor codepage dependent.
extern "C" int __cdecl _iswctype_l(wint_t const c, wctype_t const mask, _locale_t)
{
    return iswctype(c, mask);
}

extern "C" int __cdecl iswctype(wint_t const c, wctype_t const mask)
{
    if (c == WEOF)
        return 0;

    if (c < 256)
        return static_cast<int>(_pwctype[c] & mask);

    wchar_t const wide_character = c;

    wint_t char_type = 0;
    if (__acrt_GetStringTypeW(CT_CTYPE1, &wide_character, 1, &char_type) == 0)
        return 0;

    return static_cast<int>(char_type & mask);
}
