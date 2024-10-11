//
// fputwc.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the fputc() family of functions, which write a character to a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>



static wint_t __cdecl fputwc_binary_nolock(wchar_t const c, __crt_stdio_stream const stream, __crt_cached_ptd_host& ptd) throw()
{
    stream->_cnt -= sizeof(wchar_t);

    // If there is no room for the character in the buffer, flush the buffer and
    // write the character:
    if (stream->_cnt < 0)
    {
        return static_cast<wint_t>(__acrt_stdio_flush_and_write_wide_nolock(c, stream.public_stream(), ptd));
    }

    // Otherwise, there is sufficient room, so we can write directly to the buffer:
    wchar_t*& wide_stream_ptr = reinterpret_cast<wchar_t*&>(stream->_ptr);
    *wide_stream_ptr++ = c;

    return c;
}



extern "C" wint_t __cdecl _fputwc_nolock_internal(wchar_t const c, FILE* const public_stream, __crt_cached_ptd_host& ptd)
{
    __crt_stdio_stream const stream(public_stream);

    // If this stream is not file-backed, we can write the character directly:
    if (stream.is_string_backed())
    {
        return fputwc_binary_nolock(c, stream, ptd);
    }

    __crt_lowio_text_mode const text_mode = _textmode_safe(_fileno(stream.public_stream()));

    // If the file is open in a Unicode text mode, we can write the character
    // directly to the stream:
    if (text_mode == __crt_lowio_text_mode::utf16le || text_mode == __crt_lowio_text_mode::utf8)
    {
        return fputwc_binary_nolock(c, stream, ptd);
    }

    // If the file is open in binary mode, we can write the character directly
    // to the stream:
    if ((_osfile_safe(_fileno(stream.public_stream())) & FTEXT) == 0)
    {
        return fputwc_binary_nolock(c, stream, ptd);
    }

    // Otherwise, the file is open in ANSI text mode, and we need to translate
    // the wide character to multibyte before we can write it:
    char mbc[MB_LEN_MAX];

    int size;
    if (_wctomb_internal(&size, mbc, MB_LEN_MAX, c, ptd) != 0)
    {
        // If conversion failed, errno is set by wctomb_s and we return WEOF:
        return WEOF;
    }

    // Write the character, byte-by-byte:
    for (int i = 0; i < size; ++i)
    {
        if (_fputc_nolock_internal(mbc[i], stream.public_stream(), ptd) == EOF)
        {
            return WEOF;
        }
    }

    return c;
}

extern "C" wint_t __cdecl _fputwc_nolock(wchar_t const c, FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return _fputwc_nolock_internal(c, public_stream, ptd);
}

extern "C" wint_t __cdecl _putwc_nolock(wchar_t const c, FILE* const stream)
{
    return _fputwc_nolock(c, stream);
}



// Writes a wide character to a stream.  Returns the wide character on success;
// WEOF on failure.
static wint_t __cdecl _fputwc_internal(wchar_t const c, FILE* const stream, __crt_cached_ptd_host& ptd)
{
    _UCRT_VALIDATE_RETURN(ptd, stream != nullptr, EINVAL, WEOF);

    wint_t return_value = WEOF;

    _lock_file(stream);
    __try
    {
        return_value = _fputwc_nolock_internal(c, stream, ptd);
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}

extern "C" wint_t __cdecl fputwc(wchar_t const c, FILE* const stream)
{
    __crt_cached_ptd_host ptd;
    return _fputwc_internal(c, stream, ptd);
}



// Writes a wide character to a stream.  See fputwc() for details.
extern "C" wint_t __cdecl putwc(wchar_t const c, FILE* const stream)
{
    return fputwc(c, stream);
}



// Writes a wide character to stdout.  See fputwc() for details.
extern "C" wint_t __cdecl _fputwchar(wchar_t const c)
{
    return fputwc(c, stdout);
}



// Writes a wide character to stdout.  See fputwc() for details.
extern "C" wint_t __cdecl putwchar(wchar_t const c)
{
    return _fputwchar(c);
}
