/***
*x10fout.c - floating point output for 10-byte long double
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Support conversion of a long double into a string
*
*Revision History:
*   07/15/91	GDP	Initial version in C (ported from assembly)
*   01/23/92	GDP	Support MIPS encoding for NaN
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "mathsup.h"



#define STRCPY strcpy

#define PUT_ZERO_FOS(fos)	 \
		fos->exp = 1,	 \
		fos->sign = ' ', \
		fos->ManLen = 1, \
		fos->man[0] = '0',\
		fos->man[1] = 0;

#define SNAN_STR      "1#SNAN"
#define SNAN_STR_LEN  6
#define QNAN_STR      "1#QNAN"
#define QNAN_STR_LEN  6
#define INF_STR	      "1#INF"
#define INF_STR_LEN   5
#define IND_STR	      "1#IND"
#define IND_STR_LEN   5

#define MAX_10_LEN  30  //max length of string including NULL

/***
char * _uldtoa (_ULDOUBLE *px,
*               int maxchars,
*               char *ldtext)
*
*
*Purpose:
*   Return pointer to filled in string "ldtext" for
*   a given _UDOUBLE ponter px
*   with a maximum character width of maxchars
*
*Entry:
*   _ULDOUBLE * px:  a pointer to the long double to be converted into a string
*   int maxchars: number of digits allowed in the output format.
*
*   (default is 'e' format)
*
*   char * ldtext: a pointer to the output string
*
*Exit:
*    returns pointer to the output string
*
*Exceptions:
*
*******************************************************************************/


char * _uldtoa (_ULDOUBLE *px, int maxchars, char *ldtext)
{
    char        in_str [250];
    char        in_str2 [250];
    char        cExp[20];
    FOS         foss;
    char *      lpszMan;
    char *      lpIndx;
    int         nErr;
    int         len1,  len2;

    maxchars -= 8;    /* sign, dot, E+0001 */

    nErr = $I10_OUTPUT (*px, maxchars, 0, &foss);

    lpszMan = foss.man;
 		  
    ldtext[0] = foss.sign;
    ldtext[1] = *lpszMan;
    ldtext[2] = '.';
    ldtext[3] = '\0';

    maxchars += 2;               /* sign, dot */

    lpszMan++;
    strcat (ldtext, lpszMan);

    len1 = strlen (ldtext);  // for 'e'


    strcpy (cExp, "e");

    foss.exp -= 1;              /* Adjust for the shift decimal shift above */
    _itoa (foss.exp, in_str, 10);

	 
    if (foss.exp < 0) {
        strcat (cExp, "-");

        strcpy (in_str2, &in_str[1]);
        strcpy (in_str, in_str2);
 		  
        while (strlen(in_str) < 4) {
            strcpy (in_str2, in_str);
            strcpy (in_str,"0");
            strcat (in_str,in_str2);
        }
    } else {
        while (strlen(in_str) < 4) {
            strcpy (in_str2, in_str);
            strcpy (in_str,"0");
            strcat (in_str,in_str2);
        }
    }

    if (foss.exp >= 0) {
        strcat (cExp, "+");
    }

    strcat (cExp, in_str);

    len2 = strlen (cExp);

    if (len1 == maxchars) {
        ;
    } 
    else if (len1 < maxchars) {
        do {
            strcat (ldtext,"0");
            len1++;
        } while (len1 < maxchars);
    }
    else {
        lpIndx = &ldtext[len1 - 1]; // point to last char and round
        do {
            *lpIndx = '\0';
            lpIndx--;
            len1--;           //NOTENOTE v-griffk we really need to round
        } while (len1 > maxchars);
    }
    
    strcat (ldtext, cExp);
    return ldtext;
}


