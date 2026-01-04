/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
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

_ACRTIMP const unsigned short* __cdecl __pctype_func(void);
_ACRTIMP int     __cdecl _isleadbyte_l(int,_locale_t);
_ACRTIMP int     __cdecl _iswalnum_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswalpha_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswblank_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswcntrl_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswctype_l(wint_t,wctype_t,_locale_t);
_ACRTIMP int     __cdecl _iswdigit_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswgraph_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswlower_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswprint_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswpunct_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswspace_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswupper_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl _iswxdigit_l(wint_t,_locale_t);
_ACRTIMP wint_t  __cdecl _towlower_l(wint_t,_locale_t);
_ACRTIMP wint_t  __cdecl _towupper_l(wint_t,_locale_t);
_ACRTIMP int     __cdecl is_wctype(wint_t,wctype_t);
_ACRTIMP int     __cdecl isleadbyte(int);
_ACRTIMP int     __cdecl iswalnum(wint_t);
_ACRTIMP int     __cdecl iswalpha(wint_t);
_ACRTIMP int     __cdecl iswascii(wint_t);
_ACRTIMP int     __cdecl iswblank(wint_t);
_ACRTIMP int     __cdecl iswcntrl(wint_t);
_ACRTIMP int     __cdecl iswctype(wint_t,wctype_t);
_ACRTIMP int     __cdecl iswdigit(wint_t);
_ACRTIMP int     __cdecl iswgraph(wint_t);
_ACRTIMP int     __cdecl iswlower(wint_t);
_ACRTIMP int     __cdecl iswprint(wint_t);
_ACRTIMP int     __cdecl iswpunct(wint_t);
_ACRTIMP int     __cdecl iswspace(wint_t);
_ACRTIMP int     __cdecl iswupper(wint_t);
_ACRTIMP int     __cdecl iswxdigit(wint_t);
_ACRTIMP wint_t  __cdecl towlower(wint_t);
_ACRTIMP wint_t  __cdecl towupper(wint_t);

#ifdef __cplusplus
}
#endif

#endif /* _WCTYPE_DEFINED */
