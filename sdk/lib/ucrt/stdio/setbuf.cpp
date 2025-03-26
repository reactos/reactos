//
// setbuf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines setbuf(), which enables or disables buffering on a stream.
//
#include <corecrt_internal_stdio.h>



// If the buffer is null, buffering is disabled for the stream.  If the buffer is
// non-null, it must point to a buffer of BUFSIZ characters; the stream will be
// configured to use that buffer.  The functionality of setbuf() is a strict
// subset of the functionality of setvbuf().
extern "C" void __cdecl setbuf(FILE* const stream, char* const buffer)
{
    _ASSERTE(stream != nullptr);

    if (buffer == nullptr)
    {
        setvbuf(stream, nullptr, _IONBF, 0);
    }
    else
    {
        setvbuf(stream, buffer, _IOFBF, BUFSIZ);
    }
}
