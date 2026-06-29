//
// LoadLibraryExA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_LoadLibraryExA.
//

#include <corecrt_internal_win32_buffer.h>

HMODULE __cdecl __acrt_LoadLibraryExA(
    LPCSTR const lpFilename,
    HANDLE const hFile,
    DWORD const  dwFlags
    )
{
    __crt_internal_win32_buffer<wchar_t> wide_file_name;

    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(
        lpFilename,
        wide_file_name,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt1 != 0) {
        return nullptr;
    }

    return ::LoadLibraryExW(
        wide_file_name.data(),
        hFile,
        dwFlags
        );
}
