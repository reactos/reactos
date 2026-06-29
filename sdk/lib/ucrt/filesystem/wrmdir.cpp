//
// wrmdir.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _wrmdir() function, which removes a directory.
//
#include <corecrt_internal.h>
#include <direct.h>



// Removes the directory specified by the path.  The directory must be empty, it
// must not be the current working directory, and it must not be the root of any
// drive.  Returns 0 on success; returns -1 and sets errno and _doserrno on
// failure.
extern "C" int __cdecl _wrmdir(wchar_t const* const path)
{
    if (!RemoveDirectoryW(path))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }
    
    return 0;
}
