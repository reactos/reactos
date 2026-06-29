//
// feoferr.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines feof() and ferror(), which test the end-of-file and error states of a
// stream, respectively.
//
#include <corecrt_internal_stdio.h>



// Tests the stream for the end-of-file condition.  Returns nonzero if and only
// if the stream is at end-of-file.
extern "C" int __cdecl feof(FILE* const public_stream)
{
    _VALIDATE_RETURN(public_stream != nullptr, EINVAL, 0);
    return __crt_stdio_stream(public_stream).eof();
}



// Tests the stream error indicator.  Returns nonzero if and only if the error
// indicator for the stream is set.
extern "C" int __cdecl ferror(FILE* const public_stream)
{
    _VALIDATE_RETURN(public_stream != nullptr, EINVAL, 0);
    return __crt_stdio_stream(public_stream).error();
}
