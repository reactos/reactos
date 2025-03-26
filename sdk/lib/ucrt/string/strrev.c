/***
*strrev.c - reverse a string in place
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _strrev() - reverse a string in place (not including
*       '\0' character)
*
*******************************************************************************/

#include <string.h>

/***
*char *_strrev(string) - reverse a string in place
*
*Purpose:
*       Reverses the order of characters in the string.  The terminating
*       null character remains in place.
*
*Entry:
*       char *string - string to reverse
*
*Exit:
*       returns string - now with reversed characters
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _strrev (
        char * string
        )
{
        char *start = string;
        char *left = string;
        char ch;

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
