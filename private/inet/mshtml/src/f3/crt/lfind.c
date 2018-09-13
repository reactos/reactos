/***
*lfind.c - do a linear search
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _lfind() - do a linear search of an array.
*
*******************************************************************************/

#include <search.h>
#include <stddef.h>

/***
*char *_lfind(key, base, num, width, compare) - do a linear search
*
*Purpose:
*       Performs a linear search on the array, looking for the value key
*       in an array of num elements of width bytes in size.  Returns
*       a pointer to the array value if found, NULL if not found.
*
*Entry:
*       char *key - key to search for
*       char *base - base of array to search
*       unsigned *num - number of elements in array
*       int width - number of bytes in each array element
*       int (*compare)() - pointer to function that compares two
*               array values, returning 0 if they are equal and non-0
*               if they are different.  Two pointers to array elements
*               are passed to this function.
*
*Exit:
*       if key found:
*               returns pointer to array element
*       if key not found:
*               returns NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _lfind (
        const void *key,
        const void *base,
        unsigned int *num,
        unsigned int width,
        int (__cdecl *compare)(const void *, const void *)
        )
{
        unsigned int place = 0;
        while (place < *num )
                if (!(*compare)(key,base))
                        return( (void *)base );
                else
                {
                        base = (char *)base + width;
                        place++;
                }
        return( NULL );
}
