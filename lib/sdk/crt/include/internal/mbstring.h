#ifndef __CRT_INTERNAL_MBSTRING_H
#define __CRT_INTERNAL_MBSTRING_H

#define _KNJ_M  ((char)0x01)    /* Non-punctuation of Kana-set */
#define _KNJ_P  ((char)0x02)    /* Punctuation of Kana-set */
#define _KNJ_1  ((char)0x04)    /* Legal 1st byte of double byte stream */
#define _KNJ_2  ((char)0x08)    /* Legal 2nd btye of double byte stream */


#define ___     0
#define _1_     _KNJ_1 /* Legal 1st byte of double byte code */
#define __2     _KNJ_2 /* Legal 2nd byte of double byte code */
#define _M_     _KNJ_M /* Non-puntuation in Kana-set */
#define _P_     _KNJ_P /* Punctuation of Kana-set */
#define _12     (_1_|__2)
#ifndef _M2
#define _M2     (_M_|__2)
#endif
#define _P2     (_P_|__2)

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
