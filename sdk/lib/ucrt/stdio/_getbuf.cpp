
//
// _getbuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines __acrt_stdio_allocate_buffer_nolock(), which allocates a buffer for a stream.
//
#include <corecrt_internal_stdio.h>



// Allocates a buffer for the provided stream.  This function assumes that the
// caller has already checked to ensure that the stream does not already have a
// buffer.
extern "C" void __cdecl __acrt_stdio_allocate_buffer_nolock(FILE* const public_stream)
{
    _ASSERTE(public_stream != nullptr);

    __crt_stdio_stream const stream(public_stream);

    #ifndef CRTDLL
    ++_cflush; // Force the library pre-termination procedure to run
    #endif

    // Try to get a big buffer:
    stream->_base = _calloc_crt_t(char, _INTERNAL_BUFSIZ).detach();
    if (stream->_base != nullptr)
    {
        stream.set_flags(_IOBUFFER_CRT);
        stream->_bufsiz = _INTERNAL_BUFSIZ;
    }
    // If we couldn't get a big buffer, use single character buffering:
    else
    {
        stream.set_flags(_IOBUFFER_NONE);
        stream->_base = reinterpret_cast<char *>(&stream->_charbuf);
        stream->_bufsiz = 2;
    }

    stream->_ptr = stream->_base;
    stream->_cnt = 0;
}
