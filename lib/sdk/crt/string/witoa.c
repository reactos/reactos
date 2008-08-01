/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/itoa.c
 * PURPOSE:     converts a integer to ascii
 * PROGRAMER:
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa by Ariadne
 *              2006 : replace all api in this file to wine cvs 2006-05-21
 */
/*  */
#include <precomp.h>

/*
 * @implemented
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
char* _i64toa(__int64 value, char* string, int radix)
{
    ULONGLONG val;
    int negative;
    char buffer[65];
    char *pos;
    int digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    memcpy(string, pos, &buffer[64] - pos + 1);
    return string;
}


/*
 * @implemented
 */
char* _ui64toa(unsigned __int64 value, char* string, int radix)
{
    char buffer[65];
    char *pos;
    int digit;

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[64] - pos + 1);
    return string;
}
