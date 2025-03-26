//
// chmod.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _chmod() function, which changes file attributes.
//
#include <corecrt_internal.h>
#include <io.h>
#include <sys/stat.h>
#include <corecrt_internal_win32_buffer.h>

// Changes the mode of a file.  The only supported mode bit is _S_IWRITE, which
// controls the user write (read-only) attribute of the file.  Returns zero if
// successful; returns -1 and sets errno and _doserrno on failure.
extern "C" int __cdecl _chmod(char const* const path, int const mode)
{
    if (path == nullptr) {
        return _wchmod(nullptr, mode);
    }

    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return _wchmod(wide_path.data(), mode);
}
