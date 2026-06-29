//
// ftell.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the ftell() family of functions, which computes the current position
// of the file pointer of a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>


static bool __cdecl buffer_contains_wide_characters(__crt_lowio_text_mode const text_mode) throw()
{
    return text_mode == __crt_lowio_text_mode::utf8
        || text_mode == __crt_lowio_text_mode::utf16le;
}

static size_t __cdecl buffer_character_size(__crt_lowio_text_mode const text_mode) throw()
{
    return buffer_contains_wide_characters(text_mode) ? sizeof(wchar_t) : sizeof(char);
}

// This function counts the number of newlines (LFs) in the buffer that contains
// elements of type Character.  When sizeof(Character) != 1, the caller must
// ensure that the buffer consists of a whole number of Character elements.
template <typename Character>
static __int64 __cdecl count_newlines_of_type(
    _In_reads_(buffer_last - buffer_first) char const* const buffer_first,
    _In_reads_(0)                          char const* const buffer_last
    ) throw()
{
    // This invariant is maintained by the lowio library.  When Character==wchar_t,
    // all writes to the buffer are in wide characters.
    _ASSERTE((buffer_last - buffer_first) % sizeof(Character) == 0);

    Character const* const typed_first = reinterpret_cast<Character const*>(buffer_first);
    Character const* const typed_last  = reinterpret_cast<Character const*>(buffer_last);

    __int64 newline_count = 0;
    for (Character const* it = typed_first; it != typed_last; ++it)
    {
        if (*it == '\n')
        {
            ++newline_count;
        }
    }

    return newline_count;
}

static __int64 __cdecl count_newline_bytes(
    _In_reads_(buffer_last - buffer_first) char const*           const buffer_first,
    _In_reads_(0)                          char const*           const buffer_last,
    _In_                                   __crt_lowio_text_mode const text_mode
    ) throw()
{
    if (buffer_contains_wide_characters(text_mode))
    {
        return count_newlines_of_type<wchar_t>(buffer_first, buffer_last) * sizeof(wchar_t);
    }
    else
    {
        return count_newlines_of_type<char>(buffer_first, buffer_last);
    }
}

// This function handles the case where the file is open in UTF-8 text mode and
// the translated UTF-16 form of the text has a different number of characters
// than the original UTF-8 text (remember: when reading a file in UTF-8 mode, the
// lowio library converts the UTF-8 to UTF-16).
static __int64 __cdecl common_ftell_translated_utf8_nolock(
    __crt_stdio_stream const stream,
    __int64            const lowio_position,
    __crt_cached_ptd_host&   ptd
    ) throw()
{
    int const fh = _fileno(stream.public_stream());

    // If the buffer has been exhausted, then the current lowio position is also
    // the current stdio position:
    if (stream->_cnt == 0)
    {
        return lowio_position;
    }

    __int64 const current_buffer_position = (stream->_ptr - stream->_base) / static_cast<__int64>(sizeof(wchar_t));

    // Otherwise, we have to re-read the buffer, in binary mode, so that we can
    // analyze the original UTF-8 text to compute the actual position in the
    // file.  To do this, we seek the lowio pointer back to the beginning of
    // the stdio buffer, re-read the buffer, then seek the lowio pointer back
    // to its original location:
    __int64 const base_buffer_position = _lseeki64_internal(fh, _startpos(fh), SEEK_SET, ptd);
    if (base_buffer_position != _startpos(fh))
    {
        return -1;
    }

    DWORD bytes_read;
    char  raw_buffer[_INTERNAL_BUFSIZ];
    if (!ReadFile(reinterpret_cast<HANDLE>(_osfhnd(fh)), raw_buffer, _INTERNAL_BUFSIZ, &bytes_read, nullptr))
        return -1;

    // Seek back to where we were, to ensure the stdio stream is left in a
    // consistent state (and "unmodified" from before the call):
    if (_lseeki64_internal(fh, lowio_position, SEEK_SET, ptd) < 0)
    {
        return -1;
    }

    // This should not normally happen:  we should always read enough bytes:
    if (current_buffer_position > static_cast<__int64>(bytes_read))
    {
        return -1;
    }

    // Scan the raw, untranslated buffer to find the current position, updating
    // the file pointer to account for newline translation in the buffer:
    char const* const raw_first = raw_buffer;
#pragma warning(disable:__WARNING_UNUSED_POINTER_ASSIGNMENT) // 28930
    char const* const raw_last = raw_buffer + bytes_read;

    char const* raw_it = raw_first;
    for (__int64 i = 0; i != current_buffer_position && raw_it < raw_last; ++i, ++raw_it)
    {
        if (*raw_it == CR)
        {
            if (raw_it < raw_last - 1 && *(raw_it + 1) == LF)
                ++raw_it;
        }
        else
        {
            raw_it += _utf8_no_of_trailbytes(static_cast<const unsigned char>(*raw_it));
        }
    }

    return base_buffer_position + (raw_it - raw_first);
}



