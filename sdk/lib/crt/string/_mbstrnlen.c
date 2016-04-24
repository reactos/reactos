/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS CRT
 * PURPOSE:           Implementation of _mbstrnlen
 * FILE:              lib/sdk/crt/string/_mbstrnlen.c
 * PROGRAMMER:        Timo Kreuzer
 */

#include <precomp.h>
#include <mbctype.h>
#include <specstrings.h>

_Success_(return>0)
_Check_return_
_CRTIMP
size_t
__cdecl
_mbstrnlen(
    _In_z_ const char *pmbstr,
    _In_ size_t cjMaxLen)
{
    size_t cchCount = 0;
    unsigned char jMbsByte;

    /* Check parameters */
    if (!MSVCRT_CHECK_PMT((pmbstr != 0)) && (cjMaxLen <= INT_MAX))
    {
        _set_errno(EINVAL);
        return -1;
    }

    /* Loop while we have bytes to process */
    while (cjMaxLen-- > 0)
    {
        /* Get next mb byte */
        jMbsByte = *pmbstr++;

        /* If this is 0, we're done */
        if (jMbsByte == 0) break;

        /* if this is a lead byte, continue with next char */
        if (_ismbblead(jMbsByte))
        {
            // FIXME: check if this is a valid char.
            continue;
        }

        /* Count this character */
        cchCount++;
    }

    return cchCount;
}

