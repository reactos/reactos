//
// wcsspn.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcsspn(), which finds the length of the initial substring of 'string'
// that consists entirely of characters in 'control'.  This function returns the
// index of the first character in the string that does not belong to the set of
// characters in the control string.  If there is no such character, the length
// of the string is returned.
//
#include <string.h>



extern "C" size_t __cdecl wcsspn(
    wchar_t const* const string,
    wchar_t const* const control
    )
{
    wchar_t const* string_it = string;
    for (; *string_it; ++string_it)
    {
        for (wchar_t const* control_it = control; *string_it != *control_it; ++control_it)
        {
            // Reached the end of the control string without finding a match:
            if (*control_it == 0)
            {
                return static_cast<size_t>(string_it - string);
            }
        }
    }

    // The whole string consisted of characters from the control:
    return static_cast<size_t>(string_it - string);
}
