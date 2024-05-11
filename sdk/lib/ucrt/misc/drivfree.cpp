/***
*drivfree.c - Get the size of a disk
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file has _getdiskfree()
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <direct.h>



// Gets the size information for the given drive.  The 'drive_number' must be a
// number, 1 through 26, corresponding to drives A: through Z:.  Returns zero on
// success; a system error code on failure.
extern "C" unsigned __cdecl _getdiskfree(
    unsigned     const drive_number,
    _diskfree_t* const result
)
{
    _VALIDATE_RETURN(result != nullptr,  EINVAL, ERROR_INVALID_PARAMETER);
    _VALIDATE_RETURN(drive_number <= 26, EINVAL, ERROR_INVALID_PARAMETER);

    *result = _diskfree_t();

    wchar_t drive_name_buffer[4] = { '_', ':', '\\', '\0' };

    wchar_t const* const drive_name = drive_number == 0
        ? nullptr
        : drive_name_buffer;

    if (drive_number != 0)
    {
        drive_name_buffer[0] = static_cast<wchar_t>(drive_number) + static_cast<wchar_t>(L'A' - 1);
    }

    static_assert(sizeof(result->sectors_per_cluster) == sizeof(DWORD), "Unexpected sizeof long");

    if (!GetDiskFreeSpaceW(drive_name,
        reinterpret_cast<LPDWORD>(&result->sectors_per_cluster),
        reinterpret_cast<LPDWORD>(&result->bytes_per_sector),
        reinterpret_cast<LPDWORD>(&result->avail_clusters),
        reinterpret_cast<LPDWORD>(&result->total_clusters)))
    {
        int const os_error = GetLastError();
        errno = __acrt_errno_from_os_error(os_error);

        return os_error;
    }

    return 0;
}
