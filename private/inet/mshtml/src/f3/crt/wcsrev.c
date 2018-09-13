/***
*wcsrev.c - reverse a wide-character string in place
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wcsrev() - reverse a wchar_t string in place (not including
*       L'\0' character)
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *_wcsrev(string) - reverse a wide-character string in place
*
*Purpose:
*       Reverses the order of characters in the string.  The terminating
*       null character remains in place (wide-characters).
*
*Entry:
*       wchar_t *string - string to reverse
*
*Exit:
*       returns string - now with reversed characters
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _wcsrev (
        wchar_t * string
        )
{
        wchar_t *start = string;
        wchar_t *left = string;
        wchar_t ch;

        while (*string++)                 /* find end of string */
                ;
        string -= 2;

        while (left < string)
        {
                ch = *left;
                *left++ = *string;
                *string-- = ch;
        }

        return(start);
}

