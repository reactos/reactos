/***
*strncmp.c - compare first n characters of two strings
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strncmp() - compare first n characters of two strings
*       for lexical order.
*
*******************************************************************************/

#include <string.h>

/***
*int strncmp(first, last, count) - compare first count chars of strings
*
*Purpose:
*       Compares two strings for lexical order.  The comparison stops
*       after: (1) a difference between the strings is found, (2) the end
*       of the strings is reached, or (3) count characters have been
*       compared.
*
*Entry:
*       char *first, *last - strings to compare
*       unsigned count - maximum number of characters to compare
*
*Exit:
*       returns <0 if first < last
*       returns  0 if first == last
*       returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl strncmp (
        const char * first,
        const char * last,
        size_t count
        )
{
        if (!count)
                return(0);

        count--;
        while (count && *first && (*first == *last)) {
                first++;
                last++;
                count--;
        }

        return( *(unsigned char *)first - *(unsigned char *)last );
}
