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

#include <msvcrt/stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/* character routines */
int _ismbcalnum(unsigned int);
int _ismbcalpha(unsigned int);
int _ismbcdigit(unsigned int);
int _ismbcgraph(unsigned int);
int _ismbcprint(unsigned int);
int _ismbcpunct(unsigned int);
int _ismbcspace(unsigned int);
int _ismbclower(unsigned int);
int _ismbcupper(unsigned int);
int _ismbclegal(unsigned int);

int _ismbblead(unsigned int);
int _ismbbtrail(unsigned int);
int _ismbslead(const unsigned char*, const unsigned char*);
int _ismbstrail(const unsigned char*, const unsigned char*);

unsigned int _mbctolower(unsigned int);
unsigned int _mbctoupper(unsigned int);

void _mbccpy(unsigned char*, const unsigned char*);
size_t _mbclen(const unsigned char*);

unsigned int _mbbtombc(unsigned int);
unsigned int _mbctombb(unsigned int);

/* Return value constants for these are defined in mbctype.h.  */
int _mbbtype(unsigned char, int);
int _mbsbtype(const unsigned char*, size_t);

unsigned char* _mbscpy(unsigned char*, const unsigned char*);
unsigned char* _mbsncpy(unsigned char*, const unsigned char*, size_t);
unsigned char* _mbsnbcpy(unsigned char*, const unsigned char*, size_t);
unsigned char* _mbsset(unsigned char*, unsigned int);
unsigned char* _mbsnset(unsigned char*, unsigned int, size_t);
unsigned char* _mbsnbset(unsigned char*, unsigned int, size_t);

unsigned char* _mbsdup(const unsigned char*);
unsigned char* _mbsrev(unsigned char*);
unsigned char* _mbscat(unsigned char*, const unsigned char*);
unsigned char* _mbsncat(unsigned char*, const unsigned char*, size_t);
unsigned char* _mbsnbcat(unsigned char*, const unsigned char*, size_t);
size_t _mbslen(const unsigned char*);
size_t _mbsnbcnt(const unsigned char*, size_t);
size_t _mbsnccnt(const unsigned char*, size_t);
unsigned char* _mbschr(unsigned char*, unsigned char*);
unsigned char* _mbsrchr(const unsigned char*, unsigned int);
size_t _mbsspn(const unsigned char*, const unsigned char*);
size_t _mbscspn(const unsigned char*, const unsigned char*);
unsigned char* _mbsspnp(const unsigned char*, const unsigned char*);
unsigned char* _mbspbrk(const unsigned char*, const unsigned char*);
int _mbscmp(const unsigned char*, const unsigned char*);
int _mbsicmp(const unsigned char*, const unsigned char*);
int _mbsncmp(const unsigned char*, const unsigned char*, size_t);
int _mbsnicmp(const unsigned char*, const unsigned char*, size_t);
int _mbsnbcmp(const unsigned char*, const unsigned char*, size_t);
int _mbsnbicmp(const unsigned char*, const unsigned char*, size_t);
int _mbscoll(const unsigned char*, const unsigned char*);
int _mbsicoll(const unsigned char*, const unsigned char*);
int _mbsncoll(const unsigned char*, const unsigned char*, size_t);
int _mbsnicoll(const unsigned char*, const unsigned char*, size_t);
int _mbsnbcoll(const unsigned char*, const unsigned char*, size_t);

int _mbsnbicoll(const unsigned char*, const unsigned char*, size_t);

unsigned char* _mbsinc(const unsigned char*);
unsigned char* _mbsninc(const unsigned char*, size_t);
unsigned char* _mbsdec(const unsigned char*, const unsigned char*);
unsigned int _mbsnextc (const unsigned char*);
unsigned char* _mbslwr(unsigned char*);
unsigned char* _mbsupr(unsigned char*);
unsigned char* _mbstok(unsigned char*, unsigned char*);

unsigned char* _mbsstr(const unsigned char*, const unsigned char*);
size_t _mbstrlen(const char*str);


#ifdef __cplusplus
}
#endif

#endif	/* Not _MBSTRING_H_ */

