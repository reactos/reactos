//
// wcscpy_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcscpy_s(), which copies a string from one buffer to another.
//
#include <corecrt_internal_string_templates.h>
#include <string.h>

extern "C" errno_t __cdecl wcscpy_s(
    wchar_t*       const destination,
    size_t         const size_in_elements,
    wchar_t const* const source
    )
{
    return common_tcscpy_s(destination, size_in_elements, source);
}
