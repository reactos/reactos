//
// wcscpy.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcscpy(), which copies a string from one buffer to another.
//
#include <string.h>



#if defined _M_X64 || defined _M_IX86 || defined _M_ARM || defined _M_ARM64
    #pragma warning(disable:4163)
    #pragma function(wcscpy)
#endif



extern "C" wchar_t* __cdecl wcscpy(
    wchar_t*       const destination,
    wchar_t const*       source)
{
    wchar_t* destination_it = destination;
    while ((*destination_it++ = *source++) != '\0') { }

    return destination;
}
