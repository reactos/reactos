//
// setvbuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines setvbuf(), which is used to set buffering mode for a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Helper for setvbuf() that sets various buffer properties and return zero.
static int __cdecl set_buffer(
                                            __crt_stdio_stream const stream,
    _In_reads_opt_(buffer_size_in_bytes)    char*              const buffer,
                                            size_t             const buffer_size_in_bytes,
                                            int                const new_flag_bits
    ) throw()
{
    stream.set_flags(new_flag_bits);
    stream->_bufsiz = static_cast<int>(buffer_size_in_bytes);
    stream->_ptr    = buffer;
    stream->_base   = buffer;
    stream->_cnt    = 0;

    return 0;
}



// Controls buffering and buffer size for the specified stream.  The array
// pointed to by 'buffer' is used as a buffer for the stream.  If 'buffer' is
// null, the CRT allocates a buffer of the requested size.  The 'type' specifies
// the type of buffering, which must be one of _IONBF (no buffering) or _IOLBF
// or _IOFBF (both of which mean full buffering).
//
// Returns zero on success; nonzero on failure.
static int __cdecl _setvbuf_internal(
    FILE*              const public_stream,
    char*              const buffer,
    int                const type,
    size_t             const buffer_size_in_bytes,
    __crt_cached_ptd_host&   ptd
    )
{
    __crt_stdio_stream const stream(public_stream);

    _UCRT_VALIDATE_RETURN(ptd, stream.valid(), EINVAL, -1);

    // Make sure 'type' is one of the three allowed values, and if we are
    // buffering, make sure the size is between 2 and INT_MAX:
    _UCRT_VALIDATE_RETURN(ptd, type == _IONBF || type == _IOFBF || type == _IOLBF, EINVAL, -1);

    if (type == _IOFBF || type == _IOLBF)
    {
        _UCRT_VALIDATE_RETURN(ptd, 2 <= buffer_size_in_bytes && buffer_size_in_bytes <= INT_MAX, EINVAL, -1);
    }

    return __acrt_lock_stream_and_call(stream.public_stream(), [&]
    {
        // Force the buffer size to be even by masking the low order bit:
        size_t const usable_buffer_size = buffer_size_in_bytes & ~static_cast<size_t>(1);

        // Flush the current buffer and free it, if it is ours:
        __acrt_stdio_flush_nolock(stream.public_stream(), ptd);
        __acrt_stdio_free_buffer_nolock(stream.public_stream());

        // Clear the stream state bits related to buffering.  Most of these
        // should never be set when setvbuf() is called, but it doesn't cost
        // anything to be safe.
        stream.unset_flags(_IOBUFFER_CRT     | _IOBUFFER_USER  | _IOBUFFER_NONE |
                           _IOBUFFER_SETVBUF | _IOBUFFER_STBUF | _IOCTRLZ);

        // Case 1:  No buffering:
        if (type & _IONBF)
        {
            return set_buffer(stream, reinterpret_cast<char*>(&stream->_charbuf), 2, _IOBUFFER_NONE);
        }

        // Cases 2 and 3 (below) cover the _IOFBF and _IOLBF types of buffering.
        // Line buffering is treated the same as full buffering, so the _IOLBF
        // bit in the flag is never set.  Finally, since _IOFBF is defined to
        // be zero, full buffering is simply assumed whenever _IONBF is not set.

        // Case 2:  Default buffering, CRT-allocated buffer:
        if (buffer == nullptr)
        {
            char* const crt_buffer = _calloc_crt_t(char, usable_buffer_size).detach();
            if (!crt_buffer)
            {
                #ifndef CRTDLL
                // Force library pre-termination procedure (this is placed here
                // because the code path should almost never be hit):
                ++_cflush;
                #endif

                return -1;
            }

            return set_buffer(stream, crt_buffer, usable_buffer_size, _IOBUFFER_CRT | _IOBUFFER_SETVBUF);
        }

        // Case 3:  Default buffering, user-provided buffer:
        return set_buffer(stream, buffer, usable_buffer_size, _IOBUFFER_USER | _IOBUFFER_SETVBUF);
    });
}

extern "C" int __cdecl setvbuf(
    FILE*  const public_stream,
    char*  const buffer,
    int    const type,
    size_t const buffer_size_in_bytes
    )
{
    __crt_cached_ptd_host ptd;
    return _setvbuf_internal(public_stream, buffer, type, buffer_size_in_bytes, ptd);
}
