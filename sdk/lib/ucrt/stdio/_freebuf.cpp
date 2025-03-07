//
// _freebuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines __acrt_stdio_free_buffer(), which releaes a buffer from a stream.
//
#include <corecrt_internal_stdio.h>



// Releases a buffer from a stream.  If the stream is buffered and the buffer
// was allocated by the CRT, the space is freed.  If the buffer was provided by
// the user, it is not freed (since we do not know how to free it).
extern "C" void __cdecl __acrt_stdio_free_buffer_nolock(FILE* const public_stream)
{
    _ASSERTE(public_stream != nullptr);

    __crt_stdio_stream const stream(public_stream);

    if (!stream.is_in_use())
        return;
    
    if (!stream.has_crt_buffer())
        return;

    _free_crt(stream->_base);

    stream.unset_flags(_IOBUFFER_CRT | _IOBUFFER_SETVBUF);
    stream->_base  = nullptr;
    stream->_ptr   = nullptr;
    stream->_cnt   = 0;
}
