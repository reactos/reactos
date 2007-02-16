/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/itow.c
 * PURPOSE:     converts a integer to wchar_t
 * PROGRAMER:
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa by Ariadne
 *              2000: derived from ./itoa.c by ea
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <precomp.h>

/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t* _itow(int value, wchar_t* string, int radix)
{
   return _ltow(value, string, radix);
}

/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t* _ltow(long value, wchar_t* string, int radix)
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

    if (str != NULL) {
	memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}

/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t* _ultow(unsigned long value, wchar_t* string, int radix)
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
