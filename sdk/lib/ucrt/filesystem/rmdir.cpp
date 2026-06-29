//
// rmdir.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _rmdir() function, which removes a directory.
//
#include <corecrt_internal.h>
#include <direct.h>
#include <corecrt_internal_win32_buffer.h>

// Removes the directory specified by the path.  The directory must be empty, it
// must not be the current working directory, and it must not be the root of any
// drive.  Returns 0 on success; returns -1 and sets errno and _doserrno on
// failure.
extern "C" int __cdecl _rmdir(char const* const path)
{
    if (path == nullptr) {
        return _wrmdir(nullptr);
    }

    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return _wrmdir(wide_path.data());
}
