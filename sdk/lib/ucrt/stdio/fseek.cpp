//
// fseek.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the fseek() family of functions, which repositions a file pointer to
// a new place in a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>



// If a binary mode stream is open for reading and the target of the requested
// seek is within the stream buffer, we can simply adjust the buffer pointer to
// the new location.  This allows us to avoid flushing the buffer.  If the user
// has set a large buffer on the stream and performs frequent seeks that are
// likely to result in targets within the buffer, this is a huge performance win.
// This function handles the most common cases.
static bool __cdecl common_fseek_binary_mode_read_only_fast_track_nolock(
    __crt_stdio_stream const stream,
    __int64                  offset,
    int                      whence
    ) throw()
{
    // This fast-track path does not handle the seek-from-end case (this is not
    // nearly as commonly used as seeking from the beginning or from the current
    // position).
    if (whence == SEEK_END)
    {
        return false;
    }

    // This fast-track path is only useful if the stream is buffered.
    if (!stream.has_any_buffer())
    {
        return false;
    }

    // This fast-track path requires a stream opened only for reading.  It may
    // be possible to handle streams opened for writing or update similarly;
    // further investigation would be required.
    if (stream.has_any_of(_IOWRITE | _IOUPDATE))
    {
        return false;
    }

    // The ftell function handles a case where _cnt is negative.  It isn't clear
    // why _cnt may be negative, so if _cnt is negative, fall back to the
    // expensive path.  If the _cnt is zero, do not assume that the buffer
    // contents contain the immediately preceding file content; data may have
    // been read from the file bypassing the buffer.  Fall back to the expensive
    // path to be sure.
    if (stream->_cnt <= 0)
    {
        return false;
    }

    // This fast-track path requires a binary mode file handle.  When text mode
    // or UTF transformations are enabled, the contents of the buffer do not
    // exactly match the contents of the underlying file.
    int const fh = stream.lowio_handle();
    if ((_osfile(fh) & FTEXT) != 0 || _textmode(fh) != __crt_lowio_text_mode::ansi)
    {
        return false;
    }

    // Handle the SEEK_SET case by transforming the SEEK_SET offset into a
    // SEEK_CUR offset:
    if (whence == SEEK_SET)
    {
        __int64 const lowio_position = _lseeki64_nolock(fh, 0, SEEK_CUR);
        if (lowio_position < 0)
        {
            return false;
        }

        __int64 const stdio_position = lowio_position - stream->_cnt;
        if (FAILED(LongLongSub(offset, stdio_position, &offset)))
        {
            return false;
        }
        whence = SEEK_CUR;
    }

    // Compute the maximum number of bytes that we can seek in each direction
    // within the buffer and verify that the requested offset is within that
    // range.  Note that the minimum reverse seek is a negative value.
    __int64 const minimum_reverse_seek = -(stream->_ptr - stream->_base);
    __int64 const maximum_forward_seek = stream->_cnt;

    bool const seek_is_within_buffer = minimum_reverse_seek <= offset && offset <= maximum_forward_seek;
    if (!seek_is_within_buffer)
    {
        return false;
    }

    stream->_ptr += offset;

    // Note that the cast here is safe:  The buffer will never be larger than
    // INT_MAX bytes in size.  The setvbuf function validates this constraint.
    stream->_cnt -= static_cast<int>(offset);
    return true;
}



static int __cdecl common_fseek_nolock(
    __crt_stdio_stream const stream,
    __int64                  offset,
    int                      whence,
    __crt_cached_ptd_host&   ptd
    ) throw()
{
    if (!stream.is_in_use())
    {
        ptd.get_errno().set(EINVAL);
        return -1;
    }

    stream.unset_flags(_IOEOF);

    if (common_fseek_binary_mode_read_only_fast_track_nolock(stream, offset, whence))
    {
        return 0;
    }

    // If seeking relative to the current location, then convert to a seek
    // relative to the beginning of the file.  This accounts for buffering,
    // etc., by letting fseek() tell us where we are:
    if (whence == SEEK_CUR)
    {
        offset += _ftelli64_nolock_internal(stream.public_stream(), ptd);
        whence = SEEK_SET;
    }

    __acrt_stdio_flush_nolock(stream.public_stream(), ptd);
    // If the stream is opened in update mode and is currently in use for reading,
    // the buffer must be abandoned to ensure consistency when transitioning from
    // reading to writing.
    // __acrt_stdio_flush_nolock will not reset the buffer when _IOWRITE flag
    // is not set.
    __acrt_stdio_reset_buffer(stream);

    // If the file was opened for read/write, clear flags since we don't know
    // what the user will do next with the file.  If the file was opened for
    // read only access, decrease the _bufsiz so that the next call to
    // __acrt_stdio_refill_and_read_{narrow,wide}_nolock won't cost quite so
    // much:
    if (stream.has_all_of(_IOUPDATE))
    {
        stream.unset_flags(_IOWRITE | _IOREAD);
    }
    else if (stream.has_all_of(_IOREAD | _IOBUFFER_CRT) && !stream.has_any_of(_IOBUFFER_SETVBUF))
    {
        stream->_bufsiz = _SMALL_BUFSIZ;
    }

    if (_lseeki64_nolock_internal(stream.lowio_handle(), offset, whence, ptd) == -1)
    {
        return -1;
    }

    return 0;
}



// Repositions the file pointer of a stream to the specified location, relative
// to 'whence', which can be SEEK_SET (the beginning of the file), SEEK_CUR (the
// current pointer position), or SEEK_END (the end of the file).  The offset may
// be negative.  Returns 0 on success; returns -1 and sets errno on failure.
static int __cdecl common_fseek(
    __crt_stdio_stream const stream,
    __int64            const offset,
    int                const whence,
    __crt_cached_ptd_host&   ptd
    ) throw()
{
    _UCRT_VALIDATE_RETURN(ptd, stream.valid(), EINVAL, -1);
    _UCRT_VALIDATE_RETURN(ptd, whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END, EINVAL, -1);

    int return_value = -1;

    _lock_file(stream.public_stream());
    __try
    {
        return_value = common_fseek_nolock(stream, offset, whence, ptd);
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return return_value;
}



extern "C" int __cdecl fseek(
    FILE* const public_stream,
    long  const offset,
    int   const whence
    )
{
    __crt_cached_ptd_host ptd;
    return common_fseek(__crt_stdio_stream(public_stream), offset, whence, ptd);
}



extern "C" int __cdecl _fseek_nolock(
    FILE* const public_stream,
    long  const offset,
    int   const whence
    )
{
    __crt_cached_ptd_host ptd;
    return common_fseek_nolock(__crt_stdio_stream(public_stream), offset, whence, ptd);
}



extern "C" int __cdecl _fseeki64(
    FILE*   const public_stream,
    __int64 const offset,
    int     const whence
    )
{
    __crt_cached_ptd_host ptd;
    return common_fseek(__crt_stdio_stream(public_stream), offset, whence, ptd);
}



extern "C" int __cdecl _fseeki64_nolock(
    FILE*   const public_stream,
    __int64 const offset,
    int     const whence
    )
{
    __crt_cached_ptd_host ptd;
    return common_fseek_nolock(__crt_stdio_stream(public_stream), offset, whence, ptd);
}
