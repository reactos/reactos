//
// wrename.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _wrename() function, which renames a file.
//
#include <corecrt_internal.h>
#include <io.h>



// Renames the file named 'old_name' to be named 'new_name'.  Returns zero if
// successful; returns -1 and sets errno and _doserrno on failure.
extern "C" int __cdecl _wrename(wchar_t const* const old_name, wchar_t const* const new_name)
{
    // The MOVEFILE_COPY_ALLOWED flag alloes moving to a different volume.
    if (!MoveFileExW(old_name, new_name, MOVEFILE_COPY_ALLOWED))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    return 0;
}
