/***
*mbsncat_s.inl - general implementation of _mbsncat_s and _mbsnbcat_s
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the general algorithm for _mbsncat_s and _mbsnbcat_s.
*
****/

errno_t __cdecl _FUNC_NAME(unsigned char *_Dst, size_t _SizeInBytes, const unsigned char *_Src, size_t _COUNT, _LOCALE_ARG_DECL)
{
    unsigned char *p;
    size_t available;
    BOOL fIsLeadPrefix;
    BOOL fFoundInvalidMBC;

    fFoundInvalidMBC = FALSE;

    if (_COUNT == 0 && _Dst == nullptr && _SizeInBytes == 0)
    {
        /* this case is allowed; nothing to do */
        _RETURN_NO_ERROR;
    }

    /* validation section */
    _VALIDATE_STRING(_Dst, _SizeInBytes);
    if (_COUNT != 0)
    {
        _VALIDATE_POINTER_RESET_STRING(_Src, _Dst, _SizeInBytes);
    }

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return strncat_s((char *)_Dst, _SizeInBytes, (const char *)_Src, _COUNT);
    }

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


    if (available < _SizeInBytes)
    {
        /*
         * Dst may have terminated with an invalid MBCS, in that case we clear
         * the bogus lead byte.
         */
        fIsLeadPrefix = FALSE;
        _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, &p[-1]);
        if (fIsLeadPrefix) {
            /* the original string ended with a lead byte: we remove it */
            p--;
            *p = 0;
            available++;
            fFoundInvalidMBC = TRUE;
        }
    }

    if (_COUNT == _TRUNCATE)
    {
        while ((*p++ = *_Src++) != 0 && --available > 0)
        {
        }
    }
    else
    {
#if _COUNT_IN_BYTES
        while (_COUNT > 0 && (*p++ = *_Src++) != 0 && --available > 0)
        {
            _COUNT--;
        }
#else  /* _COUNT_IN_BYTES */
        while (_COUNT > 0)
        {
            if (_ISMBBLEAD(*_Src))
            {
                if (_Src[1] == 0)
                {
                    /* the source string ended with a lead byte: we remove it */
                    *p = 0;
                    fFoundInvalidMBC = TRUE;
                    break;
                }
                if (available <= 2)
                {
                    /* not enough space */
                    available = 0;
                    break;
                }
                *p++ = *_Src++;
                *p++ = *_Src++;
                available -= 2;
            }
            else
            {
                if ((*p++ = *_Src++) == 0 || --available == 0)
                {
                    break;
                }
            }
            _COUNT--;
        }
#endif  /* _COUNT_IN_BYTES */
        if (_COUNT == 0)
        {
            *p++ = 0;
        }
    }

    if (available == 0)
    {
#if _COUNT_IN_BYTES
        /*
         * defined(_COUNT_IN_BYTES) loop does not track mbc context,
         * so we must iterate backwards to discover character context.
         */
        if (*_Src == 0 || _COUNT == 1)
        {
            _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, &p[-1]);
            if (fIsLeadPrefix)
            {
                /* the source string ended with a lead byte: we remove it */
                p[-1] = 0;
                _RETURN_MBCS_ERROR;
            }
        }
#endif  /* _COUNT_IN_BYTES */

        /*
         * _COUNT == _TRUNCATE loop terminated because available became 0.
         * This means that we copied at least one character, and it wasn't
         * a null. If this last character acted as a lead then overwrite
         * it with null. Do not set the mbcs error in this case, due that the
         * user cannot predict this case and he/she's only asking for truncation.
         */
        if (_COUNT == _TRUNCATE)
        {
            if (fFoundInvalidMBC)
            {
                _SET_MBCS_ERROR;
            }

            if (_SizeInBytes > 1)
            {
                fIsLeadPrefix = FALSE;
                _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, &_Dst[_SizeInBytes - 2]);
                if (fIsLeadPrefix)
                {
                    _Dst[_SizeInBytes - 2] = 0;
                    _FILL_BYTE(_Dst[_SizeInBytes - 1]);
                    _RETURN_TRUNCATE;
                }
            }

            _Dst[_SizeInBytes - 1] = 0;

            _RETURN_TRUNCATE;
        }
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_BUFFER_TOO_SMALL(_Dst, _SizeInBytes);
    }
#if _COUNT_IN_BYTES
    if (available < _SizeInBytes)
    {
        _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, &p[-2]);
        if (fIsLeadPrefix)
        {
            /* the source string ended with a lead byte: we remove it */
            p[-2] = 0;
            available++;
            fFoundInvalidMBC = TRUE;
        }
    }
#endif  /* _COUNT_IN_BYTES */
    _FILL_STRING(_Dst, _SizeInBytes, _SizeInBytes - available + 1);

    if (fFoundInvalidMBC)
    {
        _RETURN_MBCS_ERROR;
    }

    _RETURN_NO_ERROR;
}

