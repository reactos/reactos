/*** 
*cvt.c - C floating-point output conversions
*
*   Copyright (c) 1987-89, Microsoft Corporation
*
*Purpose:
*   contains routines for performing %e, %f, and %g output conversions
*   for printf, etc.
*
*   routines include _cfltcvt(), _cftoe(), _cftof(), _cftog(), _fassign(),
*		     _positive(), _cropzeros(), _forcdecpt()
*
*Revision History:
*   04-18-84  RN  author
*   01-15-87  BCM corrected processing of %g formats (to handle precision
*		    as the maximum number of signifcant digits displayed)
*   03-24-87  BCM Evaluation Issues: (fccvt.obj version for ?LIBFA)
*	      ------------------
*		  SDS - no problem
*		  GD/TS :
*			char g_fmt = 0; 		(local,   initialized)
*			int g_magnitude =0;		(local,   initialized)
*			char g_round_expansion = 0;	(local,   initialized)
*			STRFLT g_pflt;				(local, uninitialized)
*		  other INIT :
*		ALTMATH __fpmath() initialization (perhaps)
*		  TERM - nothing
*   10-22-87  BCM   changes for OS/2 Support Library -
*				including elimination of g_... static variables
*				in favor of stack-based variables & function arguments
*				under MTHREAD switch;  changed interfaces to _cfto? routines
*   01-15-88  BCM   remove IBMC20 switches; use only memmove, not memcpy;
*				use just MTHREAD switch, not SS_NEQ_DGROUP
*   06-13-88  WAJ   Fixed %.1g processing for small x
*   08-02-88  WAJ   Made changes to _fassign() for new input().
*   03-09-89  WAJ   Added some long double support.
*   06-05-89  WAJ   Made changes for C6. ULDOUBLE => long double
*   06-12-89  WAJ   Renamed this file from cvtn.c to cvt.c
*   11-02-89  WAJ   Removed register.h
*   06-28-90  WAJ   Removed fars.
*   11-15-90  WAJ   Added _cdecl where needed. Also "pascal" => "_pascal".
*   09-12-91  GDP   _cdecl=>_CALLTYPE2 _pascal=>_CALLTYPE5 near=>_NEAR
*   04-30-92  GDP   Removed floating point code. Instead used S/W routines
*		    (_atodbl, _atoflt _atoldbl), so that to avoid generation
*		    of IEEE exceptions from the lib code.
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/



#include <ctype.h>
#include <string.h>
#include <math.h>
#include "mathsup.h"


//NOTENOTE the following takes the place of the isdigit() macro
//       which does not work for a yet to be determined reason
#define ISADIGIT(x) ((x)>='0' && (x)<='9' )




#ifdef i386
// Uncomment this for enabling 10-byte long double string conversions
#define LONG_DOUBLE
#endif


/* this routine resides in the crt32 tree */
extern void _fptostr(char *buf, int digits, STRFLT pflt);


static void _shift( char *s, int dist );

#ifdef	MTHREAD
    static char * _cftoe2( char * buf, int ndec, int caps, STRFLT pflt, char g_fmt );
    static char * _cftof2( char * buf, int ndec, STRFLT pflt, char g_fmt );

#else	/* not MTHREAD */
    static char * _cftoe_g( double * pvalue, char * buf, int ndec, int caps );
    static char * _cftof_g( double * pvalue, char * buf, int ndec );
#endif	/* not MTHREAD */

/*** 
*_forcdecpt(buffer) - force a decimal point in floating-point output
*Purpose:
*  	force a decimal point in floating point output. we are only called if '#'
*  	flag is given and precision is 0; so we know the number has no '.'. insert
*  	the '.' and move everybody else back one position, until '\0' seen
*
* 	side effects: futzes around with the buffer, trying to insert a '.' 
*	after the initial string of digits. the first char can usually be 
*   skipped since it will be a digit or a '-'.  but in the 0-precision case, 
* 	the number could start with 'e' or 'E', so we'd want the '.' before the 
*	exponent in that case.
*
*Entry:
*	buffer = (char *) pointer to buffer to modify
*
*Exit:
*	returns : (void)
*
*Exceptions:
*******************************************************************************/

