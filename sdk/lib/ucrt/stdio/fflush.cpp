//
// fflush.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fflush() and related functions, which flush stdio streams.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



static bool __cdecl is_stream_allocated(long const stream_flags) throw()
{
    return (stream_flags & _IOALLOCATED) != 0;
}

static bool __cdecl is_stream_flushable(long const stream_flags) throw()
{
    if ((stream_flags & (_IOREAD | _IOWRITE)) != _IOWRITE)
    {
        return false;
    }

    if ((stream_flags & (_IOBUFFER_CRT | _IOBUFFER_USER)) == 0)
    {
        return false;
    }

    return true;
}

static bool __cdecl is_stream_flushable_or_commitable(long const stream_flags) throw()
{
    if (is_stream_flushable(stream_flags))
    {
        return true;
    }

    if (stream_flags & _IOCOMMIT)
    {
        return true;
    }

    return false;
}

// Returns true if the common_flush_all function should attempt to flush the
// stream; otherwise returns false.  This function returns false for streams
// that are not in use (not allocated) and for streams for which the flush
// operation will be a no-op.
//
// In the case where this function determines that the flush would be a no-op,
// it increments the flushed_stream_count.  This allows common_flush_all to
// keep track of the number of streams that it would have flushed.
static bool __cdecl common_flush_all_should_try_to_flush_stream(
    _In_    __crt_stdio_stream const stream,
    _Inout_ int*               const flushed_stream_count
    ) throw()
{
    if (!stream.valid())
    {
        return false;
    }

    long const stream_flags = stream.get_flags();
    if (!is_stream_allocated(stream_flags))
    {
        return false;
    }

    if (!is_stream_flushable_or_commitable(stream_flags))
    {
        ++*flushed_stream_count;
        return false;
    }

    return true;
}



// Internal implementation of the "flush all" functionality.  If the
// flush_read_mode_streams argument is false, only write mode streams are
// flushed and the return value is zero on success, EOF on failure.
//
// If the flush_read_mode_streams argument is true, this function flushes
// all streams regardless of mode and returns the number of streams that it
// flushed.
//
// Note that in both cases, if we can determine that a call to fflush for a
// particular stream would be a no-op, then we will not call fflush for that
// stream.  This allows us to avoid acquiring the stream lock unnecessarily,
// which can help to avoid deadlock-like lock contention.
//
// Notably, by doing this, we can avoid attempting to acquire the stream lock
// for most read mode streams.  Attempting to acquire the stream lock for a
// read mode stream can be problematic beause another thread may hold the lock
// and be blocked on an I/O operation (e.g., a call to fread on stdin may block
// until the user types input into the console).
static int __cdecl common_flush_all(bool const flush_read_mode_streams) throw()
{
    int count = 0;
    int error = 0;

    __acrt_lock_and_call(__acrt_stdio_index_lock, [&]
    {
        __crt_stdio_stream_data** const first_file = __piob;
        __crt_stdio_stream_data** const last_file  = first_file + _nstream;

        for (__crt_stdio_stream_data** it = first_file; it != last_file; ++it)
        {
            __crt_stdio_stream const stream(*it);

            // Before we acquire the stream lock, check to see if flushing the
            // stream would be a no-op.  If it would be, then skip this stream.
            if (!common_flush_all_should_try_to_flush_stream(stream, &count))
            {
                continue;
            }

            __acrt_lock_stream_and_call(stream.public_stream(), [&]
            {
                // Re-verify the state of the stream.  Another thread may have
                // closed the stream, reopened it into a different mode, or
                // otherwise altered the state of the stream such that this
                // flush would be a no-op.
                if (!common_flush_all_should_try_to_flush_stream(stream, &count))
                {
                    return;
                }

                if (!flush_read_mode_streams && !stream.has_all_of(_IOWRITE))
                {
                    return;
                }

                if (_fflush_nolock(stream.public_stream()) != EOF)
                {
                    ++count;
                }
                else
                {
                    error = EOF;
                }
            });
        }
    });

    return flush_read_mode_streams ? count : error;
}



// Flushes the buffer of the given stream.  If the file is open for writing and
// is buffered, the buffer is flushed.  On success, returns 0.  On failure (e.g.
// if there is an error writing the buffer), returns EOF and sets errno.
extern "C" int __cdecl fflush(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    // If the stream is null, flush all the streams:
    if (!stream.valid())
    {
        return common_flush_all(false);
    }

    // Before acquiring the stream lock, inspect the stream to see if the flush
    // is a no-op.  If it will be a no-op then we can return without attempting
    // to acquire the lock (this can help prevent locking conflicts; see the
    // common_flush_all implementation for more information).
    if (!is_stream_flushable_or_commitable(stream.get_flags()))
    {
        return 0;
    }

    return __acrt_lock_stream_and_call(stream.public_stream(), [&]
    {
        return _fflush_nolock(stream.public_stream());
    });
}



static int __cdecl _fflush_nolock_internal(FILE* const public_stream, __crt_cached_ptd_host& ptd)
{
    __crt_stdio_stream const stream(public_stream);

    // If the stream is null, flush all the streams.
    if (!stream.valid())
    {
        return common_flush_all(false);
    }

    if (__acrt_stdio_flush_nolock(stream.public_stream(), ptd) != 0)
    {
        // If the flush fails, do not attempt to commit:
        return EOF;
    }

    // Perform the lowio commit to ensure data is written to disk:
    if (stream.has_all_of(_IOCOMMIT))
    {
        if (_commit(_fileno(public_stream)))
        {
            return EOF;
        }
    }

    return 0;
}

extern "C" int __cdecl _fflush_nolock(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    return _fflush_nolock_internal(public_stream, ptd);
}

// Flushes the buffer of the given stream.  If the file is open for writing and
// is buffered, the buffer is flushed.  On success, returns 0.  On failure (e.g.
// if there is an error writing the buffer), returns EOF and sets errno.
extern "C" int __cdecl __acrt_stdio_flush_nolock(FILE* const public_stream, __crt_cached_ptd_host& ptd)
{
    __crt_stdio_stream const stream(public_stream);

    if (!is_stream_flushable(stream.get_flags()))
    {
        return 0;
    }

    int const bytes_to_write = static_cast<int>(stream->_ptr - stream->_base);

    __acrt_stdio_reset_buffer(stream);

    if (bytes_to_write <= 0)
    {
        return 0;
    }

    int const bytes_written = _write_internal(_fileno(stream.public_stream()), stream->_base, bytes_to_write, ptd);
    if (bytes_to_write != bytes_written)
    {
        stream.set_flags(_IOERROR);
        return EOF;
    }

    // If this is a read/write file, clear _IOWRITE so that the next operation can
    // be a read:
    if (stream.has_all_of(_IOUPDATE))
    {
        stream.unset_flags(_IOWRITE);
    }

    return 0;
}



// Flushes the buffers for all output streams and clears all input buffers.
// Returns the number of open streams.
extern "C" int __cdecl _flushall()
{
    return common_flush_all(true);
}
