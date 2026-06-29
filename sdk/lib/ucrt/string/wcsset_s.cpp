//
// wcsset_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _wcsset_s(), which sets all of the characters of a null-terminated
// string to the given character.
//
#include <corecrt_internal_string_templates.h>
#include <string.h>

extern "C" errno_t __cdecl _wcsset_s(
    wchar_t* const destination,
    size_t   const size_in_elements,
    wchar_t  const value
    )
{
    return common_tcsset_s(destination, size_in_elements, value);
}
