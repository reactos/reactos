/***
* intrncvt.c - internal floating point conversions
*
*	Copyright (c) 1992-1992, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   All fp string conversion routines use the same core conversion code
*   that converts strings into an internal long double representation
*   with an 80-bit mantissa field. The mantissa is represented
*   as an array (man) of 32-bit unsigned longs, with man[0] holding
*   the high order 32 bits of the mantissa. The binary point is assumed
*   to be between the MSB and MSB-1 of man[0].
*
*   Bits are counted as follows:
*
*
*     +-- binary point
*     |
*     v 		 MSB	       LSB
*   ----------------	 ------------------	 --------------------
*   |0 1    .... 31|	 | 32 33 ...	63|	 | 64 65 ...	  95|
*   ----------------	 ------------------	 --------------------
*
*   man[0]		    man[1]		     man[2]
*
*   This file provides the final conversion routines from this internal
*   form to the single, double, or long double precision floating point
*   format.
*
*   All these functions do not handle NaNs (it is not necessary)
*
*
*Revision History:
*   04-29-92	GDP	written
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/


#include <assert.h>
#include "mathsup.h"




#define INTRNMAN_LEN  3	      /* internal mantissa length in int's */

//
//  internal mantissaa representation
//  for string conversion routines
//

typedef u_long *intrnman;


typedef struct {
   int max_exp;      // maximum base 2 exponent (reserved for special values)
   int min_exp;      // minimum base 2 exponent (reserved for denormals)
   int precision;    // bits of precision carried in the mantissa
   int exp_width;    // number of bits for exponent
   int format_width; // format width in bits
   int bias;	     // exponent bias
} FpFormatDescriptor;



static FpFormatDescriptor
DoubleFormat = {
    0x7ff - 0x3ff,  //	1024, maximum base 2 exponent (reserved for special values)
    0x0   - 0x3ff,  // -1023, minimum base 2 exponent (reserved for denormals)
    53, 	    // bits of precision carried in the mantissa
    11, 	    // number of bits for exponent
    64, 	    // format width in bits
    0x3ff,	    // exponent bias
};

static FpFormatDescriptor
FloatFormat = {
    0xff - 0x7f,    //	128, maximum base 2 exponent (reserved for special values)
    0x0  - 0x7f,    // -127, minimum base 2 exponent (reserved for denormals)
    24, 	    // bits of precision carried in the mantissa
    8,		    // number of bits for exponent
    32, 	    // format width in bits
    0x7f,	    // exponent bias
};



//
// function prototypes
//

int _RoundMan (intrnman man, int nbit);
int _ZeroTail (intrnman man, int nbit);
int _IncMan (intrnman man, int nbit);
void _CopyMan (intrnman dest, intrnman src);
void _CopyMan (intrnman dest, intrnman src);
void _FillZeroMan(intrnman man);
void _Shrman (intrnman man, int n);

INTRNCVT_STATUS _ld12cvt(_ULDBL12 *pld12, void *d, FpFormatDescriptor *format);

