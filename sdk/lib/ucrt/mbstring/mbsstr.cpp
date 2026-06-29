/***
* mbsstr.c - Search for one MBCS string inside another
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Search for one MBCS string inside another
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

/***
* _mbsstr - Search for one MBCS string inside another
*
*Purpose:
*       Find the first occurrence of str2 in str1.
*
*Entry:
*       unsigned char *str1 = beginning of string
*       unsigned char *str2 = string to search for
*
*Exit:
*       Returns a pointer to the first occurrence of str2 in
*       str1, or nullptr if str2 does not occur in str1
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" _CONST_RETURN unsigned char * __cdecl _mbsstr_l(
        const unsigned char *str1,
        const unsigned char *str2,
        _locale_t plocinfo
        )
{
        unsigned char *cp, *s1, *s2, *endp;
        _LocaleUpdate _loc_update(plocinfo);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (unsigned char *)strstr((const char *)str1, (const char *)str2);

        /* validation section */
        _VALIDATE_RETURN(str2 != nullptr, EINVAL, 0);
        if ( *str2 == '\0')
            return (unsigned char *)str1;
        _VALIDATE_RETURN(str1 != nullptr, EINVAL, 0);

        cp = (unsigned char *) str1;
        endp = (unsigned char *) (str1 + (strlen((const char *)str1) - strlen((const char *)str2)));

        while (*cp && (cp <= endp))
        {
            s1 = cp;
            s2 = (unsigned char *) str2;

            /*
             * MBCS: ok to ++ since doing equality comparison.
             * [This depends on MBCS strings being "legal".]
             */
            while ( *s1 && *s2 && (*s1 == *s2) )
                s1++, s2++;

            if (!(*s2))
                return(cp);     /* success! */

            /*
             * bump pointer to next char
             */
            if ( _ismbblead_l(*(cp++), _loc_update.GetLocaleT()) )
            {
                /*  don't move forward if we have leadbyte, EOS
                    means dud string was passed in.
                    Don't assert - too low level
                */
                if(*cp!='\0')
                {
                    cp++;
                }
            }
        }

        return(nullptr);

}

extern "C" _CONST_RETURN unsigned char * (__cdecl _mbsstr)(
        const unsigned char *str1,
        const unsigned char *str2
        )
{
    return _mbsstr_l(str1, str2, nullptr);
}
