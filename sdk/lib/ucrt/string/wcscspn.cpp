//
// wcscspn.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcscspn(), which searches a string for an initial substring that does
// not contain any of the specified control characters.
//
// This function returns the index of the first character in the string that
// belongs to the set of characters specified by 'control'.  This is equivalent
// to the length of the initial substring of 'string' composed entirely of
// characters that are not in 'control'.
//
#include <string.h>



extern "C" size_t __cdecl wcscspn(
    wchar_t const* const string,
    wchar_t const* const control
    )
{
    wchar_t const* string_it = string;
    for (; *string_it; ++string_it)
    {
        for (wchar_t const* control_it = control; *control_it; ++control_it)
        {
            if (*string_it == *control_it)
            {
                return static_cast<size_t>(string_it - string);
            }
        }
    }

    return static_cast<size_t>(string_it - string);
}
