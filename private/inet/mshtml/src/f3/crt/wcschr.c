/***
*wcschr.c - search a wchar_t string for a given wchar_t character
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wcschr() - search a wchar_t string for a wchar_t character
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *wcschr(string, c) - search a string for a wchar_t character
*
*Purpose:
*       Searches a wchar_t string for a given wchar_t character,
*       which may be the null character L'\0'.
*
*Entry:
*       wchar_t *string - wchar_t string to search in
*       wchar_t c - wchar_t character to search for
*
*Exit:
*       returns pointer to the first occurence of c in string
*       returns NULL if c does not occur in string
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcschr (
        const wchar_t * string,
        wchar_t ch
        )
{
        while (*string && *string != (wchar_t)ch)
                string++;

        if (*string == (wchar_t)ch)
                return((wchar_t *)string);
        return(NULL);
}

