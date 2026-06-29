//
// wcsnset.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _wcsnset(), which sets all of the characters of a null-terminated
// string to the given character.  This function sets at most 'count' characters.
//
#include <string.h>



extern "C" wchar_t* __cdecl _wcsnset(
    wchar_t* const string,
    wchar_t  const value,
    size_t   const count
    )
{
    wchar_t* it = string;
    for (size_t i = 0; i != count && *it; ++i, ++it)
    {
        *it = value;
    }

    return string;
}
