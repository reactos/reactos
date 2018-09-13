/***
*strchr.c - search a string for a given character
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strchr() - search a string for a character
*
*******************************************************************************/

#include <string.h>

/***
*char *strchr(string, c) - search a string for a character
*
*Purpose:
*       Searches a string for a given character, which may be the
*       null character '\0'.
*
*Entry:
*       char *string - string to search in
*       char c - character to search for
*
*Exit:
*       returns pointer to the first occurence of c in string
*       returns NULL if c does not occur in string
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl strchr (
        const char * string,
        int ch
        )
{
        while (*string && *string != (char)ch)
                string++;

        if (*string == (char)ch)
                return((char *)string);
        return(NULL);
}
