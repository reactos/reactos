//
// wcsncat.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcsncat(), which appends n characters of the source string onto the
// end of the destination string.  The destination string is always null
// terminated.  Unlike wcsncpy, this function does not pad out to 'count'
// characters.
//
#include <string.h>


#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

extern "C" wchar_t* __cdecl wcsncat(
    wchar_t*       const destination,
    wchar_t const* const source,
    size_t         const count
    )
{
    wchar_t* destination_it = destination;

    // Find the end of the destination string:
    while (*destination_it)
        ++destination_it;

    // Append the source string:
    wchar_t const* source_it = source;
    for (size_t i = 0; i != count; ++i)
    {
        if ((*destination_it++ = *source_it++) == 0)
            return destination;
    }

    *destination_it = 0;

    return destination;
}
