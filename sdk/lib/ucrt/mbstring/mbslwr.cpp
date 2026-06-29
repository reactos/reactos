/***
*mbslwr.c - Convert string lower case (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Convert string lower case (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbslwr - Convert string lower case (MBCS)
*
*Purpose:
*       Convrts all the upper case characters in a string
*       to lower case in place.  MBCS chars are handled
*       correctly.
*
*Entry:
*       unsigned char *string = pointer to string
*
*Exit:
*       Returns a pointer to the input string.
*       Returns nullptr on error.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

errno_t __cdecl _mbslwr_s_l(
        unsigned char *string,
        size_t sizeInBytes,
        _locale_t plocinfo
        )
{
        size_t stringlen;

        /* validation section */
        _VALIDATE_RETURN_ERRCODE((string != nullptr && sizeInBytes > 0) || (string == nullptr && sizeInBytes == 0), EINVAL);

        if (string == nullptr)
        {
            /* nothing to do */
            return 0;
        }

        stringlen = strnlen((char *)string, sizeInBytes);
        if (stringlen >= sizeInBytes)
        {
            _RESET_STRING(string, sizeInBytes);
            _RETURN_DEST_NOT_NULL_TERMINATED(string, sizeInBytes);
        }
        _FILL_STRING(string, sizeInBytes, stringlen + 1);

        unsigned char *cp, *dst;
        _LocaleUpdate _loc_update(plocinfo);

        for (cp = string, dst = string; *cp != '\0'; ++cp)
        {
            if (_ismbblead_l(*cp, _loc_update.GetLocaleT()))
            {


                int retval;
                unsigned char ret[4];
                if ((retval = __acrt_LCMapStringA(
                                _loc_update.GetLocaleT(),
                                _loc_update.GetLocaleT()->mbcinfo->mblocalename,
                                LCMAP_LOWERCASE,
                                (const char *)cp,
                                2,
                                (char *)ret,
                                2,
                                _loc_update.GetLocaleT()->mbcinfo->mbcodepage,
                                TRUE )) == 0 )
                {
                    errno = EILSEQ;
                    _RESET_STRING(string, sizeInBytes);
                    return errno;
                }

                *(dst++) = ret[0];
                ++cp;
                if (retval > 1)
                {
                    *(dst++) = ret[1];
                }


            }
            else
            {
                /* single byte, macro version */
                *(dst++) = (unsigned char) _mbbtolower_l(*cp, _loc_update.GetLocaleT());
            }
        }
        /* null terminate the string */
        *dst = '\0';

        return 0;
}

errno_t (__cdecl _mbslwr_s)(
        unsigned char *string,
        size_t sizeInBytes
        )
{
    return _mbslwr_s_l(string, sizeInBytes, nullptr);
}

unsigned char * (__cdecl _mbslwr_l)(
        unsigned char *string,
        _locale_t plocinfo
        )
{
    return (_mbslwr_s_l(string, (string == nullptr ? 0 : (size_t)-1), plocinfo) == 0 ? string : nullptr);
}

unsigned char * (__cdecl _mbslwr)(
        unsigned char *string
        )
{
    return (_mbslwr_s_l(string, (string == nullptr ? 0 : (size_t)-1), nullptr) == 0 ? string : nullptr);
}
