//
// drive.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _getdrive() and _chdrive() functions, which get and set the current drive.
//
#include <corecrt_internal.h>
#include <ctype.h>
#include <direct.h>
#include <errno.h>

static int __cdecl get_drive_number_from_path(wchar_t const* const path) throw()
{
    if (path[0] == L'\0' || path[1] != L':')
        return 0;

    return __ascii_towupper(path[0]) - L'A' + 1;
}



// Gets the current drive number (A: => 1, B: => 2, etc.).  Returns zero if the
// current drive cannot be determined.
extern "C" int __cdecl _getdrive()
{
    wchar_t cwd[MAX_PATH + 1] = { 0 };

    DWORD const cwd_length = GetCurrentDirectoryW(MAX_PATH + 1, cwd);
    if (cwd_length <= MAX_PATH)
    {
        return get_drive_number_from_path(cwd);
    }

    // Otherwise, we need more space for the path, so we'll need to dynamically
    // allocate a buffer:
    __crt_unique_heap_ptr<wchar_t> const heap_cwd(_calloc_crt_t(wchar_t, cwd_length + 1));
    if (heap_cwd.get() == nullptr)
    {
        errno = ENOMEM;
        return 0;
    }

    DWORD const heap_cwd_length = GetCurrentDirectoryW(cwd_length + 1, heap_cwd.get());
    if (heap_cwd_length == 0)
    {
        errno = ENOMEM;
        return 0;
    }

    return get_drive_number_from_path(heap_cwd.get());
}



// Changes the current drive to the specified drive number.  Returns zero on
// success; returns -1 and sets errno and _doserrno on failure.
extern "C" int __cdecl _chdrive(int const drive_number)
{
    if (drive_number < 1 || drive_number > 26)
    {
        _doserrno = ERROR_INVALID_DRIVE;
        _VALIDATE_RETURN(("Invalid Drive Index", 0), EACCES, -1);
    }

#pragma warning(suppress:__WARNING_UNUSED_ASSIGNMENT) // 28931 unused assignment of variable drive_letter
    wchar_t const drive_letter   = static_cast<wchar_t>(L'A' + drive_number - 1);
    wchar_t const drive_string[] = { drive_letter, L':', L'\0' };

    if (!SetCurrentDirectoryW(drive_string))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}
