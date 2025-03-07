//
// wcsncpy_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcsncpy_s(), which appends n characters of the source string onto the
// end of the destination string.
//
#include <corecrt_internal_string_templates.h>
#include <string.h>

extern "C" errno_t __cdecl wcsncpy_s(
    wchar_t*       const destination,
    size_t         const size_in_elements,
    wchar_t const* const source,
    size_t         const count
    )
{
    return common_tcsncpy_s(destination, size_in_elements, source, count);
}
