//
// wctrans.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Implementations of the towctrans and wctrans functions.
//
#include <corecrt.h>
#include <ctype.h>
#include <string.h>
#include <wctype.h>

#pragma warning(disable:4244)



typedef wchar_t wctrans_t;

static struct wctab
{
    char const* s;
    wctype_t    value;
}
const tab[] =
{
    { "tolower", 2 },
    { "toupper", 1 },
    { nullptr,   0 }
};



extern "C" wint_t __cdecl towctrans(wint_t const c, wctrans_t const value)
{
    return value == 1 ? towupper(c) : towlower(c);
}

extern "C" wctrans_t __cdecl wctrans(char const* const name)
{
    for (unsigned n = 0; tab[n].s != 0; ++n)
    {
        if (strcmp(tab[n].s, name) == 0)
            return tab[n].value;
    }

    return 0;
}

/*
 * Copyright (c) 1992-2007 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V5.03:0009 */
