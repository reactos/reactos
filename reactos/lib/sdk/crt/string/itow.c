/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/itow.c
 * PURPOSE:     converts a integer to wchar_t
 * PROGRAMER:
 * UPDATE HISTORY:
 */

#include <precomp.h>

/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t *
_i64tow(__int64 value, wchar_t *string, int radix)
{
    ULONGLONG val;
    int negative;
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}


/*
 * @implemented
 */
wchar_t *
_ui64tow(unsigned __int64 value, wchar_t *string, int radix)
{
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}


/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t *
_itow(int value, wchar_t *string, int radix)
{
  return _ltow(value, string, radix);
}


/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t *
_ltow(long value, wchar_t *string, int radix)
{
    unsigned long val;
    int negative;
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}


/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t *
_ultow(unsigned long value, wchar_t *string, int radix)
{
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

    pos = &buffer[32];
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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}
