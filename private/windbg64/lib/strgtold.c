/***
*strgtold.c - conversion of a string into a long double
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose: convert a fp constant into a 10 byte long double (IEEE format)
*
*Revision History:
*   7-17-91	GDP	Initial version (ported from assembly)
*   4-03-92	GDP	Preserve sign of -0
*   4-30-92	GDP	Now returns _ULDBL12 instead of _ULDOUBLE
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

//#include <ctype.h>  /* for 'isdigit' macro */
#include "mathsup.h"



/* local macros */
#define ISNZDIGIT(x) ((x)>='1' && (x)<='9' )

//NOTENOTE the following takes the place of the isdigit() macro
//       which does not work for a yet to be determined reason
#define ISADIGIT(x) ((x)>='0' && (x)<='9' )

#define ISWHITE(x) ((x)==' ' || (x)=='\t' || (x)=='\n' || (x)=='\r' )




/****
*unsigned int __strgtold( _ULDBL12 *pld12,
*			  char * * pEndPtr,
*			  char * str,
*			  int Mult12 )
*
*Purpose:
*   converts a character string into a long double
*
*Entry:
*   pld12   - pointer to the _ULDBL12 where the result should go.
*   pEndStr - pointer to a far pointer that will be set to the end of string.
*   str     - pointer to the string to be converted.
*   Mult12  - set to non zero if the _ULDBL12 multiply should be used instead of
*		the long double mulitiply.
*
*Exit:
*   Returns the SLD_* flags or'ed together.
*
*Uses:
*
*Exceptions:
*
********************************************************************************/

