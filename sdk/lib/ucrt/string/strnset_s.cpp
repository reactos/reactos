//
// strnset_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _strnset_s(), which sets at most the first 'count' characters of a
// string to the given character and ensures that the resulting string is null-
// terminated.
//
#include <corecrt_internal_string_templates.h>
#include <string.h>



extern "C" errno_t __cdecl _strnset_s(
    char*  const destination,
    size_t const size_in_elements,
    int    const value,
    size_t const count
    )
{
    return common_tcsnset_s(destination, size_in_elements, static_cast<char>(value), count);
}
