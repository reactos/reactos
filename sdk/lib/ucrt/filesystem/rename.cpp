//
// rename.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The rename() function, which renames a file.
//
#include <corecrt_internal.h>
#include <io.h>
#include <corecrt_internal_win32_buffer.h>



// See _wrename() for details about the behavior of this function.  (This
// function simply converts the multibyte strings to wide strings and calls
// _wrename().)
extern "C" int __cdecl rename(char const* const old_name, char const* const new_name)
{
    unsigned int const code_page = __acrt_get_utf8_acp_compatibility_codepage();

    __crt_internal_win32_buffer<wchar_t> wide_old_name;

    errno_t cvt1 = __acrt_mbs_to_wcs_cp(old_name, wide_old_name, code_page);
    if (cvt1 != 0)
    {
        errno = cvt1;
        return -1;
    }

    __crt_internal_win32_buffer<wchar_t> wide_new_name;
    errno_t cvt2 = __acrt_mbs_to_wcs_cp(new_name, wide_new_name, code_page);
    if (cvt2 != 0)
    {
        errno = cvt2;
        return -1;
    }

    return _wrename(wide_old_name.data(), wide_new_name.data());
}
