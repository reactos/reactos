//
// _filbuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that re-fill a stdio stream buffer and return the next character.
//
#include <corecrt_internal_stdio.h>



namespace {

    struct filwbuf_context
    {
        bool          _is_split_character;
        unsigned char _leftover_low_order_byte;
    };

}



// These functions store the pre-_read() state of the stream so that it can be
// used later when we read a character from the newly-filled buffer.
static int get_context_nolock(__crt_stdio_stream const, char) throw()
{
    return 0;
}

static filwbuf_context get_context_nolock(__crt_stdio_stream const stream, wchar_t) throw()
{
    // When reading wide character elements, we must handle the case where a two
    // byte character straddles the buffer boundary, with the low order byte at
    // the end of the old buffer and the high order byte at the start of the new
    // buffer.
    //
    // We do this here:  if there is exactly one character left in the buffer, we
    // store that and set a flag so we know to pick it up later.
    filwbuf_context context;
    if (stream->_cnt == 1)
    {
        context._is_split_character = true;
        context._leftover_low_order_byte = static_cast<unsigned char>(*stream->_ptr);
    }
    else
    {
        context._is_split_character = false;
        context._leftover_low_order_byte = 0;
    }
    return context;
}


// These functions test whether a buffer is valid following a call to _read().
static bool is_buffer_valid_nolock(__crt_stdio_stream const stream, char) throw()
{
    return stream->_cnt !=  0
        && stream->_cnt != -1;
}

static bool is_buffer_valid_nolock(__crt_stdio_stream const stream, wchar_t) throw()
{
    return stream->_cnt !=  0
        && stream->_cnt !=  1
        && stream->_cnt != -1;
}



// These functions read a character from the stream after the _read() has
// completed successfully.
static unsigned char read_character_nolock(__crt_stdio_stream const stream, int, char) throw()
{
    --stream->_cnt;
    return static_cast<unsigned char>(*stream->_ptr++);
}



static wchar_t read_character_nolock(
    __crt_stdio_stream const stream,
    filwbuf_context    const context,
    wchar_t
    ) throw()
{
    if (context._is_split_character)
    {
        // If the character was split across buffers, we read only one byte
        // from the new buffer and or it with the leftover byte from the old
        // buffer.
        unsigned char high_order_byte = static_cast<unsigned char>(*stream->_ptr);
        wchar_t result = (high_order_byte << 8) | context._leftover_low_order_byte;

        --stream->_cnt;
        ++stream->_ptr;
        return (result);
    }
    else
    {
        wchar_t const result = 0xffff & reinterpret_cast<wchar_t const&>(*stream->_ptr);

        stream->_cnt -= sizeof(wchar_t);
        stream->_ptr += sizeof(wchar_t);

        return result;
    }
}



// Fills a buffer and reads the first character.  Allocates a buffer for the
// stream if the stream does not yet have one.  This function is intended for
// internal usage only.  This function assumes that the caller has acquired
// the lock for the stream.
//
// Returns the first character from the new buffer.  For the wide character
// version, the case is handled where a character straddles the old and new
// buffers.  Returns EOF if the file is string-backed or is not open for
// reading, or if there are no more characters to be read.
template <typename Character>
static int __cdecl common_refill_and_read_nolock(__crt_stdio_stream const stream) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _VALIDATE_RETURN(stream.valid(), EINVAL, stdio_traits::eof);

    if (!stream.is_in_use() || stream.is_string_backed())
        return stdio_traits::eof;

    if (stream.has_all_of(_IOWRITE))
    {
        stream.set_flags(_IOERROR);
        return stdio_traits::eof;
    }

    stream.set_flags(_IOREAD);

    // Get a buffer, if necessary:
    if (!stream.has_any_buffer())
        __acrt_stdio_allocate_buffer_nolock(stream.public_stream());

    auto const context = get_context_nolock(stream, Character());

    stream->_ptr = stream->_base;
    stream->_cnt = _read(_fileno(stream.public_stream()), stream->_base, stream->_bufsiz);

    if (!is_buffer_valid_nolock(stream, Character()))
    {
        stream.set_flags(stream->_cnt != 0 ? _IOERROR : _IOEOF);
        stream->_cnt = 0;
        return stdio_traits::eof;
    }

    if (!stream.has_any_of(_IOWRITE | _IOUPDATE) &&
        ((_osfile_safe(_fileno(stream.public_stream())) & (FTEXT | FEOFLAG)) == (FTEXT | FEOFLAG)))
    {
        stream.set_flags(_IOCTRLZ);
    }

    // Check for small _bufsiz (_SMALL_BUFSIZ). If it is small and if it is our
    // buffer, then this must be the first call to this function after an fseek
    // on a read-access-only stream. Restore _bufsiz to its larger value
    // (_INTERNAL_BUFSIZ) so that the next call to this function, if one is made,
    // will fill the whole buffer.
    if (stream->_bufsiz == _SMALL_BUFSIZ &&
        stream.has_crt_buffer() &&
        !stream.has_all_of(_IOBUFFER_SETVBUF))
    {
        stream->_bufsiz = _INTERNAL_BUFSIZ;
    }

    return read_character_nolock(stream, context, Character());
}



extern "C" int __cdecl __acrt_stdio_refill_and_read_narrow_nolock(FILE* const stream)
{
    return common_refill_and_read_nolock<char>(__crt_stdio_stream(stream));
}



extern "C" int __cdecl __acrt_stdio_refill_and_read_wide_nolock(FILE* const stream)
{
    return common_refill_and_read_nolock<wchar_t>(__crt_stdio_stream(stream));
}
