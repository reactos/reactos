//
// waccess.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _waccess() and _waccess_s() functions, which test file accessibility.
//
#include <corecrt_internal.h>
#include <io.h>



// Tests whether the specified file can be accessed with the specified mode.  The
// access_mode must be one of:  0 (exist only), 2 (write), 4 (read), 6 (read and
// write).  Returns zero if the file can be accessed with the given mode; returns
// an error code otherwise or if an error occurs.
extern "C" errno_t __cdecl _waccess_s(wchar_t const* const path, int const access_mode)
{
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(path != nullptr,           EINVAL);
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE((access_mode & (~6)) == 0, EINVAL);

    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes))
    {
        __acrt_errno_map_os_error(GetLastError());
        return errno;
    }

    // All directories have both read and write access:
    if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return 0;

    // If we require write access, make sure the read only flag is not set:
    bool const file_is_read_only = (attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    bool const mode_requires_write = (access_mode & 2) != 0;
    
    if (file_is_read_only && mode_requires_write)
    {
        _doserrno = ERROR_ACCESS_DENIED;
        errno = EACCES;
        return errno;
    }

    // Otherwise, the file is accessible:
    return 0;

}



// The same as _waccess_s, but transforms all errors into a -1 return value.
extern "C" int __cdecl _waccess(wchar_t const* const path, int const access_mode)
{
    return _waccess_s(path, access_mode) == 0 ? 0 : -1;
}