void _forcdecpt( char * buffer )
{
char	holdchar;
char	nextchar;

    if (tolower(*buffer) != 'e'){
	do {
	    buffer++;
	    }
	while (ISADIGIT(*buffer));
	}

    holdchar = *buffer;
    *buffer++ = '.';

    do	{
	nextchar = *buffer;
	*buffer = holdchar;
	holdchar = nextchar;
	}

    while(*buffer++);
}


/*** 
*_cropzeros(buffer) - removes trailing zeros from floating-point output
*Purpose:
*	removes trailing zeros (after the '.') from floating-point output;
*	called only when we're doing %g format, there's no '#' flag, and 
*	precision is non-zero.  plays around with the buffer, looking for
*	trailing zeros.  when we find them, then we move everbody else forward
*	so they overlay the zeros.  if we eliminate the entire fraction part,
*	then we overlay the decimal point ('.'), too.	
*
* 	side effects: changes the buffer from
*   	[-] digit [digit...] [ . [digits...] [0...] ] [(exponent part)]
*	to
*		[-] digit [digit...] [ . digit [digits...] ] [(exponent part)]
*	or
*   	[-] digit [digit...] [(exponent part)]
*
*Entry:
*	buffer = (char *) pointer to buffer to modify
*
*Exit:
*	returns : (void)
*
*Exceptions:
*******************************************************************************/

void _cropzeros( char * buf )
{
char	*stop;

    while (*buf && *buf != '.')
	buf++;

    if (*buf++) {
	while (*buf && *buf != 'e' && *buf != 'E')
	    buf++;

	stop = buf--;

	while (*buf == '0')
	    buf--;

	if (*buf == '.')
	    buf--;

	while( (*++buf = *stop++) != '\0' );
	}
}


int _positive( double * arg )
{
    return( (*arg >= 0.0) );
}


void  _fassign( int flag, char * argument, char * number )
{

/* flag should normally be 2 for long double and 1 for double
 * The 32bit input.c sets 3 for long double and 2 for double
 * Until this is fixed, decrement flag before use
 */

    --flag;

#ifdef	LONG_DOUBLE

    switch( flag ){
	case 2:
	    _atoldbl( (_ULDOUBLE *)argument, number );
	    break;

	case 1:
	    _atodbl( (UDOUBLE *)argument, number );
	    break;

	default:
	    _atoflt( (FLOAT *)argument, number );
	}

#else	/* not LONG_DOUBLE */
    if (flag)
	_atodbl( (UDOUBLE *)argument, number );
    else
	_atoflt( (FLOAT *)argument, number );
#endif	/* not LONG_DOUBLE */
}


#ifndef MTHREAD
    static char   g_fmt = 0;
    static int	  g_magnitude = 0;
    static char   g_round_expansion = 0;
    static STRFLT g_pflt;
#endif


/*
 *  Function name:  _cftoe
 *
 *  Arguments:	    pvalue -  double * pointer
 *		    buf    -  char * pointer
 *		    ndec   -  int
 *		    caps   -  int
 *
 *  Description:    _cftoe converts the double pointed to by pvalue to a null
 *		    terminated string of ASCII digits in the c language
 *		    printf %e format, nad returns a pointer to the result.
 *		    This format has the form [-]d.ddde(+/-)ddd, where there
 *		    will be ndec digits following the decimal point.  If
 *		    ndec <= 0, no decimal point will appear.  The low order
 *		    digit is rounded.  If caps is nonzero then the exponent
 *		    will appear as E(+/-)ddd.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *		    of CVTBUFSIZE (defined in cvt.h) and the routines will
 *		    not write over this size.
 *
 *  Copyright:	    written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *		    copyright (c) Microsoft Corp. 1983
 *
 *  History:
 *
 */

#ifdef MTHREAD
    static char * _cftoe2( char * buf, int ndec, int caps, STRFLT pflt, char g_fmt )
#else
    char * _cftoe( double * pvalue, char * buf, int ndec, int caps )
