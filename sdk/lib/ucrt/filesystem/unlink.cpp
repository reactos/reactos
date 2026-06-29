//
// unlink.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The remove() and unlink() functions, which remove (delete) files.
//
#include <corecrt_internal.h>
#include <stdio.h>
#include <corecrt_internal_win32_buffer.h>

// Deletes the specified file.  Returns zero if successful; returns -1 and sets
// errno and _doserrno on failure.
extern "C" int __cdecl remove(char const* const path)
{
    if (path == nullptr)
        return _wremove(nullptr);

    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return _wremove(wide_path.data());
}



extern "C" int __cdecl _unlink(char const* const path)
{
    return remove(path);
}
