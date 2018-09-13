/***
*strtoli.c - Contains C runtimes strtol and strtoul
*
*       Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*       Copyright (c) 1989-1993, Digital Equipment Corporation. All rights reserved.
*
*Purpose:
*       strtoli - convert ascii string to long signed large integer
*       strtouli - convert ascii string to long unsigned large integer
*
*Revision History:
*       04-28-93  MBH   Module created, based on strtol.c
*
*******************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <shellapi.h>




// #include <cruntime.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>


/***
*strtoli, strtouli(nptr,endptr,ibase) - Convert ascii string to long un/signed
*       large integer.
*
*Purpose:
*       Convert an ascii string to a long 64-bit value.  The base
*       used for the caculations is supplied by the caller.  The base
*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
*       ascii string must be examined to determine the base of the
*       number:
*               (a) First char = '0', second char = 'x' or 'X',
*                   use base 16.
*               (b) First char = '0', use base 8
*               (c) First char in range '1' - '9', use base 10.
*
*       If the 'endptr' value is non-NULL, then strtoli/strtouli places
*       a pointer to the terminating character in this value.
*       See ANSI standard for details
*
*Entry:
*       nptr == NEAR/FAR pointer to the start of string.
*       endptr == NEAR/FAR pointer to the end of the string.
*       ibase == integer base to use for the calculations.
*
*       string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*       Good return:
*               result
*
*       Overflow return:
*               strtoli -- LONG_MAX or LONG_MIN in HighPart
*               strtouli -- ULONG_MAX in HighPart
*               strtoli/strtouli -- errno == ERANGE
*
*       No digits or bad base return:
*               0
*               endptr = nptr*
*
*Exceptions:
*       None.
*******************************************************************************/

/* flag values */
#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static
LARGE_INTEGER
// MBH - if this is a library, this should be CRTAPI3
/*_CRTAPI2*/ strtoxl (
        const char *nptr,
        const char **endptr,
        int ibase,
        int flags
        )
{
    const char *p;
    char c;

    LARGE_INTEGER number;        // being computed
    unsigned long  digval;        // each digit in turn
    LARGE_INTEGER maxval;        // value more than which --> overflow
    unsigned long  remainder;     // MAXLONGLONG=maxval/radix + remainder

    p = nptr;                       /* p is our scanning pointer */
    number.QuadPart = 0;

    c = *p++;                       /* read char */
    while ( isspace((int)(unsigned char)c) ) {
        c = *p++;               /* skip whitespace */
    }

    if (c == '-') {
        flags |= FL_NEG;        /* remember minus sign */
        c = *p++;
    } else if (c == '+') {
        c = *p++;               /* skip sign */
    }

    if (ibase < 0 || ibase == 1 || ibase > 36) {
        /* bad base! */
        if (endptr) {
            /* store beginning of string in endptr */
            *endptr = nptr;
        }
        number.QuadPart = 0;
        return number;
    }
    else if (ibase == 0) {
    /* determine base free-lance, based on first two chars of
        string */
        if (c != '0') {
            ibase = 10;
        } else if (*p == 'x' || *p == 'X') {
            ibase = 16;
        } else {
            ibase = 8;
        }
    }

    if (ibase == 16) {
        /* we might have 0x in front of number; remove if there */
        if (c == '0' && (*p == 'x' || *p == 'X')) {
            ++p;
            c = *p++;       /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval.QuadPart = MAXLONGLONG / ibase;
    remainder = (unsigned long)(MAXLONGLONG - maxval.QuadPart);

    for (;;) {      /* exit in middle of loop */
        /* convert c to value */
        if ( isdigit((int)(unsigned char)c) ) {
            digval = c - '0';
        } else if ( isalpha((int)(unsigned char)c) ) {
            digval = toupper(c) - 'A' + 10;
        } else {
            break;
        }
        if (digval >= (unsigned)ibase) {
            break;          /* exit loop if bad digit found */
        }

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        // we now need to compute number = number * base + digval,
        // but we need to know if overflow occured. For the computation
        // to succeed the following must be true.
        //      maxval.QuadPart * ibase + remainder >= number.QuadPart * ibase + digval
        //      (maxval.QuadPart - number.QuadPart) * ibase + (remainder - digval) >= 0
        //
        // Given:
        //      0 <= remainder < ibase
        //      0 <= digval < ibase
        //
        // won't overflow IF:
        //      (maxval.QuadPart - number.QuadPart) == 0 && (remainder - digval) >= 0
        //      (maxval.QuadPart - number.QuadPart) > 0


        if ( ( (maxval.QuadPart - number.QuadPart) == 0 && (remainder - digval) >= 0 )
            || ( (maxval.QuadPart - number.QuadPart) > 0 ) ) {
            //
            // we won't overflow, go ahead and multiply
            //
            number.QuadPart = (number.QuadPart * ibase) + digval;
        } else {
            //
            // we would have overflowed -- set the overflow flag
            //
            flags |= FL_OVERFLOW;
        }

        c = *p++;               /* read next digit */
    }

    --p;                            /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT)) {
    /* no number there; return 0 and point to beginning of
        string */
        if (endptr) {
            /* store beginning of string in endptr later on */
            p = nptr;
        }
        /* return 0 */
        number.QuadPart = 0;
    }
    else if ((flags & FL_OVERFLOW) || (!(flags & FL_UNSIGNED) && (number.QuadPart < 0))) {

        /* overflow occurred or signed overflow occurred */
        errno = ERANGE;
        if (flags & FL_UNSIGNED) {
            number.QuadPart = (ULONG64)(-1);
        } else {
            /* set error code, will be negated if necc. */
            number.QuadPart = -1;
        }
    }

    if (endptr != NULL) {
        /* store pointer to char that stopped the scan */
        *endptr = p;
    }

    if (flags & FL_NEG) {
        /* negate result if there was a neg sign */
        number.QuadPart = number.QuadPart * -1;
    }

    return number;                  /* done. */
}

LARGE_INTEGER
strtoli (
        const char *nptr,
        char **endptr,
        int ibase
        )
{
        return strtoxl(nptr, endptr, ibase, 0);
}

ULARGE_INTEGER
strtouli (
        const char *nptr,
        char **endptr,
        int ibase
        )
{
        ULARGE_INTEGER uli;
        LARGE_INTEGER li;

        li = strtoxl(nptr, endptr, ibase, FL_UNSIGNED);
        uli.LowPart = li.LowPart;
        uli.HighPart = li.HighPart;
        return uli;
}
