/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS CRT
 * PURPOSE:           Implementation of _mbsnlen
 * FILE:              lib/sdk/crt/string/_mbsnlen.c
 * PROGRAMMER:        Timo Kreuzer
 */

#include <mbstring.h>

_Check_return_
_CRTIMP
size_t
__cdecl
_mbsnlen(
    _In_z_ const unsigned char *pmbstr,
    _In_ size_t cjMaxLen)
{
    size_t cchCount = 0;
    unsigned char jMbsByte;

    /* Loop while we have bytes to process */
    while (cjMaxLen-- > 0)
    {
        /* Get next mb byte */
        jMbsByte = *pmbstr++;

        /* If this is 0, we're done */
        if (jMbsByte == 0) break;

        /* Don't count lead bytes */
        if (!_ismbblead(jMbsByte)) cchCount++;
    }

    return cchCount;
}
