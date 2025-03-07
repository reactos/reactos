//
// wcsrev.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _wcsrev(), which reverses a wide chraacter string in place.  The
// pointer to the string is returned.
//
#include <string.h>



extern "C" wchar_t* __cdecl _wcsrev(wchar_t* const string)
{
    // Find the end of the string:
    wchar_t* right = string;
    while (*right++) { }
    right -= 2;

    // Reverse the strong:
    wchar_t* left = string;
    while (left < right)
    {
        wchar_t const c = *left;
        *left++  = *right;
        *right-- = c;
    }

    return string;
}
