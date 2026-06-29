//
// _sftbuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines functions that enable and disable temporary buffering and flushing
// for stdout and stderr.  When __acrt_stdio_begin_temporary_buffering_nolock()
// is called for one of these streams, it tests whether the stream is buffered.
// If the stream is not buffered, it gives it a temporary buffer, so that we can
// avoid making sequences of one-character writes.
//
// __acrt_stdio_end_temporary_buffering_nolock() must be called to disable
// temporary buffering when it is no longer needed.  This function flushes the
// stream before tearing down the buffer.
//
// Note that these functions are only to be used for output streams--note that
// __acrt_stdio_begin_temporary_buffering_nolock() sets the _IOWRITE flag.  These
// functions are intended for internal library use only.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Buffer pointers for stdout and stderr
extern "C" { void* __acrt_stdout_buffer = nullptr; }
extern "C" { void* __acrt_stderr_buffer = nullptr; }

// The temporary buffer has the data of one stdio call. Stderr and Stdout use a
// temporary buffer for the duration of stdio output calls instead of having a
// full buffer and write after flush. The temporary buffer prevents the stdio
// functions from writing to the disk more than once per call when the stream is
// unbuffered (except when _IONBF is specified).
bool __acrt_should_use_temporary_buffer(FILE* const stream)
{
    if (stream == stderr)
    {
        return true;
    }

    if (stream == stdout && _isatty(_fileno(stream)))
    {
        return true;
    }

    return false;
}


// Sets a temporary buffer if necessary (see __acrt_should_use_temporary_buffer).
// On success, the buffer is initialized for the stream and 1 is returned. On
// failure, 0 is returned. The temporary buffer ensures that only one write to
// the disk occurs per stdio output call.
extern "C" bool __cdecl __acrt_stdio_begin_temporary_buffering_nolock(
    FILE* const public_stream
    )
{
    _ASSERTE(public_stream != nullptr);

    __crt_stdio_stream const stream(public_stream);

    if (!__acrt_should_use_temporary_buffer(stream.public_stream()))
    {
        return false;
    }

    void** buffer;
    if (stream.public_stream() == stdout)
    {
        buffer = &__acrt_stdout_buffer;
    }
    else if (stream.public_stream() == stderr)
    {
        buffer = &__acrt_stderr_buffer;
    }
    else
    {
        return false;
    }

    #ifndef CRTDLL
    _cflush++; // Force library pre-termination procedure to run
    #endif

    // Make sure the stream is not already buffered:
    if (stream.has_any_buffer())
    {
        return false;
    }

    stream.set_flags(_IOWRITE | _IOBUFFER_USER | _IOBUFFER_STBUF);
    if (*buffer == nullptr)
    {
        *buffer = _malloc_crt_t(char, _INTERNAL_BUFSIZ).detach();
    }

    if (*buffer == nullptr)
    {
        // If we failed to allocate a buffer, use the small character buffer:
        stream->_base   = reinterpret_cast<char*>(&stream->_charbuf);
        stream->_ptr    = reinterpret_cast<char*>(&stream->_charbuf);
        stream->_cnt    = 2;
        stream->_bufsiz = 2;
        return true;
    }

    // Otherwise, we have a new buffer, so set it up for use:
    stream->_base   = reinterpret_cast<char*>(*buffer);
    stream->_ptr    = reinterpret_cast<char*>(*buffer);
    stream->_cnt    = _INTERNAL_BUFSIZ;
    stream->_bufsiz = _INTERNAL_BUFSIZ;
    return true;
}



// If the stream currently has a temporary buffer that was set via a call to
// __acrt_stdio_begin_temporary_buffering_nolock(), and if the flag value is
// 1, this function flushes the stream and disables buffering of the stream.
extern "C" void __cdecl __acrt_stdio_end_temporary_buffering_nolock(
    bool               const flag,
    FILE*              const public_stream,
    __crt_cached_ptd_host&   ptd
    )
{
    __crt_stdio_stream const stream(public_stream);

    if (!flag)
        return;

    if (stream.has_temporary_buffer())
    {
        // Flush the stream and tear down temporary buffering:
        __acrt_stdio_flush_nolock(stream.public_stream(), ptd);
        stream.unset_flags(_IOBUFFER_USER | _IOBUFFER_STBUF);
        stream->_bufsiz = 0;
        stream->_base   = nullptr;
        stream->_ptr    = nullptr;
    }

    // Note: If we expand the functionality of the _IOBUFFER_STBUF bit to
    // include other streams, we may want to clear that bit here under an
    // 'else' clause (i.e., clear bit in the case that we leave the buffer
    // permanently assigned.  Given our current use of the bit, the extra
    // code is not needed.
}
