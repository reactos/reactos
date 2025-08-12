/*
 * msvcrt.dll mbcs functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
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
 *
 * FIXME
 * Not currently binary compatible with win32. MSVCRT_mbctype must be
 * populated correctly and the ismb* functions should reference it.
 */

#include <stdio.h>
#include <limits.h>
#include <mbctype.h>
#include <mbstring.h>

#include "msvcrt.h"
#include "mtdll.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned char MSVCRT_mbctype[257] = { 0 };

/* It seems that the data about valid trail bytes is not available from kernel32
 * so we have to store is here. The format is the same as for lead bytes in CPINFO */
struct cp_extra_info_t
{
    int cp;
    BYTE TrailBytes[MAX_LEADBYTES];
};

static struct cp_extra_info_t g_cpextrainfo[] =
{
    {932, {0x40, 0x7e, 0x80, 0xfc, 0, 0}},
    {936, {0x40, 0xfe, 0, 0}},
    {949, {0x41, 0xfe, 0, 0}},
    {950, {0x40, 0x7e, 0xa1, 0xfe, 0, 0}},
    {1361, {0x31, 0x7e, 0x81, 0xfe, 0, 0}},
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};

/* Maps cp932 single byte character to multi byte character */
static const unsigned char mbbtombc_932[] = {
  0x40,0x49,0x68,0x94,0x90,0x93,0x95,0x66,0x69,0x6a,0x96,0x7b,0x43,0x7c,0x44,0x5e,
  0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x46,0x47,0x83,0x81,0x84,0x48,
  0x97,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,
  0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x6d,0x8f,0x6e,0x4f,0x76,
  0x77,0x78,0x79,0x6d,0x8f,0x6e,0x4f,0x51,0x65,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
  0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x50,
       0x42,0x75,0x76,0x41,0x45,0x92,0x40,0x42,0x44,0x46,0x48,0x83,0x85,0x87,0x62,
  0x5b,0x41,0x43,0x45,0x47,0x49,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,
  0x5e,0x60,0x63,0x65,0x67,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x71,0x74,0x77,0x7a,0x7d,
  0x7e,0x80,0x81,0x82,0x84,0x86,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8f,0x93,0x4a,0x4b };

/* Maps multibyte cp932 punctuation marks to single byte equivalents */
static const unsigned char mbctombb_932_punct[] = {
  0x20,0xa4,0xa1,0x2c,0x2e,0xa5,0x3a,0x3b,0x3f,0x21,0xde,0xdf,0x00,0x00,0x00,0x5e,
  0x7e,0x5f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x00,0x00,0x2f,0x00,
  0x00,0x00,0x7c,0x00,0x00,0x60,0x27,0x00,0x22,0x28,0x29,0x00,0x00,0x5b,0x5d,0x7b,
  0x7d,0x00,0x00,0x00,0x00,0xa2,0xa3,0x00,0x00,0x00,0x00,0x2b,0x2d,0x00,0x00,0x00,
  0x00,0x3d,0x00,0x3c,0x3e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5c,
  0x24,0x00,0x00,0x25,0x23,0x26,0x2a,0x40};

/* Maps multibyte cp932 hiragana/katakana to single-byte equivalents */
static const unsigned char mbctombb_932_kana[] = {
  0xa7,0xb1,0xa8,0xb2,0xa9,0xb3,0xaa,0xb4,0xab,0xb5,0xb6,0xb6,0xb7,0xb7,0xb8,0xb8,
  0xb9,0xb9,0xba,0xba,0xbb,0xbb,0xbc,0xbc,0xbd,0xbd,0xbe,0xbe,0xbf,0xbf,0xc0,0xc0,
  0xc1,0xc1,0xaf,0xc2,0xc2,0xc3,0xc3,0xc4,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xca,
  0xca,0xcb,0xcb,0xcb,0xcc,0xcc,0xcc,0xcd,0xcd,0xcd,0xce,0xce,0xce,0xcf,0xd0,0xd1,
  0xd2,0xd3,0xac,0xd4,0xad,0xd5,0xae,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdc,0xb2,
  0xb4,0xa6,0xdd,0xb3,0xb6,0xb9};

static wchar_t msvcrt_mbc_to_wc_l(unsigned int ch, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    wchar_t chW;
    char mbch[2];
    int n_chars;

    if(locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (ch <= 0xff) {
        mbch[0] = ch;
        n_chars = 1;
    } else {
        mbch[0] = (ch >> 8) & 0xff;
        mbch[1] = ch & 0xff;
        n_chars = 2;
    }
    if (!MultiByteToWideChar(mbcinfo->mbcodepage, 0, mbch, n_chars, &chW, 1))
    {
        WARN("MultiByteToWideChar failed on %x\n", ch);
        return 0;
    }
    return chW;
}

static inline size_t u_strlen( const unsigned char *str )
{
  return strlen( (const char*) str );
}

static inline unsigned char* u_strncat( unsigned char* dst, const unsigned char* src, size_t len )
{
  return (unsigned char*)strncat( (char*)dst, (const char*)src, len);
}

static inline int u_strcmp( const unsigned char *s1, const unsigned char *s2 )
{
  return strcmp( (const char*)s1, (const char*)s2 );
}

static inline int u_strcasecmp( const unsigned char *s1, const unsigned char *s2 )
{
  return _stricmp( (const char*)s1, (const char*)s2 );
}

static inline int u_strncmp( const unsigned char *s1, const unsigned char *s2, size_t len )
{
  return strncmp( (const char*)s1, (const char*)s2, len );
}

static inline int u_strncasecmp( const unsigned char *s1, const unsigned char *s2, size_t len )
{
  return _strnicmp( (const char*)s1, (const char*)s2, len );
}

static inline unsigned char *u_strchr( const unsigned char *s, unsigned char x )
{
  return (unsigned char*) strchr( (const char*)s, x );
}

static inline unsigned char* u_strpbrk(const unsigned char *str, const unsigned char *accept)
{
  return (unsigned char*)strpbrk((const char*)str, (const char*)accept);
}

static inline unsigned char *u_strrchr( const unsigned char *s, unsigned char x )
{
  return (unsigned char*) strrchr( (const char*)s, x );
}

static inline unsigned char* u__strrev(unsigned char *str)
{
  return (unsigned char*)_strrev((char *)str);
}

static inline unsigned char *u__strset( unsigned char *s, unsigned char c )
{
  return (unsigned char*) _strset( (char*)s, c);
}

static inline unsigned char *u__strnset( unsigned char *s, unsigned char c, size_t len )
{
  return (unsigned char*) _strnset( (char*)s, c, len );
}

/*********************************************************************
 *		__p__mbctype (MSVCRT.@)
 */
unsigned char* CDECL __p__mbctype(void)
{
  return get_mbcinfo()->mbctype;
}

/*********************************************************************
 *		__p___mb_cur_max(MSVCRT.@)
 */
int* CDECL __p___mb_cur_max(void)
{
  return &get_locinfo()->mb_cur_max;
}

/*********************************************************************
 *		___mb_cur_max_func(MSVCRT.@)
 */
int CDECL ___mb_cur_max_func(void)
{
  return get_locinfo()->mb_cur_max;
}

#if _MSVCR_VER>=80
/*********************************************************************
 *		___mb_cur_max_l_func  (MSVCR80.@)
 */
int CDECL ___mb_cur_max_l_func(_locale_t locale)
{
  pthreadlocinfo locinfo;

  if(!locale)
    locinfo = get_locinfo();
  else
    locinfo = locale->locinfo;

  return locinfo->mb_cur_max;
}
#endif

threadmbcinfo* create_mbcinfo(int cp, LCID lcid, threadmbcinfo *old_mbcinfo)
{
  threadmbcinfo *mbcinfo;
  int newcp;
  CPINFO cpi;
  BYTE *bytes;
  WORD chartypes[256];
  char bufA[256];
  WCHAR bufW[256], lowW[256], upW[256];
  int charcount, maxchar;
  int ret;
  int i;

  if(old_mbcinfo && cp==old_mbcinfo->mbcodepage
          && (lcid==-1 || lcid==old_mbcinfo->mblcid)) {
    InterlockedIncrement(&old_mbcinfo->refcount);
    return old_mbcinfo;
  }

  mbcinfo = malloc(sizeof(threadmbcinfo));
  if(!mbcinfo)
    return NULL;
  mbcinfo->refcount = 1;

  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = get_locinfo()->lc_codepage;
      if(newcp)
          break;
      /* fall through (C locale) */
    case _MB_CP_SBCS:
      newcp = 20127;   /* ASCII */
      break;
    default:
      newcp = cp;
      break;
  }

  if(lcid == -1) {
    WCHAR wbuf[LOCALE_NAME_MAX_LENGTH];
    sprintf(bufA, ".%d", newcp);
    mbcinfo->mblcid = locale_to_sname(bufA, NULL, NULL, wbuf) ? LocaleNameToLCID(wbuf, LOCALE_ALLOW_NEUTRAL_NAMES) : -1;
  } else {
    mbcinfo->mblcid = lcid;
  }

  if(mbcinfo->mblcid == -1)
  {
    WARN("Can't assign LCID to codepage (%d)\n", mbcinfo->mblcid);
    mbcinfo->mblcid = 0;
  }

  if (!GetCPInfo(newcp, &cpi))
  {
    WARN("Codepage %d not found\n", newcp);
    free(mbcinfo);
    return NULL;
  }

  /* setup the _mbctype */
  memset(mbcinfo->mbctype, 0, sizeof(unsigned char[257]));
  memset(mbcinfo->mbcasemap, 0, sizeof(unsigned char[256]));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      mbcinfo->mbctype[i + 1] |= _M1;
    bytes += 2;
  }

  if (cpi.MaxCharSize == 2)
  {
    /* trail bytes not available through kernel32 but stored in a structure in msvcrt */
    struct cp_extra_info_t *cpextra = g_cpextrainfo;

    mbcinfo->ismbcodepage = 1;
    while (TRUE)
    {
      if (cpextra->cp == 0 || cpextra->cp == newcp)
      {
        if (cpextra->cp == 0)
          FIXME("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);

        bytes = cpextra->TrailBytes;
        while (bytes[0] || bytes[1])
        {
          for (i = bytes[0]; i <= bytes[1]; i++)
            mbcinfo->mbctype[i + 1] |= _M2;
          bytes += 2;
        }
        break;
      }
      cpextra++;
    }
  }
  else
    mbcinfo->ismbcodepage = 0;

  maxchar = (newcp == CP_UTF8) ? 128 : 256;

  /* we can't use GetStringTypeA directly because we don't have a locale - only a code page
   */
  charcount = 0;
  for (i = 0; i < maxchar; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
  {
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%ld\n",
            newcp, ret, charcount, GetLastError());
  }

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);
  LCMapStringW(mbcinfo->mblcid, LCMAP_LOWERCASE, bufW, charcount, lowW, charcount);
  LCMapStringW(mbcinfo->mblcid, LCMAP_UPPERCASE, bufW, charcount, upW, charcount);

  charcount = 0;
  for (i = 0; i < maxchar; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if (chartypes[charcount] & C1_UPPER)
      {
        mbcinfo->mbctype[i + 1] |= _SBUP;
        bufW[charcount] = lowW[charcount];
      }
      else if (chartypes[charcount] & C1_LOWER)
      {
	mbcinfo->mbctype[i + 1] |= _SBLOW;
        bufW[charcount] = upW[charcount];
      }
      charcount++;
    }

  ret = WideCharToMultiByte(newcp, 0, bufW, charcount, bufA, charcount, NULL, NULL);
  if (ret != charcount)
  {
    ERR("WideCharToMultiByte failed for cp %d, ret=%d (exp %d), error=%ld\n",
            newcp, ret, charcount, GetLastError());
  }

  charcount = 0;
  for (i = 0; i < maxchar; i++)
  {
    if(!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if(mbcinfo->mbctype[i + 1] & (_SBUP | _SBLOW))
        mbcinfo->mbcasemap[i] = bufA[charcount];
      charcount++;
    }
  }

  if (newcp == 932)   /* CP932 only - set _MP and _MS */
  {
    /* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
     * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
     * it hard. As this is set only for codepage 932 we hardcode it what gives
     * also faster execution.
     */
    for (i = 161; i <= 165; i++)
      mbcinfo->mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      mbcinfo->mbctype[i + 1] |= _MS;
  }

  mbcinfo->mbcodepage = newcp;
  return mbcinfo;
}