unsigned int
__strgtold12(_ULDBL12 *pld12,
	    char * *p_end_ptr,
	    char *str,
	    int mult12)
{
    typedef enum {
	S_INIT,  /* initial state */
	S_EAT0L, /* eat 0's at the left of mantissa */
	S_SIGNM, /* just read sign of mantissa */
	S_GETL,  /* get integer part of mantissa */
	S_GETR,  /* get decimal part of mantissa */
	S_POINT, /* just found decimal point */
	S_E,	 /* just found	'E', or 'e', etc  */
	S_SIGNE, /* just read sign of exponent */
	S_EAT0E, /* eat 0's at the left of exponent */
	S_GETE,  /* get exponent */
	S_END	 /* final state */
    } state_t;

    /* this will accomodate the digits of the mantissa in BCD form*/
    static char buf[LD_MAX_MAN_LEN1];
    char *manp = buf;

    /* a temporary _ULDBL12 */
    _ULDBL12 tmpld12;

    u_short man_sign = 0; /* to be ORed with result */
    int exp_sign = 1; /* default sign of exponent (values: +1 or -1)*/
    /* number of decimal significant mantissa digits so far*/
    unsigned manlen = 0;
    int found_digit = 0;
    int overflow = 0;
    int underflow = 0;
    int pow = 0;
    int exp_adj = 0;  /* exponent adjustment */
    u_long ul0,ul1;
    u_short u,uexp;

    unsigned int result_flags = 0;

    state_t state = S_INIT;

    char c;  /* the current input symbol */
    char *p; /* a pointer to the next input symbol */
    char *savedp;

    for(savedp=p=str;ISWHITE(*p);p++); /* eat up white space */

    while (state != S_END) {
	c = *p++;
	switch (state) {
	case S_INIT:
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case '+':
		    state = S_SIGNM;
		    man_sign = 0x0000;
		    break;
		case '-':
		    state = S_SIGNM;
		    man_sign = 0x8000;
		    break;
		case '.':
		    state = S_POINT;
		    break;
		default:
		    state = S_END;
		    p--;
		    break;
		}
	    break;
	case S_EAT0L:
	    found_digit = 1;
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case 'E':
		case 'e':
		case 'D':
		case 'd':
		    state = S_E;
		    break;
		case '.':
		    state = S_GETR;
		    break;
		default:
		    state = S_END;
		    p--;
		}
	    break;
	case S_SIGNM:
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case '.':
		    state = S_POINT;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	    break;
	case S_GETL:
	    found_digit = 1;
	    for (;ISADIGIT(c);c=*p++) {
		if (manlen < LD_MAX_MAN_LEN+1){
		    manlen++;
		    *manp++ = c - (char)'0';
		}
		else
		   exp_adj++;
	    }
	    switch (c) {
	    case '.':
		state = S_GETR;
		break;
	    case 'E':
	    case 'e':
	    case 'D':
	    case 'd':
		state = S_E;
		break;
	    default:
		state = S_END;
		p--;
	    }
	break;
	case S_GETR:
	    found_digit = 1;
	    if (manlen == 0)
		for (;c=='0';c=*p++)
		    exp_adj--;
	    for(;ISADIGIT(c);c=*p++){
		if (manlen < LD_MAX_MAN_LEN+1){
		    manlen++;
		    *manp++ = c - (char)'0';
		    exp_adj--;
		}
	    }
	    switch (c){
	    case 'E':
	    case 'e':
	    case 'D':
	    case 'd':
		state = S_E;
		break;
	    default:
		state = S_END;
		p--;
	    }
	    break;
	case S_POINT:
	    if (ISADIGIT(c)){
		state = S_GETR;
		p--;
	    }
	    else{
		state = S_END;
		p = savedp;
	    }
	    break;
	case S_E:
	    savedp = p-2; /* savedp points to 'E' */
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else
		switch (c){
		case '0':
		    state = S_EAT0E;
		    break;
		case '-':
		    state = S_SIGNE;
		    exp_sign = -1;
		    break;
		case '+':
		    state = S_SIGNE;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	break;
	case S_EAT0E:
	    for(;c=='0';c=*p++);
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else {
		state = S_END;
		p--;
	    }
	    break;
	case S_SIGNE:
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else
		switch (c){
		case '0':
		    state = S_EAT0E;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	    break;
	case S_GETE:
	    {
		long longpow=0; /* TMAX10*10 should fit in a long */
		for(;ISADIGIT(c);c=*p++){
		    longpow = longpow*10 + (c - '0');
		    if (longpow > TMAX10){
			longpow = TMAX10+1; /* will force overflow */
			break;
		    }
		}
		pow = (int)longpow;
	    }
	    for(;ISADIGIT(c);c=*p++); /* eat up remaining digits */
	    state = S_END;
	    p--;
	    break;
	}  /* switch */
    }  /* while */

    *p_end_ptr = p;	/* set end pointer */

    /*
     * Compute result
     */

    if (found_digit && !overflow && !underflow) {
	if (manlen>LD_MAX_MAN_LEN){
	    if (buf[LD_MAX_MAN_LEN-1]>=5) {
	       /*
		* Round mantissa to MAX_MAN_LEN digits
		* It's ok to round 9 to 0ah
		*/
		buf[LD_MAX_MAN_LEN-1]++;
	    }
	    manlen = LD_MAX_MAN_LEN;
	    manp--;
	    exp_adj++;
	}
	if (manlen>0) {
	   /*
	    * Remove trailing zero's from mantissa
	    */
	    for(manp--;*manp==0;manp--) {
		/* there is at least one non-zero digit */
		manlen--;
		exp_adj++;
	    }
	    __mtold12(buf,manlen,&tmpld12);

	    if (exp_sign < 0)
		pow = -pow;
	    pow += exp_adj;
	    if (pow > TMAX10)
		overflow = 1;
	    else if (pow < TMIN10)
		underflow = 1;
	    else {
		__multtenpow12(&tmpld12,pow,mult12);

		u = *U_XT_12(&tmpld12);
		ul0 =*UL_MANLO_12(&tmpld12);
		ul1 = *UL_MANHI_12(&tmpld12);
		uexp = *U_EXP_12(&tmpld12);

	    }
	}
	else {
	    /* manlen == 0, so	return 0 */
	    u = (u_short)0;
	    ul0 = ul1 = uexp = 0;
	}
    }

    if (!found_digit) {
       /* return 0 */
       u = (u_short)0;
       ul0 = ul1 = uexp = 0;
       result_flags |= SLD_NODIGITS;
    }
    else if (overflow) {
	/* return +inf or -inf */
	uexp = (u_short)0x7fff;
	ul1 = 0x80000000;
	ul0 = 0;
	u = (u_short)0;
	result_flags |= SLD_OVERFLOW;
    }
    else if (underflow) {
       /* return 0 */
       u = (u_short)0;
       ul0 = ul1 = uexp = 0;
       result_flags |= SLD_UNDERFLOW;
    }

    /*
     * Assemble	result
     */

    *U_XT_12(pld12) = u;
    *UL_MANLO_12(pld12) = ul0;
    *UL_MANHI_12(pld12) = ul1;
    *U_EXP_12(pld12) = uexp | man_sign;

    return result_flags;
}
