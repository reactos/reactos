#include <string.h>
#include <stdlib.h>
#include <windows.h>

/*
 * @implemented
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
char *
_i64toa(__int64 value, char *string, int radix)
{
    ULONGLONG  val;
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
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
char *
_ui64toa(unsigned __int64 value, char *string, int radix)
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


/*
 * @implemented
 */
char *
_itoa(int value, char *string, int radix)
{
  return _ltoa(value, string, radix);
}


/*
 * @implemented
 */
char *
_ltoa(long value, char *string, int radix)
{
    unsigned long val;
    int negative;
    char buffer[33];
    char *pos;
    int digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[32];
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

    memcpy(string, pos, &buffer[32] - pos + 1);
    return string;
}


/*
 * @implemented 
 *  copy it from wine 0.9.0 with small modifcations do check for NULL
 */
char *
_ultoa(unsigned long value, char *string, int radix)
{
    char buffer[33];
    char *pos;
    int digit;
    
    pos = &buffer[32];
    *pos = '\0';

    if (string == NULL)
    {
      return NULL;         
    }
    
    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[32] - pos + 1);
    
    return string;
}
