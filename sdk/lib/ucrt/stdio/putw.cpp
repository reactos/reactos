//
// putw.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _putw(), which writes a binary integer to a stream.
//
#include <corecrt_internal_stdio.h>



// Writes the binary int value to the stream, byte-by-byte.  On success, returns
// the value written; on failure, returns EOF.  Note, however, that the value may
// be EOF--that is, after all, a valid integer--so be sure to test ferror() and
// feof() to check for error conditions.
extern "C" int __cdecl _putw(int const value, FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, EOF);

    int return_value = EOF;

    _lock_file(stream);
    __try
    {
        char const* const first = reinterpret_cast<char const*>(&value);
        char const* const last  = first + sizeof(value);
        for (char const* it = first; it != last; ++it)
        {
            _fputc_nolock(*it, stream);
        }

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
