/***
*mbtoupr.c - Convert character to upper case (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Convert character to upper case (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

/***
* _mbctoupper - Convert character to upper case (MBCS)
*
*Purpose:
*       If the given character is lower case, convert to upper case.
*       Handles MBCS chars correctly.
*
*       Note:  Use test against 0x00FF instead of _ISLEADBYTE
*       to ensure that we don't call SBCS routine with a two-byte
*       value.
*
*Entry:
*       unsigned int c = character to convert
*
*Exit:
*       Returns converted character
*
*Exceptions:
*
*******************************************************************************/

extern "C" unsigned int __cdecl _mbctoupper_l(
        unsigned int c,
        _locale_t plocinfo
        )
{
        unsigned char val[2];
        unsigned char ret[4];
        _LocaleUpdate _loc_update(plocinfo);

        if (c > 0x00FF)
        {

            val[0] = (c >> 8) & 0xFF;
            val[1] = c & 0xFF;

            if ( !_ismbblead_l(val[0], _loc_update.GetLocaleT()) )
                return c;


            if ( __acrt_LCMapStringA(
                        _loc_update.GetLocaleT(),
                        _loc_update.GetLocaleT()->mbcinfo->mblocalename,
                        LCMAP_UPPERCASE,
                        (const char *)val,
                        2,
                        (char *)ret,
                        2,
                        _loc_update.GetLocaleT()->mbcinfo->mbcodepage,
                        TRUE ) == 0 )
                return c;

            c = ret[1];
            c += ret[0] << 8;

            return c;


        }
        else
            return (unsigned int)_mbbtoupper_l((int)c, _loc_update.GetLocaleT());
}
unsigned int (__cdecl _mbctoupper)(
        unsigned int c
        )
{
    return _mbctoupper_l(c, nullptr);
}