/***
* _ZeroTail - check if a mantissa ends in 0's
*
*Purpose:
*   Return TRUE if all mantissa bits after nbit (including nbit) are 0,
*   otherwise return FALSE
*
*
*Entry:
*   man: mantissa
*   nbit: order of bit where the tail begins
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _ZeroTail (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;


    //
    //		     |<---- tail to be checked --->
    //
    //	--  ------------------------	       ----
    //	|...	|		  |  ...	  |
    //	--  ------------------------	       ----
    //	^	^    ^
    //	|	|    |<----nb----->
    //	man	nl   nbit
    //



    u_long bitmask = ~(MAX_ULONG << nb);

    if (man[nl] & bitmask)
	return 0;

    nl++;

    for (;nl < INTRNMAN_LEN; nl++)
	if (man[nl])
	    return 0;

    return 1;
}




/***
* _IncMan - increment mantissa
*
*Purpose:
*
*
*Entry:
*   man: mantissa in internal long form
*   nbit: order of bit that specifies the end of the part to be incremented
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _IncMan (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;

    //
    //	|<--- part to be incremented -->|
    //
    //	--	       ---------------------------     ----
    //	|...		  |			|   ...	  |
    //	--	       ---------------------------     ----
    //	^		  ^		^
    //	|		  |		|<--nb-->
    //	man		  nl		nbit
    //

    u_long one = (u_long) 1 << nb;
    int carry;

    carry = __addl(man[nl], one, &man[nl]);

    nl--;

    for (; nl >= 0 && carry; nl--) {
	carry = (u_long) __addl(man[nl], (u_long) 1, &man[nl]);
    }

    return carry;
}




/***
* _RoundMan -  round mantissa
*
*Purpose:
*   round mantissa to nbit precision
*
*
*Entry:
*   man: mantissa in internal form
*   precision: number of bits to be kept after rounding
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _RoundMan (intrnman man, int precision)
{
    int i,rndbit,nl,nb;
    u_long rndmask;
    int nbit;
    int retval = 0;

    //
    // The order of the n'th bit is n-1, since the first bit is bit 0
    // therefore decrement precision to get the order of the last bit
    // to be kept
    //
    nbit = precision - 1;

    rndbit = nbit+1;

    nl = rndbit / 32;
    nb = 31 - rndbit % 32;

    //
    // Get value of round bit
    //

    rndmask = (u_long)1 << nb;

    if ((man[nl] & rndmask) &&
	 !_ZeroTail(man, rndbit+1)) {

	//
	// round up
	//

	retval = _IncMan(man, nbit);
    }


    //
    // fill rest of mantissa with zeroes
    //

    man[nl] &= MAX_ULONG << nb;
    for(i=nl+1; i<INTRNMAN_LEN; i++) {
	man[i] = (u_long)0;
    }

    return retval;
}


/***
* _CopyMan - copy mantissa
*
*Purpose:
*    copy src to dest
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _CopyMan (intrnman dest, intrnman src)
{
    u_long *p, *q;
    int i;

    p = src;
    q = dest;

    for (i=0; i < INTRNMAN_LEN; i++) {
	*q++ = *p++;
    }
}



/***
* _FillZeroMan - fill mantissa with zeroes
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _FillZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
	man[i] = (u_long)0;
}



/***
* _IsZeroMan - check if mantissa is zero
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _IsZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
	if (man[i])
	    return 0;

    return 1;
}





/***
* _ShrMan - shift mantissa to the right
*
*Purpose:
*  shift man by n bits to the right
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _ShrMan (intrnman man, int n)
{
    int i, n1, n2, mask;
    int carry_from_left;

    //
    // declare this as volatile in order to work around a C8
    // optimization bug
    //

    volatile int carry_to_right;

    n1 = n / 32;
    n2 = n % 32;

    mask = ~(MAX_ULONG << n2);


    //
    // first deal with shifts by less than 32 bits
    //

    carry_from_left = 0;
    for (i=0; i<INTRNMAN_LEN; i++) {

	carry_to_right = man[i] & mask;

	man[i] >>= n2;

	man[i] |= carry_from_left;

	carry_from_left = carry_to_right << (32 - n2);
    }


    //
    // now shift whole 32-bit ints
    //

    for (i=INTRNMAN_LEN-1; i>=0; i--) {
	if (i >= n1) {
	    man[i] = man[i-n1];
	}
	else {
	    man[i] = 0;
	}
    }
}




/***
* _ld12tocvt - _ULDBL12 floating point conversion
*
*Purpose:
*   convert a internal _LBL12 structure into an IEEE floating point
*   representation
*
*
*Entry:
*   pld12:  pointer to the _ULDBL12
*   format: pointer to the format descriptor structure
*
*Exit:
*   *d contains the IEEE representation
*   returns the INTRNCVT_STATUS
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12cvt(_ULDBL12 *pld12, void *d, FpFormatDescriptor *format)
{
    u_long man[INTRNMAN_LEN];
    u_long saved_man[INTRNMAN_LEN];
    u_long msw;
    unsigned int bexp;			// biased exponent
    int exp_shift;
    int exponent, sign;
    INTRNCVT_STATUS retval;

    exponent = (*U_EXP_12(pld12) & 0x7fff) - 0x3fff;   // unbias exponent
    sign = *U_EXP_12(pld12) & 0x8000;

    //
    // bexp is the final biased value of the exponent to be used
    // Each of the following blocks should provide appropriate
    // values for man, bexp and retval. The mantissa is also
    // shifted to the right, leaving space for the exponent
    // and sign to be inserted
    //


    if (exponent == 0 - 0x3fff) {

	// either a denormal or zero
	bexp = 0;

	if (_IsZeroMan(man)) {

	    retval = INTRNCVT_OK;
	}
	else {

	    _FillZeroMan(man);

	    // denormal has been flushed to zero

	    retval = INTRNCVT_UNDERFLOW;
	}
    }
    else {

	man[0] = *UL_MANHI_12(pld12);
	man[1] = *UL_MANLO_12(pld12);
	man[2] = *U_XT_12(pld12) << 16;

	// save mantissa in case it needs to be rounded again
	// at a different point (e.g., if the result is a denormal)

	_CopyMan(saved_man, man);

	if (_RoundMan(man, format->precision)) {
	    exponent ++;
	}

	if (exponent < format->min_exp - format->precision ) {

	    //
	    // underflow that produces a zero
	    //

	    _FillZeroMan(man);
	    bexp = 0;
	    retval = INTRNCVT_UNDERFLOW;
	}

	else if (exponent <= format->min_exp) {

	    //
	    // underflow that produces a denormal
	    //
	    //

	    // The (unbiased) exponent will be MIN_EXP
	    // Find out how much the mantissa should be shifted
	    // One shift is done implicitly by moving the
	    // binary point one bit to the left, i.e.,
	    // we treat the mantissa as .ddddd instead of d.dddd
	    // (where d is a binary digit)

	    int shift = format->min_exp - exponent;

	    // The mantissa should be rounded again, so it
	    // has to be restored

	    _CopyMan(man,saved_man);

	    _ShrMan(man, shift);
	    _RoundMan(man, format->precision); // need not check for carry

	    // make room for the exponent + sign

	    _ShrMan(man, format->exp_width + 1);

	    bexp = 0;
	    retval = INTRNCVT_UNDERFLOW;

	}

	else if (exponent >= format->max_exp) {

	    //
	    // overflow, return infinity
	    //

	    _FillZeroMan(man);
	    man[0] |= (1 << 31); // set MSB

	    // make room for the exponent + sign

	    _ShrMan(man, (format->exp_width + 1) - 1);

	    bexp = format->max_exp + format->bias;

	    retval = INTRNCVT_OVERFLOW;
	}

	else {

	    //
	    // valid, normalized result
	    //

	    bexp = exponent + format->bias;


	    // clear implied bit

	    man[0] &= (~( 1 << 31));

	    //
	    // shift right to make room for exponent + sign
	    //

	    _ShrMan(man, (format->exp_width + 1) - 1);

	    retval = INTRNCVT_OK;

	}
    }


    exp_shift = 32 - (format->exp_width + 1);
    msw =  man[0] |
	   (bexp << exp_shift) |
	   (sign ? 1<<31 : 0);

    if (format->format_width == 64) {

	*UL_HI_D(d) = msw;
	*UL_LO_D(d) = man[1];
    }

    else if (format->format_width == 32) {

	*(u_long *)d = msw;

    }

    return retval;
}


/***
* _ld12tod - convert _ULDBL12 to double
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12tod(_ULDBL12 *pld12, UDOUBLE *d)
{
    return _ld12cvt(pld12, d, &DoubleFormat);
}



/***
* _ld12tof - convert _ULDBL12 to float
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12tof(_ULDBL12 *pld12, FLOAT *f)
{
    return _ld12cvt(pld12, f, &FloatFormat);
}


/***
* _ld12told - convert _ULDBL12 to 80 bit long double
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _ld12told(_ULDBL12 *pld12, _ULDOUBLE *pld)
{

    //
    // This implementation is based on the fact that the _ULDBL12 format is
    // identical to the long double and has 2 extra bytes of mantissa
    //

    u_short exp, sign;
    u_long man[INTRNMAN_LEN];

    exp = *U_EXP_12(pld12) & (u_short)0x7fff;
    sign = *U_EXP_12(pld12) & (u_short)0x8000;

    man[0] = *UL_MANHI_12(pld12);
    man[1] = *UL_MANLO_12(pld12);
    man[2] = *U_XT_12(pld12) << 16;

    if (_RoundMan(man, 64))
	exp ++;

    *UL_MANHI_LD(pld) = man[0];
    *UL_MANLO_LD(pld) = man[1];
    *U_EXP_LD(pld) = sign | exp;
}


void _atodbl(UDOUBLE *d, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12tod(&ld12, d);
}


void _atoldbl(_ULDOUBLE *ld, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12told(&ld12, ld);
}


void _atoflt(FLOAT *f, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12tof(&ld12, f);
}
