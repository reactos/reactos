/***
*stat64.c - get file status
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _stat64() - get file status
*
*******************************************************************************/

#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_time.h>
#include <corecrt_internal_win32_buffer.h>
#include <io.h>
#include <share.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>



namespace
{
    struct file_handle_traits
    {
        typedef int type;

        inline static bool close(_In_ type const fh) throw()
        {
            _close(fh);
            return true;
        }

        inline static type get_invalid_value() throw()
        {
            return -1;
        }
    };

    struct find_handle_traits
    {
        typedef HANDLE type;

        static bool close(_In_ type const handle) throw()
        {
            return ::FindClose(handle) != FALSE;
        }

        static type get_invalid_value() throw()
        {
            return INVALID_HANDLE_VALUE;
        }
    };



    typedef __crt_unique_handle_t<file_handle_traits> scoped_file_handle;
    typedef __crt_unique_handle_t<find_handle_traits> unique_find_handle;
}

static bool __cdecl is_slash(wchar_t const c) throw()
{
    return c == L'\\' || c == L'/';
}

static bool __cdecl compute_size(BY_HANDLE_FILE_INFORMATION const& file_info, long& size) throw()
{
    size = 0;
    _VALIDATE_RETURN_NOEXC(file_info.nFileSizeHigh == 0 && file_info.nFileSizeLow <= LONG_MAX, EOVERFLOW, false);

    size = static_cast<long>(file_info.nFileSizeLow);
    return true;
}

static bool __cdecl compute_size(BY_HANDLE_FILE_INFORMATION const& file_info, __int64& size) throw()
{
    size = 0;
    _VALIDATE_RETURN_NOEXC(file_info.nFileSizeHigh <= LONG_MAX, EOVERFLOW, false);

    size = static_cast<__int64>(
        static_cast<unsigned __int64>(file_info.nFileSizeHigh) * 0x100000000ll +
        static_cast<unsigned __int64>(file_info.nFileSizeLow));
    return true;
}

_Success_(return != 0)
static wchar_t* __cdecl call_wfullpath(
    _Out_writes_z_(buffer_size) wchar_t*        const   buffer,
                                wchar_t const*  const   path,
                                size_t          const   buffer_size,
    _Inout_                     wchar_t**       const   buffer_result
    ) throw()
{
    errno_t const saved_errno = errno;
    errno = 0;

    wchar_t* const result = _wfullpath(buffer, path, buffer_size);
    if (result != nullptr)
    {
        errno = saved_errno;
        return result;
    }

    if (errno != ERANGE)
        return nullptr;

    errno = saved_errno;

    *buffer_result = _wfullpath(nullptr, path, 0);
    return *buffer_result;
}

static bool __cdecl has_executable_extension(wchar_t const* const path) throw()
{
    if (!path)
    {
        return false;
    }

    wchar_t const* const last_dot = wcsrchr(path, L'.');
    if (!last_dot)
    {
        return false;
    }

    if (_wcsicmp(last_dot, L".exe") != 0 &&
        _wcsicmp(last_dot, L".cmd") != 0 &&
        _wcsicmp(last_dot, L".bat") != 0 &&
        _wcsicmp(last_dot, L".com") != 0)
    {
        return false;
    }

    return true;
}

static bool __cdecl is_root_or_empty(wchar_t const* const path) throw()
{
    if (!path)
    {
        return false;
    }

    bool const has_drive_letter_and_colon = __ascii_iswalpha(path[0]) && path[1] == L':';
    wchar_t const* const path_start = has_drive_letter_and_colon
        ? path + 2
        : path;

    if (path_start[0] == L'\0')
    {
        return true;
    }

    if (is_slash(path_start[0]) && path_start[1] == L'\0')
    {
        return true;
    }

    return false;
}

static unsigned short __cdecl convert_to_stat_mode(
    int            const attributes,
    wchar_t const* const path
    ) throw()
{
    unsigned const os_mode = attributes & 0xff;

    // check to see if this is a directory - note we must make a special
    // check for the root, which DOS thinks is not a directory:
    bool const is_directory = (os_mode & FILE_ATTRIBUTE_DIRECTORY) != 0;

    unsigned short stat_mode = is_directory || is_root_or_empty(path)
        ? _S_IFDIR | _S_IEXEC
        : _S_IFREG;

    // If attribute byte does not have read-only bit, it is read-write:
    stat_mode |= (os_mode & FILE_ATTRIBUTE_READONLY) != 0
        ? _S_IREAD
        : _S_IREAD | _S_IWRITE;

    // See if file appears to be an executable by checking its extension:
    if (has_executable_extension(path))
        stat_mode |= _S_IEXEC;

    // propagate user read/write/execute bits to group/other fields:
    stat_mode |= (stat_mode & 0700) >> 3;
    stat_mode |= (stat_mode & 0700) >> 6;

    return stat_mode;
}

