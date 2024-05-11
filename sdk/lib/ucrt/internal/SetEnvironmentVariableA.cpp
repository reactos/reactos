//
// SetEnvironmentVariableA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_SetEnvironmentVariableA.
//

#include <corecrt_internal_win32_buffer.h>

BOOL __cdecl __acrt_SetEnvironmentVariableA(
    LPCSTR const lpName,
    LPCSTR const lpValue
    )
{
    __crt_internal_win32_buffer<wchar_t> wide_name;
    __crt_internal_win32_buffer<wchar_t> wide_value;

    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(
        lpName,
        wide_name,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt1 != 0) {
        return FALSE;
    }

    errno_t const cvt2 = __acrt_mbs_to_wcs_cp(
        lpValue,
        wide_value,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt2 != 0) {
        return FALSE;
    }

    return ::SetEnvironmentVariableW(wide_name.data(), wide_value.data());
}
