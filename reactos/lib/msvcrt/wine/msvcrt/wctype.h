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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_WCTYPE_H
#define __WINE_WCTYPE_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

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

#ifndef USE_MSVCRT_PREFIX
# ifndef WEOF
#  define WEOF        (wint_t)(0xFFFF)
# endif
#else
# ifndef MSVCRT_WEOF
#  define MSVCRT_WEOF (MSVCRT_wint_t)(0xFFFF)
# endif
#endif /* USE_MSVCRT_PREFIX */

#ifndef MSVCRT_WCTYPE_T_DEFINED
typedef MSVCRT(wchar_t) MSVCRT(wint_t);
typedef MSVCRT(wchar_t) MSVCRT(wctype_t);
#define MSVCRT_WCTYPE_T_DEFINED
#endif

/* FIXME: there's something to do with __p__pctype and __p__pwctype */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSVCRT_WCTYPE_DEFINED
#define MSVCRT_WCTYPE_DEFINED
int MSVCRT(is_wctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(isleadbyte)(int);
int MSVCRT(iswalnum)(MSVCRT(wint_t));
int MSVCRT(iswalpha)(MSVCRT(wint_t));
int MSVCRT(iswascii)(MSVCRT(wint_t));
int MSVCRT(iswcntrl)(MSVCRT(wint_t));
int MSVCRT(iswctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(iswdigit)(MSVCRT(wint_t));
int MSVCRT(iswgraph)(MSVCRT(wint_t));
int MSVCRT(iswlower)(MSVCRT(wint_t));
int MSVCRT(iswprint)(MSVCRT(wint_t));
int MSVCRT(iswpunct)(MSVCRT(wint_t));
int MSVCRT(iswspace)(MSVCRT(wint_t));
int MSVCRT(iswupper)(MSVCRT(wint_t));
int MSVCRT(iswxdigit)(MSVCRT(wint_t));
MSVCRT(wchar_t) MSVCRT(towlower)(MSVCRT(wchar_t));
MSVCRT(wchar_t) MSVCRT(towupper)(MSVCRT(wchar_t));
#endif /* MSVCRT_WCTYPE_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCTYPE_H */
