/***
*mbsncpy_s.inl - general implementation of _mbsncpy_s and _mbsnbcpy_s
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the general algorithm for _mbsncpy_s and _mbsnbcpy_s.
*
*       _COUNT_IN_BYTES defined to 1 implements _mbsnbcpy_s
*       _COUNT_IN_BYTES defined to 0 implements _mbsncpy_s
*
****/

errno_t __cdecl _FUNC_NAME(unsigned char *_Dst, size_t _SizeInBytes, const unsigned char *_Src, size_t _COUNT, _LOCALE_ARG_DECL)
{
    unsigned char *p;
    size_t available;
    BOOL fFoundInvalidMBC;
    BOOL fIsLeadPrefix;

    fFoundInvalidMBC = FALSE;

    if (_COUNT == 0 && _Dst == nullptr && _SizeInBytes == 0)
    {
        /* this case is allowed; nothing to do */
        _RETURN_NO_ERROR;
    }

    /* validation section */
    _VALIDATE_STRING(_Dst, _SizeInBytes);
    if (_COUNT == 0)
    {
        /* notice that the source string pointer can be NULL in this case */
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) /* 26015 */
        _RESET_STRING(_Dst, _SizeInBytes);
        _RETURN_NO_ERROR;
    }
    _VALIDATE_POINTER_RESET_STRING(_Src, _Dst, _SizeInBytes);

    _LOCALE_UPDATE;
    if (_LOCALE_SHORTCUT_TEST)
    {
        return strncpy_s((char *)_Dst, _SizeInBytes, (const char *)_Src, _COUNT);
    }

    p = _Dst;
    available = _SizeInBytes;
    if (_COUNT == _TRUNCATE)
    {
        while ((*p++ = *_Src++) != 0 && --available > 0)
        {
        }

        /*
         * loop terminates with either:
         * - src, p pointing 1 byte past null, avail includes the null
         * - available == 0, p points 1 past end of dst buffer
         */
    }
    else
    {
#if _COUNT_IN_BYTES
        while ((*p++ = *_Src++) != 0 && --available > 0 && --_COUNT > 0)
        {
        }

        /*
         * loop terminates with either:
         * - p points 1 byte past null, avail includes null, count includes null
         * - available == 0, p points 1 past end of dst buffer (inaccessible)
         * - count == 0, p points 1 past last written byte, space available in dst buffer
         *
         * always p[-1] is written.
         * sometimes p[-1] is null.
         */
#else  /* _COUNT_IN_BYTES */

        /* at this point, avail count be 1. */

        /* Need to track lead-byte context in order to track character count. */
        do
        {
            if (_ISMBBLEAD(*_Src))
            {
                if (_Src[1] == 0)
                {
                    /*
                     * Invalid MBC, write null to dst string, we are finished
                     * copying. We know that available is >= 1, so there is
                     * room for the null termination. If we decrement available
                     * then we will incorrectly report BUFFER_TOO_SMALL.
                     */

                    *p++ = 0;
                    fFoundInvalidMBC = TRUE;
                    break;
                }
                if (available <= 2)
                {
                    /* not enough space for a dbc and null */
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
        }
        while (--_COUNT > 0);
#endif  /* _COUNT_IN_BYTES */

        /* If count == 0 then at least one byte was copied and available is still > 0 */
        if (_COUNT == 0)
        {
            *p++ = 0;
            /* Note that available is not decremented here. */
        }
    }

    if (available == 0)
    {
#if _COUNT_IN_BYTES
        /*
         * For COUNT_IN_BYTES, the above loop copied at least one byte so src,p point
         * past a written byte.
         */

        if (*_Src == 0 || _COUNT == 1)
        {
            fIsLeadPrefix = FALSE;
            _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst, &p[-1]);
            if (fIsLeadPrefix)
            {
                /* the source string ended with a lead byte: we remove it */
                p[-1] = 0;
                _RETURN_MBCS_ERROR;
            }
        }
#endif  /* _COUNT_IN_BYTES */

        if (_COUNT == _TRUNCATE)
        {
            if (fFoundInvalidMBC)
            {
                _SET_MBCS_ERROR;
            }

            if (_SizeInBytes > 1)
            {
                fIsLeadPrefix = FALSE;
                /* Check if 2nd to last copied byte acted as a lead.
                 * Do not set mbcs error because we are truncating.
                 */
                _ISMBBLEADPREFIX(fIsLeadPrefix,_Dst,&_Dst[_SizeInBytes - 2]);
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
    /*
     * COUNT_IN_BYTES copy loop doesn't track lead-byte context, so can't detect
     * invalid mbc. Detect them here.

     * available < _SizeInBytes means that at least one byte was copied so p is >= &dstBuffer[1]
     */

    if ((p - _Dst) >= 2)
    {
        _ISMBBLEADPREFIX(fIsLeadPrefix, _Dst,&p[-2]);
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

#pragma warning(suppress:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) /* 26036 REVIEW TODO test _mbsnbcpy_s_l */
    _RETURN_NO_ERROR;
}

