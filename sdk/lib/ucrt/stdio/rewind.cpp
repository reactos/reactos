//
// rewind.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines rewind(), which rewinds a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Rewinds a stream back to the beginning, if the stream supports seeking.  The
// stream is flushed and errors are cleared before the rewind.
static void __cdecl _rewind_internal(FILE* const public_stream, __crt_cached_ptd_host& ptd)
{
    __crt_stdio_stream const stream(public_stream);

    _UCRT_VALIDATE_RETURN_VOID(ptd, stream.valid(), EINVAL);

    int const fh = _fileno(stream.public_stream());

    _lock_file(stream.public_stream());
    __try
    {
        // Flush the streeam, reset the error state, and seek back to the
        // beginning:
        __acrt_stdio_flush_nolock(stream.public_stream(), ptd);
        // If the stream is opened in update mode and is currently in use for reading,
        // the buffer must be abandoned to ensure consistency when transitioning from
        // reading to writing.
        // __acrt_stdio_flush_nolock will not reset the buffer when _IOWRITE flag
        // is not set.
        __acrt_stdio_reset_buffer(stream);


        stream.unset_flags(_IOERROR | _IOEOF);
        _osfile_safe(fh) &= ~(FEOFLAG);

       if (stream.has_all_of(_IOUPDATE))
       {
           stream.unset_flags(_IOREAD | _IOWRITE);
       }

        if (_lseek_internal(fh, 0, 0, ptd) == -1)
        {
            stream.set_flags(_IOERROR);
        }
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry
}

extern "C" void __cdecl rewind(FILE* const public_stream)
{
    __crt_cached_ptd_host ptd;
    _rewind_internal(public_stream, ptd);
}
