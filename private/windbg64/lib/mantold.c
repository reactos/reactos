/***
*mantold.c - conversion of a decimal mantissa to _ULDBL12
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Conversion of a decimal mantissa into _ULDBL12 format (i.e. long
*   double with two additional bytes of significand)
*
*Revision History:
*   7-17-91	GDP	Initial version (ported from assembly)
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

#include "mathsup.h"






/***
*int  __addl(u_long x, u_long y, u_long *sum) - u_long addition
*
*Purpose: add two u_long numbers and return carry
*
*Entry: u_long x, u_long y : the numbers to be added
*	u_long *sum : where to store the result
*
*Exit: *sum receives the value of x+y
*      the value of the carry is returned
*
*Exceptions:
*
*******************************************************************************/

int __addl(u_long x, u_long y, u_long *sum)
{
    u_long r;
    int carry=0;
    r = x+y;
    if (r < x || r < y)
	carry++;
    *sum = r;
    return carry;
}






/***
*void __add_12(_ULDBL12 *x, _ULDBL12 *y) -	_ULDBL12 addition
*
*Purpose: add two _ULDBL12 numbers. The numbers are added
*   as 12-byte integers. Overflow is ignored.
*
*Entry: x,y: pointers to the operands
*
*Exit: *x receives the sum
*
*Exceptions:
*
*******************************************************************************/

void __add_12(_ULDBL12 *x, _ULDBL12 *y)
{
    int c0,c1,c2;
    c0 = __addl(*UL_LO_12(x),*UL_LO_12(y),UL_LO_12(x));
    if (c0) {
	c1 = __addl(*UL_MED_12(x),(u_long)1,UL_MED_12(x));
	if (c1) {
	    (*UL_HI_12(x))++;
	}
    }
    c2 = __addl(*UL_MED_12(x),*UL_MED_12(y),UL_MED_12(x));
    if (c2) {
	(*UL_HI_12(x))++;
    }
    /* ignore next carry -- assume no overflow will occur */
    (void) __addl(*UL_HI_12(x),*UL_HI_12(y),UL_HI_12(x));
}





/***
*void __shl_12(_ULDBL12 *x) - _ULDBL12 shift left
*void __shr_12(_ULDBL12 *x) - _ULDBL12 shift right
*
*Purpose: Shift a _ULDBL12 number one bit to the left (right). The number
*   is shifted as a 12-byte integer. The MSB is lost.
*
*Entry: x: a pointer to the operand
*
*Exit: *x is shifted one bit to the left (or right)
*
*Exceptions:
*
*******************************************************************************/

void __shl_12(_ULDBL12 *p)
{
    u_long c0,c1;

    c0 = *UL_LO_12(p) & MSB_ULONG ? 1: 0;
    c1 = *UL_MED_12(p) & MSB_ULONG ? 1: 0;
    *UL_LO_12(p) <<= 1;
    *UL_MED_12(p) = *UL_MED_12(p)<<1 | c0;
    *UL_HI_12(p) = *UL_HI_12(p)<<1 | c1;
}

void __shr_12(_ULDBL12 *p)
{
    u_long c2,c1;
    c2 = *UL_HI_12(p) & 0x1 ? MSB_ULONG: 0;
    c1 = *UL_MED_12(p) & 0x1 ? MSB_ULONG: 0;
    *UL_HI_12(p) >>= 1;
    *UL_MED_12(p) = *UL_MED_12(p)>>1 | c2;
    *UL_LO_12(p) = *UL_LO_12(p)>>1 | c1;
}






/***
*void  __mtold12(char *manptr,unsigned manlen,_ULDBL12 *ld12) -
*   convert a mantissa into a _ULDBL12
*
*Purpose: convert a mantissa into a _ULDBL12. The mantissa is
*   in the form of an array of manlen BCD digits and is
*   considered to be an integer.
*
*Entry: manptr: the array containing the packed BCD digits of the mantissa
*	manlen: the size of the array
*	ld12: a pointer to the long double where the result will be stored
*
*Exit:
*	ld12 gets the result of the conversion
*
*Exceptions:
*
*******************************************************************************/

void  __mtold12(char *manptr,
			 unsigned manlen,
			 _ULDBL12 *ld12)
{
    _ULDBL12 tmp;
    u_short expn = LD_BIASM1+80;

    *UL_LO_12(ld12) = 0;
    *UL_MED_12(ld12) = 0;
    *UL_HI_12(ld12) = 0;
    for (;manlen>0;manlen--,manptr++){
	tmp = *ld12;
	__shl_12(ld12);
	__shl_12(ld12);
	__add_12(ld12,&tmp);
	__shl_12(ld12);	       /* multiply by 10 */
	*UL_LO_12(&tmp) = (u_long)*manptr;
	*UL_MED_12(&tmp) = 0;
	*UL_HI_12(&tmp) = 0;
	__add_12(ld12,&tmp);
    }

    /* normalize mantissa -- first shift word by word */
    while (*UL_HI_12(ld12) == 0) {
	*UL_HI_12(ld12) = *UL_MED_12(ld12) >> 16;
	*UL_MED_12(ld12) = *UL_MED_12(ld12) << 16 | *UL_LO_12(ld12) >> 16;
	(*UL_LO_12(ld12)) <<= 16;
	expn -= 16;
    }
    while ((*UL_HI_12(ld12) & 0x8000) == 0) {
	__shl_12(ld12);
	expn--;
    }
    *U_EXP_12(ld12) = expn;
}
