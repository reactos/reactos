/***
*mbscat_s_l.c - Concatenate one string to another (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Concatenate one string to another (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>



errno_t __cdecl _mbscat_s_l(unsigned char *_Dst, size_t _SizeInBytes, const unsigned char *_Src, _LOCALE_ARG_DECL)
{
    unsigned char *p;
    size_t available;
    BOOL fFoundInvalidMBC, fIsLeadPrefix;

    /* validation section */
    _VALIDATE_STRING(_Dst, _SizeInBytes);
    _VALIDATE_POINTER_RESET_STRING(_Src, _Dst, _SizeInBytes);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return strcat_s((char *)_Dst, _SizeInBytes, (const char *)_Src);
    }

    fFoundInvalidMBC = FALSE;
    p = _Dst;
    available = _SizeInBytes;
    while (available > 0 && *p != 0)
    {
        p++;
        available--;
    }

    /*
     * Ran out of room while looking for end of dst string.
     * p points 1 past end of buffer. We can't look past
     * end of buffer so can't tell if dst ended with an
     * invalid mbc.
     */

    if (available == 0)
    {
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_DEST_NOT_NULL_TERMINATED(_Dst, _SizeInBytes);
    }

    /*
     * Otherwise we have space available, p points at null that lies
     * within _SizeInBytes, so available > 0. Check if dst ended with
     * an invalid MBC (lead+null), if so then clear that lead byte,
     * move the pointer back one and increase available by one.
     */

    _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, p-1);
    if (fIsLeadPrefix)
    {
        fFoundInvalidMBC = TRUE;
        p--;
        *p = 0;
        available++;
    }

    /* Append dst to src. */

    while ((*p++ = *_Src++) != 0 && --available > 0)
    {
    }

    /*
     * We've run out of room in the destination before finding null in the src.
     * It could be that the src was terminated with an invalid mbc (lead+null).
     * In that case its ok to clear the copied lead byte and return mbcs_error.
     */

    if (available == 0)
    {
        if (*_Src == 0)
        {
            _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, p-1);
            if (fIsLeadPrefix)
            {
                /* the source string ended with a lead byte: we remove it */
                p[-1] = 0;
                _RETURN_MBCS_ERROR;
            }
        }
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_BUFFER_TOO_SMALL(_Dst, _SizeInBytes);
    }

    /*
     * If the src string ended with an invalid mbc (lead+null) then clear the
     * lead byte.
     */

    _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, p-2);
    if (fIsLeadPrefix)
    {
        p[-2] = 0;
        available++;
        fFoundInvalidMBC = TRUE;
    }

    _FILL_STRING(_Dst, _SizeInBytes, _SizeInBytes - available + 1);

    if (fFoundInvalidMBC)
    {
        _RETURN_MBCS_ERROR;
    }

    _RETURN_NO_ERROR;
}
