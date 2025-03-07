//
// SetCurrentDirectoryA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_SetCurrentDirectoryA.
//

#include <corecrt_internal_win32_buffer.h>

BOOL __cdecl __acrt_SetCurrentDirectoryA(
    LPCSTR const lpPathName
    )
{
    __crt_internal_win32_buffer<wchar_t> wide_path_name;

    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(
        lpPathName,
        wide_path_name,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt1 != 0) {
        return FALSE;
    }

    return ::SetCurrentDirectoryW(wide_path_name.data());
}
