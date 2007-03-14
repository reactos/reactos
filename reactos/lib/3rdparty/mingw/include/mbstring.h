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
_CRTIMP int __cdecl _ismbcalnum (unsigned int);
_CRTIMP int __cdecl _ismbcalpha (unsigned int);
_CRTIMP int __cdecl _ismbcdigit (unsigned int);
_CRTIMP int __cdecl _ismbcgraph (unsigned int);
_CRTIMP int __cdecl _ismbcprint (unsigned int);
_CRTIMP int __cdecl _ismbcpunct (unsigned int);
_CRTIMP int __cdecl _ismbcspace (unsigned int);
_CRTIMP int __cdecl _ismbclower (unsigned int);
_CRTIMP int __cdecl _ismbcupper (unsigned int);
_CRTIMP int __cdecl _ismbclegal (unsigned int);
_CRTIMP int __cdecl _ismbcsymbol (unsigned int);


/* also in mbctype.h */
_CRTIMP int __cdecl _ismbblead (unsigned int );
_CRTIMP int __cdecl _ismbbtrail (unsigned int );
_CRTIMP int __cdecl _ismbslead ( const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _ismbstrail ( const unsigned char*, const unsigned char*);

_CRTIMP unsigned int __cdecl _mbctolower (unsigned int);
_CRTIMP unsigned int __cdecl _mbctoupper (unsigned int);

_CRTIMP void __cdecl _mbccpy (unsigned char*, const unsigned char*);
_CRTIMP size_t __cdecl _mbclen (const unsigned char*);

_CRTIMP unsigned int __cdecl _mbbtombc (unsigned int);
_CRTIMP unsigned int __cdecl _mbctombb (unsigned int);

/* Return value constants for these are defined in mbctype.h.  */
_CRTIMP int __cdecl _mbbtype (unsigned char, int);
_CRTIMP int __cdecl _mbsbtype (const unsigned char*, size_t);

_CRTIMP unsigned char* __cdecl  _mbscpy (unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsncpy (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl  _mbsnbcpy (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl  _mbsset (unsigned char*, unsigned int);
_CRTIMP unsigned char* __cdecl  _mbsnset (unsigned char*, unsigned int, size_t);
_CRTIMP unsigned char* __cdecl  _mbsnbset (unsigned char*, unsigned int, size_t);
_CRTIMP unsigned char* __cdecl  _mbsdup (const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsrev (unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbscat (unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsncat (unsigned char*, const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl  _mbsnbcat (unsigned char*, const unsigned char*, size_t);
_CRTIMP size_t __cdecl _mbslen (const unsigned char*);
_CRTIMP size_t __cdecl _mbsnbcnt (const unsigned char*, size_t);
_CRTIMP size_t __cdecl _mbsnccnt (const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl  _mbschr (const unsigned char*, unsigned int);
_CRTIMP unsigned char* __cdecl  _mbsrchr (const unsigned char*, unsigned int);
_CRTIMP size_t __cdecl _mbsspn (const unsigned char*, const unsigned char*);
_CRTIMP size_t __cdecl _mbscspn (const unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsspnp (const unsigned char*, const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbspbrk (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _mbscmp (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _mbsicmp (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _mbsncmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnicmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnbcmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnbicmp (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbscoll (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _mbsicoll (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl _mbsncoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnicoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnbcoll (const unsigned char*, const unsigned char*, size_t);
_CRTIMP int __cdecl _mbsnbicoll (const unsigned char*, const unsigned char*, size_t);

_CRTIMP unsigned char* __cdecl  _mbsinc (const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsninc (const unsigned char*, size_t);
_CRTIMP unsigned char* __cdecl  _mbsdec (const unsigned char*, const unsigned char*);
_CRTIMP unsigned int __cdecl _mbsnextc  (const unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbslwr (unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbsupr (unsigned char*);
_CRTIMP unsigned char* __cdecl  _mbstok (unsigned char*, const unsigned char*);

/* Kanji */
_CRTIMP int __cdecl _ismbchira (unsigned int);
_CRTIMP int __cdecl _ismbckata (unsigned int);
_CRTIMP int __cdecl _ismbcl0 (unsigned int);
_CRTIMP int __cdecl _ismbcl1 (unsigned int);
_CRTIMP int __cdecl _ismbcl2 (unsigned int);
_CRTIMP unsigned int __cdecl _mbcjistojms (unsigned int);
_CRTIMP unsigned int __cdecl _mbcjmstojis (unsigned int);
_CRTIMP unsigned int __cdecl _mbctohira (unsigned int);
_CRTIMP unsigned int __cdecl _mbctokata (unsigned int);

#endif	/* Not strict ANSI */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */
#endif	/* Not _MBSTRING_H_ */