// This function handles the extra adjustments that need to be made to the file
// position returned by ftell when a stream is opened in read mode.
static __int64 __cdecl common_ftell_read_mode_nolock(
    __crt_stdio_stream const stream,
    __int64            const lowio_position,
    __int64            const buffer_offset,
    __crt_cached_ptd_host&   ptd
    ) throw()
{
    int const fh = _fileno(stream.public_stream());

    // We will need to adjust the file position of UTF-8 files to account for
    // UTF-8 to UTF-16 translation:
    __crt_lowio_text_mode const text_mode = _textmode(fh);

    __int64 const translation_factor = text_mode == __crt_lowio_text_mode::utf8
        ? static_cast<__int64>(sizeof(wchar_t))
        : static_cast<__int64>(sizeof(char));

    // If the buffer has been exhausted, then the current lowio position is also
    // the current stdio position:
    if (stream->_cnt == 0)
    {
        return lowio_position;
    }

    // The lowio position points one-past-the-end of the current stdio buffer.
    // We need to find the position of the beginning of the buffer.  To start,
    // we compute the number of bytes in the buffer.  Note that we cannot just
    // use the buffer size, because the buffer will not be full if EOF is
    // readhed before the buffer is full.
    __int64 bytes_read = stream->_cnt + static_cast<__int64>(stream->_ptr - stream->_base);

    // If this is a binary mode stream, we can simply subtract this from the
    // lowio position, and combine it with the buffer offset to get the stdio
    // position:
    if ((_osfile(fh) & FTEXT) == 0)
    {
        return lowio_position
            - (bytes_read    / translation_factor)
            + (buffer_offset / translation_factor);
    }

    // If this is a text mode stream, we need to adjust the number of bytes that
    // were read into the buffer to account for newline translation.
    //
    // If we are _not_ at EOF, the number of untranslated characters read is the
    // buffer size.  However, if we are not at EOF, the buffer may not be full,
    // so we need to scan the buffer to count newline characters.  (Note:  we
    // only count newline characters if the stream is at EOF, because doing so
    // is more expensive than seeking to the end and seeking back).

    // Seek to the end of the file.  If the current position is the end of the
    // file, then scan the buffer for newlines and adjust bytes_read:
    if (_lseeki64_internal(fh, 0, SEEK_END, ptd) == lowio_position)
    {
        bytes_read += count_newline_bytes(stream->_base, stream->_base + bytes_read, text_mode);

        // If the last byte was a ^Z, that character will not be present in the
        // buffer (it is omitted by lowio):
        if (stream.ctrl_z())
        {
            bytes_read += buffer_character_size(text_mode);
        }
    }
    // Otherwise, the current position is not at the end of the file; we need to
    // seek back to the original position and compute the size of the buffer:
    else
    {
        if (_lseeki64_internal(fh, lowio_position, SEEK_SET, ptd) == -1)
            return -1;

        // If the number of bytes read is smaller than the small buffer and was
        // not user-provided, the buffer size was set to _SMALL_BUFSIZ during
        // the last call to __acrt_stdio_refill_and_read_{narrow,wide}_nolock:
        if (bytes_read <= _SMALL_BUFSIZ &&
            stream.has_crt_buffer() &&
            !stream.has_setvbuf_buffer())
        {
            bytes_read = _SMALL_BUFSIZ;
        }
        // Otherwise, the buffer size is what is stated in the stream object:
        else
        {
            bytes_read = stream->_bufsiz;
        }

        // If the first byte in the untranslated buffer was a '\n', we assume it
        // was preceded by a '\r', which was discarded by the previous read
        // operation:
        if (_osfile(fh) & FCRLF)
        {
            bytes_read += buffer_character_size(text_mode);
        }
    }

    return lowio_position
        - (bytes_read    / translation_factor)
        + (buffer_offset / translation_factor);
}



template <typename Integer>
static Integer __cdecl common_ftell_nolock(__crt_stdio_stream, __crt_cached_ptd_host& ptd) throw();

