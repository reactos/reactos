/***
*mbsdec.c - Move MBCS string pointer backward one charcter.
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Move MBCS string pointer backward one character.
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stddef.h>

/***
*_mbsdec - Move MBCS string pointer backward one charcter.
*
*Purpose:
*       Move the supplied string pointer backwards by one
*       character.  MBCS characters are handled correctly.
*
*Entry:
*       const unsigned char *string = pointer to beginning of string
*       const unsigned char *current = current char pointer (legal MBCS boundary)
*
*Exit:
*       Returns pointer after moving it.
*       Returns nullptr if string >= current.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbsdec_l(
        const unsigned char *string,
        const unsigned char *current,
        _locale_t plocinfo
        )
{
        const unsigned char *temp;

        /* validation section */
        _VALIDATE_RETURN(string != nullptr, EINVAL, nullptr);
        _VALIDATE_RETURN(current != nullptr, EINVAL, nullptr);

        if (string >= current)
                return(nullptr);

        _LocaleUpdate _loc_update(plocinfo);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (unsigned char *)--current;

        temp = current - 1;

/* There used to be an optimisation here:
 *
 *  If (current-1) returns true from _ismbblead, it is a trail byte, because
 *  current is a known character start point, and so current-1 would have to be a
 *  legal single byte MBCS character, which a lead byte is not.  Therefore, if so,
 *  return (current-2) because it must be the trailbyte's lead.
 *
 *      if ( _ismbblead(*temp) )
 *           return (unsigned char *)(temp - 1);
 *
 * But this is not a valid optimisation if you want to cope correctly with an
 * MBCS string which is terminated by a leadbyte and a 0 byte, when you are passed
 * an initial position pointing to the \0 at the end of the string.
 *
 * This optimisation is also invalid if you are passed a pointer to half-way
 * through an MBCS pair.
 *
 * Neither of these are truly valid input conditions, but to ensure predictably
 * correct behaviour in the presence of these conditions, we have removed
 * the optimisation.
 */

/*
 *  It is unknown whether (current - 1) is a single byte character or a
 *  trail.  Now decrement temp until
 *      a)  The beginning of the string is reached, or
 *      b)  A non-lead byte (either single or trail) is found.
 *  The difference between (current-1) and temp is the number of non-single
 *  byte characters preceding (current-1).  There are two cases for this:
 *      a)  (current - temp) is odd, and
 *      b)  (current - temp) is even.
 *  If odd, then there are an odd number of "lead bytes" preceding the
 *  single/trail byte (current - 1), indicating that it is a trail byte.
 *  If even, then there are an even number of "lead bytes" preceding the
 *  single/trail byte (current - 1), indicating a single byte character.
 */
        while ( (string <= --temp) && (_ismbblead_l(*temp, _loc_update.GetLocaleT())) )
                ;

        return (unsigned char *)(current - 1 - ((current - temp) & 0x01) );
}

extern "C" unsigned char * (__cdecl _mbsdec)(
        const unsigned char *string,
        const unsigned char *current
        )
{
    return _mbsdec_l(string, current, nullptr);
}
