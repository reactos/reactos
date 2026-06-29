//
// putch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _putch(), which writes a character to the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>
#include <stdlib.h>

// Writes a wide character to the console.  Returns the character on success,
// EOF on failure.
extern "C" int __cdecl _putch(int const c)
{
    return __acrt_lock_and_call(__acrt_conio_lock, [&]
    {
        return _putch_nolock(c);
    });
}

extern "C" int __cdecl _putch_nolock_internal(int const c, __crt_cached_ptd_host& ptd)
{
    __acrt_ptd*     const raw_ptd      = ptd.get_raw_ptd();
    unsigned char*  const ch_buf       = raw_ptd->_putch_buffer;
    unsigned short* const pch_buf_used = &raw_ptd->_putch_buffer_used;

    // We can only use the character directly if we are sure that the machine
    // is big-endian.
    int result = c;

    // Why are we using putwch to write to Console when we could have
    // written straight away to Console? The problem we have in writing to
    // Console is that CRT codepage is different from Console codepage and
    // thus to write to console, we will need to convert the codepage. Here
    // we can use unicode version of these routines and this way we will
    // only have to do one conversion and rest will be handled by putwch.

    // The usual way people call putch is character by character. Also
    // there is no way we can convert partial MBCS to unicode character. To
    // address this issue, we buffer all the lead bytes and combine them
    // with trail bytes and then do the conversion.
    if (*pch_buf_used == 1)
    {
        _ASSERTE(isleadbyte(ch_buf[0]) != 0);

        ch_buf[1] = static_cast<unsigned char>(c);
    }
    else
    {
        ch_buf[0] = static_cast<unsigned char>(c);
    }

    if (*pch_buf_used == 0 && isleadbyte(ch_buf[0]))
    {
        // We still need trail byte, wait for it.
        *pch_buf_used = 1;
    }
    else
    {
        wchar_t wchar;
        if (_mbtowc_internal(&wchar, reinterpret_cast<char const*>(ch_buf), *pch_buf_used + 1, ptd) == -1 ||
            _putwch_nolock(wchar) == WEOF)
        {
            result = EOF;
        }

        // Since we have processed full MBCS character, we should reset ch_buf_used.
        *pch_buf_used = 0;
    }

    return result;
}

extern "C" int __cdecl _putch_nolock(int const c)
{
     __crt_cached_ptd_host ptd;
     return _putch_nolock_internal(c, ptd);
}