#endif
{
#ifndef MTHREAD
    STRFLT pflt;
#endif

char	*p;
int	exp;

#ifdef MTHREAD
    int g_magnitude = pflt->decpt - 1;
#endif

    /* first convert the value */

    /* place the output in the buffer and round.  Leave space in the buffer
     * for the '-' sign (if any) and the decimal point (if any)
     */

    if (g_fmt) {
#ifndef MTHREAD
	pflt = g_pflt;
#endif
	/* shift it right one place if nec. for '.' */

	p = buf + (pflt->sign == '-');
	_shift(p, (ndec > 0));
		}
#ifndef MTHREAD
    else {


#ifdef MIPS

          UDOUBLE dtemp;
          dtemp = *pvalue;
 	
          pflt = _fltout(dtemp);
	

#else

          UDOUBLE dtemp;
          dtemp.x = *pvalue;				//i386 case
 	
          pflt = _fltout(dtemp);
	


#endif

    _fptostr(buf + (pflt->sign == '-') + (ndec > 0), ndec + 1, pflt);
	}
#endif


    /* now fix the number up to be in e format */

    p = buf;

    /* put in negative sign if needed */

    if (pflt->sign == '-')
	*p++ = '-';

    /* put in decimal point if needed.	Copy the first digit to the place
     * left for it and put the decimal point in its place
     */

    if (ndec > 0) {
	*p = *(p+1);
	*(++p) = '.';
	}

    /* find the end of the string and attach the exponent field */

    p = strcpy(p+ndec+(!g_fmt), "e+000");

    /* adjust exponent indicator according to caps flag and increment
     * pointer to point to exponent sign
     */

    if (caps)
	*p = 'E';

    p++;

    /* if mantissa is zero, then the number is 0 and we are done; otherwise
     * adjust the exponent sign (if necessary) and value.
     */

    if (*pflt->mantissa != '0') {

	/* check to see if exponent is negative; if so adjust exponent sign and
	 * exponent value.
	 */

	if( (exp = pflt->decpt - 1) < 0 ) {
	    exp = -exp;
	    *p = '-';
	    }

	p++;

	if (exp >= 100) {
	    *p += (char)(exp / 100);
	    exp %= 100;
	    }
	p++;

	if (exp >= 10) {
	    *p += (char)(exp / 10);
	    exp %= 10;
	    }

	*++p += (char)exp;
	}

    return(buf);
}


#ifdef MTHREAD

char * _cftoe( double * pvalue, char * buf, int ndec, int caps )
{
struct _strflt retstrflt;
char  resstr[21];
STRFLT pflt = &retstrflt;

    _fltout2(*pvalue, (struct _strflt *)&retstrflt,
	      (char *)resstr);
    _fptostr(buf + (pflt->sign == '-') + (ndec > 0), ndec + 1, pflt);
    _cftoe2(buf, ndec, caps, pflt, /* g_fmt = */ 0);

    return( buf );
}

#else	/* not MTHREAD */

static char * _cftoe_g( double * pvalue, char * buf, int ndec, int caps )
{
    char *res;
    g_fmt = 1;
    res = _cftoe(pvalue, buf, ndec, caps);
    g_fmt = 0;
    return (res);
}

#endif	/* not MTHREAD */


#ifdef MTHREAD
static char * _cftof2( char * buf, int ndec, STRFLT pflt, char g_fmt )

#else
char * _cftof( double * pvalue, char * buf, int ndec )
#endif

