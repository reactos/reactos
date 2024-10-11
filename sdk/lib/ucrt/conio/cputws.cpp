//
// cputws.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _cputws(), which writes a wide string directly to the console.
//
#include <conio.h>
#include <errno.h>
#include <corecrt_internal_lowio.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// Writes the given string directly to the console.  No newline is appended.
//
// Returns 0 on success; nonzero on failure.
extern "C" int __cdecl _cputws(wchar_t const* string)
{
    _VALIDATE_CLEAR_OSSERR_RETURN((string != nullptr), EINVAL, -1);

    if (__dcrt_lowio_ensure_console_output_initialized() == FALSE)
        return -1;

    // Write string to console file handle:
    size_t length = wcslen(string);

    __acrt_lock(__acrt_conio_lock);

    int result = 0;

    __try
    {
        while (length > 0)
        {
            static size_t const max_write_bytes = 65535;
            static size_t const max_write_wchars = max_write_bytes / sizeof(wchar_t);

            DWORD const wchars_to_write = length > max_write_wchars
                ? max_write_wchars
                : static_cast<DWORD>(length);

            DWORD wchars_written;
            if (__dcrt_write_console(
                string,
                wchars_to_write,
                &wchars_written) == FALSE)
            {
                result = -1;
                __leave;
            }

            string += wchars_to_write;
            length -= wchars_to_write;
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry

    return result;
}
