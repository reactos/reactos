//
// puts.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines puts(), which writes a string to stdout.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Writes a string to stdout.  Does not write the string's null terminator, but
// _does_ append a newline to the output.  Return 0 on success; EOF on failure.
static int __cdecl _puts_internal(char const* const string, __crt_cached_ptd_host& ptd)
{
    _UCRT_VALIDATE_RETURN(ptd, string != nullptr,  EINVAL, EOF);

    FILE* const stream = stdout;
    _UCRT_VALIDATE_STREAM_ANSI_RETURN(ptd, stream, EINVAL, EOF);

    size_t const length = strlen(string);

    return __acrt_lock_stream_and_call(stream, [&]() -> int
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        size_t const bytes_written = _fwrite_nolock_internal(string, 1, length, stream, ptd);

        // If we failed to write the entire string, or if we fail to write the
        // newline, reset the buffering and return failure:
        if (bytes_written != length || _fputc_nolock_internal('\n', stream, ptd) == EOF)
        {
            return EOF;
        }

        return 0;
    });
}

extern "C" int __cdecl puts(char const* const string)
{
    __crt_cached_ptd_host ptd;
    return _puts_internal(string, ptd);
}
