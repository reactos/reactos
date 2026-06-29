//
// wcsset.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _wcsset(), which sets all of the characters in a wide character
// string to the given value.
//
#include <string.h>



#if defined _M_X64 || defined _M_IX86 || defined _M_ARM || defined _M_ARM64
    #pragma warning(disable:4163)
    #pragma function(_wcsset)
#endif



extern "C" wchar_t* __cdecl _wcsset(
	wchar_t* const string,
	wchar_t  const value
	)
{
    for (wchar_t* p = string; *p; ++p)
        *p = value;

    return string;
}
