/***
*bsearch.c - do a binary search
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines bsearch() - do a binary search an an array
*
*******************************************************************************/

#include <stdlib.h>
#include <search.h>

/***
*char *bsearch() - do a binary search on an array
*
*Purpose:
*       Does a binary search of a sorted array for a key.
*
*Entry:
*       const char *key    - key to search for
*       const char *base   - base of sorted array to search
*       unsigned int num   - number of elements in array
*       unsigned int width - number of bytes per element
*       int (*compare)()   - pointer to function that compares two array
*               elements, returning neg when #1 < #2, pos when #1 > #2, and
*               0 when they are equal. Function is passed pointers to two
*               array elements.
*
*Exit:
*       if key is found:
*               returns pointer to occurrence of key in array
*       if key is not found:
*               returns NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl bsearch (
        const void *key,
        const void *base,
        size_t num,
        size_t width,
        int (__cdecl *compare)(const void *, const void *)
        )
{
        char *lo = (char *)base;
        char *hi = (char *)base + (num - 1) * width;
        char *mid;
        unsigned int half;
        int result;

        while (lo <= hi)
        {
            half = num / 2;
            if (half)
                {
                        mid = lo + (num & 1 ? half : (half - 1)) * width;
                        result = (*compare)(key,mid);
                        if (!result)
                                return(mid);
                        else if (result < 0)
                        {
                                hi = mid - width;
                                num = num & 1 ? half : half-1;
                        }
                        else    {
                                lo = mid + width;
                                num = half;
                        }
                }
                else if (num)
                        return((*compare)(key,lo) ? NULL : lo);
                else
                        break;
        }

        return(NULL);
}
