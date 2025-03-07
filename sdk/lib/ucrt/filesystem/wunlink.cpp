//
// wunlink.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _wremove() and _wunlink() functions, which remove (delete) files.
//
#include <corecrt_internal.h>
#include <stdio.h>



// Deletes the specified file.  Returns zero if successful; returns -1 and sets
// errno and _doserrno on failure.
extern "C" int __cdecl _wremove(wchar_t const* const path)
{
    if (!DeleteFileW(path))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}



extern "C" int __cdecl _wunlink(wchar_t const* const path)
{
    return _wremove(path);
}
