//
// fwrite.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fwrite() and related functions, which write unformatted data to a
// stdio stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>

// Writes data from the provided buffer to the specified stream.  The function
// writes 'count' elements of 'size' size to the stream, and returns when
// either all of the elements have been written or no more data can be written
// (e.g. if EOF is encountered or an error occurs).
//
// Returns the number of "whole" elements that were written to the stream.  This
// may be fewer than the requested number of an error occurs or EOF is encountered.
// In this case, ferror() or feof() should be used to distinguish between the two
// conditions.
extern "C" size_t __cdecl _fwrite_internal(
    void const*        const buffer,
    size_t             const size,
    size_t             const count,
    FILE*              const stream,
    __crt_cached_ptd_host&   ptd
    )
{
    if (size == 0 || count == 0)
    {
        return 0;
    }

    // The _nolock version will do the rest of the validation.
    _UCRT_VALIDATE_RETURN(ptd, stream != nullptr, EINVAL, 0);

    return __acrt_lock_stream_and_call(stream, [&]() -> size_t
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        return _fwrite_nolock_internal(buffer, size, count, stream, ptd);
    });
}

extern "C" size_t __cdecl fwrite(
    void const* const buffer,
    size_t      const size,
    size_t      const count,
    FILE*       const stream
    )
{
    __crt_cached_ptd_host ptd;
    return _fwrite_internal(buffer, size, count, stream, ptd);
}

extern "C" size_t __cdecl _fwrite_nolock_internal(
    void const*        const buffer,
    size_t             const element_size,
    size_t             const element_count,
    FILE*              const public_stream,
    __crt_cached_ptd_host&   ptd
    )
{
    if (element_size == 0 || element_count == 0)
    {
        return 0;
    }

    __crt_stdio_stream const stream(public_stream);

    _UCRT_VALIDATE_RETURN(ptd, stream.valid(),                             EINVAL, 0);
    _UCRT_VALIDATE_RETURN(ptd, buffer != nullptr,                          EINVAL, 0);
    _UCRT_VALIDATE_RETURN(ptd, element_count <= (SIZE_MAX / element_size), EINVAL, 0);

    // Figure out how big the buffer is; if the stream doesn't currently have a
    // buffer, we assume that we'll get one with the usual internal buffer size:
    unsigned stream_buffer_size = stream.has_any_buffer()
        ? stream->_bufsiz
        : _INTERNAL_BUFSIZ;

    // The total number of bytes to be written to the stream:
    size_t const total_bytes = element_size * element_count;

    char const* data = static_cast<char const*>(buffer);

    // Write blocks of data from the buffer until there is no more data left:
    size_t remaining_bytes = total_bytes;
    while (remaining_bytes != 0)
    {
        // If the buffer is big and is not full, copy data into the buffer:
        if (stream.has_big_buffer() && stream->_cnt != 0)
        {
            if (stream->_cnt < 0)
            {
                _ASSERTE(("Inconsistent Stream Count. Flush between consecutive read and write", stream->_cnt >= 0));
                stream.set_flags(_IOERROR);
                return (total_bytes - remaining_bytes) / element_size;
            }

            if (stream.has_any_of(_IOREAD))
            {
                _ASSERTE(("Flush between consecutive read and write.", !stream.has_any_of(_IOREAD)));
                return (total_bytes - remaining_bytes) / element_size;
            }

            size_t const bytes_to_write = __min(remaining_bytes, static_cast<size_t>(stream->_cnt));

            memcpy(stream->_ptr, data, bytes_to_write);

            remaining_bytes -= bytes_to_write;
            stream->_cnt    -= static_cast<int>(bytes_to_write);
            stream->_ptr    += bytes_to_write;
            data            += bytes_to_write;
        }
        // If we have more than stream_buffer_size bytes to write, write data by
        // calling _write() with an integral number of stream_buffer_size blocks.
        else if (remaining_bytes >= stream_buffer_size)
        {
            // If we reach here and we have a big buffer, it must be full, so
            // flush it.  If the flush fails, there's nothing we can do to
            // recover:
            if (stream.has_big_buffer() && __acrt_stdio_flush_nolock(stream.public_stream(), ptd))
            {
                return (total_bytes - remaining_bytes) / element_size;
            }

            // Calculate the number of bytes to write.  The _write API takes a
            // 32-bit unsigned byte count and returns -1 (UINT_MAX) on failure,
            // so clamp the value to UINT_MAX - 1:
            size_t const max_bytes_to_write = stream_buffer_size > 0
                ? remaining_bytes - remaining_bytes % stream_buffer_size
                : remaining_bytes;

            unsigned const bytes_to_write = static_cast<unsigned>(__min(max_bytes_to_write, UINT_MAX - 1));

            unsigned const bytes_actually_written = _write_internal(_fileno(stream.public_stream()), data, bytes_to_write, ptd);
            if (bytes_actually_written == UINT_MAX) // UINT_MAX == -1
            {
                stream.set_flags(_IOERROR);
                return (total_bytes - remaining_bytes) / element_size;
            }

            // VSWhidbey #326224:  _write can return more bytes than we requested
            // due to Unicode conversions in text files.  We do not care how many
            // bytes were written as long as the number is as least as large as we
            // requested:
            unsigned const bytes_written = bytes_actually_written > bytes_to_write
                ? bytes_to_write
                : bytes_actually_written;

            // Update the remaining bytes and data to reflect the write:
            remaining_bytes -= bytes_written;
            data            += bytes_written;

            if (bytes_actually_written < bytes_to_write)
            {
                stream.set_flags(_IOERROR);
                return (total_bytes - remaining_bytes) / element_size;
            }
        }
        // Otherwise, the stream does not have a buffer, or the buffer is full
        // and there are not enough characters to do a direct write, so use
        // __acrt_stdio_flush_and_write_narrow_nolock:
        else
        {
            // Write the first character.  If this fails, there is nothing we can
            // do.  (Note that if this fails, it will update the stream error state.)
            if (__acrt_stdio_flush_and_write_narrow_nolock(*data, stream.public_stream(), ptd) == EOF)
            {
                return (total_bytes - remaining_bytes) / element_size;
            }

            // Update the remaining bytes to account for the byte we just wrote:
            ++data;
            --remaining_bytes;

            stream_buffer_size = stream->_bufsiz > 0
                ? stream->_bufsiz
                : 1;
        }
    }

    return element_count; // Success!
}

extern "C" size_t __cdecl _fwrite_nolock(
    void const* const buffer,
    size_t      const element_size,
    size_t      const element_count,
    FILE*       const public_stream
    )
{
    __crt_cached_ptd_host ptd;
    return _fwrite_nolock_internal(buffer, element_size, element_count, public_stream, ptd);
}

