

/* @(#)s_modf.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
FUNCTION
       <<modf>>, <<modff>>---split fractional and integer parts

INDEX
	modf
INDEX
	modff

ANSI_SYNOPSIS
	#include <math.h>
	double modf(double <[val]>, double *<[ipart]>);
        float modff(float <[val]>, float *<[ipart]>);

TRAD_SYNOPSIS
	#include <math.h>
	double modf(<[val]>, <[ipart]>)
        double <[val]>;
        double *<[ipart]>;

	float modff(<[val]>, <[ipart]>)
	float <[val]>;
        float *<[ipart]>;

DESCRIPTION
	<<modf>> splits the double <[val]> apart into an integer part
	and a fractional part, returning the fractional part and
	storing the integer part in <<*<[ipart]>>>.  No rounding
	whatsoever is done; the sum of the integer and fractional
	parts is guaranteed to be exactly  equal to <[val]>.   That
	is, if . <[realpart]> = modf(<[val]>, &<[intpart]>); then
	`<<<[realpart]>+<[intpart]>>>' is the same as <[val]>.
	<<modff>> is identical, save that it takes and returns
	<<float>> rather than <<double>> values. 

RETURNS
	The fractional part is returned.  Each result has the same
	sign as the supplied argument <[val]>.

PORTABILITY
	<<modf>> is ANSI C. <<modff>> is an extension.

QUICKREF
	modf  ansi pure 
	modff - pure

*/

/*
 * modf(double x, double *iptr) 
 * return fraction part of x, and return x's integral part in *iptr.
 * Method:
 *	Bit twiddling.
 *
 * Exception:
 *	No exception.
 */


static const double one = 1.0;

#define __int32_t long
#define __uint32_t unsigned long
#define __IEEE_LITTLE_ENDIAN

#ifdef __IEEE_BIG_ENDIAN

typedef union 
{
  struct 
  {
    __uint32_t msw;
    __uint32_t lsw;
  } parts;
  double value;
} ieee_double_shape_type;

#endif

#ifdef __IEEE_LITTLE_ENDIAN

typedef union 
{
  struct 
  {
    __uint32_t lsw;
    __uint32_t msw;
  } parts;
  double value;
} ieee_double_shape_type;

#endif


/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)					\
do {								\
  ieee_double_shape_type gl_u;					\
  gl_u.value = (d);						\
  (i) = gl_u.parts.lsw;						\
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)




double modf(double x, double *iptr)
{
	__int32_t i0,i1,j_0;
	__uint32_t i;
	EXTRACT_WORDS(i0,i1,x);
	j_0 = ((i0>>20)&0x7ff)-0x3ff;	/* exponent of x */
	if(j_0<20) {			/* integer part in high x */
	    if(j_0<0) {			/* |x|<1 */
	        INSERT_WORDS(*iptr,i0&0x80000000U,0);	/* *iptr = +-0 */
		return x;
	    } else {
		i = (0x000fffff)>>j_0;
		if(((i0&i)|i1)==0) {		/* x is integral */
		    __uint32_t high;
		    *iptr = x;
		    GET_HIGH_WORD(high,x);
		    INSERT_WORDS(x,high&0x80000000U,0);	/* return +-0 */
		    return x;
		} else {
		    INSERT_WORDS(*iptr,i0&(~i),0);
		    return x - *iptr;
		}
	    }
	} else if (j_0>51) {		/* no fraction part */
	    __uint32_t high;
	    *iptr = x*one;
	    GET_HIGH_WORD(high,x);
	    INSERT_WORDS(x,high&0x80000000U,0);	/* return +-0 */
	    return x;
	} else {			/* fraction part in low x */
	    i = ((__uint32_t)(0xffffffffU))>>(j_0-20);
	    if((i1&i)==0) { 		/* x is integral */
	        __uint32_t high;
		*iptr = x;
		GET_HIGH_WORD(high,x);
		INSERT_WORDS(x,high&0x80000000U,0);	/* return +-0 */
		return x;
	    } else {
	        INSERT_WORDS(*iptr,i0,i1&(~i));
		return x - *iptr;
	    }
	}
}

//#endif /* _DOUBLE_IS_32BITS */
