//
// CreateProcessA.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of __acrt_CreateProcessA.
//

#include <corecrt_internal_win32_buffer.h>

BOOL __cdecl __acrt_CreateProcessA(
    LPCSTR const                lpApplicationName,
    LPSTR const                 lpCommandLine,
    LPSECURITY_ATTRIBUTES const lpProcessAttributes,
    LPSECURITY_ATTRIBUTES const lpThreadAttributes,
    BOOL const                  bInheritHandles,
    DWORD const                 dwCreationFlags,
    LPVOID const                lpEnvironment,
    LPCSTR const                lpCurrentDirectory,
    LPSTARTUPINFOW const        lpStartupInfo,
    LPPROCESS_INFORMATION const lpProcessInformation
    )
{
    __crt_internal_win32_buffer<wchar_t> wide_application_name;
    __crt_internal_win32_buffer<wchar_t> wide_command_line;
    __crt_internal_win32_buffer<wchar_t> wide_current_directory;

    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(
        lpApplicationName,
        wide_application_name,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt1 != 0) {
        return FALSE;
    }

    errno_t const cvt2 = __acrt_mbs_to_wcs_cp(
        lpCommandLine,
        wide_command_line,
        __acrt_get_utf8_acp_compatibility_codepage()
        );

    if (cvt2 != 0) {
        return FALSE;
    }

    LPWSTR wide_current_directory_ptr = nullptr;

    if (lpCurrentDirectory != nullptr) {
        errno_t const cvt3 = __acrt_mbs_to_wcs_cp(
            lpCurrentDirectory,
            wide_current_directory,
            __acrt_get_utf8_acp_compatibility_codepage()
            );

        if (cvt3 != 0) {
            return FALSE;
        }

        wide_current_directory_ptr = wide_current_directory.data();
    }

    // converted_command_line is an out parameter, but is not reconverted to utf8 string,
    // since it is only written to in the Wide version. CreateProcessA does not expect it
    // to have changed.
    return ::CreateProcessW(
        wide_application_name.data(),
        wide_command_line.data(),
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        wide_current_directory_ptr,
        lpStartupInfo,
        lpProcessInformation
        );
}
