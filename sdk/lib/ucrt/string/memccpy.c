/***
*memccpy.c - copy bytes until a character is found
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _memccpy() - copies bytes until a specifed character
*       is found, or a maximum number of characters have been copied.
*
*******************************************************************************/

#include <string.h>

/***
*char *_memccpy(dest, src, c, count) - copy bytes until character found
*
*Purpose:
*       Copies bytes from src to dest until count bytes have been
*       copied, or up to and including the character c, whichever
*       comes first.
*
*Entry:
*       void *dest - pointer to memory to receive copy
*       void *src  - source of bytes
*       int  c     - character to stop copy at
*       size_t count - max number of bytes to copy
*
*Exit:
*       returns pointer to byte immediately after c in dest
*       returns NULL if c was never found
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _memccpy (
        void * dest,
        const void * src,
        int c,
        size_t count
        )
{
        while ( count && (*((char *)(dest = (char *)dest + 1) - 1) =
        *((char *)(src = (char *)src + 1) - 1)) != (char)c )
                count--;

        return(count ? dest : NULL);
}