/*********************************************************************
 *              _setmbcp (MSVCRT.@)
 */
int CDECL _setmbcp(int cp)
{
    thread_data_t *data = msvcrt_get_thread_data();
    threadmbcinfo *mbcinfo;

    mbcinfo = create_mbcinfo(cp, -1, get_mbcinfo());
    if(!mbcinfo)
    {
        *_errno() = EINVAL;
        return -1;
    }

    if(data->locale_flags & LOCALE_THREAD)
    {
        if(data->locale_flags & LOCALE_FREE)
            free_mbcinfo(data->mbcinfo);
        data->mbcinfo = mbcinfo;
    }
    else
    {
        _lock(_MB_CP_LOCK);
        free_mbcinfo(MSVCRT_locale->mbcinfo);
        MSVCRT_locale->mbcinfo = mbcinfo;
        memcpy(MSVCRT_mbctype, MSVCRT_locale->mbcinfo->mbctype, sizeof(MSVCRT_mbctype));
        _unlock(_MB_CP_LOCK);
    }
    return 0;
}

/*********************************************************************
 *		_getmbcp (MSVCRT.@)
 */
int CDECL _getmbcp(void)
{
  return get_mbcinfo()->mbcodepage;
}

/*********************************************************************
 *		_mbsnextc_l(MSVCRT.@)
 */
unsigned int CDECL _mbsnextc_l(const unsigned char* str, _locale_t locale)
{
  if(_ismbblead_l(*str, locale))
    return *str << 8 | str[1];
  return *str;
}

/*********************************************************************
 *		_mbsnextc(MSVCRT.@)
 */
unsigned int CDECL _mbsnextc(const unsigned char* str)
{
    return _mbsnextc_l(str, NULL);
}

/*********************************************************************
 *		_mbctolower_l(MSVCRT.@)
 */
unsigned int CDECL _mbctolower_l(unsigned int c, _locale_t locale)
{
    unsigned char str[2], ret[2];
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if (c > 0xff)
    {
        if (!_ismbblead_l((c >> 8) & 0xff, locale))
            return c;

        str[0] = c >> 8;
        str[1] = c;
        switch(__crtLCMapStringA(mbcinfo->mblcid, LCMAP_LOWERCASE,
                    (char*)str, 2, (char*)ret, 2, mbcinfo->mbcodepage, 0))
        {
        case 0:
            return c;
        case 1:
            return ret[0];
        default:
            return ret[1] + (ret[0] << 8);
        }
    }

    return mbcinfo->mbctype[c + 1] & _SBUP ? mbcinfo->mbcasemap[c] : c;
}

/*********************************************************************
 *		_mbctolower(MSVCRT.@)
 */
unsigned int CDECL _mbctolower(unsigned int c)
{
    return _mbctolower_l(c, NULL);
}

/*********************************************************************
 *		_mbctoupper_l(MSVCRT.@)
 */
unsigned int CDECL _mbctoupper_l(unsigned int c, _locale_t locale)
{
    unsigned char str[2], ret[2];
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if (c > 0xff)
    {
        if (!_ismbblead_l((c >> 8) & 0xff, locale))
            return c;

        str[0] = c >> 8;
        str[1] = c;
        switch(__crtLCMapStringA(mbcinfo->mblcid, LCMAP_UPPERCASE,
                    (char*)str, 2, (char*)ret, 2, mbcinfo->mbcodepage, 0))
        {
        case 0:
            return c;
        case 1:
            return ret[0];
        default:
            return ret[1] + (ret[0] << 8);
        }
    }

    return mbcinfo->mbctype[c + 1] & _SBLOW ? mbcinfo->mbcasemap[c] : c;
}

/*********************************************************************
 *		_mbctoupper(MSVCRT.@)
 */
unsigned int CDECL _mbctoupper(unsigned int c)
{
    return _mbctoupper_l(c, NULL);
}

/*********************************************************************
 *		_mbctombb_l (MSVCRT.@)
 */
unsigned int CDECL _mbctombb_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    unsigned int value;

    if(locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if(mbcinfo->mbcodepage == 932)
    {
        if(c >= 0x829f && c <= 0x82f1)    /* Hiragana */
            return mbctombb_932_kana[c - 0x829f];
        if(c >= 0x8340 && c <= 0x8396 && c != 0x837f)    /* Katakana */
            return mbctombb_932_kana[c - 0x8340 - (c >= 0x837f ? 1 : 0)];
        if(c >= 0x8140 && c <= 0x8197)    /* Punctuation */
        {
            value = mbctombb_932_punct[c - 0x8140];
            return value ? value : c;
        }
        if((c >= 0x824f && c <= 0x8258) || /* Fullwidth digits */
           (c >= 0x8260 && c <= 0x8279))   /* Fullwidth capitals letters */
            return c - 0x821f;
        if(c >= 0x8281 && c <= 0x829a)     /* Fullwidth small letters */
            return c - 0x8220;
        /* all other cases return c */
    }
    return c;
}

/*********************************************************************
 *		_mbctombb (MSVCRT.@)
 */
unsigned int CDECL _mbctombb(unsigned int c)
{
    return _mbctombb_l(c, NULL);
}

/*********************************************************************
 *		_mbcjistojms_l(MSVCRT.@)
 *
 *		Converts a jis character to sjis.
 *		Based on description from
 *		http://www.slayers.ne.jp/~oouchi/code/jistosjis.html
 */
