//
// wchmod.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _wchmod() function, which changes file attributes.
//
#include <corecrt_internal.h>
#include <io.h>
#include <sys\stat.h>



// Changes the mode of a file.  The only supported mode bit is _S_IWRITE, which
// controls the user write (read-only) attribute of the file.  Returns zero if
// successful; returns -1 and sets errno and _doserrno on failure.
extern "C" int __cdecl _wchmod(wchar_t const* const path, int const mode)
{
    _VALIDATE_CLEAR_OSSERR_RETURN(path != nullptr, EINVAL, -1);

    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    // Set or clear the read-only flag:
    if (mode & _S_IWRITE)
    {
        attributes.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    }
    else
    {
        attributes.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    }

    if (!SetFileAttributesW(path, attributes.dwFileAttributes))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}
