/***
*mbsnbset.c - Sets first n bytes of string to given character (MBCS)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets first n bytes of string to given character (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

/***
* _mbsnbset - Sets first n bytes of string to given character (MBCS)
*
*Purpose:
*       Sets the first n bytes of string to the supplied
*       character value.  If the length of string is less than n,
*       the length of string is used in place of n.  Handles
*       MBCS chars correctly.
*
*       There are several factors that make this routine complicated:
*               (1) The fill value may be 1 or 2 bytes long.
*               (2) The fill operation may end by hitting the count value
*               or by hitting the end of the string.
*               (3) A null terminating char is NOT placed at the end of
*               the string.
*
*       Cases to be careful of (both of these can occur at once):
*               (1) Leaving an "orphaned" trail byte in the string (e.g.,
*               overwriting a lead byte but not the corresponding trail byte).
*               (2) Writing only the 1st byte of a 2-byte fill value because the
*               end of string was encountered.
*
*Entry:
*       unsigned char *string = string to modify
*       unsigned int val = value to fill string with
*       size_t count = count of characters to fill
*
*
*Exit:
*       Returns string = now filled with char val
*
*Uses:
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbsnbset_l(
        unsigned char *string,
        unsigned int val,
        size_t count,
        _locale_t plocinfo
        )
{
        unsigned char *start = string;
        unsigned char highval, lowval;
        _LocaleUpdate _loc_update(plocinfo);

_BEGIN_SECURE_CRT_DEPRECATION_DISABLE
        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (unsigned char *)_strnset((char *)string, val, count);
_END_SECURE_CRT_DEPRECATION_DISABLE

        /* validation section */
        _VALIDATE_RETURN(string != nullptr || count == 0, EINVAL, nullptr);

        /*
         * leadbyte flag indicates if the last byte we overwrote was
         * a lead byte or not.
         */

        highval = static_cast<unsigned char>(val >> 8);
        if (highval)
        {
                /* double byte value */

                lowval = (unsigned char)(val & 0x00ff);

                if(lowval=='\0')
                {
                    _ASSERTE(("invalid MBCS pair passed to mbsnbset",0));

                    /* Ideally we would return nullptr here and signal an error
                       condition. But since this function has no other
                       error modes, there would be a good chance of crashing
                       the caller. So instead we fill the string with spaces
                       to ensure that no information leaks through
                       unexpectedly. Anyway, we do set errno to EINVAL.
                    */
                    errno = EINVAL;
                    lowval=highval=' ';
                }

                while ((count--) && *string) {

                        /* pad with ' ' if no room for both bytes -- odd len */
                        if ((!count--) || (!*(string+1))) {
                                *string = ' ';
                                break;
                        }

                        *string++ = highval;
                        *string++ = lowval;
                }
        }

        else {
                /* single byte value */

                while (count-- && *string) {
                        *string++ = (unsigned char)val;
                }

        }

        return( start );
}

extern "C" unsigned char * (__cdecl _mbsnbset)(
        unsigned char *string,
        unsigned int val,
        size_t count
        )
{
_BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    return _mbsnbset_l(string, val, count, nullptr);
_END_SECURE_CRT_DEPRECATION_DISABLE
}
