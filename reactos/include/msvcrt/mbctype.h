/* 
 * mbctype.h
 *
 * Functions for testing multibyte character types and converting characters.
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

#ifndef _MBCTYPE_H_
#define _MBCTYPE_H_


/* return values for _mbsbtype  and  _mbbtype in mbstring.h */
#define _MBC_SINGLE   0
#define _MBC_LEAD     1
#define _MBC_TRAIL    2
#define _MBC_ILLEGAL (-1)

/*  args for setmbcp (in lieu of actual codepage)  */
#define _MB_CP_SBCS      0
#define _MB_CP_OEM      (-2)
#define _MB_CP_ANSI     (-3)
#define _MB_CP_LOCALE   (-4)

/* TODO: bit masks */
/*
//#define _MS   0x01
//#define _MP   0x02
//#define _M1   0x04
//#define _M2   0x08
#define _SBUP
#define _SBLOW
*/

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
#define _M2     (_M_|__2)
#define _P2     (_P_|__2)

#ifdef __cplusplus
extern "C" {
#endif

extern char _jctype[257];


int _ismbbkana(unsigned char);
int _ismbbkalnum(unsigned int);


#ifdef __cplusplus
}
#endif

#endif  /* Not _MCTYPE_H_ */

