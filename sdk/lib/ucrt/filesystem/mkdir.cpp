//
// mkdir.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _mkdir() function, which creates a directory.
//
#include <corecrt_internal.h>
#include <direct.h>
#include <corecrt_internal_win32_buffer.h>

// Creates a directory.  Returns 0 on success; returns -1 and sets errno and
// _doserrno on failure.
extern "C" int __cdecl _mkdir(char const* const path)
{
    if (path == nullptr) {
        return _wmkdir(nullptr);
    }

    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return _wmkdir(wide_path.data());
}
