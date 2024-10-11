//
// ungetc.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines ungetc(), which pushes a character back into a stream.
//
#include <corecrt_internal_stdio.h>



// Pushes a character ("ungets" it) back into a stream.  It is possible to push
// back one character.  It may not be possible to push back more than one
// character in a row.  Returns the pushed-back character on success; returns
// EOF on failure.  Ungetting EOF is expressly forbidden.
extern "C" int __cdecl ungetc(int const c, FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, EOF);

    int return_value = EOF;

    _lock_file(stream);
    __try
    {
        return_value = _ungetc_nolock(c, stream);
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}



extern "C" int __cdecl _ungetc_nolock(int const c, FILE* public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    _VALIDATE_STREAM_ANSI_RETURN(stream, EINVAL, EOF);

    // Ungetting EOF is expressly forbidden:
    if (c == EOF)
        return EOF;

    // The stream must either be open in read-only mode, or must be open in
    // read-write mode and must not currently be in write mode:
    bool const is_in_read_only_mode = stream.has_all_of(_IOREAD);
    bool const is_in_rw_write_mode =  stream.has_all_of(_IOUPDATE | _IOWRITE);

    if (!is_in_read_only_mode && !is_in_rw_write_mode)
        return EOF;

    // If the stream is currently unbuffered, buffer it:
    if (stream->_base == nullptr)
        __acrt_stdio_allocate_buffer_nolock(stream.public_stream());

    // At this point, we know that _base is not null, since the file is buffered.

    if (stream->_ptr == stream->_base)
    {
        // If we've already buffered a pushed-back character, there's no room for
        // another, and there's nothing we can do:
        if (stream->_cnt)
            return EOF;

        ++stream->_ptr;
    }

    // If the stream is string-backed (and not file-backed), do not modify the buffer:
    if (stream.is_string_backed())
    {
        --stream->_ptr;
        if (*stream->_ptr != static_cast<char>(c))
        {
            ++stream->_ptr;
            return EOF;
        }
    }
    else
    {
        --stream->_ptr;
        *stream->_ptr = static_cast<char>(c);
    }

    ++stream->_cnt;
    stream.unset_flags(_IOEOF);
    stream.set_flags(_IOREAD);

    return c & 0xff;
}
