//
// putws.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _putws(), which writes a wide character string to stdout.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_ptd_propagation.h>



// Writes a wide character string to stdout.  Does not write the string's null
// terminator, but _does_ append a newline to the output.  Returns 0 on success;
// returns WEOF on failure.
static int __cdecl _putws_internal(wchar_t const* const string, __crt_cached_ptd_host& ptd)
{
    _UCRT_VALIDATE_RETURN(ptd, string != nullptr, EINVAL, WEOF);

    FILE* const stream = stdout;

    return __acrt_lock_stream_and_call(stream, [&]() -> int
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        // Write the string, character-by-character:
        for (wchar_t const* it = string; *it; ++it)
        {
            if (_fputwc_nolock_internal(*it, stream, ptd) == WEOF)
            {
                return WEOF;
            }
        }

        if (_fputwc_nolock_internal(L'\n', stream, ptd) == WEOF)
        {
            return WEOF;
        }

        return 0;
    });
}

extern "C" int __cdecl _putws(wchar_t const* const string)
{
    __crt_cached_ptd_host ptd;
    return _putws_internal(string, ptd);
}
