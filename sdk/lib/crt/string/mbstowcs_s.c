/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS CRT
 * PURPOSE:           Implementation of mbstowcs_s
 * FILE:              lib/sdk/crt/string/mbstowcs_s.c
 * PROGRAMMER:        Timo Kreuzer
 */

#include <precomp.h>
#include <mbstring.h>

_Success_(return!=EINVAL)
_Check_return_opt_
_CRTIMP
errno_t
__cdecl
mbstowcs_s(
   _Out_opt_ size_t *pcchConverted,
   _Out_writes_to_opt_(sizeInWords, *pcchConverted) wchar_t *pwcstr,
   _In_ size_t sizeInWords,
   _In_reads_or_z_(count) const char *pmbstr,
   _In_ size_t count)
{
    size_t cchMax, cwcWritten;
    errno_t retval = 0;

    /* Make sure, either we have a target buffer > 0 bytes, or no buffer */
    if (!MSVCRT_CHECK_PMT( ((sizeInWords != 0) && (pwcstr != 0)) ||
                           ((sizeInWords == 0) && (pwcstr == 0)) ))
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

    if (!MSVCRT_CHECK_PMT((count == 0) || (pmbstr != 0)))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if there is anything to do */
    if ((pwcstr == 0) && (pmbstr == 0))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Check if we have a multibyte string */
    if (pmbstr)
    {
        /* Check if we also have a wchar buffer */
        if (pwcstr)
        {
            /* Calculate the maximum the we can write */
            cchMax = (count < sizeInWords) ? count + 1 : sizeInWords;

            /* Now do the conversion */
            cwcWritten = mbstowcs(pwcstr, pmbstr, cchMax);

            /* Check if the buffer was not zero terminated */
            if (cwcWritten == cchMax)
            {
                /* Check if we reached the max size of the dest buffer */
                if (cwcWritten == sizeInWords)
                {
                    /* Does the caller allow this? */
                    if (count != _TRUNCATE)
                    {
                        /* Not allowed, truncate to 0 length */
                        pwcstr[0] = L'\0';

                        /* Return error */
                        _set_errno(ERANGE);
                        return ERANGE;
                    }

                    /* Inform the caller about truncation */
                    retval = STRUNCATE;
                }

                /* zero teminate the buffer */
                pwcstr[cwcWritten - 1] = L'\0';
            }
            else
            {
                /* The buffer is zero terminated, count the terminating char */
                cwcWritten++;
            }
        }
        else
        {
            /* Get the length of the string, plus 0 terminator */
            cwcWritten = _mbsnlen((const unsigned char *)pmbstr, count) + 1;
        }
    }
    else
    {
        cwcWritten = count + 1;
    }

    /* Check if we have a return value pointer */
    if (pcchConverted)
    {
        /* Default to 0 bytes written */
        *pcchConverted = cwcWritten;
    }

    return retval;
}
