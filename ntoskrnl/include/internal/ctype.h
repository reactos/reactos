#ifndef _CTYPE_H
#define _CTYPE_H

#define __need_wchar_t
#define __need_wint_t
#include <stddef.h>

/*
 * The following flags are used to tell iswctype and _isctype what character
 * types you are looking for.
 */

#define _UPPER      0x0001
#define _LOWER      0x0002
#define _DIGIT      0x0004
#define _SPACE      0x0008 /* HT  LF  VT  FF  CR  SP */
#define _PUNCT      0x0010
#define _CONTROL    0x0020
#define _BLANK      0x0040 /* this is SP only, not SP and HT as in C99  */
#define _HEX        0x0080
#define _LEADBYTE   0x8000

#define _ALPHA      0x0103

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif


#if defined(_MSC_VER)
#define inline __inline
typedef wchar_t wint_t;
#endif

extern inline int isspace(int c);
extern inline int toupper(int c);
extern inline int tolower(int c);
extern inline int islower(int c);
extern inline int isdigit(int c);
extern inline int isxdigit(int c);

#endif
