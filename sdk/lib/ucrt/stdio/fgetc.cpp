//
// fgetc.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that read the next character from a stream and return it.  If the
// read causes the stream to reach EOF, EOF is returned and the EOF bit is set
// on the stream.
//
#include <corecrt_internal_stdio.h>



extern "C" int __cdecl _fgetc_nolock(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    _VALIDATE_RETURN(stream.valid(), EINVAL, EOF);

    --stream->_cnt;

    if (stream->_cnt < 0)
        return __acrt_stdio_refill_and_read_narrow_nolock(stream.public_stream());

    char const c = *stream->_ptr;
    ++stream->_ptr;
    return c & 0xff;
}



extern "C" int __cdecl _getc_nolock(FILE* const stream)
{
    return _fgetc_nolock(stream);
}



extern "C" int __cdecl fgetc(FILE* const public_stream)
{
    __crt_stdio_stream const stream(public_stream);

    _VALIDATE_RETURN(stream.valid(), EINVAL, EOF);

    int return_value = 0;

    _lock_file(stream.public_stream());
    __try
    {
        _VALIDATE_STREAM_ANSI_RETURN(stream, EINVAL, EOF);
        
        return_value = _fgetc_nolock(stream.public_stream());
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return return_value;
}



extern "C" int __cdecl getc(FILE* const stream)
{
    return fgetc(stream);
}



extern "C" int __cdecl _fgetchar()
{
    return fgetc(stdin);
}



extern "C" int __cdecl getchar()
{
    return _fgetchar();
}
