/***
*wcsrchr.c - find last occurrence of wchar_t character in wide string
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wcsrchr() - find the last occurrence of a given character
*       in a string (wide-characters).
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *wcsrchr(string, ch) - find last occurrence of ch in wide string
*
*Purpose:
*       Finds the last occurrence of ch in string.  The terminating
*       null character is used as part of the search (wide-characters).
*
*Entry:
*       wchar_t *string - string to search in
*       wchar_t ch - character to search for
*
*Exit:
*       returns a pointer to the last occurrence of ch in the given
*       string
*       returns NULL if ch does not occurr in the string
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcsrchr (
        const wchar_t * string,
        wchar_t ch
        )
{
        wchar_t *start = (wchar_t *)string;

        while (*string++)                       /* find end of string */
                ;
                                                /* search towards front */
        while (--string != start && *string != (wchar_t)ch)
                ;

        if (*string == (wchar_t)ch)             /* wchar_t found ? */
                return( (wchar_t *)string );

        return(NULL);
}

