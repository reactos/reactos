/* $Id: itow.c,v 1.6 2003/07/11 21:58:09 royce Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdlib/itow.c
 * PURPOSE:     converts a integer to wchar_t
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              1995: Created
 *              1998: Added ltoa Boudewijn Dekker
 *              2000: derived from ./itoa.c by ea
 */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/errno.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
wchar_t* _itow(int value, wchar_t* string, int radix)
{
    wchar_t     tmp [33];
    wchar_t     * tp = tmp;
    int     i;
    unsigned int    v;
    int     sign;
    wchar_t     * sp;

    if (radix > 36 || radix <= 1)
    {
        __set_errno(EDOM);
        return 0;
    }

    sign = ((radix == 10) && (value < 0));
    if (sign) {
        v = -value;
    } else {
        v = (unsigned) value;
    }
    while (v || tp == tmp) {
        i = v % radix;
        v = v / radix;
        if (i < 10) {
            *tp++ = i+ (wchar_t) '0';
        } else {
            *tp++ = i + (wchar_t) 'a' - 10;
        }
    }

    if (string == 0) {
        string = (wchar_t*) malloc((tp-tmp) + (sign + 1) * sizeof(wchar_t));
    }
    sp = string;

    if (sign) {
        *sp++ = (wchar_t) '-';
    }
    while (tp > tmp) {
        *sp++ = *--tp;
    }
    *sp = (wchar_t) 0;
    return string;
}

/*
 * @implemented
 */
wchar_t* _ltow(long value, wchar_t* string, int radix)
{
    wchar_t tmp [33];
    wchar_t* tp = tmp;
    long int i;
    unsigned long int v;
    int sign;
    wchar_t* sp;

    if (radix > 36 || radix <= 1) {
        __set_errno(EDOM);
        return 0;
    }

    sign = ((radix == 10) && (value < 0));
    if (sign) {
        v = -value;
    } else {
        v = (unsigned long) value;
    }
    while (v || tp == tmp) {
        i = v % radix;
        v = v / radix;
        if (i < 10) {
            *tp++ = i + (wchar_t) '0';
        } else {
            *tp++ = i + (wchar_t) 'a' - 10;
        }
    }

    if (string == 0) {
        string = (wchar_t*) malloc((tp - tmp) + (sign + 1) * sizeof(wchar_t));
    }
    sp = string;

    if (sign) {
        *sp++ = (wchar_t) '-';
    }
    while (tp > tmp) {
        *sp++ = *--tp;
    }
    *sp = (wchar_t) 0;
    return string;
}

/*
 * @unimplemented
 */
wchar_t* _ultow(unsigned long value, wchar_t* string, int radix)
{
    wchar_t tmp [33];
    wchar_t* tp = tmp;
    long int i;
    unsigned long int v = value;
    wchar_t* sp;

    if (radix > 36 || radix <= 1) {
        __set_errno(EDOM);
        return 0;
    }
    while (v || tp == tmp) {
        i = v % radix;
        v = v / radix;
        if (i < 10) {
            *tp++ = i + (wchar_t) '0';
        } else {
            *tp++ = i + (wchar_t) 'a' - 10;
        }
    }

    if (string == 0) {
#ifdef _MSVCRT_LIB_    // TODO: check on difference?
        string = (wchar_t*)malloc(((tp-tmp)+1)*sizeof(wchar_t));
#else // TODO: FIXME: review which is correct
        string = (wchar_t*)malloc((tp - tmp) + sizeof(wchar_t));
#endif /*_MSVCRT_LIB_*/
    }
    sp = string;
 
    while (tp > tmp) {
        *sp++ = *--tp;
    }
    *sp = (wchar_t) 0;
    return string;
}
