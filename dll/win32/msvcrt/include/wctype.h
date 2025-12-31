/*
 * Unicode definitions
 *
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_WCTYPE_H
#define __WINE_WCTYPE_H

#include <corecrt.h>

#include <pshpack8.h>

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

/* FIXME: there's something to do with __p__pctype and __p__pwctype */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
_ACRTIMP int __cdecl is_wctype(wint_t,wctype_t);
_ACRTIMP int __cdecl isleadbyte(int);
_ACRTIMP int __cdecl iswalnum(wint_t);
_ACRTIMP int __cdecl iswalpha(wint_t);
_ACRTIMP int __cdecl iswascii(wint_t);
_ACRTIMP int __cdecl iswcntrl(wint_t);
_ACRTIMP int __cdecl iswctype(wint_t,wctype_t);
_ACRTIMP int __cdecl iswdigit(wint_t);
_ACRTIMP int __cdecl iswgraph(wint_t);
_ACRTIMP int __cdecl iswlower(wint_t);
_ACRTIMP int __cdecl iswprint(wint_t);
_ACRTIMP int __cdecl iswpunct(wint_t);
_ACRTIMP int __cdecl iswspace(wint_t);
_ACRTIMP int __cdecl iswupper(wint_t);
_ACRTIMP int __cdecl iswxdigit(wint_t);
_ACRTIMP wint_t __cdecl towlower(wint_t);
_ACRTIMP wint_t __cdecl towupper(wint_t);
#endif /* _WCTYPE_DEFINED */

typedef wchar_t wctrans_t;
wint_t __cdecl towctrans(wint_t,wctrans_t);
wctrans_t __cdecl wctrans(const char *);
wctype_t __cdecl wctype(const char *);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_WCTYPE_H */