{
#ifndef MTHREAD
STRFLT pflt;
#endif

char	*p;
char	addzero = 0;

#ifdef MTHREAD
int	g_magnitude = pflt->decpt - 1;
#endif


    /* first convert the value */

    /* place the output in the users buffer and round.	Save space for
     * the minus sign now if it will be needed
     */

    if (g_fmt) {
#ifndef MTHREAD
	pflt = g_pflt;
#endif

	p = buf + (pflt->sign == '-');
	if (g_magnitude == ndec) {
	    char *q = p + g_magnitude;
	    *q++ = '0';
	    *q = '\0';
	    /* allows for extra place-holding '0' in the exponent == precision
	     * case of the g format
	     */
	    }
	}
#ifndef MTHREAD
    else {


#ifdef MIPS

          UDOUBLE dtemp;
          dtemp = *pvalue;
 	
          pflt = _fltout(dtemp);
	

#else

          UDOUBLE dtemp;
          dtemp.x = *pvalue;				//i386 case
 	
          pflt = _fltout(dtemp);
	


#endif

	_fptostr(buf+(pflt->sign == '-'), ndec + pflt->decpt, pflt);
	}
#endif


    /* now fix up the number to be in the correct f format */

    p = buf;

    /* put in negative sign, if necessary */

    if (pflt->sign == '-')
	*p++ = '-';

    /* insert leading 0 for purely fractional values and position ourselves
     * at the correct spot for inserting the decimal point
     */

    if (pflt->decpt <= 0) {
	_shift(p, 1);
	*p++ = '0';
	}
    else
	p += pflt->decpt;

	/* put in decimal point if required and any zero padding needed */

    if (ndec > 0) {
	_shift(p, 1);
	*p++ = '.';

	/* if the value is less than 1 then we may need to put 0's out in
	 * front of the first non-zero digit of the mantissa
	 */

	if (pflt->decpt < 0) {
	    if( g_fmt )
		ndec = -pflt->decpt;
	    else
		ndec = (ndec < -pflt->decpt ) ? ndec : -pflt->decpt;
	    _shift(p, ndec);
	    memset( p, '0', ndec);
	    }
	}

    return( buf);
}


/*
 *  Function name:  _cftof
 *
 *  Arguments:	    value  -  double * pointer
 *		    buf    -  char * pointer
 *		    ndec   -  int
 *
 *  Description:    _cftof converts the double pointed to by pvalue to a null
 *		    terminated string of ASCII digits in the c language
 *		    printf %f format, and returns a pointer to the result.
 *		    This format has the form [-]ddddd.ddddd, where there will
 *		    be ndec digits following the decimal point.  If ndec <= 0,
 *		    no decimal point will appear.  The low order digit is
 *		    rounded.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *		    of CVTBUFSIZE (defined in cvt.h) and the routines will
 *		    not write over this size.
 *
 *  Copyright:	    written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *		    copyright (c) Microsoft Corp. 1983
 *
 *  History:
 *
 */

#ifdef MTHREAD

char * _cftof( double * pvalue, char * buf, int ndec )
{
    struct _strflt retstrflt;
    char  resstr[21];
    STRFLT pflt = &retstrflt;
    _fltout2(*pvalue, (struct _strflt *) &retstrflt,
				      (char *) resstr);
    _fptostr(buf+(pflt->sign == '-'), ndec + pflt->decpt, pflt);
    _cftof2(buf, ndec, pflt, /* g_fmt = */ 0);

    return( buf );
}

#else	/* not MTHREAD */


static char * _cftof_g( double * pvalue, char * buf, int ndec )
{
    char *res;
    g_fmt = 1;
    res = _cftof(pvalue, buf, ndec);
    g_fmt = 0;
    return (res);
}

#endif	/* not MTHREAD */

/*
 *  Function name:  _cftog
 *
 *  Arguments:	    value  -  double * pointer
 *		    buf    -  char * pointer
 *		    ndec   -  int
 *
 *  Description:    _cftog converts the double pointed to by pvalue to a null
 *		    terminated string of ASCII digits in the c language
 *		    printf %g format, and returns a pointer to the result.
 *		    The form used depends on the value converted.  The printf
 *		    %e form will be used if the magnitude of valude is less
 *		    than -4 or is greater than ndec, otherwise printf %f will
 *		    be used.  ndec always specifies the number of digits
 *		    following the decimal point.  The low order digit is
 *		    appropriately rounded.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *		    of CVTBUFSIZE (defined in cvt.h) and the routines will
 *		    not write over this size.
 *
 *  Copyright:	    written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *		    copyright (c) Microsoft Corp. 1983
 *
 *  History:
 *
 */

