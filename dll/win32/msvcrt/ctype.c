/*
 * msvcrt.dll ctype functions
 *
 * Copyright 2000 Jon Griffiths
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

#include <locale.h>
#include "msvcrt.h"
#include "winnls.h"

/* Some abbreviations to make the following table readable */
#define _C_ _CONTROL
#define _S_ _SPACE
#define _P_ _PUNCT
#define _D_ _DIGIT
#define _H_ _HEX
#define _U_ _UPPER
#define _L_ _LOWER

WORD MSVCRT__ctype [257] = {
  0, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_C_, _S_|_C_,
  _S_|_C_, _S_|_C_, _S_|_C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_,
  _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_BLANK,
  _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_,
  _P_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_,
  _D_|_H_, _D_|_H_, _D_|_H_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _U_|_H_,
  _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_, _U_, _U_, _U_, _U_,
  _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_,
  _U_, _P_, _P_, _P_, _P_, _P_, _P_, _L_|_H_, _L_|_H_, _L_|_H_, _L_|_H_,
  _L_|_H_, _L_|_H_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_,
  _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _P_, _P_, _P_, _P_,
  _C_, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#if _MSVCR_VER <= 110
# define B110 _BLANK
#else
# define B110 0
#endif

#if _MSVCR_VER == 120
# define D120 0
#else
# define D120 4
#endif

#if _MSVCR_VER >= 140
# define S140 _SPACE
# define L140 _LOWER | 0x100
# define C140 _CONTROL
#else
# define S140 0
# define L140 0
# define C140 0
#endif
WORD MSVCRT__wctype[257] =
{
    0,
    /* 00 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0028 | B110, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020,
    /* 10 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 20 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 30 */
    0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084,
    0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 40 */
    0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* 50 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 60 */
    0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* 70 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020,
    /* 80 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020 | S140, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 90 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* a0 */
    0x0008 | B110, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010 | L140, 0x0010, 0x0010, 0x0010 | C140, 0x0010, 0x0010,
    /* b0 */
    0x0010, 0x0010, 0x0010 | D120, 0x0010 | D120, 0x0010, 0x0010 | L140, 0x0010, 0x0010,
    0x0010, 0x0010 | D120, 0x0010 | L140, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* c0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* d0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0010,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0102,
    /* e0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* f0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0010,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102
};

WORD *MSVCRT__pwctype = MSVCRT__wctype + 1;

/*********************************************************************
 *		__p__pctype (MSVCRT.@)
 */
unsigned short** CDECL __p__pctype(void)
{
    return &get_locinfo()->pctype;
}

/*********************************************************************
 *		__pctype_func (MSVCRT.@)
 */
const unsigned short* CDECL __pctype_func(void)
{
    return get_locinfo()->pctype;
}

/*********************************************************************
 *		__p__pwctype (MSVCRT.@)
 */
unsigned short** CDECL __p__pwctype(void)
{
    return &MSVCRT__pwctype;
}

/*********************************************************************
 *		__pwctype_func (MSVCRT.@)
 */
const unsigned short* CDECL __pwctype_func(void)
{
    return MSVCRT__pwctype;
}

/*********************************************************************
 *		_isctype_l (MSVCRT.@)
 */
int CDECL _isctype_l(int c, int type, _locale_t locale)
{
  pthreadlocinfo locinfo;

  if(!locale)
    locinfo = get_locinfo();
  else
    locinfo = locale->locinfo;

  if (c >= -1 && c <= 255)
    return locinfo->pctype[c] & type;

  if (locinfo->mb_cur_max != 1 && c > 0)
  {
    /* FIXME: Is there a faster way to do this? */
    WORD typeInfo;
    char convert[3], *pconv = convert;

    if (locinfo->pctype[(UINT)c >> 8] & _LEADBYTE)
      *pconv++ = (UINT)c >> 8;
    *pconv++ = c & 0xff;
    *pconv = 0;

    if (GetStringTypeExA(locinfo->lc_handle[LC_CTYPE],
                CT_CTYPE1, convert, convert[1] ? 2 : 1, &typeInfo))
      return typeInfo & type;
  }
  return 0;
}

/*********************************************************************
 *              _isctype (MSVCRT.@)
 */
int CDECL _isctype(int c, int type)
{
    return _isctype_l(c, type, NULL);
}

/*********************************************************************
 *		_isalnum_l (MSVCRT.@)
 */
int CDECL _isalnum_l(int c, _locale_t locale)
{
  return _isctype_l( c, _ALPHA | _DIGIT, locale );
}

/*********************************************************************
 *		isalnum (MSVCRT.@)
 */
int CDECL isalnum(int c)
{
  return _isctype( c, _ALPHA | _DIGIT );
}

/*********************************************************************
 *		_isalpha_l (MSVCRT.@)
 */
int CDECL _isalpha_l(int c, _locale_t locale)
{
  return _isctype_l( c, _ALPHA, locale );
}

/*********************************************************************
 *		isalpha (MSVCRT.@)
 */
int CDECL isalpha(int c)
{
  return _isctype( c, _ALPHA );
}

/*********************************************************************
 *		_iscntrl_l (MSVCRT.@)
 */
int CDECL _iscntrl_l(int c, _locale_t locale)
{
  return _isctype_l( c, _CONTROL, locale );
}

/*********************************************************************
 *		iscntrl (MSVCRT.@)
 */
int CDECL iscntrl(int c)
{
  return _isctype( c, _CONTROL );
}

/*********************************************************************
 *		_isdigit_l (MSVCRT.@)
 */
int CDECL _isdigit_l(int c, _locale_t locale)
{
  return _isctype_l( c, _DIGIT, locale );
}

/*********************************************************************
 *		isdigit (MSVCRT.@)
 */
int CDECL isdigit(int c)
{
  return _isctype( c, _DIGIT );
}

/*********************************************************************
 *		_isgraph_l (MSVCRT.@)
 */
int CDECL _isgraph_l(int c, _locale_t locale)
{
  return _isctype_l( c, _ALPHA | _DIGIT | _PUNCT, locale );
}

/*********************************************************************
 *		isgraph (MSVCRT.@)
 */
int CDECL isgraph(int c)
{
  return _isctype( c, _ALPHA | _DIGIT | _PUNCT );
}

/*********************************************************************
 *		_isleadbyte_l (MSVCRT.@)
 */
int CDECL _isleadbyte_l(int c, _locale_t locale)
{
  return _isctype_l( c, _LEADBYTE, locale );
}

/*********************************************************************
 *		isleadbyte (MSVCRT.@)
 */
int CDECL isleadbyte(int c)
{
  return _isctype( c, _LEADBYTE );
}

/*********************************************************************
 *		_islower_l (MSVCRT.@)
 */
int CDECL _islower_l(int c, _locale_t locale)
{
  return _isctype_l( c, _LOWER, locale );
}

/*********************************************************************
 *		islower (MSVCRT.@)
 */
int CDECL islower(int c)
{
  return _isctype( c, _LOWER );
}

/*********************************************************************
 *		_isprint_l (MSVCRT.@)
 */
int CDECL _isprint_l(int c, _locale_t locale)
{
  return _isctype_l( c, _ALPHA | _DIGIT | _BLANK | _PUNCT, locale );
}

/*********************************************************************
 *		isprint (MSVCRT.@)
 */
int CDECL isprint(int c)
{
  return _isctype( c, _ALPHA | _DIGIT | _BLANK | _PUNCT );
}

/*********************************************************************
 *		ispunct (MSVCRT.@)
 */
int CDECL ispunct(int c)
{
  return _isctype( c, _PUNCT );
}

/*********************************************************************
 *		_ispunct_l (MSVCR80.@)
 */
int CDECL _ispunct_l(int c, _locale_t locale)
{
  return _isctype_l( c, _PUNCT, locale );
}

/*********************************************************************
 *		_isspace_l (MSVCRT.@)
 */
int CDECL _isspace_l(int c, _locale_t locale)
{
  return _isctype_l( c, _SPACE, locale );
}

/*********************************************************************
 *		isspace (MSVCRT.@)
 */
int CDECL isspace(int c)
{
  return _isctype( c, _SPACE );
}

/*********************************************************************
 *		_isupper_l (MSVCRT.@)
 */
int CDECL _isupper_l(int c, _locale_t locale)
{
  return _isctype_l( c, _UPPER, locale );
}

/*********************************************************************
 *		isupper (MSVCRT.@)
 */
int CDECL isupper(int c)
{
  return _isctype( c, _UPPER );
}

/*********************************************************************
 *		_isxdigit_l (MSVCRT.@)
 */
int CDECL _isxdigit_l(int c, _locale_t locale)
{
  return _isctype_l( c, _HEX, locale );
}

/*********************************************************************
 *		isxdigit (MSVCRT.@)
 */
int CDECL isxdigit(int c)
{
  return _isctype( c, _HEX );
}

/*********************************************************************
 *		_isblank_l (MSVCRT.@)
 */
int CDECL _isblank_l(int c, _locale_t locale)
{
#if _MSVCR_VER < 140
    if (c == '\t') return _BLANK;
#endif
    return _isctype_l( c, _BLANK, locale );
}

/*********************************************************************
 *		isblank (MSVCRT.@)
 */
int CDECL isblank(int c)
{
  return c == '\t' || _isctype( c, _BLANK );
}

/*********************************************************************
 *		__isascii (MSVCRT.@)
 */
int CDECL __isascii(int c)
{
  return ((unsigned)c < 0x80);
}

/*********************************************************************
 *		__toascii (MSVCRT.@)
 */
int CDECL __toascii(int c)
{
  return (unsigned)c & 0x7f;
}

/*********************************************************************
 *		iswascii (MSVCRT.@)
 *
 */
int CDECL iswascii(wchar_t c)
{
  return ((unsigned)c < 0x80);
}

/*********************************************************************
 *		__iscsym (MSVCRT.@)
 */
int CDECL __iscsym(int c)
{
  return (c < 127 && (isalnum(c) || c == '_'));
}

/*********************************************************************
 *		__iscsymf (MSVCRT.@)
 */
int CDECL __iscsymf(int c)
{
  return (c < 127 && (isalpha(c) || c == '_'));
}

/*********************************************************************
 *		__iswcsym (MSVCRT.@)
 */
int CDECL __iswcsym(wint_t c)
{
  return (iswalnum(c) || c == '_');
}

/*********************************************************************
 *		__iswcsymf (MSVCRT.@)
 */
int CDECL __iswcsymf(wint_t c)
{
  return (iswalpha(c) || c == '_');
}

/*********************************************************************
 *		_toupper_l (MSVCRT.@)
 */
int CDECL _toupper_l(int c, _locale_t locale)
{
    pthreadlocinfo locinfo;
    unsigned char str[2], *p = str, ret[2];

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if((unsigned)c < 256)
    {
        if(locinfo->pctype[c] & _LEADBYTE)
            return c;
        return locinfo->pcumap[c];
    }

    if(locinfo->pctype[(c>>8)&255] & _LEADBYTE)
        *p++ = (c>>8) & 255;
    else {
        *_errno() = EILSEQ;
        str[1] = 0;
    }
    *p++ = c & 255;

    switch(__crtLCMapStringA(locinfo->lc_handle[LC_CTYPE], LCMAP_UPPERCASE,
                (char*)str, p-str, (char*)ret, 2, locinfo->lc_codepage, 0))
    {
    case 0:
        return c;
    case 1:
        return ret[0];
    default:
        return ret[0] + (ret[1]<<8);
    }
}

/*********************************************************************
 *		toupper (MSVCRT.@)
 */
int CDECL toupper(int c)
{
    if(initial_locale)
        return c>='a' && c<='z' ? c-'a'+'A' : c;
    return _toupper_l(c, NULL);
}

/*********************************************************************
 *		_toupper (MSVCRT.@)
 */
int CDECL _toupper(int c)
{
    return c - 0x20;  /* sic */
}

/*********************************************************************
 *              _tolower_l (MSVCRT.@)
 */
int CDECL _tolower_l(int c, _locale_t locale)
{
    pthreadlocinfo locinfo;
    unsigned char str[2], *p = str, ret[2];

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if((unsigned)c < 256)
    {
        if(locinfo->pctype[c] & _LEADBYTE)
            return c;
        return locinfo->pclmap[c];
    }

    if(locinfo->pctype[(c>>8)&255] & _LEADBYTE)
        *p++ = (c>>8) & 255;
    else {
        *_errno() = EILSEQ;
        str[1] = 0;
    }
    *p++ = c & 255;

    switch(__crtLCMapStringA(locinfo->lc_handle[LC_CTYPE], LCMAP_LOWERCASE,
                (char*)str, p-str, (char*)ret, 2, locinfo->lc_codepage, 0))
    {
    case 0:
        return c;
    case 1:
        return ret[0];
    default:
        return ret[0] + (ret[1]<<8);
    }
}

/*********************************************************************
 *              tolower (MSVCRT.@)
 */
int CDECL tolower(int c)
{
    if(initial_locale)
        return c>='A' && c<='Z' ? c-'A'+'a' : c;
    return _tolower_l(c, NULL);
}

/*********************************************************************
 *		_tolower (MSVCRT.@)
 */
int CDECL _tolower(int c)
{
    return c + 0x20;  /* sic */
}

#if _MSVCR_VER>=120
/*********************************************************************
 *              wctype (MSVCR120.@)
 */
unsigned short __cdecl wctype(const char *property)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum", _DIGIT|_ALPHA },
        { "alpha", _ALPHA },
        { "cntrl", _CONTROL },
        { "digit", _DIGIT },
        { "graph", _DIGIT|_PUNCT|_ALPHA },
        { "lower", _LOWER },
        { "print", _DIGIT|_PUNCT|_BLANK|_ALPHA },
        { "punct", _PUNCT },
        { "space", _SPACE },
        { "upper", _UPPER },
        { "xdigit", _HEX }
    };
    unsigned int i;

    for(i=0; i<ARRAY_SIZE(properties); i++)
        if(!strcmp(property, properties[i].name))
            return properties[i].mask;

    return 0;
}
#endif
