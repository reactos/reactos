//
// fputws.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fputws(), which writes a wide character string to a stream.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Writes the given wide character string to the given stream.  Does not write
// the string's null terminator, and does not append a '\n' to the file.  Returns
// a nonnegative value on success; EOF on failure.  (Note well that we return EOF
// and not WEOF on failure.  This is intentional, and is the correct behavior per
// the C Language Standard).
static int __cdecl _fputws_internal(wchar_t const* const string, FILE* const stream, __crt_cached_ptd_host& ptd)
{
    _UCRT_VALIDATE_RETURN(ptd, string != nullptr, EINVAL, EOF);
    _UCRT_VALIDATE_RETURN(ptd, stream != nullptr, EINVAL, EOF);

    return __acrt_lock_stream_and_call(stream, [&]() -> int
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        for (wchar_t const* it = string; *it != L'\0'; ++it)
        {
            if (_fputwc_nolock_internal(*it, stream, ptd) == WEOF)
            {
                return EOF;
            }
        }

        return 0;
    });
}

extern "C" int __cdecl fputws(wchar_t const* const string, FILE* const stream)
{
    __crt_cached_ptd_host ptd;
    return _fputws_internal(string, stream, ptd);
}
