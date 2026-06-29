/***
*mbsnset_s.inl - general implementation of _mbsnset_s and _mbsnbset_s
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the general algorithm for _mbsnset_s and _mbsnbset_s.
*
****/

errno_t __cdecl _FUNC_NAME(unsigned char *_Dst, size_t _SizeInBytes, unsigned int _Value, size_t _COUNT, _LOCALE_ARG_DECL)
{
    int mbcs_error = 0;
    unsigned char *p;
    size_t available;
    unsigned char highval, lowval;
    int mblead = 0;

    /* validation section */
    if (_COUNT == 0 && _Dst == nullptr && _SizeInBytes == 0)
    {
        /* this case is allowed; nothing to do */
        _RETURN_NO_ERROR;
    }
    _VALIDATE_STRING(_Dst, _SizeInBytes);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return _strnset_s((char *)_Dst, _SizeInBytes, (int)_Value, _COUNT);
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
#if _COUNT_IN_BYTES
        /* if _COUNT is odd, make it even subtracting 1 */
        /* divide _COUNT by 2 to have a count in chars instead of bytes */
        if ((_COUNT & 1) == 1)
        {
            --_COUNT;
            mbcs_error = 1;
        }
        _COUNT = _COUNT / 2;
#endif  /* _COUNT_IN_BYTES */
        while (*p != 0 && _COUNT > 0 && --available > 0)
        {
            if (p[1] == 0)
            {
                /* do not orphan leadbyte */
                *p = 0;
                ++available;
                mbcs_error = 1;
                break;
            }
            /* track the mblead status in the original string */
            mblead = mblead ? 0 : _ISMBBLEAD(*p);
            *p++ = highval;
            if (--available == 0)
            {
                break;
            }
            mblead = mblead ? 0 : _ISMBBLEAD(*p);
            *p++ = lowval;
            --_COUNT;
        }
    }
    else
    {
        while (*p != 0 && _COUNT > 0 && --available > 0)
        {
            /* track the mblead status in the original string */
            mblead = mblead ? 0 : _ISMBBLEAD(*p);
            *p++ = lowval;
            --_COUNT;
        }
    }

    /* if the last written byte overlapps a leadbyte,
     * copy a space over the corresponding trailbyte */
    if (_COUNT == 0 && mblead && *p != 0 && --available > 0)
    {
        mbcs_error = 1;
        if (p[1] == 0)
        {
            /* special case: if the space would be followed by 0, just put a 0 */
            *p = 0;
            ++available;
        }
        else
        {
            *p++ = ' ';
        }
    }

    if (_COUNT == 0)
    {
        /* ensure the string is null-terminated */
        while (*p != 0 && --available > 0)
        {
            ++p;
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