unsigned int CDECL _mbcjistojms_l(unsigned int c, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(mbcinfo->mbcodepage == 932)
  {
    if(HIBYTE(c) >= 0x21 && HIBYTE(c) <= 0x7e &&
       LOBYTE(c) >= 0x21 && LOBYTE(c) <= 0x7e)
    {
      if(HIBYTE(c) % 2)
        c += 0x1f;
      else
        c += 0x7d;

      if(LOBYTE(c) >= 0x7F)
        c += 0x1;

      c = (((HIBYTE(c) - 0x21)/2 + 0x81) << 8) | LOBYTE(c);

      if(HIBYTE(c) > 0x9f)
        c += 0x4000;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

/*********************************************************************
 *		_mbcjistojms(MSVCRT.@)
 */
unsigned int CDECL _mbcjistojms(unsigned int c)
{
    return _mbcjistojms_l(c, NULL);
}

/*********************************************************************
 *		_mbcjmstojis_l(MSVCRT.@)
 *
 *		Converts a sjis character to jis.
 */
unsigned int CDECL _mbcjmstojis_l(unsigned int c, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(mbcinfo->mbcodepage == 932)
  {
    if(_ismbclegal_l(c, locale) && HIBYTE(c) < 0xf0)
    {
      if(HIBYTE(c) >= 0xe0)
        c -= 0x4000;

      c = (((HIBYTE(c) - 0x81)*2 + 0x21) << 8) | LOBYTE(c);

      if(LOBYTE(c) > 0x7f)
        c -= 0x1;

      if(LOBYTE(c) > 0x9d)
        c += 0x83;
      else
        c -= 0x1f;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

/*********************************************************************
 *		_mbcjmstojis(MSVCRT.@)
 */
unsigned int CDECL _mbcjmstojis(unsigned int c)
{
    return _mbcjmstojis_l(c, NULL);
}

/*********************************************************************
 *		_mbclen_l(MSVCRT.@)
 */
size_t CDECL _mbclen_l(const unsigned char* str, _locale_t locale)
{
    return _ismbblead_l(*str, locale) && str[1] ? 2 : 1;
}

/*********************************************************************
 *		_mbclen(MSVCRT.@)
 */
size_t CDECL _mbclen(const unsigned char* str)
{
    return _mbclen_l(str, NULL);
}

/*********************************************************************
 *		_mbsinc_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsinc_l(const unsigned char* str, _locale_t locale)
{
    return (unsigned char *)(str + _mbclen_l(str, locale));
}

/*********************************************************************
 *		_mbsinc(MSVCRT.@)
 */
unsigned char* CDECL _mbsinc(const unsigned char* str)
{
    return _mbsinc_l(str, NULL);
}

/*********************************************************************
 *		_mbsninc(MSVCRT.@)
 */
unsigned char* CDECL _mbsninc(const unsigned char* str, size_t num)
{
  if(!str)
    return NULL;

  while (num > 0 && *str)
  {
    if (_ismbblead(*str))
    {
      if (!*(str+1))
         break;
      str++;
    }
    str++;
    num--;
  }

  return (unsigned char*)str;
}

/*********************************************************************
 *              _mbsnlen_l(MSVCRT.@)
 */
size_t CDECL _mbsnlen_l(const unsigned char *str,
        size_t maxsize, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    size_t i = 0, len = 0;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(!mbcinfo->ismbcodepage)
        return strnlen((const char*)str, maxsize);

    while(i<maxsize && str[i])
    {
        if (_ismbblead_l(str[i], locale))
        {
            i++;
            if (!str[i])  /* count only full chars */
                break;
        }
        i++;
        len++;
    }
    return i < maxsize ? len : maxsize;
}

/*********************************************************************
 *		_mbslen(MSVCRT.@)
 */
size_t CDECL _mbslen(const unsigned char* str)
{
    return _mbsnlen_l(str, -1, NULL);
}

/*********************************************************************
 *              _mbslen_l(MSVCRT.@)
 */
size_t CDECL _mbslen_l(const unsigned char* str, _locale_t locale)
{
    return _mbsnlen_l(str, -1, locale);
}

/*********************************************************************
 *              _mbsnlen(MSVCRT.@)
 */
size_t CDECL _mbsnlen(const unsigned char* str, size_t maxsize)
{
    return _mbsnlen_l(str, maxsize, NULL);
}

/*********************************************************************
 *              _mbccpy_s_l(MSVCRT.@)
 */
int CDECL _mbccpy_s_l(unsigned char* dest, size_t maxsize,
        int *copied, const unsigned char* src, _locale_t locale)
{
    if(copied) *copied = 0;
    if(!MSVCRT_CHECK_PMT(dest != NULL && maxsize >= 1)) return EINVAL;
    dest[0] = 0;
    if(!MSVCRT_CHECK_PMT(src != NULL)) return EINVAL;

    if(_ismbblead_l(*src, locale)) {
        if(!src[1]) {
            if(copied) *copied = 1;
            *_errno() = EILSEQ;
            return EILSEQ;
        }

        if(maxsize < 2) {
            MSVCRT_INVALID_PMT("dst buffer is too small", ERANGE);
            return ERANGE;
        }

        *dest++ = *src++;
        *dest = *src;
        if(copied) *copied = 2;
    }else {
        *dest = *src;
        if(copied) *copied = 1;
    }

    return 0;
}

/*********************************************************************
 *		_mbccpy(MSVCRT.@)
 */
void CDECL _mbccpy(unsigned char* dest, const unsigned char* src)
{
    _mbccpy_s_l(dest, 2, NULL, src, NULL);
}

/*********************************************************************
 *              _mbccpy_l(MSVCRT.@)
 */
void CDECL _mbccpy_l(unsigned char* dest, const unsigned char* src,
        _locale_t locale)
{
    _mbccpy_s_l(dest, 2, NULL, src, locale);
}

/*********************************************************************
 *              _mbccpy_s(MSVCRT.@)
 */
int CDECL _mbccpy_s(unsigned char* dest, size_t maxsize,
        int *copied, const unsigned char* src)
{
    return _mbccpy_s_l(dest, maxsize, copied, src, NULL);
}


/*********************************************************************
 *		_mbsncpy_l(MSVCRT.@)
 * REMARKS
 *  The parameter n is the number or characters to copy, not the size of
 *  the buffer. Use _mbsnbcpy_l for a function analogical to strncpy
 */
unsigned char* CDECL _mbsncpy_l(unsigned char* dst, const unsigned char* src, size_t n, _locale_t locale)
{
    unsigned char* ret = dst;
    pthreadmbcinfo mbcinfo;

    if (!n)
        return dst;
    if (!MSVCRT_CHECK_PMT(dst && src))
        return NULL;
    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        while (*src && n)
        {
            n--;
            if (_ismbblead_l(*src, locale))
            {
                if (!*(src + 1))
                {
                    *dst++ = 0;
                    *dst++ = 0;
                    break;
                }
                *dst++ = *src++;
            }
            *dst++ = *src++;
        }
    }
    else
    {
        while (n)
        {
            n--;
            if (!(*dst++ = *src++)) break;
        }
    }
    while (n--) *dst++ = 0;
    return ret;
}

#if _MSVCR_VER>=80
errno_t CDECL _mbsncpy_s_l(unsigned char* dst, size_t maxsize, const unsigned char* src, size_t n, _locale_t locale)
{
    BOOL truncate = (n == _TRUNCATE);
    unsigned char *start = dst, *last;
    pthreadmbcinfo mbcinfo;
    unsigned int curlen;

    if (!dst && !maxsize && !n)
        return 0;

    if (!MSVCRT_CHECK_PMT(dst != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(maxsize != 0)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL))
    {
        *start = 0;
        return EINVAL;
    }

    if (!n)
    {
        *start = 0;
        return 0;
    }

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    curlen = 0;
    last = dst;
    while (*src && n && maxsize)
    {
        if (curlen)
        {
            --maxsize;
            *dst++ = *src++;
            if (!--curlen) --n;
            continue;
        }
        last = dst;
        if (!(mbcinfo->ismbcodepage && _ismbblead_l(*src, locale)))
        {
            curlen = 1;
            continue;
        }
        curlen = 2;
        if (!truncate && maxsize <= curlen) maxsize = 0;
    }

    if (!maxsize && truncate)
    {
        *last = 0;
        return STRUNCATE;
    }
    if (!truncate && curlen && !src[curlen - 1])
    {
        *_errno() = EILSEQ;
        *start = 0;
        return EILSEQ;
    }
    if (!maxsize)
    {
        *start = 0;
        if (!MSVCRT_CHECK_PMT_ERR(FALSE, ERANGE)) return ERANGE;
    }
    *dst = 0;
    return 0;
}

errno_t CDECL _mbsncpy_s(unsigned char* dst, size_t maxsize, const unsigned char* src, size_t n)
{
    return _mbsncpy_s_l(dst, maxsize, src, n, NULL);
}
#endif

/*********************************************************************
 *		_mbsncpy(MSVCRT.@)
 * REMARKS
 *  The parameter n is the number or characters to copy, not the size of
 *  the buffer. Use _mbsnbcpy for a function analogical to strncpy
 */
unsigned char* CDECL _mbsncpy(unsigned char* dst, const unsigned char* src, size_t n)
{
    return _mbsncpy_l(dst, src, n, NULL);
}

/*********************************************************************
 *              _mbsnbcpy_s_l(MSVCRT.@)
 * REMARKS
 * Unlike _mbsnbcpy this function does not pad the rest of the dest
 * string with 0
 */
int CDECL _mbsnbcpy_s_l(unsigned char* dst, size_t size,
        const unsigned char* src, size_t n, _locale_t locale)
{
    size_t pos = 0;

    if(!dst || size == 0)
        return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }
    if(!n)
        return 0;

    if((locale ? locale->mbcinfo : get_mbcinfo())->ismbcodepage)
    {
        BOOL is_lead = FALSE;
        while (*src && n)
        {
            if(pos == size)
            {
                dst[0] = '\0';
                return ERANGE;
            }
            is_lead = (!is_lead && _ismbblead(*src));
            n--;
            dst[pos++] = *src++;
        }

        if (is_lead) /* if string ends with a lead, remove it */
            dst[pos - 1] = 0;
    }
    else
    {
        while (n)
        {
            n--;
            if(pos == size)
            {
                dst[0] = '\0';
                return ERANGE;
            }

            if(!(*src)) break;
            dst[pos++] = *src++;
        }
    }

    if(pos < size)
        dst[pos] = '\0';
    else
    {
        dst[0] = '\0';
        return ERANGE;
    }

    return 0;
}

/*********************************************************************
 *              _mbsnbcpy_s(MSVCRT.@)
 */
int CDECL _mbsnbcpy_s(unsigned char* dst, size_t size, const unsigned char* src, size_t n)
{
    return _mbsnbcpy_s_l(dst, size, src, n, NULL);
}

/*********************************************************************
 *              _mbscpy_s_l(MSVCRT.@)
 */
int CDECL _mbscpy_s_l(unsigned char *dst, size_t size,
        const unsigned char *src, _locale_t locale)
{
    return _mbsnbcpy_s_l(dst, size, src, -1, locale);
}

/*********************************************************************
 *              _mbscpy_s(MSVCRT.@)
 */
int CDECL _mbscpy_s(unsigned char *dst, size_t size, const unsigned char *src)
{
    return _mbscpy_s_l(dst, size, src, NULL);
}

/*********************************************************************
 *              _mbsnbcpy_l(MSVCRT.@)
 * REMARKS
 *  Like strncpy this function doesn't enforce the string to be
 *  NUL-terminated
 */
unsigned char* CDECL _mbsnbcpy_l(unsigned char* dst, const unsigned char* src, size_t n, _locale_t locale)
{
    unsigned char* ret = dst;
    pthreadmbcinfo mbcinfo;

    if (!n)
        return dst;
    if (!MSVCRT_CHECK_PMT(dst && src))
        return NULL;
    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();
    if (mbcinfo->ismbcodepage)
    {
        BOOL is_lead = FALSE;
        while (*src && n)
        {
            is_lead = (!is_lead && _ismbblead_l(*src, locale));
            n--;
            *dst++ = *src++;
        }

        if (is_lead) /* if string ends with a lead, remove it */
            *(dst - 1) = 0;
    }
    else
    {
        while (n)
        {
            n--;
            if (!(*dst++ = *src++)) break;
        }
    }
    while (n--) *dst++ = 0;
    return ret;
}

/*********************************************************************
 *              _mbsnbcpy(MSVCRT.@)
 * REMARKS
 *  Like strncpy this function doesn't enforce the string to be
 *  NUL-terminated
 */
unsigned char* CDECL _mbsnbcpy(unsigned char* dst, const unsigned char* src, size_t n)
{
    return _mbsnbcpy_l(dst, src, n, NULL);
}

/*********************************************************************
 *		_mbscmp_l(MSVCRT.@)
 */
int CDECL _mbscmp_l(const unsigned char* str, const unsigned char* cmp, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if (!MSVCRT_CHECK_PMT(str && cmp))
    return _NLSCMPERROR;

  mbcinfo = locale ? locale->mbcinfo : get_mbcinfo();

  if(mbcinfo->ismbcodepage)
  {
    unsigned int strc, cmpc;
    do {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbsnextc_l(str, locale);
      cmpc = _mbsnextc_l(cmp, locale);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* equal, use same increment */
    } while(1);
  }
  return u_strcmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbscmp(MSVCRT.@)
 */
int CDECL _mbscmp(const unsigned char* str, const unsigned char* cmp)
{
    return _mbscmp_l(str, cmp, NULL);
}

/*********************************************************************
 *              _mbsnbicoll_l(MSVCRT.@)
 */
int CDECL _mbsnbicoll_l(const unsigned char *str1, const unsigned char *str2, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(!mbcinfo->ismbcodepage)
        return _strnicoll_l((const char*)str1, (const char*)str2, len, locale);
    return CompareStringA(mbcinfo->mblcid, NORM_IGNORECASE,
            (const char*)str1, strnlen((const char*)str1, len),
            (const char*)str2, strnlen((const char*)str2, len)) - CSTR_EQUAL;
}

/*********************************************************************
 *              _mbsicoll_l(MSVCRT.@)
 */
int CDECL _mbsicoll_l(const unsigned char *str1, const unsigned char *str2, _locale_t locale)
{
    return _mbsnbicoll_l(str1, str2, INT_MAX, locale);
}

/*********************************************************************
 *              _mbsnbicoll(MSVCRT.@)
 */
int CDECL _mbsnbicoll(const unsigned char *str1, const unsigned char *str2, size_t len)
{
    return _mbsnbicoll_l(str1, str2, len, NULL);
}

/*********************************************************************
 *		_mbsicoll(MSVCRT.@)
 */
int CDECL _mbsicoll(const unsigned char* str, const unsigned char* cmp)
{
#if _MSVCR_VER>=60 && _MSVCR_VER<=71
    return CompareStringA(get_mbcinfo()->mblcid, NORM_IGNORECASE,
            (const char*)str, -1, (const char*)cmp, -1)-CSTR_EQUAL;
#else
    return _mbsnbicoll_l(str, cmp, INT_MAX, NULL);
#endif
}

/*********************************************************************
 *              _mbsnbcoll_l(MSVCRT.@)
 */
int CDECL _mbsnbcoll_l(const unsigned char *str1, const unsigned char *str2, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(!mbcinfo->ismbcodepage)
        return _strncoll_l((const char*)str1, (const char*)str2, len, locale);
    return CompareStringA(mbcinfo->mblcid, 0,
            (const char*)str1, strnlen((const char*)str1, len),
            (const char*)str2, strnlen((const char*)str2, len)) - CSTR_EQUAL;
}

/*********************************************************************
 *              _mbscoll_l(MSVCRT.@)
 */
int CDECL _mbscoll_l(const unsigned char *str1, const unsigned char *str2, _locale_t locale)
{
    return _mbsnbcoll_l(str1, str2, INT_MAX, locale);
}

/*********************************************************************
 *              _mbsnbcoll(MSVCRT.@)
 */
int CDECL _mbsnbcoll(const unsigned char *str1, const unsigned char *str2, size_t len)
{
    return _mbsnbcoll_l(str1, str2, len, NULL);
}

/*********************************************************************
 *		_mbscoll(MSVCRT.@)
 */
int CDECL _mbscoll(const unsigned char* str, const unsigned char* cmp)
{
#if _MSVCR_VER>=60 && _MSVCR_VER<=71
    return CompareStringA(get_mbcinfo()->mblcid, 0,
            (const char*)str, -1, (const char*)cmp, -1)-CSTR_EQUAL;
#else
    return _mbsnbcoll_l(str, cmp, INT_MAX, NULL);
#endif
}

/*********************************************************************
 *		_mbsicmp_l(MSVCRT.@)
 */
int CDECL _mbsicmp_l(const unsigned char* str, const unsigned char* cmp, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(!MSVCRT_CHECK_PMT(str && cmp))
    return _NLSCMPERROR;

  if(!locale)
    mbcinfo = get_mbcinfo();
  else
    mbcinfo = locale->mbcinfo;
  if(mbcinfo->ismbcodepage)
  {
    unsigned int strc, cmpc;
    do {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbctolower_l(_mbsnextc_l(str, locale), locale);
      cmpc = _mbctolower_l(_mbsnextc_l(cmp, locale), locale);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* equal, use same increment */
    } while(1);
  }
  return u_strcasecmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbsicmp(MSVCRT.@)
 */
int CDECL _mbsicmp(const unsigned char* str, const unsigned char* cmp)
{
  return _mbsicmp_l(str, cmp, NULL);
}

/*********************************************************************
 *		_mbsncmp_l(MSVCRT.@)
 */
int CDECL _mbsncmp_l(const unsigned char* str, const unsigned char* cmp,
        size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    unsigned int strc, cmpc;

    if (!len)
        return 0;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage)
        return u_strncmp(str, cmp, len); /* ASCII CP */

    if (!MSVCRT_CHECK_PMT(str && cmp))
        return _NLSCMPERROR;

    while (len--)
    {
        int inc;

        if (!*str)
            return *cmp ? -1 : 0;
        if (!*cmp)
            return 1;
        strc = _mbsnextc_l(str, locale);
        cmpc = _mbsnextc_l(cmp, locale);
        if (strc != cmpc)
            return strc < cmpc ? -1 : 1;
        inc = (strc > 255) ? 2 : 1; /* Equal, use same increment */
        str += inc;
        cmp += inc;
    }
    return 0; /* Matched len chars */
}

/*********************************************************************
 *		_mbsncmp(MSVCRT.@)
 */
int CDECL _mbsncmp(const unsigned char* str, const unsigned char* cmp, size_t len)
{
    return _mbsncmp_l(str, cmp, len, NULL);
}

/*********************************************************************
 *              _mbsnbcmp_l(MSVCRT.@)
 */
int CDECL _mbsnbcmp_l(const unsigned char* str, const unsigned char* cmp, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!len)
        return 0;

    if (!MSVCRT_CHECK_PMT(str && cmp))
        return _NLSCMPERROR;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned int strc, cmpc;
        while (len)
        {
            int clen;
            if (!*str)
                return *cmp ? -1 : 0;
            if (!*cmp)
                return 1;
            if (_ismbblead_l(*str, locale))
            {
                strc = (len >= 2) ? _mbsnextc_l(str, locale) : 0;
                clen = 2;
            }
            else
            {
                strc = *str;
                clen = 1;
            }
            if (_ismbblead_l(*cmp, locale))
                cmpc = (len >= 2) ? _mbsnextc_l(cmp, locale) : 0;
            else
                cmpc = *cmp;
            if(strc != cmpc)
                return strc < cmpc ? -1 : 1;
            len -= clen;
            str += clen;
            cmp += clen;
        }
        return 0; /* Matched len chars */
    }
    return u_strncmp(str, cmp, len);
}

/*********************************************************************
 *              _mbsnbcmp(MSVCRT.@)
 */
int CDECL _mbsnbcmp(const unsigned char* str, const unsigned char* cmp, size_t len)
{
    return _mbsnbcmp_l(str, cmp, len, NULL);
}

/*********************************************************************
 *		_mbsnicmp(MSVCRT.@)
 *
 * Compare two multibyte strings case insensitively to 'len' characters.
 */
int CDECL _mbsnicmp_l(const unsigned char* str, const unsigned char* cmp, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!len)
        return 0;
    if (!MSVCRT_CHECK_PMT(str && cmp))
        return _NLSCMPERROR;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();
    /* FIXME: No tolower() for mb strings yet */
    if (mbcinfo->ismbcodepage)
    {
        unsigned int strc, cmpc;
        while (len--)
        {
            if (!*str)
                return *cmp ? -1 : 0;
            if (!*cmp)
                return 1;
            strc = _mbctolower_l(_mbsnextc_l(str, locale), locale);
            cmpc = _mbctolower_l(_mbsnextc_l(cmp, locale), locale);
            if (strc != cmpc)
                return strc < cmpc ? -1 : 1;
            str += (strc > 255) ? 2 : 1;
            cmp += (strc > 255) ? 2 : 1; /* Equal, use same increment */
        }
        return 0; /* Matched len chars */
    }
    return u_strncasecmp(str, cmp, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnicmp(MSVCRT.@)
 *
 * Compare two multibyte strings case insensitively to 'len' characters.
 */
int CDECL _mbsnicmp(const unsigned char* str, const unsigned char* cmp, size_t len)
{
  return _mbsnicmp_l(str, cmp, len,  NULL);
}

/*********************************************************************
 *              _mbsnbicmp_l(MSVCRT.@)
 */
int CDECL _mbsnbicmp_l(const unsigned char* str, const unsigned char* cmp, size_t len, _locale_t locale)
{

    pthreadmbcinfo mbcinfo;

    if (!len)
        return 0;
    if (!MSVCRT_CHECK_PMT(str && cmp))
        return _NLSCMPERROR;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned int strc, cmpc;
        while (len)
        {
            int clen;
            if (!*str)
                return *cmp ? -1 : 0;
            if (!*cmp)
                return 1;
            if (_ismbblead_l(*str, locale))
            {
                strc = (len >= 2) ? _mbsnextc_l(str, locale) : 0;
                clen = 2;
            }
            else
            {
                strc = *str;
                clen = 1;
            }
            if (_ismbblead_l(*cmp, locale))
                cmpc = (len >= 2) ? _mbsnextc_l(cmp, locale) : 0;
            else
                cmpc = *cmp;
            strc = _mbctolower_l(strc, locale);
            cmpc = _mbctolower_l(cmpc, locale);
            if (strc != cmpc)
                return strc < cmpc ? -1 : 1;
            len -= clen;
            str += clen;
            cmp += clen;
        }
        return 0; /* Matched len bytes */
    }
    return u_strncasecmp(str, cmp, len);
}

/*********************************************************************
 *              _mbsnbicmp(MSVCRT.@)
 */
int CDECL _mbsnbicmp(const unsigned char* str, const unsigned char* cmp, size_t len)
{
    return _mbsnbicmp_l(str, cmp, len, NULL);
}

/*********************************************************************
 *		_mbscat (MSVCRT.@)
 */
unsigned char * CDECL _mbscat( unsigned char *dst, const unsigned char *src )
{
    strcat( (char *)dst, (const char *)src );
    return dst;
}

/*********************************************************************
 *		_mbscat_s_l (MSVCRT.@)
 */
int CDECL _mbscat_s_l( unsigned char *dst, size_t size,
        const unsigned char *src, _locale_t locale )
{
    size_t i, j;
    int ret = 0;

    if(!MSVCRT_CHECK_PMT(dst != NULL)) return EINVAL;
    if(!MSVCRT_CHECK_PMT(src != NULL)) return EINVAL;

    for(i=0; i<size; i++)
        if(!dst[i]) break;
    if(i == size) {
        MSVCRT_INVALID_PMT("dst is not NULL-terminated", EINVAL);
        if(size) dst[0] = 0;
        return EINVAL;
    }

    if(i && _ismbblead_l(dst[i-1], locale)) {
        ret = EILSEQ;
        i--;
    }

    for(j=0; src[j] && i+j<size; j++)
        dst[i+j] = src[j];
    if(i+j == size) {
        MSVCRT_INVALID_PMT("dst buffer is too small", ERANGE);
        dst[0] = 0;
        return ERANGE;
    }

    if(j && _ismbblead_l(src[j-1], locale)) {
        ret = EILSEQ;
        j--;
    }

    dst[i+j] = 0;
    return ret;
}

/*********************************************************************
 *		_mbscat_s (MSVCRT.@)
 */
int CDECL _mbscat_s( unsigned char *dst, size_t size, const unsigned char *src )
{
    return _mbscat_s_l(dst, size, src, NULL);
}

/*********************************************************************
 *		_mbscpy (MSVCRT.@)
 */
unsigned char* CDECL _mbscpy( unsigned char *dst, const unsigned char *src )
{
    strcpy( (char *)dst, (const char *)src );
    return dst;
}

/*********************************************************************
 *		_mbsstr (MSVCRT.@)
 */
unsigned char * CDECL _mbsstr(const unsigned char *haystack, const unsigned char *needle)
{
    return (unsigned char *)strstr( (const char *)haystack, (const char *)needle );
}

/*********************************************************************
 *		_mbschr_l(MSVCRT.@)
 */
unsigned char* CDECL _mbschr_l(const unsigned char* s, unsigned int x, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(!MSVCRT_CHECK_PMT(s))
    return NULL;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();
  if(mbcinfo->ismbcodepage)
  {
    unsigned int c;
    while (1)
    {
      c = _mbsnextc_l(s, locale);
      if (c == x)
        return (unsigned char*)s;
      if (!c)
        return NULL;
      s += c > 255 ? 2 : 1;
    }
  }
  return u_strchr(s, x); /* ASCII CP */
}

/*********************************************************************
 *		_mbschr(MSVCRT.@)
 */
unsigned char* CDECL _mbschr(const unsigned char* s, unsigned int x)
{
  return _mbschr_l(s, x, NULL);
}

/*********************************************************************
 *		_mbsrchr_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsrchr_l(const unsigned char *s, unsigned int x, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!MSVCRT_CHECK_PMT(s))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned char *match = NULL;
        unsigned int c;

        while (1)
        {
            c = _mbsnextc_l(s, locale);
            if (c == x)
                match = (unsigned char *)s;
            if (!c)
                return match;
            s += (c > 255) ? 2 : 1;
        }
    }
    return u_strrchr(s, x);
}

/*********************************************************************
 *		_mbsrchr(MSVCRT.@)
 */
unsigned char* CDECL _mbsrchr(const unsigned char* s, unsigned int x)
{
    return _mbsrchr_l(s, x, NULL);
}

/*********************************************************************
 *              _mbstok_s_l(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_s_l(unsigned char *str, const unsigned char *delim,
        unsigned char **ctx, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    unsigned int c;

    if(!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if(!MSVCRT_CHECK_PMT(ctx != NULL)) return NULL;
    if(!MSVCRT_CHECK_PMT(str || *ctx)) return NULL;

    if(locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if(!mbcinfo->ismbcodepage)
        return (unsigned char*)strtok_s((char*)str, (const char*)delim, (char**)ctx);

    if(!str)
        str = *ctx;

    while((c=_mbsnextc(str)) && _mbschr(delim, c))
        str += c>255 ? 2 : 1;
    if(!*str)
    {
        *ctx = str;
        return NULL;
    }

    *ctx = str + (c>255 ? 2 : 1);
    while((c=_mbsnextc(*ctx)) && !_mbschr(delim, c))
        *ctx += c>255 ? 2 : 1;
    if (**ctx) {
        *(*ctx)++ = 0;
        if(c > 255)
            *(*ctx)++ = 0;
    }

    return str;
}


/*********************************************************************
 *              _mbstok_s(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_s(unsigned char *str,
        const unsigned char *delim, unsigned char **ctx)
{
    return _mbstok_s_l(str, delim, ctx, NULL);
}

/*********************************************************************
 *              _mbstok_l(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_l(unsigned char *str,
        const unsigned char *delim, _locale_t locale)
{
    return _mbstok_s_l(str, delim, &msvcrt_get_thread_data()->mbstok_next, locale);
}

/*********************************************************************
 *		_mbstok(MSVCRT.@)
 */
unsigned char* CDECL _mbstok(unsigned char *str, const unsigned char *delim)
{
    thread_data_t *data = msvcrt_get_thread_data();

#if _MSVCR_VER == 0
    if(!str && !data->mbstok_next)
        return NULL;
#endif

    return _mbstok_s_l(str, delim, &data->mbstok_next, NULL);
}

/*********************************************************************
 *		_mbbtombc_l(MSVCRT.@)
 */
unsigned int CDECL _mbbtombc_l(unsigned int c, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  if(mbcinfo->mbcodepage == 932)
  {
    if(c >= 0x20 && c <= 0x7e) {
      if((c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a) || (c >= 0x30 && c <= 0x39))
        return mbbtombc_932[c - 0x20] | 0x8200;
      else
        return mbbtombc_932[c - 0x20] | 0x8100;
    }
    else if(c >= 0xa1 && c <= 0xdf) {
      if(c >= 0xa6 && c <= 0xdd && c != 0xb0)
        return mbbtombc_932[c - 0xa1 + 0x5f] | 0x8300;
      else
        return mbbtombc_932[c - 0xa1 + 0x5f] | 0x8100;
    }
  }
  return c;  /* not Japanese or no MB char */
}

/*********************************************************************
 *		_mbbtombc(MSVCRT.@)
 */
unsigned int CDECL _mbbtombc(unsigned int c)
{
    return _mbbtombc_l(c, NULL);
}

/*********************************************************************
 *		_ismbbkana_l(MSVCRT.@)
 */
int CDECL _ismbbkana_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if(mbcinfo->mbcodepage == 932)
    {
        /* Japanese/Katakana, CP 932 */
        return (c >= 0xa1 && c <= 0xdf);
    }
    return 0;
}

/*********************************************************************
 *              _ismbbkana(MSVCRT.@)
 */
int CDECL _ismbbkana(unsigned int c)
{
    return _ismbbkana_l( c, NULL );
}

/*********************************************************************
 *              _ismbcdigit_l(MSVCRT.@)
 */
int CDECL _ismbcdigit_l(unsigned int ch, _locale_t locale)
{
    return _iswdigit_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcdigit(MSVCRT.@)
 */
int CDECL _ismbcdigit(unsigned int ch)
{
    return _ismbcdigit_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcgraph_l(MSVCRT.@)
 */
int CDECL _ismbcgraph_l(unsigned int ch, _locale_t locale)
{
    return _iswgraph_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcgraph(MSVCRT.@)
 */
int CDECL _ismbcgraph(unsigned int ch)
{
    return _ismbcgraph_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcalpha_l (MSVCRT.@)
 */
int CDECL _ismbcalpha_l(unsigned int ch, _locale_t locale)
{
    return _iswalpha_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcalpha (MSVCRT.@)
 */
int CDECL _ismbcalpha(unsigned int ch)
{
    return _ismbcalpha_l( ch, NULL );
}

/*********************************************************************
 *              _ismbclower_l (MSVCRT.@)
 */
int CDECL _ismbclower_l(unsigned int ch, _locale_t locale)
{
    return _iswlower_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbclower (MSVCRT.@)
 */
int CDECL _ismbclower(unsigned int ch)
{
    return _ismbclower_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcupper_l (MSVCRT.@)
 */
int CDECL _ismbcupper_l(unsigned int ch, _locale_t locale)
{
    return _iswupper_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcupper (MSVCRT.@)
 */
int CDECL _ismbcupper(unsigned int ch)
{
    return _ismbcupper_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcsymbol_l(MSVCRT.@)
 */
int CDECL _ismbcsymbol_l(unsigned int ch, _locale_t locale)
{
    wchar_t wch = msvcrt_mbc_to_wc_l( ch, locale );
    WORD ctype;
    if (!GetStringTypeW(CT_CTYPE3, &wch, 1, &ctype))
    {
        WARN("GetStringTypeW failed on %x\n", ch);
        return 0;
    }
    return ((ctype & C3_SYMBOL) != 0);
}

/*********************************************************************
 *              _ismbcsymbol(MSVCRT.@)
 */
int CDECL _ismbcsymbol(unsigned int ch)
{
    return _ismbcsymbol_l(ch, NULL);
}

/*********************************************************************
 *              _ismbcalnum_l (MSVCRT.@)
 */
int CDECL _ismbcalnum_l(unsigned int ch, _locale_t locale)
{
    return _iswalnum_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcalnum (MSVCRT.@)
 */
int CDECL _ismbcalnum(unsigned int ch)
{
    return _ismbcalnum_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcspace_l (MSVCRT.@)
 */
int CDECL _ismbcspace_l(unsigned int ch, _locale_t locale)
{
    return _iswspace_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcspace (MSVCRT.@)
 */
int CDECL _ismbcspace(unsigned int ch)
{
    return _ismbcspace_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcprint_l (MSVCRT.@)
 */
int CDECL _ismbcprint_l(unsigned int ch, _locale_t locale)
{
    return _iswprint_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcprint (MSVCRT.@)
 */
int CDECL _ismbcprint(unsigned int ch)
{
    return _ismbcprint_l( ch, NULL );
}

/*********************************************************************
 *              _ismbcpunct_l (MSVCRT.@)
 */
int CDECL _ismbcpunct_l(unsigned int ch, _locale_t locale)
{
    return _iswpunct_l( msvcrt_mbc_to_wc_l(ch, locale), locale );
}

/*********************************************************************
 *              _ismbcpunct(MSVCRT.@)
 */
int CDECL _ismbcpunct(unsigned int ch)
{
    return _ismbcpunct_l( ch, NULL );
}

/*********************************************************************
 *		_ismbchira_l(MSVCRT.@)
 */
int CDECL _ismbchira_l(unsigned int c, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  if(mbcinfo->mbcodepage == 932)
  {
    /* Japanese/Hiragana, CP 932 */
    return (c >= 0x829f && c <= 0x82f1);
  }
  return 0;
}

/*********************************************************************
 *		_ismbchira(MSVCRT.@)
 */
int CDECL _ismbchira(unsigned int c)
{
    return _ismbchira_l(c, NULL);
}

/*********************************************************************
 *		_ismbckata_l(MSVCRT.@)
 */
int CDECL _ismbckata_l(unsigned int c, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  if(mbcinfo->mbcodepage == 932)
  {
    /* Japanese/Katakana, CP 932 */
    return (c >= 0x8340 && c <= 0x8396 && c != 0x837f);
  }
  return 0;
}

/*********************************************************************
 *		_ismbckata(MSVCRT.@)
 */
int CDECL _ismbckata(unsigned int c)
{
    return _ismbckata_l(c, NULL);
}

/*********************************************************************
 *		_ismbblead_l(MSVCRT.@)
 */
int CDECL _ismbblead_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    return (mbcinfo->mbctype[(c&0xff) + 1] & _M1) != 0;
}

/*********************************************************************
 *		_ismbblead(MSVCRT.@)
 */
int CDECL _ismbblead(unsigned int c)
{
    return _ismbblead_l(c, NULL);
}

/*********************************************************************
 *              _ismbbtrail_l(MSVCRT.@)
 */
int CDECL _ismbbtrail_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    return (mbcinfo->mbctype[(c&0xff) + 1] & _M2) != 0;
}

/*********************************************************************
 *		_ismbbtrail(MSVCRT.@)
 */
int CDECL _ismbbtrail(unsigned int c)
{
    return _ismbbtrail_l(c, NULL);
}

/*********************************************************************
 *              _ismbclegal_l(MSVCRT.@)
 */
int CDECL _ismbclegal_l(unsigned int c, _locale_t locale)
{
    return _ismbblead_l(HIBYTE(c), locale) && _ismbbtrail_l(LOBYTE(c), locale);
}

/*********************************************************************
 *              _ismbclegal(MSVCRT.@)
 */
int CDECL _ismbclegal(unsigned int c)
{
    return _ismbclegal_l(c, NULL);
}

/*********************************************************************
 *		_ismbslead_l(MSVCRT.@)
 */
int CDECL _ismbslead_l(const unsigned char* start, const unsigned char* str, _locale_t locale)
{
  pthreadmbcinfo mbcinfo;
  int lead = 0;

  if (!MSVCRT_CHECK_PMT(start && str))
    return 0;

  if(locale)
      mbcinfo = locale->mbcinfo;
  else
      mbcinfo = get_mbcinfo();

  if(!mbcinfo->ismbcodepage)
    return 0;

  /* Lead bytes can also be trail bytes so we need to analyse the string
   */
  while (start <= str)
  {
    if (!*start)
      return 0;
    lead = !lead && _ismbblead_l(*start, locale);
    start++;
  }

  return lead ? -1 : 0;
}

/*********************************************************************
 *		_ismbslead(MSVCRT.@)
 */
int CDECL _ismbslead(const unsigned char* start, const unsigned char* str)
{
  return _ismbslead_l(start, str, NULL);
}

/*********************************************************************
 *		_ismbstrail_l(MSVCRT.@)
 */
int CDECL _ismbstrail_l(const unsigned char* start, const unsigned char* str, _locale_t locale)
{
  if (!MSVCRT_CHECK_PMT(start && str))
    return 0;

  /* Note: this function doesn't check _ismbbtrail */
  if ((str > start) && _ismbslead_l(start, str-1, locale))
    return -1;
  else
    return 0;
}

/*********************************************************************
 *		_ismbstrail(MSVCRT.@)
 */
int CDECL _ismbstrail(const unsigned char* start, const unsigned char* str)
{
  return _ismbstrail_l(start, str, NULL);
}

/*********************************************************************
 *		_mbsdec_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsdec_l(const unsigned char *start,
        const unsigned char *cur, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!MSVCRT_CHECK_PMT(start && cur))
        return NULL;
    if (start >= cur)
        return NULL;

    if (!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if (mbcinfo->ismbcodepage)
        return (unsigned char *)(_ismbstrail_l(start, cur - 1, locale) ? cur - 2 : cur - 1);

    return (unsigned char *)cur - 1; /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbsdec(MSVCRT.@)
 */
unsigned char* CDECL _mbsdec(const unsigned char *start, const unsigned char *cur)
{
    return _mbsdec_l(start, cur, NULL);
}


/*********************************************************************
 *		_mbbtype_l(MSVCRT.@)
 */
int CDECL _mbbtype_l(unsigned char c, int type, _locale_t locale)
{
    if (type == 1)
        return _ismbbtrail_l(c, locale) ? _MBC_TRAIL : _MBC_ILLEGAL;
    else
        return _ismbblead_l(c, locale) ? _MBC_LEAD
                : _isprint_l(c, locale) ? _MBC_SINGLE : _MBC_ILLEGAL;
}

/*********************************************************************
 *		_mbbtype(MSVCRT.@)
 */
int CDECL _mbbtype(unsigned char c, int type)
{
    return _mbbtype_l(c, type, NULL);
}

/*********************************************************************
 *		_mbsbtype_l (MSVCRT.@)
 */
int CDECL _mbsbtype_l(const unsigned char *str, size_t count, _locale_t locale)
{
    int lead = 0;
    pthreadmbcinfo mbcinfo;
    const unsigned char *end = str + count;

    if (!MSVCRT_CHECK_PMT(str))
        return _MBC_ILLEGAL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    /* Lead bytes can also be trail bytes so we need to analyse the string.
    * Also we must return _MBC_ILLEGAL for chars past the end of the string
    */
    while (str < end) /* Note: we skip the last byte - will check after the loop */
    {
        if (!*str)
            return _MBC_ILLEGAL;
        lead = mbcinfo->ismbcodepage && !lead && _ismbblead_l(*str, locale);
        str++;
    }

    if (lead)
    {
        if (_ismbbtrail_l(*str, locale))
            return _MBC_TRAIL;
        else
            return _MBC_ILLEGAL;
    }
    else
    {
        if (_ismbblead_l(*str, locale))
            return _MBC_LEAD;
        else
            return _MBC_SINGLE;
    }
}

/*********************************************************************
 *		_mbsbtype (MSVCRT.@)
 */
int CDECL _mbsbtype(const unsigned char *str, size_t count)
{
    return _mbsbtype_l(str, count, NULL);
}


/*********************************************************************
 *		_mbsset_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsset_l(unsigned char* str, unsigned int c, _locale_t locale)
{
    unsigned char* ret = str;
    pthreadmbcinfo mbcinfo;

    if (!MSVCRT_CHECK_PMT(str))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage || c < 256)
        return u__strset(str, c); /* ASCII CP or SB char */

    c &= 0xffff; /* Strip high bits */

    while (str[0] && str[1])
    {
        *str++ = c >> 8;
        *str++ = c & 0xff;
    }
    if (str[0])
        str[0] = '\0'; /* FIXME: OK to shorten? */

    return ret;
}

/*********************************************************************
 *		_mbsset(MSVCRT.@)
 */
unsigned char* CDECL _mbsset(unsigned char* str, unsigned int c)
{
    return _mbsset_l(str, c, NULL);
}

/*********************************************************************
 *		_mbsnbset_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbset_l(unsigned char *str, unsigned int c, size_t len, _locale_t locale)
{
    unsigned char *ret = str;
    pthreadmbcinfo mbcinfo;

    if (!len)
        return ret;
    if (!MSVCRT_CHECK_PMT(str))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage || c < 256)
        return u__strnset(str, c, len); /* ASCII CP or SB char */

    c &= 0xffff; /* Strip high bits */

    while (str[0] && str[1] && (len > 1))
    {
        *str++ = c >> 8;
        len--;
        *str++ = c & 0xff;
        len--;
    }
    if (len && str[0])
    {
        /* as per msdn pad with a blank character */
        str[0] = ' ';
    }

    return ret;
}

/*********************************************************************
 *		_mbsnbset(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbset(unsigned char *str, unsigned int c, size_t len)
{
    return _mbsnbset_l(str, c, len, NULL);
}

/*********************************************************************
 *		_mbsnset(MSVCRT.@)
 */
unsigned char* CDECL _mbsnset_l(unsigned char* str, unsigned int c, size_t len, _locale_t locale)
{
    unsigned char *ret = str;
    pthreadmbcinfo mbcinfo;

    if (!len)
        return ret;
    if (!MSVCRT_CHECK_PMT(str))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage || c < 256)
        return u__strnset(str, c, len); /* ASCII CP or SB char */

    c &= 0xffff; /* Strip high bits */

    while (str[0] && str[1] && len--)
    {
        *str++ = c >> 8;
        *str++ = c & 0xff;
    }
    if (len && str[0])
        str[0] = '\0'; /* FIXME: OK to shorten? */

    return ret;
}

/*********************************************************************
 *		_mbsnset(MSVCRT.@)
 */
unsigned char* CDECL _mbsnset(unsigned char* str, unsigned int c, size_t len)
{
    return _mbsnset_l(str, c, len, NULL);
}

/*********************************************************************
 *		_mbsnccnt_l(MSVCRT.@)
 * 'c' is for 'character'.
 */
size_t CDECL _mbsnccnt_l(const unsigned char* str, size_t len, _locale_t locale)
{
    size_t ret;
    pthreadmbcinfo mbcinfo;

    if (!len)
        return 0;
    if (!MSVCRT_CHECK_PMT(str))
        return 0;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        ret = 0;
        while (*str && len-- > 0)
        {
            if (_ismbblead_l(*str, locale))
            {
                if (!len)
                    break;
                len--;
                str++;
            }
            str++;
            ret++;
        }
        return ret;
    }
    ret = u_strlen(str);
    return min(ret, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnccnt(MSVCRT.@)
 * 'c' is for 'character'.
 */
size_t CDECL _mbsnccnt(const unsigned char* str, size_t len)
{
    return _mbsnccnt_l(str, len, NULL);
}

/*********************************************************************
 *		_mbsnbcnt_l(MSVCRT.@)
 * 'b' is for byte count.
 */
size_t CDECL _mbsnbcnt_l(const unsigned char* str, size_t len, _locale_t locale)
{
    size_t ret;
    pthreadmbcinfo mbcinfo;

    if (!len)
        return 0;
    if (!MSVCRT_CHECK_PMT(str))
        return 0;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();
    if (mbcinfo->ismbcodepage)
    {
        const unsigned char* xstr = str;
        while (*xstr && len-- > 0)
        {
            if (_ismbblead_l(*xstr++, locale))
                xstr++;
        }
        return xstr - str;
    }
    ret = u_strlen(str);
    return min(ret, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnbcnt(MSVCRT.@)
 * 'b' is for byte count.
 */
size_t CDECL _mbsnbcnt(const unsigned char* str, size_t len)
{
    return _mbsnbcnt_l(str, len, NULL);
}

/*********************************************************************
 *		_mbsnbcat_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbcat_l(unsigned char *dst, const unsigned char *src, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!MSVCRT_CHECK_PMT(dst && src))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned char *res = dst;

        while (*dst)
        {
            if (_ismbblead_l(*dst++, locale))
            {
                if (*dst)
                {
                    dst++;
                }
                else
                {
                    /* as per msdn overwrite the lead byte in front of '\0' */
                    dst--;
                    break;
                }
            }
        }
        while (*src && len--) *dst++ = *src++;
        *dst = '\0';
        return res;
    }
    return u_strncat(dst, src, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnbcat(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbcat(unsigned char *dst, const unsigned char *src, size_t len)
{
    return _mbsnbcat_l(dst, src, len, NULL);
}

/*********************************************************************
 *		_mbsnbcat_s_l(MSVCRT.@)
 */
int CDECL _mbsnbcat_s_l(unsigned char *dst, size_t size, const unsigned char *src, size_t len, _locale_t locale)
{
    unsigned char *ptr = dst;
    size_t i;
    pthreadmbcinfo mbcinfo;

    if (!dst && !size && !len)
        return 0;

    if (!MSVCRT_CHECK_PMT(dst && size && src))
    {
        if (dst && size)
            *dst = '\0';
        return EINVAL;
    }

    /* Find the null terminator of the destination buffer. */
    while (size && *ptr)
        size--, ptr++;

    if (!size)
    {
        *dst = '\0';
        *_errno() = EINVAL;
        return EINVAL;
    }

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    /* If necessary, check that the character preceding the null terminator is
     * a lead byte and move the pointer back by one for later overwrite. */
    if (ptr != dst && mbcinfo->ismbcodepage && _ismbblead_l(*(ptr - 1), locale))
        size++, ptr--;

    for (i = 0; *src && i < len; i++)
    {
        *ptr++ = *src++;
        size--;

        if (!size)
        {
            *dst = '\0';
            *_errno() = ERANGE;
            return ERANGE;
        }
    }

    *ptr = '\0';
    return 0;
}

/*********************************************************************
 *		_mbsnbcat_s(MSVCRT.@)
 */
int CDECL _mbsnbcat_s(unsigned char *dst, size_t size, const unsigned char *src, size_t len)
{
    return _mbsnbcat_s_l(dst, size, src, len, NULL);
}

/*********************************************************************
 *		_mbsncat_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsncat_l(unsigned char* dst, const unsigned char* src, size_t len, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if (!len)
        return dst;

    if (!MSVCRT_CHECK_PMT(dst && src))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned char *res = dst;
        while (*dst)
        {
            if (_ismbblead_l(*dst++, locale))
                dst++;
        }
        while (*src && len--)
        {
            *dst++ = *src;
            if (_ismbblead_l(*src++, locale))
                *dst++ = *src++;
        }
        *dst = '\0';
        return res;
    }
    return u_strncat(dst, src, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsncat(MSVCRT.@)
 */
unsigned char* CDECL _mbsncat(unsigned char* dst, const unsigned char* src, size_t len)
{
    return _mbsncat_l(dst, src, len, NULL);
}

/*********************************************************************
 *              _mbslwr_l(MSVCRT.@)
 */
unsigned char* CDECL _mbslwr_l(unsigned char *s, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    unsigned char *ret = s;

    if (!s)
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned int c;

        while (*s)
        {
            c = _mbctolower_l(_mbsnextc_l(s, locale), locale);
            /* Note that I assume that the size of the character is unchanged */
            if (c > 255)
            {
                *s++ = (c >> 8);
                c = c & 0xff;
            }
            *s++ = c;
        }
    }
    else
    {
        for ( ; *s; s++) *s = _tolower_l(*s, locale);
    }
    return ret;
}

/*********************************************************************
 *              _mbslwr(MSVCRT.@)
 */
unsigned char* CDECL _mbslwr(unsigned char *s)
{
    return _mbslwr_l(s, NULL);
}

/*********************************************************************
 *              _mbslwr_s_l(MSVCRT.@)
 */
int CDECL _mbslwr_s_l(unsigned char* s, size_t len, _locale_t locale)
{
  unsigned char *p = s;
  pthreadmbcinfo mbcinfo;

  if (!s && !len)
    return 0;
  if (!MSVCRT_CHECK_PMT(s && len))
    return EINVAL;

  if (locale)
    mbcinfo = locale->mbcinfo;
  else
    mbcinfo = get_mbcinfo();

  if (mbcinfo->ismbcodepage)
  {
    unsigned int c;
    for ( ; *s && len > 0; len--)
    {
      c = _mbctolower_l(_mbsnextc_l(s, locale), locale);
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else
  {
    for ( ; *s && len > 0; s++, len--)
      *s = _tolower_l(*s, locale);
  }

  if (!MSVCRT_CHECK_PMT(len))
  {
    *p = 0;
    return EINVAL;
  }
  *s = 0;
  return 0;
}

/*********************************************************************
 *              _mbslwr_s(MSVCRT.@)
 */
int CDECL _mbslwr_s(unsigned char* str, size_t len)
{
  return _mbslwr_s_l(str, len, NULL);
}

/*********************************************************************
 *              _mbsupr_l(MSVCRT.@)
 */
unsigned char* CDECL _mbsupr_l(unsigned char* s, _locale_t locale)
{
    unsigned char *ret = s;
    pthreadmbcinfo mbcinfo;

    if (!MSVCRT_CHECK_PMT(s))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (mbcinfo->ismbcodepage)
    {
        unsigned int c;
        while (*s)
        {
            c = _mbctoupper_l(_mbsnextc_l(s, locale), locale);
            /* Note that I assume that the size of the character is unchanged */
            if (c > 255)
            {
                *s++ = (c >> 8);
                c = c & 0xff;
            }
            *s++ = c;
        }
    }
    else
        for ( ; *s; s++) *s = _toupper_l(*s, locale);
    return ret;
}

/*********************************************************************
 *              _mbsupr(MSVCRT.@)
 */
unsigned char* CDECL _mbsupr(unsigned char* s)
{
   return _mbsupr_l(s, NULL);
}

/*********************************************************************
 *              _mbsupr_s_l(MSVCRT.@)
 */
int CDECL _mbsupr_s_l(unsigned char* s, size_t len, _locale_t locale)
{
  unsigned char *p = s;

  if (!s && !len)
    return 0;
  if (!MSVCRT_CHECK_PMT(s && len))
    return EINVAL;

  if (get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    for ( ; *s && len > 0; len--)
    {
      c = _mbctoupper_l(_mbsnextc_l(s, locale), locale);
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else
  {
    for ( ; *s && len > 0; s++, len--)
      *s = _toupper_l(*s, locale);
  }

  if (!MSVCRT_CHECK_PMT(len))
  {
    *p = 0;
    return EINVAL;
  }
  *s = 0;
  return 0;
}

/*********************************************************************
 *              _mbsupr_s(MSVCRT.@)
 */
int CDECL _mbsupr_s(unsigned char* s, size_t len)
{
  return _mbsupr_s_l(s, len, NULL);
}

/*********************************************************************
 *              _mbsspn_l (MSVCRT.@)
 */
size_t CDECL _mbsspn_l(const unsigned char* string,
        const unsigned char* set, _locale_t locale)
{
    const unsigned char *p, *q;

    if (!MSVCRT_CHECK_PMT(string && set))
        return 0;

    for (p = string; *p; p++)
    {
        for (q = set; *q; q++)
        {
            if (_ismbblead_l(*q, locale))
            {
                /* duplicate a bug in native implementation */
                if (!q[1]) break;

                if (p[0] == q[0] && p[1] == q[1])
                {
                    p++;
                    break;
                }
                q++;
            }
            else
            {
                if (p[0] == q[0]) break;
            }
        }
        if (!*q) break;
    }
    return p - string;
}

/*********************************************************************
 *              _mbsspn (MSVCRT.@)
 */
size_t CDECL _mbsspn(const unsigned char* string, const unsigned char* set)
{
    return _mbsspn_l(string, set, NULL);
}

/*********************************************************************
 *              _mbsspnp_l (MSVCRT.@)
 */
unsigned char* CDECL _mbsspnp_l(const unsigned char* string, const unsigned char* set, _locale_t locale)
{
    if (!MSVCRT_CHECK_PMT(string && set))
        return 0;

    string += _mbsspn_l(string, set, locale);
    return *string ? (unsigned char*)string : NULL;
}

/*********************************************************************
 *              _mbsspnp (MSVCRT.@)
 */
unsigned char* CDECL _mbsspnp(const unsigned char* string, const unsigned char* set)
{
    return _mbsspnp_l(string, set, NULL);
}

/*********************************************************************
 *		_mbscspn_l (MSVCRT.@)
 */
size_t CDECL _mbscspn_l(const unsigned char* str,
        const unsigned char* cmp, _locale_t locale)
{
    const unsigned char *p, *q;

    for (p = str; *p; p++)
    {
        for (q = cmp; *q; q++)
        {
            if (_ismbblead_l(*q, locale))
            {
                /* duplicate a bug in native implementation */
                if (!q[1]) return 0;

                if (p[0] == q[0] && p[1] == q[1])
                    return p - str;
                q++;
            }
            else if (p[0] == q[0])
                return p - str;
        }
    }
    return p - str;
}

/*********************************************************************
 *		_mbscspn (MSVCRT.@)
 */
size_t CDECL _mbscspn(const unsigned char* str, const unsigned char* cmp)
{
    return _mbscspn_l(str, cmp, NULL);
}

/*********************************************************************
 *              _mbsrev_l (MSVCRT.@)
 */
unsigned char* CDECL _mbsrev_l(unsigned char* str, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;
    unsigned char *p, tmp;

    if (!MSVCRT_CHECK_PMT(str))
        return NULL;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage)
        return u__strrev(str);

    for (p = str; *p; p++)
    {
        if (_ismbblead_l(*p, locale))
        {
            if (p[1])
            {
                tmp = p[0];
                p[0] = p[1];
                p[1] = tmp;
                p++;
            }
            else
            {
                /* drop trailing lead char */
                p[0] = 0;
            }
        }
    }
    return u__strrev(str);
}

/*********************************************************************
 *              _mbsrev (MSVCRT.@)
 */
unsigned char* CDECL _mbsrev(unsigned char* str)
{
    return _mbsrev_l(str, NULL);
}

/*********************************************************************
 *		_mbspbrk_l (MSVCRT.@)
 */
unsigned char* CDECL _mbspbrk_l(const unsigned char *str,
        const unsigned char *accept, _locale_t locale)
{
    const unsigned char* p;
    pthreadmbcinfo mbcinfo;

    if (locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if (!mbcinfo->ismbcodepage)
        return u_strpbrk(str, accept);

    if (!MSVCRT_CHECK_PMT(str && accept))
        return NULL;

    while (*str)
    {
        for (p = accept; *p; p += (_ismbblead_l(*p, locale) ? 2 : 1))
        {
            if (*p == *str)
                if (!_ismbblead_l(*p, locale) || p[1] == str[1])
                    return (unsigned char*)str;
        }
        str += (_ismbblead_l(*str, locale) ? 2 : 1);
    }
    return NULL;
}

/*********************************************************************
 *		_mbspbrk (MSVCRT.@)
 */
unsigned char* CDECL _mbspbrk(const unsigned char *str, const unsigned char *accept)
{
    return _mbspbrk_l(str, accept, NULL);
}

/*
 * Functions depending on locale codepage
 */

/*********************************************************************
 *		_mblen_l(MSVCRT.@)
 * REMARKS
 *  Unlike most of the multibyte string functions this function uses
 *  the locale codepage, not the codepage set by _setmbcp
 */
int CDECL _mblen_l(const char* str, size_t size, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if (!str || !*str || !size)
        return 0;

    if (locale)
        locinfo = locale->locinfo;
    else
        locinfo = get_locinfo();

    if (locinfo->mb_cur_max == 1)
        return 1; /* ASCII CP */
    return !_isleadbyte_l((unsigned char)*str, locale) ? 1 : (size > 1 ? 2 : -1);
}

/*********************************************************************
 *		mblen(MSVCRT.@)
 */
int CDECL mblen(const char* str, size_t size)
{
    return _mblen_l(str, size, NULL);
}

/*********************************************************************
 *              mbrlen(MSVCRT.@)
 */
size_t CDECL mbrlen(const char *str, size_t len, mbstate_t *state)
{
    mbstate_t s = (state ? *state : 0);
    size_t ret;

    if(!len || !str || !*str)
        return 0;

    if(get_locinfo()->mb_cur_max == 1) {
        return 1;
    }else if(!s && isleadbyte((unsigned char)*str)) {
        if(len == 1) {
            s = (unsigned char)*str;
            ret = -2;
        }else {
            ret = 2;
        }
    }else if(!s) {
        ret = 1;
    }else {
        s = 0;
        ret = 2;
    }

    if(state)
        *state = s;
    return ret;
}

/*********************************************************************
 *		_mbstrlen_l(MSVCRT.@)
 */
size_t CDECL _mbstrlen_l(const char* str, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(locinfo->mb_cur_max > 1) {
        size_t len;
        len = MultiByteToWideChar(locinfo->lc_codepage, MB_ERR_INVALID_CHARS,
                                  str, -1, NULL, 0);
        if (!len) {
            *_errno() = EILSEQ;
            return -1;
        }
        return len - 1;
    }

    return strlen(str);
}

/*********************************************************************
 *		_mbstrlen(MSVCRT.@)
 */
size_t CDECL _mbstrlen(const char* str)
{
    return _mbstrlen_l(str, NULL);
}

/*********************************************************************
 *		_mbtowc_l(MSVCRT.@)
 */
int CDECL _mbtowc_l(wchar_t *dst, const char* str, size_t n, _locale_t locale)
{
    pthreadlocinfo locinfo;
    wchar_t tmpdst;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(n <= 0 || !str)
        return 0;

    if(!*str) {
        if(dst) *dst = 0;
        return 0;
    }

    if(!locinfo->lc_codepage) {
        if(dst) *dst = (unsigned char)*str;
        return 1;
    }
    if(n>=2 && _isleadbyte_l((unsigned char)*str, locale)) {
        if(!MultiByteToWideChar(locinfo->lc_codepage, 0, str, 2, &tmpdst, 1))
            return -1;
        if(dst) *dst = tmpdst;
        return 2;
    }
    if(!MultiByteToWideChar(locinfo->lc_codepage, 0, str, 1, &tmpdst, 1))
        return -1;
    if(dst) *dst = tmpdst;
    return 1;
}

/*********************************************************************
 *              mbtowc(MSVCRT.@)
 */
int CDECL mbtowc(wchar_t *dst, const char* str, size_t n)
{
    return _mbtowc_l(dst, str, n, NULL);
}

/*********************************************************************
 *              btowc(MSVCRT.@)
 */
wint_t CDECL btowc(int c)
{
    unsigned char letter = c;
    wchar_t ret;

    if(c == EOF)
        return WEOF;
    if(!get_locinfo()->lc_codepage)
        return c & 255;
    if(!MultiByteToWideChar(get_locinfo()->lc_codepage,
                MB_ERR_INVALID_CHARS, (LPCSTR)&letter, 1, &ret, 1))
        return WEOF;

    return ret;
}

/*********************************************************************
 *              mbrtowc(MSVCRT.@)
 */
size_t CDECL mbrtowc(wchar_t *dst, const char *str,
        size_t n, mbstate_t *state)
{
    pthreadlocinfo locinfo = get_locinfo();
    mbstate_t s = (state ? *state : 0);
    char tmpstr[2];
    int len = 0;

    if(dst)
        *dst = 0;

    if(!n || !str || !*str)
        return 0;

    if(locinfo->mb_cur_max == 1) {
        tmpstr[len++] = *str;
    }else if(!s && isleadbyte((unsigned char)*str)) {
        if(n == 1) {
            s = (unsigned char)*str;
            len = -2;
        }else {
            tmpstr[0] = str[0];
            tmpstr[1] = str[1];
            len = 2;
        }
    }else if(!s) {
        tmpstr[len++] = *str;
    }else {
        tmpstr[0] = s;
        tmpstr[1] = *str;
        len = 2;
        s = 0;
    }

    if(len > 0) {
        if(!MultiByteToWideChar(locinfo->lc_codepage, 0, tmpstr, len, dst, dst ? 1 : 0))
            len = -1;
    }

    if(state)
        *state = s;
    return len;
}

static inline int get_utf8_char_len(char ch)
{
    if((ch&0xf8) == 0xf0)
        return 4;
    else if((ch&0xf0) == 0xe0)
        return 3;
    else if((ch&0xe0) == 0xc0)
        return 2;
    return 1;
}

/*********************************************************************
 *		_mbstowcs_l(MSVCRT.@)
 */
size_t CDECL _mbstowcs_l(wchar_t *wcstr, const char *mbstr,
        size_t count, _locale_t locale)
{
    pthreadlocinfo locinfo;
    size_t i, size;

    if(!mbstr) {
        *_errno() = EINVAL;
        return -1;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_codepage) {
        if(!wcstr)
            return strlen(mbstr);

        for(i=0; i<count; i++) {
            wcstr[i] = (unsigned char)mbstr[i];
            if(!wcstr[i]) break;
        }
        return i;
    }

    /* Ignore count parameter */
    if(!wcstr) {
        size = MultiByteToWideChar(locinfo->lc_codepage,
                MB_ERR_INVALID_CHARS, mbstr, -1, NULL, 0);
        if(!size) {
            *_errno() = EILSEQ;
            return -1;
        }
        return size - 1;
    }

    for(i=0, size=0; i<count; i++) {
        if(mbstr[size] == '\0')
            break;

        if(locinfo->lc_codepage == CP_UTF8) {
            int j, chlen = get_utf8_char_len(mbstr[size]);

            for(j = 1; j < chlen; j++)
            {
                if(!mbstr[size + j])
                {
                    if(count) wcstr[0] = '\0';
                    *_errno() = EILSEQ;
                    return -1;
                }
            }
            size += chlen;
        }
        else
            size += (_isleadbyte_l((unsigned char)mbstr[size], locale) ? 2 : 1);
    }

    if(size) {
        size = MultiByteToWideChar(locinfo->lc_codepage,
                MB_ERR_INVALID_CHARS, mbstr, size, wcstr, count);
        if(!size) {
            if(count) wcstr[0] = '\0';
            *_errno() = EILSEQ;
            return -1;
        }
    }

    if(size<count)
        wcstr[size] = '\0';

    return size;
}

/*********************************************************************
 *		mbstowcs(MSVCRT.@)
 */
size_t CDECL mbstowcs(wchar_t *wcstr,
        const char *mbstr, size_t count)
{
    return _mbstowcs_l(wcstr, mbstr, count, NULL);
}

/*********************************************************************
 *              _mbstowcs_s_l(MSVCRT.@)
 */
int CDECL _mbstowcs_s_l(size_t *ret, wchar_t *wcstr,
        size_t size, const char *mbstr, size_t count, _locale_t locale)
{
    size_t conv;
    int err = 0;

    if(!wcstr && !size) {
        conv = _mbstowcs_l(NULL, mbstr, 0, locale);
        if(ret)
            *ret = conv+1;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(wcstr != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(mbstr != NULL)) {
        if(size) wcstr[0] = '\0';
        return EINVAL;
    }

    if(count==_TRUNCATE || size<count)
        conv = size;
    else
        conv = count;

    conv = _mbstowcs_l(wcstr, mbstr, conv, locale);
    if(conv<size)
        wcstr[conv++] = '\0';
    else if(conv==size && count==_TRUNCATE && wcstr[conv-1]!='\0') {
        wcstr[conv-1] = '\0';
        err = STRUNCATE;
    }else if(conv==size && wcstr[conv-1]!='\0') {
        MSVCRT_INVALID_PMT("wcstr[size] is too small", ERANGE);
        if(size)
            wcstr[0] = '\0';
        return ERANGE;
    }

    if(ret)
        *ret = conv;
    return err;
}

/*********************************************************************
 *              mbstowcs_s(MSVCRT.@)
 */
int CDECL _mbstowcs_s(size_t *ret, wchar_t *wcstr,
        size_t size, const char *mbstr, size_t count)
{
    return _mbstowcs_s_l(ret, wcstr, size, mbstr, count, NULL);
}

/*********************************************************************
 *              mbsrtowcs(MSVCRT.@)
 */
size_t CDECL mbsrtowcs(wchar_t *wcstr,
        const char **pmbstr, size_t count, mbstate_t *state)
{
    mbstate_t s = (state ? *state : 0);
    wchar_t tmpdst;
    size_t ret = 0;
    const char *p;

    if(!MSVCRT_CHECK_PMT(pmbstr != NULL))
        return -1;

    p = *pmbstr;
    while(!wcstr || count>ret) {
        int ch_len = mbrtowc(&tmpdst, p, 2, &s);
        if(wcstr)
            wcstr[ret] = tmpdst;

        if(ch_len < 0) {
            return -1;
        }else if(ch_len == 0) {
            if(wcstr) *pmbstr = NULL;
            return ret;
        }

        p += ch_len;
        ret++;
    }

    if(wcstr) *pmbstr = p;
    return ret;
}

/*********************************************************************
 *              mbsrtowcs_s(MSVCRT.@)
 */
int CDECL mbsrtowcs_s(size_t *ret, wchar_t *wcstr, size_t len,
        const char **mbstr, size_t count, mbstate_t *state)
{
    size_t tmp;

    if(!ret) ret = &tmp;
    if(!MSVCRT_CHECK_PMT(!!wcstr == !!len)) {
        *ret = -1;
        return EINVAL;
    }

    *ret = mbsrtowcs(wcstr, mbstr, count>len ? len : count, state);
    if(*ret == -1) {
        if(wcstr) *wcstr = 0;
        return *_errno();
    }
    (*ret)++;
    if(*ret > len) {
        /* no place for terminating '\0' */
        if(wcstr) *wcstr = 0;
        return 0;
    }
    if(wcstr) wcstr[(*ret)-1] = 0;
    return 0;
}

/*********************************************************************
 *		_mbctohira_l (MSVCRT.@)
 *
 *              Converts a sjis katakana character to hiragana.
 */
unsigned int CDECL _mbctohira_l(unsigned int c, _locale_t locale)
{
    if(_ismbckata_l(c, locale) && c <= 0x8393)
        return (c - 0x8340 - (c >= 0x837f ? 1 : 0)) + 0x829f;
    return c;
}

/*********************************************************************
 *		_mbctohira (MSVCRT.@)
 */
unsigned int CDECL _mbctohira(unsigned int c)
{
    return _mbctohira_l(c, NULL);
}

/*********************************************************************
 *		_mbctokata_l (MSVCRT.@)
 *
 *              Converts a sjis hiragana character to katakana.
 */
unsigned int CDECL _mbctokata_l(unsigned int c, _locale_t locale)
{
    if(_ismbchira_l(c, locale))
        return (c - 0x829f) + 0x8340 + (c >= 0x82de ? 1 : 0);
    return c;
}


/*********************************************************************
 *		_mbctokata (MSVCRT.@)
 */
unsigned int CDECL _mbctokata(unsigned int c)
{
    return _mbctokata_l(c, NULL);
}

/*********************************************************************
 *		_ismbcl0_l (MSVCRT.@)
 */
int CDECL _ismbcl0_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(mbcinfo->mbcodepage == 932)
    {
        /* JIS non-Kanji */
        return _ismbclegal_l(c, locale) && c >= 0x8140 && c <= 0x889e;
    }

    return 0;
}

/*********************************************************************
 *		_ismbcl0 (MSVCRT.@)
 */
int CDECL _ismbcl0(unsigned int c)
{
    return _ismbcl0_l(c, NULL);
}

/*********************************************************************
 *		_ismbcl1_l (MSVCRT.@)
 */
int CDECL _ismbcl1_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(mbcinfo->mbcodepage == 932)
    {
        /* JIS level-1 */
        return _ismbclegal_l(c, locale) && c >= 0x889f && c <= 0x9872;
    }

    return 0;
}

/*********************************************************************
 *		_ismbcl1 (MSVCRT.@)
 */
int CDECL _ismbcl1(unsigned int c)
{
    return _ismbcl1_l(c, NULL);
}

/*********************************************************************
 *		_ismbcl2_l (MSVCRT.@)
 */
int CDECL _ismbcl2_l(unsigned int c, _locale_t locale)
{
    pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(mbcinfo->mbcodepage == 932)
    {
        /* JIS level-2 */
        return _ismbclegal_l(c, locale) && c >= 0x989f && c <= 0xeaa4;
    }

    return 0;
}

/*********************************************************************
 *		_ismbcl2 (MSVCRT.@)
 */
int CDECL _ismbcl2(unsigned int c)
{
    return _ismbcl2_l(c, NULL);
}
