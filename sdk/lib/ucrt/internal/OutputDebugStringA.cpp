//
// OutputDebugStringA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_OutputDebugStringA.
//

#include <corecrt_internal_win32_buffer.h>

void WINAPI __acrt_OutputDebugStringA(
    LPCSTR const text
    )
{
    if (text == nullptr)
    {
        return;
    }
    size_t const text_size = strlen(text) + 1;
    if (text_size == 0)
    {
        return;
    }
    // Use alloca instead of heap, since this code is executed during asserts
    auto text_wide = (wchar_t*)_alloca(text_size * sizeof(wchar_t));
    size_t converted;
    if (mbstowcs_s(&converted, text_wide, text_size, text, text_size - 1) == 0)
    {
        OutputDebugStringW(text_wide);
    }
}
