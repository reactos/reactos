/***
*atox.c - atoi and atol conversion
*
*       Copyright (c) 1989-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a character string into an int or long.
*
*******************************************************************************/

#include "headers.hxx"

#ifndef X_STDLIB_H_
#define X_STDLIB_H_
#include <stdlib.h>
#endif

#ifndef X_CTYPE_H_
#define X_CTYPE_H_
#include <ctype.h>
#endif

/***
*long atol(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return long int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

long __cdecl atol(
        const char *nptr
        )
{
        int c;              /* current char */
        long total;         /* current total */
        int sign;           /* if '-', then negative, otherwise positive */

        /* skip whitespace */
        while ( isspace((int)(unsigned char)*nptr) )
            ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;           /* save sign indication */
        if (c == '-' || c == '+')
            c = (int)(unsigned char)*nptr++;    /* skip sign */

        total = 0;

        while (isdigit(c)) {
            total = 10 * total + (c - '0');     /* accumulate digit */
            c = (int)(unsigned char)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}


/***
*int atoi(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.  Because of this, we can just use
*       atol().
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

int __cdecl atoi(
        const char *nptr
        )
{
        return (int)atol(nptr);
}

#ifndef _NO_INT64

extern "C"
__int64 __cdecl _atoi64(
        const char *nptr
        )
{
        int c;              /* current char */
        __int64 total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */

        /* skip whitespace */
        while ( isspace((int)(unsigned char)*nptr) )
            ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;           /* save sign indication */
        if (c == '-' || c == '+')
            c = (int)(unsigned char)*nptr++;    /* skip sign */

        total = 0;

        while (isdigit(c)) {
            total = 10 * total + (c - '0');     /* accumulate digit */
            c = (int)(unsigned char)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}

#endif /* _NO_INT64 */
