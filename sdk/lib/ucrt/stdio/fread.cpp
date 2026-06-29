//
// fread.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fread() and related functions, which read unformatted data from a
// stdio stream.
//
#include <corecrt_internal_stdio.h>

#ifdef _DEBUG
    #define _BUFFER_FILL_PATTERN _SECURECRT_FILL_BUFFER_PATTERN
#else
    #define _BUFFER_FILL_PATTERN 0
#endif



// Reads data from a stream into the result buffer.  The function reads elements
// of size 'element_size' until it has read 'element_count' elements, until the
// buffer is full, or until EOF is reached.
//
// Returns the number of "whole" elements that were read into the buffer.  This
// may be fewer than the requested number of elements if an error occurs or if
// EOF is encountered.  In this case, ferror() or feof() should be used to
// distinguish between the two conditions.
//
// If the result buffer becomes full before the requested number of elements are
// read, the buffer is zero-filled, zero is returned, and errno is set to ERANGE.
extern "C" size_t __cdecl fread_s(
    void*  const buffer,
    size_t const buffer_size,
    size_t const element_size,
    size_t const element_count,
    FILE*  const stream
    )
{
    if (element_size == 0 || element_count == 0)
        return 0;

    // The rest of the argument validation is done in the _nolock function.  Here
    // we only need to validate that the stream is non-null before we lock it.
    if (stream == nullptr)
    {
        if (buffer_size != _CRT_UNBOUNDED_BUFFER_SIZE)
            memset(buffer, _BUFFER_FILL_PATTERN, buffer_size);

        _VALIDATE_RETURN(stream != nullptr, EINVAL, 0);
    }

    size_t return_value = 0;

    _lock_file(stream);
    __try
    {
        return_value = _fread_nolock_s(buffer, buffer_size, element_size, element_count, stream);
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}



extern "C" size_t __cdecl _fread_nolock_s(
    void*  const buffer,
    size_t const buffer_size,
    size_t const element_size,
    size_t const element_count,
    FILE*  const public_stream
    )
{
    __crt_stdio_stream const stream(public_stream);

    if (element_size == 0 || element_count == 0)
        return 0;

    _VALIDATE_RETURN(buffer != nullptr, EINVAL, 0);
    if (!stream.valid() || element_count > (SIZE_MAX / element_size))
    {
        if (buffer_size != _CRT_UNBOUNDED_BUFFER_SIZE)
            memset(buffer, _BUFFER_FILL_PATTERN, buffer_size);

        _VALIDATE_RETURN(stream.valid(),                             EINVAL, 0);
        _VALIDATE_RETURN(element_count <= (SIZE_MAX / element_size), EINVAL, 0);
    }

    // Figure out how big the buffer is; if the stream doesn't currently have a
    // buffer, we assume that we'll get one with the usual internal buffer size:
    unsigned stream_buffer_size = stream.has_any_buffer()
        ? stream->_bufsiz
        : _INTERNAL_BUFSIZ;

    // The total number of bytes to be read into the buffer:
    size_t const total_bytes = element_size * element_count;

    char* data = static_cast<char*>(buffer);

    // Read blocks of data from the stream until we have read the requested
    // number of elements or we fill the buffer.
    size_t remaining_bytes = total_bytes;
    size_t remaining_buffer = buffer_size;
    while (remaining_bytes != 0)
    {
        // If the stream is buffered and has characters, copy them into the
        // result buffer:
        if (stream.has_any_buffer() && stream->_cnt != 0)
        {
            if(stream->_cnt < 0)
            {
                _ASSERTE(("Inconsistent Stream Count. Flush between consecutive read and write", stream->_cnt >= 0));
                stream.set_flags(_IOERROR);
                return (total_bytes - remaining_bytes) / element_size;
            }

            unsigned const bytes_to_read = remaining_bytes < static_cast<size_t>(stream->_cnt)
                ? static_cast<unsigned>(remaining_bytes)
                : static_cast<unsigned>(stream->_cnt);

            if (bytes_to_read > remaining_buffer)
            {
                if (buffer_size != _CRT_UNBOUNDED_BUFFER_SIZE)
                    memset(buffer, _BUFFER_FILL_PATTERN, buffer_size);

                _VALIDATE_RETURN(("buffer too small", 0), ERANGE, 0)
            }

            memcpy_s(data, remaining_buffer, stream->_ptr, bytes_to_read);

            // Update the stream and local tracking variables to account for the
            // read.  Note that the number of bytes actually read is always equal
            // to the number of bytes that we expected to read, because the data
            // was already buffered in the stream.
            remaining_bytes  -= bytes_to_read;
            stream->_cnt     -= bytes_to_read;
            stream->_ptr     += bytes_to_read;
            data             += bytes_to_read;
            remaining_buffer -= bytes_to_read;
        }
        // There is no data remaining in the stream buffer to be read, and we
        // need to read more data than will fit in the buffer (or we need to read
        // at least enough data to fill the buffer completely):
        else if (remaining_bytes >= stream_buffer_size)
        {
            // We can read at most INT_MAX bytes at a time.  This is a hard limit
            // of the lowio _read() function.
            unsigned const maximum_bytes_to_read = remaining_bytes > INT_MAX
                ? static_cast<unsigned>(INT_MAX)
                : static_cast<unsigned>(remaining_bytes);

            // If the stream has a buffer, we want to read the largest chunk that
            // is a multiple of the buffer size, to keep the stream buffer state
            // consistent.  If the stream is not buffered, we can read the maximum
            // number of bytes that we can:
            unsigned const bytes_to_read = stream_buffer_size != 0
                ? static_cast<unsigned>(maximum_bytes_to_read - maximum_bytes_to_read % stream_buffer_size)
                : maximum_bytes_to_read;

            if (bytes_to_read > remaining_buffer)
            {
                if (buffer_size != _CRT_UNBOUNDED_BUFFER_SIZE)
                    memset(buffer, _BUFFER_FILL_PATTERN, buffer_size);

                _VALIDATE_RETURN(("buffer too small", 0), ERANGE, 0)
            }

            // We are about to read data directly from the underlying file
            // descriptor, bypassing the stream buffer.  We reset the stream
            // buffer state to ensure that future seeks do not incorrectly
            // assume that the buffer contents are valid.
            __acrt_stdio_reset_buffer(stream);

            // Do the read.  Note that if the stream is open in text mode, the
            // bytes_read may not be the same as the bytes_to_read, due to
            // newline translation.
            int const bytes_read = _read_nolock(_fileno(stream.public_stream()), data, bytes_to_read);
            if (bytes_read == 0)
            {
                // We encountered EOF:
                stream.set_flags(_IOEOF);
                return (total_bytes - remaining_bytes) / element_size;
            }
            else if (bytes_read < 0)
            {
                // The _read failed:
                stream.set_flags(_IOERROR);
                return (total_bytes - remaining_bytes) / element_size;
            }

            // Update the iteration state to reflect the read:
            remaining_bytes  -= bytes_read;
            data             += bytes_read;
            remaining_buffer -= bytes_read;
        }
        // Otherwise, the stream does not have a buffer, or the stream buffer
        // is full and there is insufficient space to do a direct read, so use
        // __acrt_stdio_refill_and_read_narrow_nolock:
        else
        {
            int const c = __acrt_stdio_refill_and_read_narrow_nolock(stream.public_stream());
            if (c == EOF)
                return (total_bytes - remaining_bytes) / element_size;

            // If we have filled the result buffer before we have read the
            // requested number of elements or reached EOF, it is an error:
            if (remaining_buffer == 0)
            {
                if (buffer_size != _CRT_UNBOUNDED_BUFFER_SIZE)
                    memset(buffer, _BUFFER_FILL_PATTERN, buffer_size);

                _VALIDATE_RETURN(("buffer too small", 0), ERANGE, 0)
            }

            *data++ = static_cast<char>(c);
            --remaining_bytes;
            --remaining_buffer;

            stream_buffer_size = stream->_bufsiz;
        }
    }

    return element_count; // Success!
}



extern "C" size_t __cdecl fread(
    void*  const buffer,
    size_t const element_size,
    size_t const element_count,
    FILE*  const stream
    )
{
    // Assume there is enough space in the destination buffer
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015 - fread is unsafe
    return fread_s(buffer, _CRT_UNBOUNDED_BUFFER_SIZE, element_size, element_count, stream);
}



extern "C" size_t __cdecl _fread_nolock(
    void*  const buffer,
    size_t const element_size,
    size_t const element_count,
    FILE*  const stream
    )
{
    // Assume there is enough space in the destination buffer
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015 - _fread_nolock is unsafe
    return _fread_nolock_s(buffer, _CRT_UNBOUNDED_BUFFER_SIZE, element_size, element_count, stream);
}
