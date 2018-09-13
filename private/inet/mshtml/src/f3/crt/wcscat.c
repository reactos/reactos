/***
*wcscat.c - contains wcscat() and wcscpy()
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       wcscat() appends one wchar_t string onto another.
*       wcscpy() copies one wchar_t string into another.
*
*       wcscat() concatenates (appends) a copy of the source string to the
*       end of the destination string, returning the destination string.
*       Strings are wide-character strings.
*
*       wcscpy() copies the source string to the spot pointed to be
*       the destination string, returning the destination string.
*       Strings are wide-character strings.
*
*******************************************************************************/


#include <string.h>

/***
*wchar_t *wcscat(dst, src) - concatenate (append) one wchar_t string to another
*
*Purpose:
*       Concatenates src onto the end of dest.  Assumes enough
*       space in dest.
*
*Entry:
*       wchar_t *dst - wchar_t string to which "src" is to be appended
*       const wchar_t *src - wchar_t string to be appended to the end of "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcscat (
        wchar_t * dst,
        const wchar_t * src
        )
{
        wchar_t * cp = dst;

        while( *cp )
                cp++;                   /* find end of dst */

        while( *cp++ = *src++ ) ;       /* Copy src to end of dst */

        return( dst );                  /* return dst */

}


/***
*wchar_t *wcscpy(dst, src) - copy one wchar_t string over another
*
*Purpose:
*       Copies the wchar_t string src into the spot specified by
*       dest; assumes enough room.
*
*Entry:
*       wchar_t * dst - wchar_t string over which "src" is to be copied
*       const wchar_t * src - wchar_t string to be copied over "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*******************************************************************************/

wchar_t * __cdecl wcscpy(wchar_t * dst, const wchar_t * src)
{
        wchar_t * cp = dst;

        while( *cp++ = *src++ )
                ;               /* Copy src over dst */

        return( dst );
}

