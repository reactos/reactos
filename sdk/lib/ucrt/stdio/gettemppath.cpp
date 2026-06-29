//
// gettemppath.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The __acrt_GetTempPath2A() function, which calls GetTempPathW and converts the string to multibyte.
//
#include <corecrt_internal.h>
#include <io.h>



// This function simply calls __acrt_GetTempPath2W() and converts the wide string to multibyte.
// Note that GetTempPathA is not UTF-8 aware. This is because APIs using temporary paths
// must not depend on the current locale setting and must use the ACP or OEMCP since
// the returned data must be a static buffer and this behavior is guaranteed by MSDN documentation.
extern "C" DWORD __cdecl __acrt_GetTempPath2A(DWORD nBufferLength, LPSTR lpBuffer)
{
    wchar_t wide_buffer[_MAX_PATH + 1] = {};

    DWORD const get_temp_path_result = __acrt_GetTempPath2W(_countof(wide_buffer), wide_buffer);
    if (get_temp_path_result == 0)
    {
        return 0;
    }

    bool const use_oem_code_page = !__acrt_AreFileApisANSI();
    int const code_page = use_oem_code_page ? CP_OEMCP : CP_ACP;
#pragma warning(suppress:__WARNING_W2A_BEST_FIT) // 38021 Prefast recommends WC_NO_BEST_FIT_CHARS.
    int const wctmb_result = __acrt_WideCharToMultiByte(code_page, 0, wide_buffer, -1, lpBuffer, nBufferLength, nullptr, nullptr);
    if (wctmb_result == 0)
    {
        return 0;
    }

    // The return value of WideCharToMultiByte includes the null terminator; the
    // return value of GetTempPathA does not.
    return wctmb_result - 1;
}
