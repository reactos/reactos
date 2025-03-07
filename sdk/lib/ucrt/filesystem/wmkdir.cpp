//
// wmkdir.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _wmkdir() function, which creates a directory.
//
#include <corecrt_internal.h>
#include <direct.h>



// Creates a directory.  Returns 0 on success; returns -1 and sets errno and
// _doserrno on failure.
extern "C" int __cdecl _wmkdir(wchar_t const* const path)
{
    if (!CreateDirectoryW(path, nullptr))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}
