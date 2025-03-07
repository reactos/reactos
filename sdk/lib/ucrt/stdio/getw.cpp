//
// getw.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _getw(), which reads a binary integer to a stream.
//
#include <corecrt_internal_stdio.h>



// Reads a binary int value from the stream, byte-by-byte.  On success, returns
// the value written; on failure (error or eof), returns EOF.  Note, however,
// that the value may be EOF--that is, after all, a valid integer--so be sure to
// test ferror() and feof() to check for error conditions.
extern "C" int __cdecl _getw(FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, EOF);

    int return_value = EOF;

    _lock_file(stream);
    __try
    {
        int value = 0;
        char* const first = reinterpret_cast<char*>(&value);
        char* const last  = first + sizeof(value);

        for (char* it = first; it != last; ++it)
        {
            *it = static_cast<char>(_getc_nolock(stream));
        }

        if (feof(stream))
            __leave;

        if (ferror(stream))
            __leave;

        return_value = value;
    }
    __finally
    {
        _unlock_file(stream);
    }
    __endtry

    return return_value;
}
