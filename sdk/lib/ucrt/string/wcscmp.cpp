//
// wcscmp.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcscmp(), which compares two wide character strings, determining
// their ordinal order.
//
// Note that the comparison is performed with unsigned elements (wchar_t is
// unsigned in this implementation), so the null character (0) is less than
// all other characters.
//
// Returns:
//  * -1 if a  < b
//  *  0 if a == b
//  *  1 if a  > b
//
#include <string.h>



#if defined _M_X64 || defined _M_IX86 || defined _M_ARM || defined _M_ARM64
    #pragma warning(disable: 4163)
    #pragma function(wcscmp)
#endif



extern "C" int __cdecl wcscmp(wchar_t const* a, wchar_t const* b)
{
    int result = 0;
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018
    while ((result = (int)(*a - *b)) == 0 && *b)
    {
        ++a;
        ++b;
    }

    return ((-result) < 0) - (result < 0); // (if positive) - (if negative) generates branchless code
}
