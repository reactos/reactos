#ifndef _MBSTRING_H_
#define _MBSTRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <msvcrt/stddef.h>

size_t _mbstrlen(const char *str);




int _mbbtype(unsigned char c, int type);
int _mbsbtype( const unsigned char *str, size_t n );

unsigned int _mbbtombc(unsigned int c);
unsigned int _mbctombb(unsigned int c);

unsigned char * _mbscat(unsigned char *dst, const unsigned char *src);
unsigned char * _mbschr(const unsigned char *str, unsigned int c);
int _mbscmp(const unsigned char *, const unsigned char *);
int _mbscoll(const unsigned char *, const unsigned char *);
unsigned char * _mbscpy(unsigned char *, const unsigned char *);
size_t _mbscspn(const unsigned char *, const unsigned char *);
unsigned char * _mbsdup(const unsigned char *str);
int _mbsicmp(const unsigned char *, const unsigned char *);
int _mbsicoll(const unsigned char *, const unsigned char *);
size_t _mbslen(const unsigned char *str);

unsigned char * _mbsncat(unsigned char *, const unsigned char *, size_t);
unsigned char * _mbsnbcat(unsigned char *, const unsigned char *, size_t);


int _mbsncmp(const unsigned char *, const unsigned char *, size_t);
int _mbsnbcmp(const unsigned char *, const unsigned char *, size_t);

int _mbsncoll(const unsigned char *, const unsigned char *, size_t);
int _mbsnbcoll(const unsigned char *, const unsigned char *, size_t);


unsigned char * _mbsncpy(unsigned char *, const unsigned char *, size_t);
unsigned char * _mbsnbcpy(unsigned char *, const unsigned char *, size_t);

int _mbsnicmp(const unsigned char *, const unsigned char *, size_t);
int _mbsnbicmp(const unsigned char *, const unsigned char *, size_t);

int _mbsnicoll(const unsigned char *, const unsigned char *, size_t);
int _mbsnbicoll(const unsigned char *, const unsigned char *, size_t);

unsigned char * _mbsnset(unsigned char *, unsigned int, size_t);
unsigned char * _mbsnbset(unsigned char *, unsigned int, size_t);

size_t _mbsnccnt(const unsigned char *, size_t);


unsigned char * _mbspbrk(const unsigned char *, const unsigned char *);
unsigned char * _mbsrchr(const unsigned char *, unsigned int);
unsigned char * _mbsrev(unsigned char *);
unsigned char * _mbsset(unsigned char *, unsigned int);
size_t _mbsspn(const unsigned char *, const unsigned char *);

unsigned char * _mbsstr(const unsigned char *, const unsigned char *);
unsigned char * _mbstok(unsigned char *, const unsigned char *);

unsigned char * _mbslwr(unsigned char *str);
unsigned char * _mbsupr(unsigned char *str);

size_t _mbclen(const unsigned char *);
void _mbccpy(unsigned char *, const unsigned char *);

/* tchar routines */

unsigned char * _mbsdec(const unsigned char *, const unsigned char *);
unsigned char * _mbsinc(const unsigned char *);
size_t _mbsnbcnt(const unsigned char *, size_t);
unsigned int _mbsnextc (const unsigned char *);
unsigned char * _mbsninc(const unsigned char *, size_t);
unsigned char * _mbsspnp(const unsigned char *, const unsigned char *);

/* character routines */

int _ismbcalnum(unsigned int c);
int _ismbcalpha(unsigned int c);
int _ismbcdigit(unsigned int c);
int _ismbcgraph(unsigned int c);
int _ismbclegal(unsigned int c);
int _ismbclower(unsigned int c);
int _ismbcprint(unsigned int c);
int _ismbcpunct(unsigned int c);
int _ismbcspace(unsigned int c);
int _ismbcupper(unsigned int c);

unsigned int _mbctolower(unsigned int);
unsigned int _mbctoupper(unsigned int);


int _ismbblead( unsigned int c);
int _ismbbtrail( unsigned int c);
int _ismbslead( const unsigned char *s, const unsigned char *c);
int _ismbstrail( const unsigned char *s, const unsigned char *c);

#ifdef __cplusplus
}
#endif

#endif
