/***
*mbccpy_s_l.c - Copy one character  to another (MBCS)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Copy one MBCS character to another (1 or 2 bytes)
*
*******************************************************************************/

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_securecrt.h>
#include <locale.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

errno_t __cdecl _mbccpy_s_l(unsigned char *_Dst, size_t _SizeInBytes, int *_PCopied, const unsigned char *_Src, _LOCALE_ARG_DECL)
{
    /* validation section */
    _ASSIGN_IF_NOT_NULL(_PCopied, 0);
    _VALIDATE_STRING(_Dst, _SizeInBytes);
    if (_Src == nullptr)
    {
        *_Dst = '\0';
        _RETURN_EINVAL;
    }

    _LOCALE_UPDATE;

    /* copy */
    if (_ISMBBLEAD(*_Src))
    {
        if (_Src[1] == '\0')
        {
            /* the source string contained a lead byte followed by the null terminator:
               we copy only the null terminator and return EILSEQ to indicate the
               malformed char */
            *_Dst = '\0';
            _ASSIGN_IF_NOT_NULL(_PCopied, 1);
            _RETURN_MBCS_ERROR;
        }
        if (_SizeInBytes < 2)
        {
            *_Dst = '\0';
            _RETURN_BUFFER_TOO_SMALL(_Dst, _SizeInBytes);
        }
        *_Dst++ = *_Src++;
        *_Dst = *_Src;
        _ASSIGN_IF_NOT_NULL(_PCopied, 2);
    }
    else
    {
        *_Dst = *_Src;
        _ASSIGN_IF_NOT_NULL(_PCopied, 1);
    }

    _RETURN_NO_ERROR;
}
