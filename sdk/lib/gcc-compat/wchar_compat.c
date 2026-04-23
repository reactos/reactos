/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     wctype/wctob/btowc shims for GCC 15 libstdc++
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 *
 * GCC 15's libstdc++ ctype_members.o references wctype, wctob, and btowc.
 * wctype is only in MSVCR120 (not msvcrt at all); wctob and btowc are
 * version-gated to DLL_EXPORT_VERSION >= 0x600.
 */

#include <stdlib.h>
#include <string.h>

#undef wctype
#undef wctob
#undef btowc

/*
 * MSVC CRT character classification bitmasks (used by iswctype).
 */
#define CRT_UPPER   0x0001
#define CRT_LOWER   0x0002
#define CRT_DIGIT   0x0004
#define CRT_SPACE   0x0008
#define CRT_PUNCT   0x0010
#define CRT_CONTROL 0x0020
#define CRT_BLANK   0x0040
#define CRT_HEX     0x0080
#define CRT_ALPHA   0x0103

typedef unsigned short wctype_t;

wctype_t __cdecl wctype(const char *property)
{
    static const struct {
        const char *name;
        wctype_t mask;
    } props[] = {
        { "alnum",  CRT_DIGIT | CRT_ALPHA },
        { "alpha",  CRT_ALPHA },
        { "cntrl",  CRT_CONTROL },
        { "digit",  CRT_DIGIT },
        { "graph",  CRT_DIGIT | CRT_PUNCT | CRT_ALPHA },
        { "lower",  CRT_LOWER },
        { "print",  CRT_DIGIT | CRT_PUNCT | CRT_BLANK | CRT_ALPHA },
        { "punct",  CRT_PUNCT },
        { "space",  CRT_SPACE },
        { "upper",  CRT_UPPER },
        { "xdigit", CRT_HEX },
    };
    unsigned int i;

    for (i = 0; i < sizeof(props) / sizeof(props[0]); i++)
    {
        if (strcmp(property, props[i].name) == 0)
            return props[i].mask;
    }
    return 0;
}

int __cdecl wctob(unsigned int c)
{
    char buf;
    if (wctomb(&buf, (wchar_t)c) == 1)
        return (unsigned char)buf;
    return -1; /* EOF */
}

unsigned int __cdecl btowc(int c)
{
    wchar_t wc;
    unsigned char uc;

    if (c == -1)
        return (unsigned int)-1; /* WEOF */

    uc = (unsigned char)c;
    if (mbtowc(&wc, (const char *)&uc, 1) == 1)
        return (unsigned int)wc;
    return (unsigned int)-1; /* WEOF */
}

#ifdef _M_IX86
void *_imp__wctype = wctype;
void *_imp__wctob = wctob;
void *_imp__btowc = btowc;
#else
void *__imp_wctype = wctype;
void *__imp_wctob = wctob;
void *__imp_btowc = btowc;
#endif
