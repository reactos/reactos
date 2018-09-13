/***
*wcsncpy.c - copy at most n characters of wide-character string
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wcsncpy() - copy at most n characters of wchar_t string
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *wcsncpy(dest, source, count) - copy at most n wide characters
*
*Purpose:
*       Copies count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is padded
*       with null characters to length count (wide-characters).
*
*
*Entry:
*       wchar_t *dest - pointer to destination
*       wchar_t *source - source string for copy
*       size_t count - max number of characters to copy
*
*Exit:
*       returns dest
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcsncpy (
        wchar_t * dest,
        const wchar_t * source,
        size_t count
        )
{
        wchar_t *start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = L'\0';

        return(start);
}

