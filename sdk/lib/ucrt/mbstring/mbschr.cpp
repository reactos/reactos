/***
* mbschr.c - Search MBCS string for character
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Search MBCS string for a character
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbschr - Search MBCS string for character
*
*Purpose:
*       Search the given string for the specified character.
*       MBCS characters are handled correctly.
*
*Entry:
*       unsigned char *string = string to search
*       int c = character to search for
*
*Exit:
*       returns a pointer to the first occurence of the specified char
*       within the string.
*
*       returns nullptr if the character is not found n the string.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/


extern "C" _CONST_RETURN unsigned char * __cdecl _mbschr_l(
        const unsigned char *string,
        unsigned int c,
        _locale_t plocinfo
        )
{
        unsigned short cc = '\0';
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(string != nullptr, EINVAL, nullptr);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (_CONST_RETURN unsigned char *)strchr((const char *)string, (int)c);

        for (; (cc = *string) != '\0'; string++)
        {
            if ( _ismbblead_l(cc, _loc_update.GetLocaleT()) )
            {
                if (*++string == '\0')
                    return nullptr;        /* error */
                if ( c == (unsigned int)((cc << 8) | *string) ) /* DBCS match */
                    return (unsigned char *)(string - 1);
            }
            else if (c == (unsigned int)cc)
                break;  /* SBCS match */
        }

        if (c == (unsigned int)cc)      /* check for SBCS match--handles NUL char */
            return (unsigned char *)(string);

        return nullptr;
}

extern "C" _CONST_RETURN unsigned char * (__cdecl _mbschr)(
        const unsigned char *string,
        unsigned int c
        )
{
    return _mbschr_l(string, c, nullptr);
}
