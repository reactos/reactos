/***
*mbscpy_s_l.c - Copy one string to another (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Copy one string to another (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

errno_t __cdecl _mbscpy_s_l(unsigned char *_Dst, size_t _SizeInBytes, const unsigned char *_Src, _LOCALE_ARG_DECL)
{
    unsigned char *p;
    size_t available;
    BOOL fIsLeadPrefix;

    /* validation section */
    _VALIDATE_STRING(_Dst, _SizeInBytes);
    _VALIDATE_POINTER_RESET_STRING(_Src, _Dst, _SizeInBytes);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return strcpy_s((char *)_Dst, _SizeInBytes, (const char *)_Src);
    }
    
    p = _Dst;
    available = _SizeInBytes;
    while ((*p++ = *_Src++) != 0 && --available > 0)
    {
    }

    /*
     * If we ran out of destination bytes then we did so before copying null.
     * Only exception to that is if last mbc was invalid (leadbyte+null), which
     * is treated as null. In that case clear the copied lead byte and return ok.
     */

    if (available == 0)
    {
        if (*_Src == 0) {
            _ISMBBLEADPREFIX(fIsLeadPrefix,_Dst,p-1);
            if (fIsLeadPrefix)
            {
                p[-1] = 0;
                _RETURN_MBCS_ERROR;
            }
        }
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_BUFFER_TOO_SMALL(_Dst, _SizeInBytes);
    }

    /*
     * Otherwise we have space left in the dst buffer and stopped copying because
     * we saw a null in the src. If null is part of invalid MBC (lead byte + null)
     * then clear the lead byte also.
     */

    _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, p-2);
    if (fIsLeadPrefix && (p - 2) >= _Dst)
    {
        p[-2] = 0;
        available++;
        _FILL_STRING(_Dst, _SizeInBytes, _SizeInBytes - available + 1);
        _RETURN_MBCS_ERROR;
    }

    _FILL_STRING(_Dst, _SizeInBytes, _SizeInBytes - available + 1);
    _RETURN_NO_ERROR;
}
