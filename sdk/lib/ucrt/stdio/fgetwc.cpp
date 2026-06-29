//
// fgetwc.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that read the next wide character from a stream and return it.  If
// the read causes the stream to reach EOF, WEOF is returned and the EOF bit is
// set on the stream.
//
#include <corecrt_internal_stdio.h>



extern "C" wint_t __cdecl _fgetwc_nolock(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    // If the stream is backed by a real file and is open in a Unicode text mode,
    // we need to read two bytes (note that we read two bytes for both UTF-8 and
    // UTF-16 backed streams, since lowio translates UTF-8 to UTF-16 when we read.
    if (!stream.is_string_backed() &&
        _textmode_safe(_fileno(stream.public_stream())) != __crt_lowio_text_mode::ansi)
    {
        wchar_t wc;

        // Compose the wide character by reading byte-by-byte from the stream:
        char* const wc_first = reinterpret_cast<char*>(&wc);
        char* const wc_last  = wc_first + sizeof(wc);

        for (char* it = wc_first; it != wc_last; ++it)
        {
            int const c = _getc_nolock(stream.public_stream());
            if (c == EOF)
                return WEOF;

            *it = static_cast<char>(c);
        }

        return wc;
    }

    if (!stream.is_string_backed() &&
        (_osfile_safe(_fileno(stream.public_stream())) & FTEXT))
    {
        int size = 1;
        int ch;
        char mbc[4];
        wchar_t wch;

        /* text (multi-byte) mode */
        if ((ch = _getc_nolock(stream.public_stream())) == EOF)
            return WEOF;

        mbc[0] = static_cast<char>(ch);

        if (isleadbyte(static_cast<unsigned char>(mbc[0])))
        {
            if ((ch = _getc_nolock(stream.public_stream())) == EOF)
            {
                ungetc(mbc[0], stream.public_stream());
                return WEOF;
            }
            mbc[1] = static_cast<char>(ch);
            size = 2;
        }

        if (mbtowc(&wch, mbc, size) == -1)
        {
            // Conversion failed! Set errno and return failure:
            errno = EILSEQ;
            return WEOF;
        }

        return wch;
    }

    // binary (Unicode) mode
    if (stream->_cnt >= static_cast<int>(sizeof(wchar_t)))
    {
        stream->_cnt -= static_cast<int>(sizeof(wchar_t));
        return *reinterpret_cast<wchar_t*&>(stream->_ptr)++;
    }
    else
    {
        return static_cast<wint_t>(__acrt_stdio_refill_and_read_wide_nolock(stream.public_stream()));
    }
}



extern "C" wint_t __cdecl _getwc_nolock(FILE* const stream)
{
    return _fgetwc_nolock(stream);
}



extern "C" wint_t __cdecl fgetwc(FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, WEOF);

    wint_t return_value = 0;

    _lock_file(stream);
    __try
    {
        return_value = _fgetwc_nolock(stream);
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}



extern "C" wint_t __cdecl getwc(FILE* const stream)
{
    return fgetwc(stream);
}



extern "C" wint_t __cdecl _fgetwchar()
{
    return fgetwc(stdin);
}



extern "C" wint_t __cdecl getwchar()
{
    return _fgetwchar();
}
