/*
 * Multibyte string definitions
 *
 * Copyright 2001 Francois Gouget.
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
#ifndef __WINE_MBSTRING_H
#define __WINE_MBSTRING_H

#include <corecrt.h>

#include <pshpack8.h>

#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR               ((unsigned int)0x7fffffff)
#define _NLSCMP_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int            __cdecl _ismbcalnum(unsigned int);
_ACRTIMP int            __cdecl _ismbcalpha(unsigned int);
_ACRTIMP int            __cdecl _ismbcdigit(unsigned int);
_ACRTIMP int            __cdecl _ismbcgraph(unsigned int);
_ACRTIMP int            __cdecl _ismbchira(unsigned int);
_ACRTIMP int            __cdecl _ismbckata(unsigned int);
_ACRTIMP int            __cdecl _ismbcl0(unsigned int);
_ACRTIMP int            __cdecl _ismbcl1(unsigned int);
_ACRTIMP int            __cdecl _ismbcl2(unsigned int);
_ACRTIMP int            __cdecl _ismbclegal(unsigned int);
_ACRTIMP int            __cdecl _ismbclegal_l(unsigned int, _locale_t);
_ACRTIMP int            __cdecl _ismbclower(unsigned int);
_ACRTIMP int            __cdecl _ismbcprint(unsigned int);
_ACRTIMP int            __cdecl _ismbcpunct(unsigned int);
_ACRTIMP int            __cdecl _ismbcspace(unsigned int);
_ACRTIMP int            __cdecl _ismbcsymbol(unsigned int);
_ACRTIMP int            __cdecl _ismbcupper(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbbtombc(unsigned int);
_ACRTIMP int            __cdecl _mbbtype(unsigned char,int);
_ACRTIMP int            __cdecl _mbbtype_l(unsigned char,int,_locale_t);
#define                         _mbccmp(_cpc1,_cpc2) _mbsncmp((_cpc1),(_cpc2),1)
_ACRTIMP void           __cdecl _mbccpy(unsigned char*,const unsigned char*);
_ACRTIMP unsigned int   __cdecl _mbcjistojms(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbcjmstojis(unsigned int);
_ACRTIMP size_t         __cdecl _mbclen(const unsigned char*);
_ACRTIMP unsigned int   __cdecl _mbctohira(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbctokata(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbctolower(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbctombb(unsigned int);
_ACRTIMP unsigned int   __cdecl _mbctoupper(unsigned int);
_ACRTIMP int            __cdecl _mbsbtype(const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbscat(unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbschr(const unsigned char*,unsigned int);
_ACRTIMP int            __cdecl _mbscmp(const unsigned char*,const unsigned char*);
_ACRTIMP int            __cdecl _mbscmp_l(const unsigned char*,const unsigned char*,_locale_t);
_ACRTIMP int            __cdecl _mbscoll(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbscpy(unsigned char*,const unsigned char*);
_ACRTIMP size_t         __cdecl _mbscspn(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsdec(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsdup(const unsigned char*);
_ACRTIMP int            __cdecl _mbsicmp(const unsigned char*,const unsigned char*);
_ACRTIMP int            __cdecl _mbsicoll(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsinc(const unsigned char*);
_ACRTIMP size_t         __cdecl _mbslen(const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbslwr(unsigned char*);
_ACRTIMP errno_t        __cdecl _mbslwr_s(unsigned char*, size_t);
_ACRTIMP unsigned char* __cdecl _mbsnbcat(unsigned char*,const unsigned char*,size_t);
_ACRTIMP errno_t        __cdecl _mbsnbcat_s(unsigned char*,size_t,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsnbcmp(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsnbcoll(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP size_t         __cdecl _mbsnbcnt(const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbsnbcpy(unsigned char*,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsnbicmp(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsnbicoll(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbsnbset(unsigned char*,unsigned int,size_t);
_ACRTIMP unsigned char* __cdecl _mbsncat(unsigned char*,const unsigned char*, size_t);
_ACRTIMP size_t         __cdecl _mbsnccnt(const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsncmp(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsncoll(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbsncpy(unsigned char*,const unsigned char*,size_t);
_ACRTIMP errno_t        __cdecl _mbsncpy_s(unsigned char*,size_t,const unsigned char*,size_t);
_ACRTIMP errno_t        __cdecl _mbsncpy_s_l(unsigned char*,size_t,const unsigned char*,size_t,_locale_t);
_ACRTIMP unsigned int   __cdecl _mbsnextc(const unsigned char*);
_ACRTIMP unsigned int   __cdecl _mbsnextc_l(const unsigned char*,_locale_t);
_ACRTIMP int            __cdecl _mbsnicmp(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP int            __cdecl _mbsnicoll(const unsigned char*,const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbsninc(const unsigned char*,size_t);
_ACRTIMP unsigned char* __cdecl _mbsnset(unsigned char*,unsigned int,size_t);
_ACRTIMP unsigned char* __cdecl _mbspbrk(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsrchr(const unsigned char*,unsigned int);
_ACRTIMP unsigned char* __cdecl _mbsrev(unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsset(unsigned char*,unsigned int);
_ACRTIMP size_t         __cdecl _mbsspn(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsspnp(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbsstr(const unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbstok(unsigned char*,const unsigned char*);
_ACRTIMP unsigned char* __cdecl _mbstok_l(unsigned char*,const unsigned char*,_locale_t);
_ACRTIMP unsigned char* __cdecl _mbsupr(unsigned char*);
_ACRTIMP errno_t        __cdecl _mbsupr_s(unsigned char*, size_t);

#ifndef _MBLEADTRAIL_DEFINED
#define _MBLEADTRAIL_DEFINED
_ACRTIMP int __cdecl _ismbblead(unsigned int);
_ACRTIMP int __cdecl _ismbblead_l(unsigned int,_locale_t);
_ACRTIMP int __cdecl _ismbbtrail(unsigned int);
_ACRTIMP int __cdecl _ismbslead(const unsigned char*,const unsigned char*);
_ACRTIMP int __cdecl _ismbstrail(const unsigned char*,const unsigned char*);
#endif /* _MBLEADTRAIL_DEFINED */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_MBSTRING_H */