// Returns false if and only if the path is invalid.
static bool __cdecl get_drive_number_from_path(wchar_t const* const path, int& drive_number) throw()
{
    drive_number = 0;

    // If path has a drive letter and a colon, return the value of that drive,
    // as expected from _getdrive(). A = 1, B = 2, etc.
    // If the path is relative, then use _getdrive() to get the current drive.
    if (__ascii_iswalpha(path[0]) && path[1] == L':')
    {
        // If the path is just a drive letter followed by a colon, it is not a
        // valid input to the stat functions:
        if (path[2] == L'\0')
        {
            __acrt_errno_map_os_error(ERROR_FILE_NOT_FOUND);
            return false;
        }

        drive_number = __ascii_towlower(path[0]) - L'a' + 1;
    }
    else
    {
        drive_number = _getdrive();
    }

    return true;
}

static bool __cdecl is_root_unc_name(wchar_t const* const path) throw()
{
    // The shortest allowed string is of the form //x/y:
    if (wcslen(path) < 5)
        return 0;

    // The string must begin with exactly two consecutive slashes:
    if (!is_slash(path[0]) || !is_slash(path[1]) || is_slash(path[2]))
        return 0;

    // Find the slash between the server name and share name:
    wchar_t const* p = path + 2; // Account for the two slashes
    while (*++p)
    {
        if (is_slash(*p))
            break;
    }

    // We reached the end before finding a slash, or the slash is at the end:
    if (p[0] == L'\0' || p[1] == L'\0')
        return 0;

    // Is there a further slash?
    while (*++p)
    {
        if (is_slash(*p))
            break;
    }

    // Just the final slash (or no final slash):
    if (p[0] == L'\0' || p[1] == L'\0')
        return 1;

    return 0;
}

static bool __cdecl is_usable_drive_or_unc_root(wchar_t const* const path) throw()
{
    if (wcspbrk(path, L"./\\") == nullptr)
        return false;

    wchar_t  full_path_buffer[_MAX_PATH];
    __crt_unique_heap_ptr<wchar_t, __crt_public_free_policy> full_path_pointer;
    wchar_t* const full_path = call_wfullpath(
        full_path_buffer,
        path,
        _MAX_PATH,
        full_path_pointer.get_address_of());

    if (full_path == nullptr)
        return false;

    // Check to see if the path is a root of a directory ("C:\") or a UNC root
    // directory ("\\server\share\"):
    if (wcslen(full_path) != 3 && !is_root_unc_name(full_path))
        return false;

    if (GetDriveTypeW(path) <= 1)
        return false;

    return true;
}

template <typename TimeType>
static TimeType __cdecl convert_filetime_to_time_t(
    FILETIME const file_time,
    TimeType const fallback_time
    ) throw()
{
    using time_traits = __crt_time_time_t_traits<TimeType>;

    if (file_time.dwLowDateTime == 0 && file_time.dwHighDateTime == 0)
    {
        return fallback_time;
    }

    SYSTEMTIME system_time;
    SYSTEMTIME local_time;
    if (!FileTimeToSystemTime(&file_time, &system_time) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &system_time, &local_time))
    {
        // Ignore failures from these APIs, for consistency with the logic below
        // that ignores failures in the conversion from SYSTEMTIME to time_t.
        return -1;
    }

    // If the conversion to time_t fails, it will return -1.  We'll use this as
    // the time_t value instead of failing the entire stat call, to allow callers
    // to get information about files whose time information is not representable.
    // (Callers use this API to test for file existence or to get file sizes.)
    return time_traits::loctotime(
        local_time.wYear,
        local_time.wMonth,
        local_time.wDay,
        local_time.wHour,
        local_time.wMinute,
        local_time.wSecond,
        -1);
}

template <typename StatStruct>
static bool __cdecl common_stat_handle_file_not_opened(
    wchar_t const* const path,
    StatStruct&          result
    ) throw()
{
    using time_traits = __crt_time_time_t_traits<decltype(result.st_mtime)>;

    if (!is_usable_drive_or_unc_root(path))
    {
        __acrt_errno_map_os_error(ERROR_FILE_NOT_FOUND);
        return false;
    }

    // Root directories (such as C:\ or \\server\share\) are fabricated:
    result.st_mode  = convert_to_stat_mode(FILE_ATTRIBUTE_DIRECTORY, path);
    result.st_nlink = 1;

    // Try to get the disk from the name; if there is none, get the current disk:
    int drive_number{};
    if (!get_drive_number_from_path(path, drive_number))
    {
        return false;
    }

    result.st_rdev = static_cast<_dev_t>(drive_number - 1);
    result.st_dev  = static_cast<_dev_t>(drive_number - 1); // A=0, B=1, etc.

    result.st_mtime = time_traits::loctotime(1980, 1, 1, 0, 0, 0, -1);
    result.st_atime = result.st_mtime;
    result.st_ctime = result.st_mtime;
    return true;
}

