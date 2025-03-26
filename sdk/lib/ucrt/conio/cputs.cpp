//
// cputs.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _cputs(), which writes a string directly to the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>

// Writes the given string directly to the console.  No newline is appended.
// Returns 0 on success; nonzero on failure.
extern "C" int __cdecl _cputs(char const* const string)
{
    _VALIDATE_CLEAR_OSSERR_RETURN(string != nullptr, EINVAL, -1);

    __acrt_lock(__acrt_conio_lock);
    int result = 0;
    __try
    {
        // Write the string directly to the console.  Each character is written
        // individually, as performance of this function is not considered
        // critical.
        for (char const* p = string; *p; ++p)
        {
            if (_putch_nolock(*p) == EOF)
            {
                result = -1;
                __leave;
            }
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}
