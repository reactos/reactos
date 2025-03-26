//
// wcspbrk.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcspbrk(), which returns a pointer to the first wide character in
// 'string' that is in the 'control'.
//
#include <string.h>



extern "C" wchar_t const* __cdecl wcspbrk(
    wchar_t const* const string,
    wchar_t const* const control
    )
{
    for (wchar_t const* string_it = string; *string_it; ++string_it)
    {
        for (wchar_t const* control_it = control; *control_it; ++control_it)
        {
            if (*control_it == *string_it)
            {
                return string_it;
            }
        }
    }

    return nullptr;
}
