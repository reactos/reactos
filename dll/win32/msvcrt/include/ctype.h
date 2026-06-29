/*
 * Character type definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CTYPE_H
#define __WINE_CTYPE_H

#include <corecrt_wctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

_ACRTIMP int __cdecl __isascii(int);
_ACRTIMP int __cdecl __iscsym(int);
_ACRTIMP int __cdecl __iscsymf(int);
_ACRTIMP int __cdecl __toascii(int);
_ACRTIMP int __cdecl _isblank_l(int,_locale_t);
_ACRTIMP int __cdecl _isctype(int,int);
_ACRTIMP int __cdecl _isctype_l(int,int,_locale_t);
_ACRTIMP int __cdecl _islower_l(int,_locale_t);
_ACRTIMP int __cdecl _isupper_l(int,_locale_t);
_ACRTIMP int __cdecl _isdigit_l(int,_locale_t);
_ACRTIMP int __cdecl _isxdigit_l(int,_locale_t);
_ACRTIMP int __cdecl _tolower(int);
_ACRTIMP int __cdecl _tolower_l(int,_locale_t);
_ACRTIMP int __cdecl _toupper(int);
_ACRTIMP int __cdecl _toupper_l(int,_locale_t);
_ACRTIMP int __cdecl isalnum(int);
_ACRTIMP int __cdecl isalpha(int);
_ACRTIMP int __cdecl isblank(int);
_ACRTIMP int __cdecl iscntrl(int);
_ACRTIMP int __cdecl isdigit(int);
_ACRTIMP int __cdecl isgraph(int);
_ACRTIMP int __cdecl islower(int);
_ACRTIMP int __cdecl isprint(int);
_ACRTIMP int __cdecl _isprint_l(int,_locale_t);
_ACRTIMP int __cdecl ispunct(int);
_ACRTIMP int __cdecl isspace(int);
_ACRTIMP int __cdecl _isspace_l(int,_locale_t);
_ACRTIMP int __cdecl isupper(int);
_ACRTIMP int __cdecl isxdigit(int);
_ACRTIMP int __cdecl tolower(int);
_ACRTIMP int __cdecl toupper(int);

#ifdef __cplusplus
}
#endif


static inline int isascii(int c) { return __isascii(c); }
static inline int iscsym(int c) { return __iscsym(c); }
static inline int iscsymf(int c) { return __iscsymf(c); }
static inline int toascii(int c) { return __toascii(c); }

#endif /* __WINE_CTYPE_H */
