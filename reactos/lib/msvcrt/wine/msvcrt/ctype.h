/*
 * Character type definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CTYPE_H
#define __WINE_CTYPE_H
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

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

int MSVCRT(__isascii)(int);
int MSVCRT(__iscsym)(int);
int MSVCRT(__iscsymf)(int);
int MSVCRT(__toascii)(int);
int MSVCRT(_isctype)(int,int);
int MSVCRT(_tolower)(int);
int MSVCRT(_toupper)(int);
int MSVCRT(isalnum)(int);
int MSVCRT(isalpha)(int);
int MSVCRT(iscntrl)(int);
int MSVCRT(isdigit)(int);
int MSVCRT(isgraph)(int);
int MSVCRT(islower)(int);
int MSVCRT(isprint)(int);
int MSVCRT(ispunct)(int);
int MSVCRT(isspace)(int);
int MSVCRT(isupper)(int);
int MSVCRT(isxdigit)(int);
int MSVCRT(tolower)(int);
int MSVCRT(toupper)(int);

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


#ifndef USE_MSVCRT_PREFIX
static inline int isascii(int c) { return __isascii(c); }
static inline int iscsym(int c) { return __iscsym(c); }
static inline int iscsymf(int c) { return __iscsymf(c); }
static inline int toascii(int c) { return __toascii(c); }
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_CTYPE_H */
