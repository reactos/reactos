/***
*tenpow.c - multiply a _ULDBL12 by a power of 10
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*
*Revision History:
*   7-17-91	GDP	Initial version (ported from assembly)
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/


#include "mathsup.h"



extern _ULDBL12 _pow10pos[];
extern _ULDBL12 _pow10neg[];




/***
*void  __ld12mul(_ULDBL12 *px, _ULDBL12 *py) -
*   _ULDBL12 multiplication
*
*Purpose: multiply two _ULDBL12 numbers
*
*Entry: px,py: pointers to the _ULDBL12 operands
*
*Exit: *px contains the product
*
*Exceptions:
*
*******************************************************************************/

void  __ld12mul(_ULDBL12 *px, _ULDBL12 *py)
{
    u_short sign = 0;
    u_short sticky_bits = 0;
    _ULDBL12 tempman; /*this is actually a 12-byte mantissa,
			 not a 12-byte long double */
    int i;
    u_short expx, expy, expsum;
    int roffs,poffs,qoffs;
    int sticky;

    *UL_LO_12(&tempman) = 0;
    *UL_MED_12(&tempman) = 0;
    *UL_HI_12(&tempman) = 0;

    expx = *U_EXP_12(px);
    expy = *U_EXP_12(py);

    sign = (expx ^ expy) & (u_short)0x8000;
    expx &= 0x7fff;
    expy &= 0x7fff;
    expsum = expx+expy;
    if (expx >= LD_MAXEXP
	|| expy >= LD_MAXEXP
	|| expsum > LD_MAXEXP+ LD_BIASM1){
	/* overflow to infinity */
	PUT_INF_12(px,sign);
	return;
    }
    if (expsum <= LD_BIASM1-63) {
	/* underflow to zero */
	PUT_ZERO_12(px);
	return;
    }
    if (expx == 0) {
	/*
	 * If this is a denormal temp real then the mantissa
	 * was shifted right once to set bit 63 to zero.
	 */
	expsum++; /* Correct for this */
	if (ISZERO_12(px)) {
	    /* put positive sign */
	    *U_EXP_12(px) = 0;
	    return;
	}
    }
    if (expy == 0) {
	expsum++; /* because arg2 is denormal */
	if (ISZERO_12(py)) {
	    PUT_ZERO_12(px);
	    return;
	}
    }

    roffs = 0;
    for (i=0;i<5;i++) {
	int j;
	poffs = i<<1;
	qoffs = 8;
	for (j=5-i;j>0;j--) {
	    u_long prod;
#ifdef MIPS
	    /* a variable to hold temprary sums */
	    u_long sum;
#endif
	    int carry;
	    u_short *p, *q;
	    u_long *r;
	    p = USHORT_12(px,poffs);
	    q = USHORT_12(py,qoffs);
	    r = ULONG_12(&tempman,roffs);
	    prod = (u_long)*p * (u_long)*q;
#ifdef MIPS
	    /* handle misalignment problems */
	    if (i&0x1){ /* i is odd */
        carry = __addl(*MIPSALIGN(r), prod, &sum);
        *MIPSALIGN(r) =  sum;
	    }
	    else /* i is even */
		carry = __addl(*r, prod, r);
#else
	    carry = __addl(*r,prod,r);
#endif
	    if (carry) {
		/* roffs should be less than 8 in this case */
		(*USHORT_12(&tempman,roffs+4))++;
	    }
	    poffs+=2;
	    qoffs-=2;
	}
	roffs+=2;
    }

    expsum -= LD_BIASM1;

    /* normalize */
    while ((s_short)expsum > 0 &&
	   ((*UL_HI_12(&tempman) & MSB_ULONG) == 0)) {
	 __shl_12(&tempman);
	 expsum--;
    }

    if ((s_short)expsum <= 0) {
	expsum--;
        sticky = 0;
	while ((s_short)expsum < 0) {
	    if (*U_XT_12(&tempman) & 0x1)
		sticky++;
	    __shr_12(&tempman);
	    expsum++;
	}
	if (sticky)
	    *U_XT_12(&tempman) |= 0x1;
    }

    if (*U_XT_12(&tempman) > 0x8000 ||
	 ((*UL_LO_12(&tempman) & 0x1ffff) == 0x18000)) {
	/* round up */
	if (*UL_MANLO_12(&tempman) == MAX_ULONG) {
	    *UL_MANLO_12(&tempman) = 0;
	    if (*UL_MANHI_12(&tempman) == MAX_ULONG) {
		*UL_MANHI_12(&tempman) = 0;
		if (*U_EXP_12(&tempman) == MAX_USHORT) {
		    /* 12-byte mantissa overflow */
		    *U_EXP_12(&tempman) = MSB_USHORT;
		    expsum++;
		}
		else
		    (*U_EXP_12(&tempman))++;
	    }
	    else
		(*UL_MANHI_12(&tempman))++;
	}
	else
	    (*UL_MANLO_12(&tempman))++;
    }


    /* check for exponent overflow */
    if (expsum >= 0x7fff){
	PUT_INF_12(px, sign);
	return;
    }

    /* put result in px */
    *U_XT_12(px) = *USHORT_12(&tempman,2);
    *UL_MANLO_12(px) = *UL_MED_12(&tempman);
    *UL_MANHI_12(px) = *UL_HI_12(&tempman);
    *U_EXP_12(px) = expsum | sign;
}



void __multtenpow12(_ULDBL12 *pld12, int pow, unsigned mult12)
{
    _ULDBL12 *pow_10p = _pow10pos-8;
    if (pow == 0)
	return;
    if (pow < 0) {
	pow = -pow;
	pow_10p = _pow10neg-8;
    }

    if (!mult12)
	*U_XT_12(pld12) = 0;


    while (pow) {
	int last3; /* the 3 LSBits of pow */
	_ULDBL12 unround;
	_ULDBL12 *py;

	pow_10p += 7;
	last3 = pow & 0x7;
	pow >>= 3;
	if (last3 == 0)
	    continue;
	py = pow_10p + last3;

#ifdef _ULDSUPPORT
	if (mult12) {
#endif
	    /* do an exact 12byte multiplication */
	    if (*U_XT_12(py) >= 0x8000) {
		/* copy number */
		unround = *py;
		/* unround adjacent byte */
		(*UL_MANLO_12(&unround))--;
		/* point to new operand */
		py = &unround;
	    }
	    __ld12mul(pld12,py);
#ifdef _ULDSUPPORT
	}
	else {
	    /* do a 10byte multiplication */
	    py = (_ULDBL12 *)TEN_BYTE_PART(py);
	    *(long double *)TEN_BYTE_PART(pld12) *=
		*(long double *)py;
	}
#endif
    }
}
