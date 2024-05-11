/***
*strncpy.c - copy at most n characters of string
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strncpy() - copy at most n characters of string
*
*******************************************************************************/

#include <string.h>

#if defined _M_ARM
    #pragma function(strncpy)
#endif

/***
*char *strncpy(dest, source, count) - copy at most n characters
*
*Purpose:
*       Copies count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is padded
*       with null characters to length count.
*
*
*Entry:
*       char *dest - pointer to destination
*       char *source - source string for copy
*       unsigned count - max number of characters to copy
*
*Exit:
*       returns dest
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strncpy (
        char * dest,
        const char * source,
        size_t count
        )
{
        char *start = dest;

        while (count && (*dest++ = *source++) != '\0')    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = '\0';

        return(start);
}