template <typename StatStruct>
static bool __cdecl common_stat_handle_file_opened(
    wchar_t const* const path,
    int            const fh,
    HANDLE         const handle,
    StatStruct&          result
    ) throw()
{
    using time_type = decltype(result.st_mtime);

    // Figure out what kind of file underlies the file handle:
    int const file_type = GetFileType(handle) & ~FILE_TYPE_REMOTE;

    if (file_type == FILE_TYPE_DISK)
    {
        // Okay, it's a disk file; we'll do the normal logic below.
    }
    else if (file_type == FILE_TYPE_CHAR || file_type == FILE_TYPE_PIPE)
    {
        // We treat pipes and devices similarly:  no further information is
        // available from any API, so we set the fields as reasonably as
        // possible and return.
        result.st_mode = file_type == FILE_TYPE_CHAR
            ? _S_IFCHR
            : _S_IFIFO;

        result.st_nlink = 1;

        result.st_rdev = static_cast<unsigned>(fh);
        result.st_dev  = static_cast<unsigned>(fh);

        if (file_type != FILE_TYPE_CHAR)
        {
            unsigned long available;
            if (PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr))
            {
                result.st_size = static_cast<_off_t>(available);
            }
        }

        return true;
    }
    else if (file_type == FILE_TYPE_UNKNOWN)
    {
        errno = EBADF;
        return false;
    }
    else
    {
        // Per the documentation we should not reach here, but we'll add
        // this just to be safe:
        __acrt_errno_map_os_error(GetLastError());
        return false;
    }

    // At this point, we know we have a file on disk. Set the common fields:
    result.st_nlink = 1;

    if (path)
    {
        // Try to get the disk from the name; if there is none, get the current disk:
        int drive_number{};
        if (!get_drive_number_from_path(path, drive_number))
        {
            return false;
        }

        result.st_rdev = static_cast<_dev_t>(drive_number - 1);
        result.st_dev  = static_cast<_dev_t>(drive_number - 1); // A=0, B=1, etc.
    }

    BY_HANDLE_FILE_INFORMATION file_info{};
    if (!GetFileInformationByHandle(handle, &file_info))
    {
        __acrt_errno_map_os_error(GetLastError());
        return false;
    }

    result.st_mode  = convert_to_stat_mode(file_info.dwFileAttributes, path);

    result.st_mtime = convert_filetime_to_time_t(file_info.ftLastWriteTime, static_cast<time_type>(0));
    result.st_atime = convert_filetime_to_time_t(file_info.ftLastAccessTime, result.st_mtime);
    result.st_ctime = convert_filetime_to_time_t(file_info.ftCreationTime, result.st_mtime);

    if (!compute_size(file_info, result.st_size))
    {
        return false;
    }

    return true;
}

template <typename StatStruct>
static int __cdecl common_stat(
    wchar_t const* const path,
    StatStruct*    const result
    ) throw()
{
    _VALIDATE_CLEAR_OSSERR_RETURN(result != nullptr, EINVAL, -1);
    *result = StatStruct{};

    _VALIDATE_CLEAR_OSSERR_RETURN(path   != nullptr, EINVAL, -1);

    __crt_unique_handle const file_handle(CreateFileW(
        path,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr));

    if (file_handle)
    {
        if (!common_stat_handle_file_opened(path, -1, file_handle.get(), *result))
        {
            *result = StatStruct{};
            return -1;
        }
    }
    else
    {
        if (!common_stat_handle_file_not_opened(path, *result))
        {
            *result = StatStruct{};
            return -1;
        }
    }

    return 0;
}



template <typename StatStruct>
static int __cdecl common_stat(
    char const* const path,
    StatStruct* const result
    ) throw()
{
    if (path == nullptr) {
        return common_stat(static_cast<wchar_t const*>(nullptr), result);
    }

    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return common_stat(wide_path.data(), result);
}



extern "C" int __cdecl _stat32(char const* const path, struct _stat32* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _stat32i64(char const* const path, struct _stat32i64* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _stat64(char const* const path, struct _stat64* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _stat64i32(char const* const path, struct _stat64i32* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _wstat32(wchar_t const* const path, struct _stat32* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _wstat32i64(wchar_t const* const path, struct _stat32i64* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _wstat64(wchar_t const* const path, struct _stat64* const result)
{
    return common_stat(path, result);
}

extern "C" int __cdecl _wstat64i32(wchar_t const* const path, struct _stat64i32* const result)
{
    return common_stat(path, result);
}



template <typename StatStruct>
static int __cdecl common_fstat(int const fh, StatStruct* const result) throw()
{
    _VALIDATE_CLEAR_OSSERR_RETURN(result != nullptr, EINVAL, -1);
    *result = StatStruct{};

    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && fh < _nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);

    return __acrt_lowio_lock_fh_and_call(fh, [&]()
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            return -1;
        }

        if (!common_stat_handle_file_opened(nullptr, fh, reinterpret_cast<HANDLE>(_osfhnd(fh)), *result))
        {
            *result = StatStruct{};
            return -1;
        }

        return 0;
    });
}

extern "C" int __cdecl _fstat32(int const fh, struct _stat32* const result)
{
    return common_fstat(fh, result);
}

extern "C" int __cdecl _fstat32i64(int const fh, struct _stat32i64* const result)
{
    return common_fstat(fh, result);
}

extern "C" int __cdecl _fstat64(int const fh, struct _stat64* const result)
{
    return common_fstat(fh, result);
}

extern "C" int __cdecl _fstat64i32(int const fh, struct _stat64i32* const result)
{
    return common_fstat(fh, result);
}
