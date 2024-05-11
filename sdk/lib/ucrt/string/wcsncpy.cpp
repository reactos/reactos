//
// wcsncpy.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcsncpy(), which copies a string from one buffer to another.  This
// function copies at most 'count' characters.  If fewer than 'count' characters
// are copied, the rest of the buffer is padded with null characters.
//
#include <string.h>

#pragma warning(disable:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036


#ifdef _M_ARM
    #pragma function(wcsncpy)
#endif



extern "C" wchar_t * __cdecl wcsncpy(
    wchar_t*       const destination,
    wchar_t const* const source,
    size_t         const count
    )
{
    size_t remaining = count;

    wchar_t*       destination_it = destination;
    wchar_t const* source_it      = source;
    while (remaining != 0 && (*destination_it++ = *source_it++) != 0)
    {
        --remaining;
    }

    if (remaining != 0)
    {
        while (--remaining != 0)
        {
            *destination_it++ = L'\0';
        }
    }

    return destination;
}
