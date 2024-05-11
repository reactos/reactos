//
// GetModuleFileNameA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_GetModuleFileNameA.
//

#include <corecrt_internal_win32_buffer.h>

DWORD __cdecl __acrt_GetModuleFileNameA(
    HMODULE const hModule,
    char * const  lpFilename,
    DWORD const   nSize
    )
{
    size_t const wide_buffer_size = MAX_PATH + 1;
    wchar_t wide_buffer[wide_buffer_size];

    DWORD const amount_copied = GetModuleFileNameW(
        hModule,
        wide_buffer,
        wide_buffer_size
        );

    if (amount_copied == 0) {
        __acrt_errno_map_os_error(GetLastError());
        return 0;
    }

    __crt_no_alloc_win32_buffer<char> filename_buffer(lpFilename, static_cast<size_t>(nSize));

    errno_t const cvt = __acrt_wcs_to_mbs_cp(
        wide_buffer,
        filename_buffer,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    return static_cast<DWORD>(filename_buffer.size());
}
