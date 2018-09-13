/***
*cfin.c - Encode interface for C
*
*	Copyright (c) 19xx-1991, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*   07-20-91	    GDP     Ported to C from assembly
*   04-30-92	    GDP     use __strgtold12 and _ld12tod
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

#include <string.h>
#include "mathsup.h"




#ifndef MTHREAD
static struct _flt ret;
static FLT flt = &ret;
#endif

/* The only three conditions that this routine detects */
#define CFIN_NODIGITS 512
#define CFIN_OVERFLOW 128
#define CFIN_UNDERFLOW 256

/* This version ignores the last two arguments (radix and scale)
 * Input string should be null terminated
 * len is also ignored
 */
#ifdef MTHREAD
FLT  _fltin2(FLT flt, const char *str, int len_ignore, int scale_ignore, int radix_ignore)
#else
FLT  _fltin(const char *str, int len_ignore, int scale_ignore, int radix_ignore)
#endif
{
    _ULDBL12 ld12;
    UDOUBLE x;
    char *EndPtr;
    unsigned flags;
    int retflags = 0;

    flags = __strgtold12(&ld12, &EndPtr, (char *)str, 0);
    if (flags & SLD_NODIGITS) {
	retflags |= CFIN_NODIGITS;
	*(u_long *)&x = 0;
	*((u_long *)&x+1) = 0;
    }
    else {
	INTRNCVT_STATUS intrncvt;

	intrncvt = _ld12tod(&ld12, &x);

	if (flags & SLD_OVERFLOW  ||
	    intrncvt == INTRNCVT_OVERFLOW) {
	    retflags |= CFIN_OVERFLOW;
	}
	if (flags & SLD_UNDERFLOW ||
	    intrncvt == INTRNCVT_UNDERFLOW) {
	    retflags |= CFIN_UNDERFLOW;
	}
    }

    flt->flags = retflags;
    flt->nbytes = (int) (EndPtr - (char *)str);
    flt->dval = *(double *)&x;

    return flt;
}
