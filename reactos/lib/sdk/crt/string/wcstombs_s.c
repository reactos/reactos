/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS CRT
 * PURPOSE:           Implementation of mbstowcs_s
 * FILE:              lib/sdk/crt/string/wcstombs_s.c
 * PROGRAMMER:        Timo Kreuzer
 */

#include <precomp.h>

_Success_(return!=EINVAL)
_Check_return_wat_
_CRTIMP
errno_t
__cdecl
wcstombs_s(
    _Out_opt_ size_t * pcchConverted,
    _Out_writes_bytes_to_opt_(cjDstSize, *pcchConverted)
        char * pmbstrDst,
    _In_ size_t cjDstSize,
    _In_z_ const wchar_t * pwszSrc,
    _In_ size_t cjMaxCount)
{
    size_t cchMax, cchConverted;
    errno_t retval = 0;

    /* Make sure, either we have a target buffer > 0 bytes, or no buffer */
    if (!MSVCRT_CHECK_PMT( ((cjDstSize != 0) && (pmbstrDst != 0)) ||
                           ((cjDstSize == 0) && (pmbstrDst == 0)) ))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if we have a return value pointer */
    if (pcchConverted)
    {
        /* Default to 0 bytes written */
        *pcchConverted = 0;
    }

    if (!MSVCRT_CHECK_PMT((cjMaxCount == 0) || (pwszSrc != 0)))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if there is anything to do */
    if ((pmbstrDst == 0) && (pwszSrc == 0))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if we have a wchar string */
    if (pwszSrc)
    {
        /* Check if we also have a multibyte buffer */
        if (pmbstrDst)
        {
            /* Calculate the maximum that we can write */
            cchMax = (cjMaxCount < cjDstSize) ? cjMaxCount + 1 : cjDstSize;

            /* Now do the conversion */
            cchConverted = wcstombs(pmbstrDst, pwszSrc, cchMax);

            /* Check if the buffer was not zero terminated */
            if (cchConverted == cchMax)
            {
                /* Check if we reached the max size of the dest buffer */
                if (cchConverted == cjDstSize)
                {
                    /* Does the caller allow this? */
                    if (cjMaxCount != _TRUNCATE)
                    {
                        /* Not allowed, truncate to 0 length */
                        pmbstrDst[0] = L'\0';

                        /* Return error */
                        _set_errno(ERANGE);
                        return ERANGE;
                    }

                    /* Inform the caller about truncation */
                    retval = STRUNCATE;
                }

                /* zero teminate the buffer */
                pmbstrDst[cchConverted - 1] = L'\0';
            }
            else
            {
                /* The buffer is zero terminated, count the terminating char */
                cchConverted++;
            }
        }
        else
        {
            /* Get the length of the string, plus 0 terminator */
            cchConverted = wcsnlen(pwszSrc, cjMaxCount) + 1;
        }
    }
    else
    {
        cchConverted = cjMaxCount + 1;
    }

    /* Check if we have a return value pointer */
    if (pcchConverted)
    {
        /* Default to 0 bytes written */
        *pcchConverted = cchConverted;
    }

    return retval;
}
