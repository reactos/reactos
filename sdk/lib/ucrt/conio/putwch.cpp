//
// putwch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _putwch(), which writes a wide character to the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>

// Writes a wide character to the console.  Returns the character on success,
// WEOF on failure.
extern "C" wint_t __cdecl _putwch(wchar_t const c)
{
    return __acrt_lock_and_call(__acrt_conio_lock, [&]
    {
        return _putwch_nolock(c);
    });
}

extern "C" wint_t __cdecl _putwch_nolock(wchar_t const c)
{
    if (__dcrt_lowio_ensure_console_output_initialized() == FALSE)
        return WEOF;

    // Write character to console:
    DWORD charsWritten;
    if (__dcrt_write_console(&c, 1, &charsWritten) == FALSE)
        return WEOF;

    return c;
}

extern "C" wint_t __cdecl _putwch_nolock_internal(wchar_t const c, __crt_cached_ptd_host&)
{
    // Currently _putwch_nolock does not require any PTD access. Do not need to propagate __crt_cached_ptd_host&.
    return _putwch_nolock(c);
}
