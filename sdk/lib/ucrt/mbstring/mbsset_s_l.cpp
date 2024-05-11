/***
*mbsset_s_l.c - Sets all charcaters of string to given character (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Sets all charcaters of string to given character (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>

errno_t __cdecl _mbsset_s_l(unsigned char *_Dst, size_t _SizeInBytes, unsigned int _Value, _LOCALE_ARG_DECL)
{
    int mbcs_error = 0;
    unsigned char *p;
    size_t available;
    unsigned char highval, lowval;

    /* validation section */
    _VALIDATE_STRING(_Dst, _SizeInBytes);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return _strset_s((char *)_Dst, _SizeInBytes, (int)_Value);
    }

    p = _Dst;
    available = _SizeInBytes;
    highval = (unsigned char)(_Value >> 8);
    lowval = (unsigned char)(_Value & 0x00ff);

    /* ensure _Value is a valid mbchar */
    if ((highval != 0 && (lowval == 0 || !_ISMBBLEAD(highval))) ||
        (highval == 0 && _ISMBBLEAD(lowval)))
    {
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_MBCS_ERROR;
    }

    if (highval != 0)
    {
        while (*p != 0 && --available > 0)
        {
            if (p[1] == 0)
            {
                /* do not orphan leadbyte */
                *p = 0;
                ++available;
                mbcs_error = 1;
                break;
            }
            *p++ = highval;
            if (--available == 0)
            {
                break;
            }
            *p++ = lowval;
        }
    }
    else
    {
        while (*p != 0 && --available > 0)
        {
            *p++ = lowval;
        }
    }

    if (available == 0)
    {
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_DEST_NOT_NULL_TERMINATED(_Dst, _SizeInBytes);
    }
    _FILL_STRING(_Dst, _SizeInBytes, _SizeInBytes - available + 1);

    if (mbcs_error)
    {
        _RETURN_MBCS_ERROR;
    }
    else
    {
        _RETURN_NO_ERROR;
    }
}
