/*
 * mbstring.h
 *
 * Protototypes for string functions supporting multibyte characters.
 *
 * This file is part of the Mingw32 package.
 *
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _MBSTRING_H_
#define _MBSTRING_H_

/* All the headers include this file. */
#include <_mingw.h>

#ifndef RC_INVOKED

#define __need_size_t
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	__STRICT_ANSI__

/* character classification */
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcalnum (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcalpha (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcdigit (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcgraph (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcprint (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcpunct (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcspace (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbclower (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcupper (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbclegal (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcsymbol (unsigned int);


/* also in mbctype.h */
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbblead (unsigned int );
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbtrail (unsigned int );
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbslead ( const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbstrail ( const unsigned char*, const unsigned char*);

_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbctolower (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbctoupper (unsigned int);

_CRTIMP void __cdecl __MINGW_NOTHROW _mbccpy (unsigned char*, const unsigned char*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbclen (const unsigned char*);

_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbbtombc (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbctombb (unsigned int);

/* Return value constants for these are defined in mbctype.h.  */
_CRTIMP int __cdecl __MINGW_NOTHROW _mbbtype (unsigned char, int);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsbtype (const unsigned char*, size_t);

_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbscpy (unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsncpy (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsnbcpy (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsset (unsigned char*, unsigned int);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsnset (unsigned char*, unsigned int, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsnbset (unsigned char*, unsigned int, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsdup (const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsrev (unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbscat (unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsncat (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsnbcat (unsigned char*, const unsigned char*, size_t);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbslen (const unsigned char*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbsnbcnt (const unsigned char*, size_t);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbsnccnt (const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbschr (const unsigned char*, unsigned int);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsrchr (const unsigned char*, unsigned int);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbsspn (const unsigned char*, const unsigned char*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _mbscspn (const unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsspnp (const unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbspbrk (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbscmp (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsicmp (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsncmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnicmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnbcmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnbicmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbscoll (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsicoll (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsncoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnicoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnbcoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _mbsnbicoll (const unsigned char*, const unsigned char*, size_t);

_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsinc (const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsninc (const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsdec (const unsigned char*, const unsigned char*);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbsnextc  (const unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbslwr (unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbsupr (unsigned char*);
_CRTIMP unsigned char* __cdecl __MINGW_NOTHROW  _mbstok (unsigned char*, const unsigned char*);

/* Kanji */
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbchira (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbckata (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcl0 (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcl1 (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbcl2 (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbcjistojms (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbcjmstojis (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbctohira (unsigned int);
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _mbctokata (unsigned int);

#endif	/* Not strict ANSI */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */
#endif	/* Not _MBSTRING_H_ */



