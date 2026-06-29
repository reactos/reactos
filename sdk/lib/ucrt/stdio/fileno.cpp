//
// fileno.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _fileno(), which returns the lowio file handle for the given stdio
// stream.
//
#include <corecrt_internal_stdio.h>



extern "C" int __cdecl _fileno(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    _VALIDATE_RETURN(stream.valid(), EINVAL, -1);
    return stream.lowio_handle();
}
