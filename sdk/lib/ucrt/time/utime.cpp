//
// utime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The utime and futime families of functions, which set the modification time
// for a file.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_time.h>



// Sets the modification time for a file, where 'file_name' is the name of the
// file.  Returns zero on success; returns -1 and sets errno on failure.
template <typename TimeType, typename Character, typename TimeBufferType>
static int __cdecl common_utime(
    Character const* const file_name,
    TimeBufferType*  const times
    ) throw()
{
    typedef __crt_time_traits<TimeType, Character> time_traits;

    _VALIDATE_RETURN(file_name != nullptr, EINVAL, -1)

    // Open the file, since the underlying system call needs a handle.  Note
    // that the _utime definition says you must have write permission for the
    // file to change its time, so we open the file for write only.  Also, we
    // must force it to open in binary mode so that we don't remove ^Z's from
    // binary files.
    int fh;
    if (time_traits::tsopen_s(&fh, file_name, _O_RDWR | _O_BINARY, _SH_DENYNO, 0) != 0)
        return -1;

    int const result = time_traits::futime(fh, times);

    errno_t stored_errno = 0;
    if (result == -1)
        stored_errno = errno;

    _close(fh);

    if (result == -1)
        errno = stored_errno;

    return result;
}

extern "C" int __cdecl _utime32(char const* const file_name, __utimbuf32* const times)
{
    return common_utime<__time32_t>(file_name, times);
}

extern "C" int __cdecl _wutime32(wchar_t const* const file_name, __utimbuf32* const times)
{
    return common_utime<__time32_t>(file_name, times);
}

extern "C" int __cdecl _utime64(char const* const file_name, __utimbuf64* const times)
{
    return common_utime<__time64_t>(file_name, times);
}

extern "C" int __cdecl _wutime64(wchar_t const* const file_name, __utimbuf64* const times)
{
    return common_utime<__time64_t>(file_name, times);
}



// Sets the modification time for an open file, where the 'fh' is the lowio file
// handle of the open file.  Returns zero on success; returns -1 and sets errno
// on failure.
template <typename TimeType, typename TimeBufferType>
static int __cdecl common_futime(int const fh, TimeBufferType* times) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    _CHECK_FH_RETURN(fh, EBADF, -1);

    _VALIDATE_RETURN(fh >= 0 && static_cast<unsigned>(fh) < static_cast<unsigned>(_nhandle), EBADF, -1);
    _VALIDATE_RETURN(_osfile(fh) & FOPEN, EBADF, -1);

    TimeBufferType default_times;

    if (times == nullptr)
    {
        time_traits::time(&default_times.modtime);
        default_times.actime = default_times.modtime;
        times = &default_times;
    }

    tm tm_value;
    if (time_traits::localtime_s(&tm_value, &times->modtime) != 0)
    {
        errno = EINVAL;
        return -1;
    }

    SYSTEMTIME local_time;
    local_time.wYear   = static_cast<WORD>(tm_value.tm_year + 1900);
    local_time.wMonth  = static_cast<WORD>(tm_value.tm_mon + 1);
    local_time.wDay    = static_cast<WORD>(tm_value.tm_mday);
    local_time.wHour   = static_cast<WORD>(tm_value.tm_hour);
    local_time.wMinute = static_cast<WORD>(tm_value.tm_min);
    local_time.wSecond = static_cast<WORD>(tm_value.tm_sec);
    local_time.wMilliseconds = 0;

    SYSTEMTIME system_time;
    FILETIME last_write_time;
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &local_time, &system_time) ||
        !SystemTimeToFileTime(&system_time, &last_write_time))
    {
        errno = EINVAL;
        return -1;
    }

    if (time_traits::localtime_s(&tm_value, &times->actime) != 0)
    {
        errno = EINVAL;
        return -1;
    }

    local_time.wYear   = static_cast<WORD>(tm_value.tm_year + 1900);
    local_time.wMonth  = static_cast<WORD>(tm_value.tm_mon + 1);
    local_time.wDay    = static_cast<WORD>(tm_value.tm_mday);
    local_time.wHour   = static_cast<WORD>(tm_value.tm_hour);
    local_time.wMinute = static_cast<WORD>(tm_value.tm_min);
    local_time.wSecond = static_cast<WORD>(tm_value.tm_sec);
    local_time.wMilliseconds = 0;

    FILETIME last_access_time;
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &local_time, &system_time) ||
        !SystemTimeToFileTime(&system_time, &last_access_time))
    {
        errno = EINVAL;
        return -1;
    }

    if (!SetFileTime(reinterpret_cast<HANDLE>(_get_osfhandle(fh)), nullptr, &last_access_time, &last_write_time))
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

extern "C" int __cdecl _futime32(int const fh, __utimbuf32* const times)
{
    return common_futime<__time32_t>(fh, times);
}

extern "C" int __cdecl _futime64(int const fh, __utimbuf64* const times)
{
    return common_futime<__time64_t>(fh, times);
}
