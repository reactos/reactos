/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/ismblead.c
 * PURPOSE:     Checks for a lead byte 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>


#define ___     0
#define _1_     _KNJ_1 /* Legal 1st byte of double byte code */
#define __2     _KNJ_2 /* Legal 2nd byte of double byte code */
#define _M_     _KNJ_M /* Non-puntuation in Kana-set */
#define _P_     _KNJ_P /* Punctuation of Kana-set */
#define _12     (_1_|__2)
#define _M2     (_M_|__2)
#define _P2     (_P_|__2)

static char _jctype[257] = {
/*-1*/  ___,
/*0x*/  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,
/*1x*/  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,
/*2x*/  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,
/*3x*/  ___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,___,
/*4x*/  __2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,
/*5x*/  __2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,
/*6x*/  __2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,
/*7x*/  __2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,__2,___,
/*8x*/  __2,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,
/*9x*/  _12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,
/*Ax*/  __2,_P2,_P2,_P2,_P2,_P2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,
/*Bx*/  _M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,
/*Cx*/  _M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,
/*Dx*/  _M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,_M2,
/*Ex*/  _12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,
/*Fx*/  _12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,_12,___,___,___
};

 iskanji()    :   ÉVÉtÉg JIS ÉRÅ[ÉhÇÃ1ÉoÉCÉgñ⁄(0x81 <= c <= 0x9F Ç ÇÈ
                     Ç¢ÇÕ0xE0 <= c <= 0xFC) Ç©Ç«Ç§Ç©

int _ismbblead(char c)
{
	return ((_jctype+1)[(unsigned char)(c)] & _KNJ_1);
}
//int _ismbblead(unsigned int byte)
//{
//
//	return (int)IsDBCSLeadByte(byte) 
//}

int _ismbslead( const unsigned char *str, const unsigned char *t)
{
	char *s = str;
	while(*s != 0 && s != t) 
	{
		
		s+= mblen(*s);
	}		
	return ismbblead( *s)
}

