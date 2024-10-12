//
// fputs.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fputs(), which writes a string to a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Writes the given string to the given stream.  Does not write the string's null
// terminator, and does not append a '\n' to the file.  Returns a nonnegative
// value on success; EOF on failure.
static int __cdecl _fputs_internal(char const* const string, FILE* const stream, __crt_cached_ptd_host& ptd)
{
    _UCRT_VALIDATE_RETURN(ptd, string != nullptr,  EINVAL, EOF);
    _UCRT_VALIDATE_RETURN(ptd, stream != nullptr,  EINVAL, EOF);
    _UCRT_VALIDATE_STREAM_ANSI_RETURN(ptd, stream, EINVAL, EOF);

    size_t const length = strlen(string);

    return __acrt_lock_stream_and_call(stream, [&]() -> int
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        size_t const bytes_written = _fwrite_nolock_internal(string, 1, length, stream, ptd);
        if (bytes_written == length)
        {
            return 0;
        }

        return EOF;
    });
}

extern "C" int __cdecl fputs(char const* const string, FILE* const stream)
{
    __crt_cached_ptd_host ptd;
    return _fputs_internal(string, stream, ptd);
}