/***
*int  _$i10_output(_ULDOUBLE ld,
*	    int ndigits,
*	    unsigned output_flags,
*	    FOS *fos) - output conversion of a 10-byte _ULDOUBLE
*
*Purpose:
*   Fill in a FOS structure for a given _ULDOUBLE
*
*Entry:
*   _ULDOUBLE ld:  The long double to be converted into a string
*   int ndigits: number of digits allowed in the output format.
*   unsigned output_flags: The following flags can be used:
*	SO_FFORMAT: Indicates 'f' format
*	(default is 'e' format)
*   FOS *fos: the structure that i10_output will fill in
*
*Exit:
*   modifies *fos
*   return 1 if original number was ok, 0 otherwise (infinity, NaN, etc)
*
*Exceptions:
*
*******************************************************************************/


int  $I10_OUTPUT(_ULDOUBLE ld, int ndigits,
		    unsigned output_flags, FOS *fos)
{
    u_short expn;
    u_long manhi,manlo;
    u_short sign;

    /* useful constants (see algorithm explanation below) */
    u_short const log2hi = 0x4d10;
    u_short const log2lo = 0x4d;
    u_short const log4hi = 0x9a;
    u_long const c = 0x134312f4;
#if defined(L_END)
    _ULDBL12 ld12_one_tenth = {
	   {0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
	    0xcc,0xcc,0xcc,0xcc,0xfb,0x3f}
    };
#elif defined(B_END)
    _ULDBL12 ld12_one_tenth = {
	   {0x3f,0xfb,0xcc,0xcc,0xcc,0xcc,
	    0xcc,0xcc,0xcc,0xcc,0xcc,0xcc}
    };
#endif

    _ULDBL12 ld12; /* space for a 12-byte long double */
    _ULDBL12 tmp12;
    u_short hh,ll; /* the bytes of the exponent grouped in 2 words*/
    u_short mm; /* the two MSBytes of the mantissa */
    s_long r; /* the corresponding power of 10 */
    s_short ir; /* ir = floor(r) */
    int retval = 1; /* assume valid number */
    char round; /* an additional character at the end of the string */
    char *p;
    int i;
    int ub_exp;
    int digcount;

    /* grab the components of the long double */
    expn = *U_EXP_LD(&ld);
    manhi = *UL_MANHI_LD(&ld);
    manlo = *UL_MANLO_LD(&ld);
    sign = expn & MSB_USHORT;
    expn &= 0x7fff;

    if (sign)
	fos->sign = '-';
    else
	fos->sign = ' ';

    if (expn==0 && manhi==0 && manlo==0) {
	PUT_ZERO_FOS(fos);
	return 1;
    }

    if (expn == 0x7fff) {
	fos->exp = 1; /* set a positive exponent for proper output */

	/* check for special cases */
	if (_IS_MAN_SNAN(sign, manhi, manlo)) {
	    /* signaling NAN */
	    STRCPY(fos->man,SNAN_STR);
	    fos->ManLen = SNAN_STR_LEN;
	    retval = 0;
	}
	else if (_IS_MAN_IND(sign, manhi, manlo)) {
	    /* indefinite */
	    STRCPY(fos->man,IND_STR);
	    fos->ManLen = IND_STR_LEN;
	    retval = 0;
	}
	else if (_IS_MAN_INF(sign, manhi, manlo)) {
	    /* infinity */
	    STRCPY(fos->man,INF_STR);
	    fos->ManLen = INF_STR_LEN;
	    retval = 0;
	}
	else {
	    /* quiet NAN */
	    STRCPY(fos->man,QNAN_STR);
	    fos->ManLen = QNAN_STR_LEN;
	    retval = 0;
	}
    }
    else {
       /*
	*    Algorithm for the decoding of a valid real number x
	*
	* In the  following  INT(r)  is	the largest integer less than or
	* equal to r (i.e. r rounded toward -infinity).	We want a result
	* r equal  to  1  + log(x), because then x = mantissa
	* * 10^(INT(r)) so that	.1  <=	mantissa  <  1.   Unfortunately,
	* we cannot  compute  s	exactly  so  we must alter the procedure
	* slightly.  We will  instead  compute	an  estimate  r	of  1  +
	* log(x) which	is  always  low.   This	will either result
	* in the correctly normalized number on	the  top  of  the  stack
	* or perhaps  a	number	which  is  a factor of 10 too large.  We
	* will then check to see that if  x  is	larger	 than  one
	* and if so multiply x by 1/10.
	*
	* We will  use	a  low	precision  (fixed  point 24 bit) estimate
	* of of 1 + log base 10 of x.  We  have	approximately  .mm
	* * 2^hhll  on	the  top of the stack where m, h, and l represent
	* hex digits,  mm  represents  the  high  2  hex  digits  of  the
	* mantissa, hh	represents the high 2 hex digits of the exponent,
	* and ll represents the low 2 hex digits of the exponent.   Since
	* .mm is  a  truncated	representation	of the mantissa, using it
	* in this  monotonically  increasing   polynomial   approximation
	* of the  logarithm  will  naturally  give  a  low result.  Let's
	* derive a formula for a lower	bound  r  on  1	+  log(x):
	*
	*      .4D104D42H < log(2)=.30102999...(base 10) < .4D104D43H
	*	  .9A20H < log(4)=.60205999...(base 10) < .9A21H
	*
	*  1/2 <= .mm < 1
	*  ==>	log(.mm) >= .mm * log(4) - log(4)
	*
	* Substituting in  truncated  hex  constants in the formula above
	* gives r = 1 + .4D104DH * hhll.  + .9AH *  .mm	-  .9A21H.   Now
	* multiplication of  hex  digits  5  and 6 of log(2) by ll has an
	* insignificant effect on the first 24	bits  of  the  result  so
	* it will  not	be  calculated.	 This  gives  the expression r =
	* 1 + .4D10H * hhll.  +	.4DH  *  .hh  +  .9A  *  .mm  -  .9A21H.
	* Finally we  must  add	terms to our formula to subtract out the
	* effect of the exponent bias.	We obtain the following	formula:
	*
	*			(implied decimal point)
	*   <				  >.<				   >
	*   |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1|0|0|0|0|0|0|0|0|0|0|
	*   |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
	* + <		  1		  >
	* + <			    .4D10H * hhll.			   >
	* +				    <	    .00004DH * hh00.	   >
	* +				    <	       .9AH * .mm	   >
	* -				    <		 .9A21H 	   >
	* - <			    .4D10H * 3FFEH			   >
	* -				    <	    .00004DH * 3F00H	   >
	*
	*  ==>	r = .4D10H * hhll. + .4DH * .hh + .9AH * .mm - 1343.12F4H
	*
	* The difference  between  the	lower bound r and the upper bound
	* s is calculated as follows:
	*
	*  .937EH < 1/ln(10)-log(1/ln(4))=.57614993...(base 10) < .937FH
	*
	*  1/2 <= .mm < 1
	*  ==>	log(.mm) <= .mm * log(4) - [1/ln(10) - log(1/ln(4))]
	*
	* so tenatively	s  =  r  +  log(4)  - [1/ln(10) - log(1/ln(4))],
	* but we must also add in terms to ensure we will have	an  upper
	* bound even  after  the  truncation  of various values.  Because
	* log(2) * hh00.  is truncated	to  .4D104DH  *	hh00.	we  must
	* add .0043H,  because	log(2)	*  ll.	is truncated to .4D10H *
	* ll.  we  must	add  .0005H,  because  <mantissa>  *  log(4)  is
	* truncated to .mm * .9AH we must add .009AH and .0021H.
	*
	* Thus s = r - .937EH + .9A21H + .0043H + .0005H + .009AH + .0021H
	*	= r + .07A6H
	*  ==>	s = .4D10H * hhll. + .4DH * .hh + .9AH * .mm - 1343.0B4EH
	*
	* r is	equal  to  1  +	log(x) more than (10000H - 7A6H) /
	* 10000H = 97% of the time.
	*
	* In the above formula, a u_long is use to accomodate r, and
	* there is an implied decimal point in the middle.
	*/

	hh = expn >> 8;
	ll = expn & (u_short)0xff;
	mm = (u_short) (manhi >> 24);
	r = (s_long)log2hi*(s_long)expn + log2lo*hh + log4hi*mm - c;
	ir = (s_short)(r >> 16);

       /*
	*
	* We stated that we wanted to normalize x so that
	*
	*  .1 <= x < 1
	*
	* This was	a  slight  oversimplification.	 Actually  we  want a
	* number which when rounded to 16 significant digits  is  in  the
	* desired range.   To  do  this we must normalize x so that
	*
	*  .1 - 5*10^(-18) <= x < 1 - 5*10^(-17)
	*
	* and then round.
	*
	* If we  had f = INT(1+log(x)) we could multiply by 10^(-f)
	* to get x into the desired range.	We do  not  quite  have
	* f but  we  do  have  INT(r)  from  the last step which is equal
	* to f 97% of the time and 1 less than f the rest  of  the	time.
	* We can  multiply	by  10^-[INT(r)] and if the result is greater
	* than 1 - 5*10^(-17) we can then multiply by 1/10.   This	final
	* result will lie in the proper range.
	*/

	/* convert _ULDOUBLE to _ULDBL12) */
	*U_EXP_12(&ld12) = expn;
	*UL_MANHI_12(&ld12) = manhi;
	*UL_MANLO_12(&ld12) = manlo;
	*U_XT_12(&ld12) = 0;

	/* multiply by 10^(-ir) */
	__multtenpow12(&ld12,-ir,1);

	/* if ld12 >= 1.0 then divide by 10.0 */
	if (*U_EXP_12(&ld12) >= 0x3fff) {
	    ir++;
	    __ld12mul(&ld12,&ld12_one_tenth);
	}

	fos->exp = ir;
	if (output_flags & SO_FFORMAT){
	    /* 'f' format, add exponent to ndigits */
	    ndigits += ir;
	    if (ndigits <= 0) {
		/* return 0 */
		PUT_ZERO_FOS(fos);
		return 1;
	    }
	}
	if (ndigits > MAX_MAN_DIGITS)
	    ndigits = MAX_MAN_DIGITS;

	ub_exp = *U_EXP_12(&ld12) - 0x3ffe; /* unbias exponent */
	*U_EXP_12(&ld12) = 0;

	/*
	 * Now the mantissa has to be converted to fixed point.
	 * Then we will use the MSB of ld12 for generating
	 * the decimal digits. The next 11 bytes will hold
	 * the mantissa (after it has been converted to
	 * fixed point).
	 */

	for (i=0;i<8;i++)
	    __shl_12(&ld12); /* make space for an extra byte,
			      in case we shift right later */
	if (ub_exp < 0) {
	    int shift_count = (-ub_exp) & 0xff;
	    for (;shift_count>0;shift_count--)
		__shr_12(&ld12);
	}

	p = fos->man;
	for(digcount=ndigits+1;digcount>0;digcount--) {
	    tmp12 = ld12;
	    __shl_12(&ld12);
	    __shl_12(&ld12);
	    __add_12(&ld12,&tmp12);
	    __shl_12(&ld12);	/* ld12 *= 10 */

	    /* Now we have the first decimal digit in the msbyte of exponent */
	    *p++ = (char) (*UCHAR_12(&ld12,11) + '0');
	    *UCHAR_12(&ld12,11) = 0;
	}

	round = *(--p);
	p--; /* p points now to the last character of the string
		   excluding the rounding digit */
	if (round >= '5') {
	    /* look for a non-9 digit starting from the end of string */
	    for (;p>=fos->man && *p=='9';p--) {
		*p = '0';
	    }
	    if (p < fos->man){
		p++;
		fos->exp ++;
	    }
	    (*p)++;
	}
	else {
	    /* remove zeros */
	    for (;p>=fos->man && *p=='0';p--);
	    if (p < fos->man) {
		/* return 0 */
		PUT_ZERO_FOS(fos);
		return 1;
	    }
	}
	fos->ManLen = (char) (p - fos->man + 1);
	fos->man[fos->ManLen] = '\0';
    }
    return retval;
}
