//
// wcscat.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcscat(), which concatenates (appends) a copy of the source string to
// the end of the destination string.
//
// This function assumes that the destination buffer is sufficiently large to
// store the appended string.
//
#include <string.h>



#if defined _M_X64 || defined _M_IX86 || defined _M_ARM || defined _M_ARM64
    #pragma warning(disable:4163)
    #pragma function(wcscat)
#endif



extern "C" wchar_t * __cdecl wcscat(
    wchar_t*       const destination,
    wchar_t const*       source
    )
{
    wchar_t* destination_it = destination;

    // Find the end of the destination string:
    while (*destination_it)
        ++destination_it;

    // Append the source string to the destination string:
    while ((*destination_it++ = *source++) != L'\0') { }

    return destination;
}
