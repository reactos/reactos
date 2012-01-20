#ifndef __CRT_INTERNAL_MBSTRING_H
#define __CRT_INTERNAL_MBSTRING_H

#define _MALPHA 0x01
#define _MBLANK 0x02
#define _MDIGIT 0x04
#define _MKMOJI 0x08
#define _MKPNCT 0x10
#define _MLEAD  0x20
#define _MPUNCT 0x40
#define _MTRAIL 0x80

#define _MBALNUM (_MALPHA | _MDIGIT | _MKPNCT | _MKMOJI)
#define _MBALPHA (_MALPHA | _MKPNCT | _MKMOJI)
#define _MBGRAPH (_MALPHA | _MDIGIT | _MPUNCT | _MKPNCT | _MKMOJI)
#define _MBKANA  (_MKPNCT | _MKMOJI)
#define _MBPRINT (_MALPHA | _MDIGIT | _MPUNCT | _MBLANK | _MKPNCT | _MKMOJI)
#define _MBPUNCT (_MPUNCT | _MKPNCT)

#define _MBLMASK(c) ((c) &  255)
#define _MBHMASK(c) ((c) & ~255)
#define _MBGETL(c)  ((c) &  255)
#define _MBGETH(c)  (((c) >> 8) & 255)

#define _MBIS16(c) ((c) & 0xff00)

/* Macros */
#define B _MBLANK
#define D _MDIGIT
#define P _MPUNCT
#define T _MTRAIL

/* Macros */
#define AT (_MALPHA | _MTRAIL)
#define GT (_MKPNCT | _MTRAIL)
#define KT (_MKMOJI | _MTRAIL)
#define LT (_MLEAD  | _MTRAIL)
#define PT (_MPUNCT | _MTRAIL)

#define MAX_LOCALE_LENGTH 256
extern unsigned char _mbctype[257];
extern int MSVCRT___lc_codepage;
extern char MSVCRT_current_lc_all[MAX_LOCALE_LENGTH];

#if defined (_MSC_VER)

#undef _ismbbkana
#undef _ismbbkpunct
#undef _ismbbalpha
#undef _ismbbalnum
#undef _ismbbgraph
#undef _ismbbkalnum
#undef _ismbblead
#undef _ismbbprint
#undef _ismbbpunct
#undef _ismbbtrail

#endif


#endif
