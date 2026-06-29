//
// clearerr.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines clearerr(), which clears error and EOF flags from a stream.
//
#include <corecrt_internal_stdio.h>



extern "C" errno_t __cdecl clearerr_s(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    _VALIDATE_RETURN_ERRCODE(stream.valid(), EINVAL);

    _lock_file(stream.public_stream());
    __try
    {
        stream.unset_flags(_IOERROR | _IOEOF);                               // Clear stdio flags
        _osfile_safe(_fileno(stream.public_stream())) &= ~(FEOFLAG);         // Clear lowio flags
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return 0;
}



extern "C" void __cdecl clearerr(FILE* const stream)
{
    clearerr_s(stream);
}
