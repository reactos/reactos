/***
*mbscspn.c - Find first string char in charset (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Find first string char in charset (MBCS)
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
*ifndef _RETURN_PTR
* _mbscspn - Find first string char in charset (MBCS)
*else
* _mbspbrk - Find first string char in charset, pointer return (MBCS)
*endif
*
*Purpose:
*       Returns maximum leading segment of string
*       which consists solely of characters NOT from charset.
*       Handles MBCS chars correctly.
*
*Entry:
*       char *string = string to search in
*       char *charset = set of characters to scan over
*
*Exit:
*
*ifndef _RETURN_PTR
*       Returns the index of the first char in string
*       that is in the set of characters specified by control.
*
*       Returns 0, if string begins with a character in charset.
*else
*       Returns pointer to first character in charset.
*
*       Returns nullptr if string consists entirely of characters
*       not from charset.
*endif
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

#ifndef _RETURN_PTR

extern "C" size_t __cdecl _mbscspn_l(
        const unsigned char *string,
        const unsigned char *charset,
        _locale_t plocinfo
        )
#else  /* _RETURN_PTR */

extern "C" const unsigned char * __cdecl _mbspbrk_l(
        const unsigned char *string,
        const unsigned char  *charset,
        _locale_t plocinfo
        )
#endif  /* _RETURN_PTR */

{
        unsigned char *p, *q;
        _LocaleUpdate _loc_update(plocinfo);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
#ifndef _RETURN_PTR
            return strcspn((const char *)string, (const char *)charset);
#else  /* _RETURN_PTR */
            return (const unsigned char *)strpbrk((const char *)string, (const char *)charset);
#endif  /* _RETURN_PTR */

        /* validation section */
#ifndef _RETURN_PTR
        _VALIDATE_RETURN(string != nullptr, EINVAL, 0);
        _VALIDATE_RETURN(charset != nullptr, EINVAL, 0);
#else  /* _RETURN_PTR */
        _VALIDATE_RETURN(string != nullptr, EINVAL, nullptr);
        _VALIDATE_RETURN(charset != nullptr, EINVAL, nullptr);
#endif  /* _RETURN_PTR */

        /* loop through the string to be inspected */
        for (q = (unsigned char *)string; *q ; q++) {

            /* loop through the charset */
            for (p = (unsigned char *)charset; *p ; p++) {

                if ( _ismbblead_l(*p, _loc_update.GetLocaleT()) ) {
                    if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
                        break;
                    p++;
                }
                else
                    if (*p == *q)
                        break;
            }

            if (*p != '\0')         /* end of charset? */
                break;              /* no, match on this char */

            if ( _ismbblead_l(*q, _loc_update.GetLocaleT()) )
                if (*++q == '\0')
                    break;
        }

#ifndef _RETURN_PTR
        return((size_t) (q - string));  /* index */
#else  /* _RETURN_PTR */
        return((*q) ? q : nullptr);        /* pointer */
#endif  /* _RETURN_PTR */

}

#ifndef _RETURN_PTR

extern "C" size_t (__cdecl _mbscspn)(
        const unsigned char *string,
        const unsigned char *charset
        )
#else  /* _RETURN_PTR */

extern "C" const unsigned char * (__cdecl _mbspbrk)(
        const unsigned char *string,
        const unsigned char  *charset
        )
#endif  /* _RETURN_PTR */

{
#ifndef _RETURN_PTR
        return _mbscspn_l(string, charset, nullptr);
#else  /* _RETURN_PTR */
        return _mbspbrk_l(string, charset, nullptr);
#endif  /* _RETURN_PTR */
}
