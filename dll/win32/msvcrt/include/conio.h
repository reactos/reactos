/*
 * Console I/O definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CONIO_H
#define __WINE_CONIO_H

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP char* __cdecl _cgets(char*);
_ACRTIMP int   __cdecl _cprintf(const char*,...);
_ACRTIMP int   __cdecl _cputs(const char*);
_ACRTIMP int   __cdecl _cscanf(const char*,...);
_ACRTIMP int   __cdecl _getch(void);
_ACRTIMP int   __cdecl _getche(void);
_ACRTIMP int   __cdecl _kbhit(void);
_ACRTIMP int   __cdecl _putch(int);
_ACRTIMP int   __cdecl _ungetch(int);

#ifdef _M_IX86
_ACRTIMP int            __cdecl _inp(unsigned short);
_ACRTIMP __msvcrt_ulong __cdecl _inpd(unsigned short);
_ACRTIMP unsigned short __cdecl _inpw(unsigned short);
_ACRTIMP int            __cdecl _outp(unsigned short, int);
_ACRTIMP __msvcrt_ulong __cdecl _outpd(unsigned short, __msvcrt_ulong);
_ACRTIMP unsigned short __cdecl _outpw(unsigned short, unsigned short);
#endif

#ifdef __cplusplus
}
#endif


static inline char* cgets(char* str) { return _cgets(str); }
static inline int cputs(const char* str) { return _cputs(str); }
static inline int getch(void) { return _getch(); }
static inline int getche(void) { return _getche(); }
static inline int kbhit(void) { return _kbhit(); }
static inline int putch(int c) { return _putch(c); }
static inline int ungetch(int c) { return _ungetch(c); }
#ifdef _M_IX86
static inline int inp(unsigned short i) { return _inp(i); }
static inline unsigned short inpw(unsigned short i) { return _inpw(i); }
static inline int outp(unsigned short i, int j) { return _outp(i, j); }
static inline unsigned short outpw(unsigned short i, unsigned short j) { return _outpw(i, j); }
#endif

#if defined(__GNUC__) && (__GNUC__ < 4)
_ACRTIMP int __cdecl cprintf(const char*,...) __attribute__((alias("_cprintf"),format(printf,1,2)));
_ACRTIMP int __cdecl cscanf(const char*,...) __attribute__((alias("_cscanf"),format(scanf,1,2)));
#else
#define cprintf _cprintf
#define cscanf _cscanf
#endif /* __GNUC__ */

#endif /* __WINE_CONIO_H */