template <>
__int64 __cdecl common_ftell_nolock(__crt_stdio_stream const stream, __crt_cached_ptd_host& ptd) throw()
{
    _UCRT_VALIDATE_RETURN(ptd, stream.public_stream(), EINVAL, -1);

    int const fh = _fileno(stream.public_stream());

    if (stream->_cnt < 0)
    {
        stream->_cnt = 0;
    }

    // Get the current lowio file position.  If stdio is buffering the stream,
    // this position will point one past the end of the current stdio buffer.
    __int64 const lowio_position = _lseeki64_internal(fh, 0, SEEK_CUR, ptd);
    if (lowio_position < 0)
    {
        return -1;
    }

    // If the stream is unbuffered or no buffering is designated, we can simply
    // compute the stdio position via the remaining stdio stream count:
    if (!stream.has_big_buffer())
    {
        return lowio_position - stream->_cnt;
    }

    // The above lseek validates the handle, so it's okay to get the text mode:
    __crt_lowio_text_mode const text_mode = _textmode(fh);

    // This is the current offset into the stdio buffer; we will adjust this to
    // account for translation and updates as this function progresses:
    __int64 buffer_offset = stream->_ptr - stream->_base;

    // If the file is in read or write mode, we need special handling for UTF-8
    // and text mode files, to account for newline translation and UTF-8 to
    // UTF-16 conversion:
    if (stream.has_any_of(_IOWRITE | _IOREAD))
    {
        if (text_mode == __crt_lowio_text_mode::utf8 && _utf8translations(fh))
        {
            return common_ftell_translated_utf8_nolock(stream, lowio_position, ptd);
        }

        // For text mode files, adjust the buffer offset to account for newline
        // translation:
        if (_osfile(fh) & FTEXT)
        {
            buffer_offset += count_newline_bytes(stream->_base, stream->_ptr, text_mode);
        }
    }
    // Otherwise, if the file is not in read/write mode, ftell cannot proceed:
    else if (!stream.has_all_of(_IOUPDATE))
    {
        ptd.get_errno().set(EINVAL);
        return -1;
    }

    // If the current lowio position is at the beginning of the file, the stdio
    // position is whatever the offset is:
    if (lowio_position == 0)
    {
        return buffer_offset;
    }

    if (stream.has_all_of(_IOREAD))
    {
        return common_ftell_read_mode_nolock(stream, lowio_position, buffer_offset, ptd);
    }

    if (text_mode == __crt_lowio_text_mode::utf8)
    {
        buffer_offset /= sizeof(wchar_t);
    }

    return lowio_position + buffer_offset;
}

template <>
long __cdecl common_ftell_nolock(__crt_stdio_stream const stream, __crt_cached_ptd_host& ptd) throw()
{
    __int64 const result = common_ftell_nolock<__int64>(stream, ptd);
    if (result > LONG_MAX)
    {
        ptd.get_errno().set(EINVAL);
        return -1;
    }

    return static_cast<long>(result);
}



// Queries the position of the file pointer of a stream.  This function computes
// the position of the pointer, accounting for stdio buffering.  This is not the
// same as fseek(stream, 0, SEEK_SET), because fseek will remove an ungetc and
// may flush buffers.
//
// Returns the present file position on success; returns -1 and sets errno on
// failure.
template <typename Integer>
static Integer __cdecl common_ftell(__crt_stdio_stream const stream, __crt_cached_ptd_host& ptd) throw()
{
    _UCRT_VALIDATE_RETURN(ptd, stream.valid(), EINVAL, -1);

    Integer return_value = 0;

    _lock_file(stream.public_stream());
    __try
    {
        return_value = common_ftell_nolock<Integer>(stream, ptd);
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return return_value;
}




extern "C" long __cdecl ftell(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return common_ftell<long>(__crt_stdio_stream(public_stream), ptd);
}

extern "C" long __cdecl _ftell_nolock(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return common_ftell_nolock<long>(__crt_stdio_stream(public_stream), ptd);
}

extern "C" __int64 __cdecl _ftelli64(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return common_ftell<__int64>(__crt_stdio_stream(public_stream), ptd);
}

extern "C" __int64 __cdecl _ftelli64_nolock(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return common_ftell_nolock<__int64>(__crt_stdio_stream(public_stream), ptd);
}

extern "C" __int64 __cdecl _ftelli64_nolock_internal(FILE* const public_stream, __crt_cached_ptd_host& ptd)
{
    return common_ftell_nolock<__int64>(__crt_stdio_stream(public_stream), ptd);
}
