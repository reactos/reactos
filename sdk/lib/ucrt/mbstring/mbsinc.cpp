/***
*mbsinc.c - Move MBCS string pointer ahead one charcter.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Move MBCS string pointer ahead one character.
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <stddef.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*_mbsinc - Move MBCS string pointer ahead one charcter.
*
*Purpose:
*       Move the supplied string pointer ahead by one
*       character.  MBCS characters are handled correctly.
*
*Entry:
*       const unsigned char *current = current char pointer (legal MBCS boundary)
*
*Exit:
*       Returns pointer after moving it.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbsinc_l(
        const unsigned char *current,
        _locale_t plocinfo
        )
{
        if ( (_ismbblead_l)(*(current++),plocinfo))
        {
            /* don't move forward two if we get leadbyte, EOS
               also don't assert here as we are too low level
            */
            if(*current!='\0')
            {
                current++;
            }
        }

        return (unsigned char *)current;
}

extern "C" unsigned char * (__cdecl _mbsinc)(
        const unsigned char *current
        )
{
        /* validation section */
        _VALIDATE_RETURN(current != nullptr, EINVAL, nullptr);

        if ( _ismbblead(*(current++)))
        {
            /* don't move forward two if we get leadbyte, EOS
               also don't assert here as we are too low level
            */
            if(*current!='\0')
            {
                current++;
            }
        }

        return (unsigned char *)current;
}
