/***
*strncmp.c - compare first n characters of two strings
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strncmp() - compare first n characters of two strings
*       for ordinal order.
*
*******************************************************************************/

#include <string.h>

#ifdef _M_ARM
    #pragma function(strncmp)
#endif

/***
*int strncmp(first, last, count) - compare first count chars of strings
*
*Purpose:
*       Compares two strings for ordinal order.  The comparison stops
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

int __cdecl strncmp
(
    const char *first,
    const char *last,
    size_t      count
)
{
    size_t x = 0;

    if (!count)
    {
        return 0;
    }

    /*
     * This explicit guard needed to deal correctly with boundary
     * cases: strings shorter than 4 bytes and strings longer than
     * UINT_MAX-4 bytes .
     */
    if( count >= 4 )
    {
        /* unroll by four */
        for (; x < count-4; x+=4)
        {
            first+=4;
            last +=4;

            if (*(first-4) == 0 || *(first-4) != *(last-4))
            {
                return(*(unsigned char *)(first-4) - *(unsigned char *)(last-4));
            }

            if (*(first-3) == 0 || *(first-3) != *(last-3))
            {
                return(*(unsigned char *)(first-3) - *(unsigned char *)(last-3));
            }

            if (*(first-2) == 0 || *(first-2) != *(last-2))
            {
                return(*(unsigned char *)(first-2) - *(unsigned char *)(last-2));
            }

            if (*(first-1) == 0 || *(first-1) != *(last-1))
            {
                return(*(unsigned char *)(first-1) - *(unsigned char *)(last-1));
            }
        }
    }

    /* residual loop */
    for (; x < count; x++)
    {
        if (*first == 0 || *first != *last)
        {
            return(*(unsigned char *)first - *(unsigned char *)last);
        }
        first+=1;
        last+=1;
    }

    return 0;
}