char * _cftog( double * pvalue, char * buf, int ndec, int caps )
{
char *p;

#ifdef MTHREAD
char g_round_expansion = 0;
STRFLT g_pflt;
int g_magnitude;
struct _strflt retstrflt;
char  resstr[21];

    /* first convert the number */

    g_pflt = &retstrflt;
    _fltout2(*pvalue, (struct _strflt *)&retstrflt,
		  (char *)resstr);

#else	/* not MTHREAD */

    /* first convert the number */


#ifdef MIPS

          UDOUBLE dtemp;
          dtemp = *pvalue;
 	
          g_pflt = _fltout(dtemp);
	

#else

          UDOUBLE dtemp;
          dtemp.x = *pvalue;				//i386 case
 	
          g_pflt = _fltout(dtemp);
	


#endif


#endif	/* not MTHREAD */

    g_magnitude = g_pflt->decpt - 1;
    p = buf + (g_pflt->sign == '-');

    _fptostr(p, ndec, g_pflt);
    g_round_expansion = (char)(g_magnitude < (g_pflt->decpt-1));


    /* compute the magnitude of value */

    g_magnitude = g_pflt->decpt - 1;

    /* convert value to the c language g format */

    if (g_magnitude < -4 || g_magnitude >= ndec){     /* use e format */
	/*  (g_round_expansion ==>
	 *  extra digit will be overwritten by 'e+xxx')
	 */

#ifdef MTHREAD
	return(_cftoe2(buf, ndec, caps, g_pflt, /* g_fmt = */ 1));
#else
	return(_cftoe_g(pvalue, buf, ndec, caps));
#endif

	}
    else {										     /* use f format */
	if (g_round_expansion) {
	    /* throw away extra final digit from expansion */
	    while (*p++);
	    *(p-2) = '\0';
	    }

#ifdef MTHREAD
	return(_cftof2(buf, ndec, g_pflt, /* g_fmt = */ 1));
#else
	return(_cftof_g(pvalue, buf, ndec));
#endif

	}
}

/*** 
*_cfltcvt(arg, buf, format, precision, caps) - convert floating-point output
*Purpose:
*
*Entry:
*	arg = (double *) pointer to double-precision floating-point number 
*	buf = (char *) pointer to buffer into which to put the converted
*				   ASCII form of the number
*	format = (int) 'e', 'f', or 'g'
*	precision = (int) giving number of decimal places for %e and %f formats,
*					  and giving maximum number of significant digits for
*					  %g format
*	caps = (int) flag indicating whether 'E' in exponent should be capatilized
*				 (for %E and %G formats only)
*	
*Exit:
*	returns : (void)
*
*Exceptions:
*******************************************************************************/
/*
 *  Function name:  _cfltcvt
 *
 *  Arguments:	    arg    -  double * pointer
 *		    buf    -  char * pointer
 *					format -  int
 *		    ndec   -  int
 *		    caps   -  int
 *
 *  Description:    _cfltcvt determines from the format, what routines to
 *		    call to generate the correct floating point format
 *
 *  Side Effects:   none
 *
 *	Author: 	   Dave Weil, Jan 12, 1985
 *
 *  Copyright:	   Copyright (C) Microsoft Corp. 1985
 */

void _cfltcvt( double * arg, char * buffer, int format, int precision, int caps )
{
    if (format == 'e' || format == 'E')
	_cftoe(arg, buffer, precision, caps);
    else if (format == 'f')
	_cftof(arg, buffer, precision);
    else
	_cftog(arg, buffer, precision, caps);
}

/*** 
*_shift(s, dist) - shift a null-terminated string in memory (internal routine)
*Purpose:
*	_shift is a helper routine that shifts a null-terminated string 
*	in memory, e.g., moves part of a buffer used for floating-point output
*
*	modifies memory locations (s+dist) through (s+dist+strlen(s))
*
*Entry:
*	s = (char *) pointer to string to move
*	dist = (int) distance to move the string to the right (if negative, to left)
*
*Exit:
*	returns : (void)
*
*Exceptions:
*******************************************************************************/

static void _shift( char *s, int dist )
{
    if( dist )
	memmove(s+dist, s, strlen(s)+1);
}
