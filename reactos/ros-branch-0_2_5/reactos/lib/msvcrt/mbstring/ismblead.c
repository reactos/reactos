/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismblead.c
 * PURPOSE:     Checks for a lead byte 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *		Modified from Taiji Yamada japanese code system utilities
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/mbctype.h>

size_t _mbclen2(const unsigned int s);

char _jctype[257] = {
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

char *_mbctype = _jctype;
/*
 * @implemented
 */
int _ismbblead(unsigned int c)
{
	return ((_jctype+1)[(unsigned char)(c)] & _KNJ_1);
}
//int _ismbblead(unsigned int byte)
//{
//
//	return (int)IsDBCSLeadByte(byte) 
//}

/*
 * @implemented
 */
int _ismbslead( const unsigned char *str, const unsigned char *t)
{
	unsigned char *s = (unsigned char *)str;
	while(*s != 0 && s != t) 
	{
		
		s+= _mbclen2(*s);
	}		
	return _ismbblead( *s);
}

