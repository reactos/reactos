//
// wctype.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Implementations of the wctype function.
//
#include <corecrt.h>
#include <string.h>
#include <wctype.h>



static struct wctab
{
    char const* s;
    wctype_t    value;
}
const tab[] =
{
    { "alnum",  _ALPHA | _DIGIT                   },
    { "alpha",  _ALPHA                            },
    { "cntrl",  _CONTROL                          },
    { "digit",  _DIGIT                            },
    { "graph",  _PUNCT | _ALPHA | _DIGIT          },
    { "lower",  _LOWER                            },
    { "print",  _BLANK | _PUNCT | _ALPHA | _DIGIT },
    { "punct",  _PUNCT                            },
    { "blank",  _BLANK                            },
    { "space",  _SPACE                            },
    { "upper",  _UPPER                            },
    { "xdigit", _HEX                              },
    { nullptr,  0                                 }
};



#pragma warning(push)
#pragma warning(disable: 4273)

extern "C" wctype_t __cdecl wctype(char const* const name)
{
    for (unsigned n = 0; tab[n].s != 0; ++n)
    {
        if (strcmp(tab[n].s, name) == 0)
            return tab[n].value;
    }

    return 0;
}

#pragma warning(pop)

/*
 * Copyright (c) 1992-2007 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V5.03:0009 */
