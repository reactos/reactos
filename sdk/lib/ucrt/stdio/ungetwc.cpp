//
// ungetwc.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines ungetwc(), which pushes a wide character back into a stream.
//
#include <corecrt_internal_stdio.h>



// Pushes a character ("ungets" it) back into a stream.  It is possible to push
// back one character.  It may not be possible to push back more than one
// character in a row.  Returns the pushed-back character on success; returns
// WEOF on failure.  Ungetting WEOF is expressly forbidden.
extern "C" wint_t __cdecl ungetwc(wint_t const c, FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, WEOF);

    wint_t return_value = WEOF;

    _lock_file(stream);
    __try
    {
        return_value = _ungetwc_nolock(c, stream);
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}



// Helper function for _ungetwc_nolock() that handles text mode ungetting.
static wint_t __cdecl ungetwc_text_mode_nolock(wint_t const c, __crt_stdio_stream const stream) throw()
{
    // The stream is open in text mode, and we need to do the unget differently
    // depending on whether the stream is open in ANSI or Unicode mode.
    __crt_lowio_text_mode const text_mode = _textmode_safe(_fileno(stream.public_stream()));

    int  count = 0;
    char characters[MB_LEN_MAX] = { 0 };

    // If the file is open in ANSI mode, we need to convert the wide character
    // to multibyte so that we can unget the multibyte character back into the
    // stream:
    if (text_mode == __crt_lowio_text_mode::ansi)
    {
        // If conversion fails, errno is set by wctomb_s and we can just return:
        if (wctomb_s(&count, characters, MB_LEN_MAX, c) != 0)
            return WEOF;
    }
    // Otherwise, the file is open in Unicode mode.  This means the characters
    // in the stream were originally Unicode (and not multibyte).  Hence, we
    // do not need to translate back to multibyte.  This is true for both UTF-16
    // and UTF-8, because the lowio read converts UTF-8 data to UTF-16.
    else
    {
        char const* c_bytes = reinterpret_cast<char const*>(&c);
        characters[0] = c_bytes[0];
        characters[1] = c_bytes[1];
        count = 2;
    }

    // At this point, the file must be buffered, so we know the base is non-null.
    // First we need to ensure there is sufficient room in the buffer to store
    // the translated data:
    if (stream->_ptr < stream->_base + count)
    {
        if (stream->_cnt)
            return WEOF;

        if (count > stream->_bufsiz)
            return WEOF;

        stream->_ptr = count + stream->_base;
    }

    for (int i = count - 1; i >= 0; --i)
    {
        *--stream->_ptr = characters[i];
    }

    stream->_cnt += count;

    stream.unset_flags(_IOEOF);
    stream.set_flags(_IOREAD);
    return static_cast<wint_t>(0xffff & c);
}



// Helper function for _ungetwc_nolock() that handles binary mode ungetting
static wint_t __cdecl ungetwc_binary_mode_nolock(wint_t const c, __crt_stdio_stream const stream) throw()
{
    wchar_t const wide_c = static_cast<wchar_t>(c);

    // At this point, the file must be buffered, so we know the base is non-null.
    // First, we need to ensure there is sufficient room in the buffer to store
    // the character:
    if (stream->_ptr < stream->_base + sizeof(wchar_t))
    {
        // If we've already ungotten one character and it has not yet been read,
        // there may not be room for this unget.  In this case, there's nothing
        // we can do so we simply fail:
        if (stream->_cnt)
            return WEOF;

        if (sizeof(wchar_t) > stream->_bufsiz)
            return WEOF;

        stream->_ptr = sizeof(wchar_t) + stream->_base;
    }

    wchar_t*& wide_stream_ptr = reinterpret_cast<wchar_t*&>(stream->_ptr);

    // If the stream is string-backed, we cannot modify the buffer.  We retreat
    // the stream pointer and test if the character being ungotten is the same
    // as the character that was last read.  If they are the same, then we allow
    // the unget (because we don't have to modify the buffer).  If they are not
    // the same, then we re-advance the stream pointer and fail:
    if (stream.is_string_backed())
    {
        if (*--wide_stream_ptr != wide_c)
        {
            ++wide_stream_ptr;
            return WEOF;
        }
    }
    // Otherwise, the stream is file-backed and open in binary mode, and we can
    // just write the character to the front of the stream:
    else
    {
        *--wide_stream_ptr = wide_c;
    }

    stream->_cnt += sizeof(wchar_t);

    stream.unset_flags(_IOEOF);
    stream.set_flags(_IOREAD);

    return static_cast<wint_t>(wide_c);
}



extern "C" wint_t __cdecl _ungetwc_nolock(wint_t const c, FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    // Ungetting WEOF is expressly forbidden:
    if (c == WEOF)
        return WEOF;

    // To unget, the stream must currently be in read mode, _or_ it must be open
    // for update (read and write) and must not _currently_ be in write mode:
    bool const is_in_read_mode   = stream.has_all_of(_IOREAD);
    bool const is_in_update_mode = stream.has_all_of(_IOUPDATE);
    bool const is_in_write_mode  = stream.has_all_of(_IOWRITE);

    if (!is_in_read_mode && !(is_in_update_mode && !is_in_write_mode))
        return WEOF;

    // If the stream is currently unbuffered, buffer it:
    if (stream->_base == nullptr)
        __acrt_stdio_allocate_buffer_nolock(stream.public_stream());

    // If the stream is file-backed and is open in text mode, we need to perform
    // text mode translations:
    if (!stream.is_string_backed() && (_osfile_safe(_fileno(stream.public_stream())) & FTEXT) != 0)
    {
        return ungetwc_text_mode_nolock(c, stream);
    }

    // Otherwise, the stream is string-backed or is a file-backed file open in
    // binary mode; we can simply push the character back into the stream:
    return ungetwc_binary_mode_nolock(c, stream);
}
