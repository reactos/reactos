/***
*wcspbrk.c - scans wide character string for a character from control string
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines wcspbrk()- returns pointer to the first wide-character in
*       a wide-character string in the control string.
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *wcspbrk(string, control) - scans string for a character from control
*
*Purpose:
*       Returns pointer to the first wide-character in
*       a wide-character string in the control string.
*
*Entry:
*       wchar_t *string - string to search in
*       wchar_t *control - string containing characters to search for
*
*Exit:
*       returns a pointer to the first character from control found
*       in string.
*       returns NULL if string and control have no characters in common.
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcspbrk (
        const wchar_t * string,
        const wchar_t * control
        )
{
        wchar_t *wcset;

        /* 1st char in control string stops search */
        while (*string) {
            for (wcset = (wchar_t *) control; *wcset; wcset++) {
                if (*wcset == *string) {
                    return (wchar_t *) string;
                }
            }
            string++;
        }
        return NULL;
}

