#ifndef _MBCTYPE_H
#define _MBCTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

//#define _MS	0x01
//#define _MP	0x02
//#define _M1	0x04
//#define _M2	0x08

#define _MBC_SINGLE	 0	
#define _MBC_LEAD	 1	
#define _MBC_TRAIL	 2		
#define _MBC_ILLEGAL	-1		

#define _MB_CP_SBCS      0
#define _MB_CP_OEM      -2
#define _MB_CP_ANSI     -3
#define _MB_CP_LOCALE   -4

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

extern char _jctype[257];

int _ismbbkana( unsigned char c );


#ifdef __cplusplus
}
#endif

#endif