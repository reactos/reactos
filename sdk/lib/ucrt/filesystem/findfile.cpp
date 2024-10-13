/***
*findfile.c - C find file functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _findfirst(), _findnext(), and _findclose().
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <errno.h>
#include <corecrt_internal_time.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <corecrt_internal_win32_buffer.h>


//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Utilities for working with the different file type and time data types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template <typename CrtTime>
static CrtTime __cdecl convert_system_time_to_time_t(SYSTEMTIME const& st) throw();

template <>
__time32_t __cdecl convert_system_time_to_time_t(SYSTEMTIME const& st) throw()
{
    return __loctotime32_t(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, -1);
}

template <>
__time64_t __cdecl convert_system_time_to_time_t(SYSTEMTIME const& st) throw()
{
    return __loctotime64_t(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, -1);
}



template <typename CrtTime>
static CrtTime __cdecl convert_file_time_to_time_t(FILETIME const& ft) throw()
{
    // A FILETIME of 0 becomes a time_t of -1:
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0)
        return static_cast<CrtTime>(-1);

    SYSTEMTIME st_utc;
    if (!FileTimeToSystemTime(&ft, &st_utc))
        return static_cast<CrtTime>(-1);

    SYSTEMTIME st_local;
    if (!SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st_local))
        return static_cast<CrtTime>(-1);

    return convert_system_time_to_time_t<CrtTime>(st_local);
}



template <typename Integer>
static Integer convert_file_size_to_integer(DWORD const high, DWORD const low) throw();

template <>
__int64 convert_file_size_to_integer(DWORD const high, DWORD const low) throw()
{
    return static_cast<__int64>(high) * 0x100000000ll + static_cast<__int64>(low);
}

template <>
unsigned long convert_file_size_to_integer(DWORD const high, DWORD const low) throw()
{
    UNREFERENCED_PARAMETER(high);
    return low;
}



template <typename WideFileData, typename NarrowFileData>
_Success_(return)
static bool __cdecl copy_wide_to_narrow_find_data(WideFileData const& wfd, _Out_ NarrowFileData& fd, unsigned int const code_page) throw()
{
    __crt_internal_win32_buffer<char> name;

    errno_t const cvt = __acrt_wcs_to_mbs_cp(wfd.name, name, code_page);

    if (cvt != 0) {
        return false;
    }

    _ERRCHECK(strcpy_s(fd.name, _countof(fd.name), name.data()));

    fd.attrib       = wfd.attrib;
    fd.time_create  = wfd.time_create;
    fd.time_access  = wfd.time_access;
    fd.time_write   = wfd.time_write;
    fd.size         = wfd.size;

    return true;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// The _findfirst family of functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions find the first file matching the given wildcard pattern. If a
// file is found, information about that file is stored in the pointed-to result
// parameter.  The return value is a handle that identifies the group of files
// that match the pattern.  If no file is found, or if an error occurs, errno is
// set and -1 is returned.
//
// There are eight functions in this family, combining {wide name, narrow name}
// x {32-bit file size, 64-bit file size} x {32-bit time_t, 64-bit time_t}.
template <typename WideFileData>
_Success_(return != -1)
static intptr_t __cdecl common_find_first_wide(wchar_t const* const pattern, _Out_ WideFileData* const result) throw()
{
    _VALIDATE_RETURN(result  != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(pattern != nullptr, EINVAL, -1);

    // Ensure the underlying WIN32_FIND_DATA's file name buffer is not larger
    // than ours.
    static_assert(sizeof(WideFileData().name) <= sizeof(WIN32_FIND_DATAW().cFileName), "");

    WIN32_FIND_DATAW wfd;
    HANDLE const hFile = FindFirstFileExW(pattern, FindExInfoStandard, &wfd, FindExSearchNameMatch, nullptr, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD const os_error = GetLastError();
        switch (os_error)
        {
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
            errno = ENOMEM;
            break;

        default:
            errno = EINVAL;
            break;
        }

        return -1;
    }

    result->attrib = wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL
        ? 0
        : wfd.dwFileAttributes;

    typedef decltype(result->time_create) crt_time_type;
    result->time_create = convert_file_time_to_time_t<crt_time_type>(wfd.ftCreationTime);
    result->time_access = convert_file_time_to_time_t<crt_time_type>(wfd.ftLastAccessTime);
    result->time_write  = convert_file_time_to_time_t<crt_time_type>(wfd.ftLastWriteTime);

    typedef decltype(result->size) file_size_type;
    result->size = convert_file_size_to_integer<file_size_type>(wfd.nFileSizeHigh, wfd.nFileSizeLow);

    _ERRCHECK(wcscpy_s(result->name, _countof(result->name), wfd.cFileName));

    return reinterpret_cast<intptr_t>(hFile);
}

template <typename WideFileData, typename NarrowFileData>
_Success_(return != -1)
static intptr_t __cdecl common_find_first_narrow(char const* const pattern, _Out_ NarrowFileData* const result, unsigned int const code_page) throw()
{
    _VALIDATE_RETURN(result != nullptr, EINVAL, -1);

    __crt_internal_win32_buffer<wchar_t> wide_pattern;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(pattern, wide_pattern, code_page);

    if (cvt != 0) {
        return -1;
    }

    WideFileData wide_result;
    intptr_t const handle = common_find_first_wide(wide_pattern.data(), &wide_result);
    if (handle == -1)
        return -1;

    if (!copy_wide_to_narrow_find_data(wide_result, *result, code_page))
        return -1;

    return handle;
}

// Narrow name, 32-bit time_t, 32-bit size
extern "C" intptr_t __cdecl _findfirst32(char const* const pattern, _finddata32_t* const result)
{
    return common_find_first_narrow<_wfinddata32_t>(pattern, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 32-bit time_t, 64-bit size
extern "C" intptr_t __cdecl _findfirst32i64(char const* const pattern, _finddata32i64_t* const result)
{
    return common_find_first_narrow<_wfinddata32i64_t>(pattern, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 64-bit time_t, 32-bit size
extern "C" intptr_t __cdecl _findfirst64i32(char const* const pattern, _finddata64i32_t* const result)
{
    return common_find_first_narrow<_wfinddata64i32_t>(pattern, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 64-bit time_t, 64-bit size
extern "C" intptr_t __cdecl _findfirst64(char const* const pattern, __finddata64_t* const result)
{
    return common_find_first_narrow<_wfinddata64_t>(pattern, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Wide name, 32-bit time_t, 32-bit size
extern "C" intptr_t __cdecl _wfindfirst32(wchar_t const* const pattern, _wfinddata32_t* const result)
{
    return common_find_first_wide(pattern, result);
}

// Wide name, 32-bit time_t, 64-bit size
extern "C" intptr_t __cdecl _wfindfirst32i64(wchar_t const* const pattern, _wfinddata32i64_t* const result)
{
    return common_find_first_wide(pattern, result);
}

// Wide name, 64-bit time_t, 32-bit size
extern "C" intptr_t __cdecl _wfindfirst64i32(wchar_t const* const pattern, _wfinddata64i32_t* const result)
{
    return common_find_first_wide(pattern, result);
}

// Wide name, 64-bit time_t, 64-bit size
extern "C" intptr_t __cdecl _wfindfirst64(wchar_t const* const pattern, _wfinddata64_t* const result)
{
    return common_find_first_wide(pattern, result);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// The _findnext family of functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions perform iteration over a set of files matching a wildcard
// pattern.  The handle argument must be a handle returned by a previous call to
// one of the _findfirst functions that completed successfully.  Each call to a
// _findnext function advances the internal iterator and returns information
// about the next file in the set.
//
// If iteration has not yet completed and a file is found, information about that
// file is stored in the pointed-to result parameter.  The return value is a
// handle that identifies the group of files that match the pattern.  If no file
// is found, or if an error occurs, errno is set and -1 is returned.
//
// There are eight functions in this family, combining {wide name, narrow name}
// x {32-bit file size, 64-bit file size} x {32-bit time_t, 64-bit time_t}.
template <typename WideFileData>
static int __cdecl common_find_next_wide(intptr_t const handle, WideFileData* const result) throw()
{
    HANDLE const os_handle = reinterpret_cast<HANDLE>(handle);

    _VALIDATE_RETURN(os_handle != 0                   , EINVAL, -1);
    _VALIDATE_RETURN(os_handle != INVALID_HANDLE_VALUE, EINVAL, -1);
    _VALIDATE_RETURN(result != nullptr,                 EINVAL, -1);

    WIN32_FIND_DATAW wfd;
    if (!FindNextFileW(os_handle, &wfd))
    {
        DWORD const os_error = GetLastError();
        switch (os_error)
        {
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
            errno = ENOMEM;
            break;

        default:
            errno = EINVAL;
            break;
        }

        return -1;
    }

    result->attrib = wfd.dwFileAttributes == FILE_ATTRIBUTE_NORMAL
        ? 0
        : wfd.dwFileAttributes;

    typedef decltype(result->time_create) crt_time_type;
    result->time_create = convert_file_time_to_time_t<crt_time_type>(wfd.ftCreationTime);
    result->time_access = convert_file_time_to_time_t<crt_time_type>(wfd.ftLastAccessTime);
    result->time_write  = convert_file_time_to_time_t<crt_time_type>(wfd.ftLastWriteTime);

    typedef decltype(result->size) file_size_type;
    result->size = convert_file_size_to_integer<file_size_type>(wfd.nFileSizeHigh, wfd.nFileSizeLow);

    _ERRCHECK(wcscpy_s(result->name, _countof(result->name), wfd.cFileName));

    return 0;
}

template <typename WideFileData, typename NarrowFileData>
static int __cdecl common_find_next_narrow(intptr_t const pattern, NarrowFileData* const result, unsigned int const code_page) throw()
{
    WideFileData wide_result;
    int const return_value = common_find_next_wide(pattern, &wide_result);
    if (return_value == -1)
        return -1;

    if (!copy_wide_to_narrow_find_data(wide_result, *result, code_page))
        return -1;

    return return_value;
}

// Narrow name, 32-bit time_t, 32-bit size
extern "C" int __cdecl _findnext32(intptr_t const handle, _finddata32_t* const result)
{
    return common_find_next_narrow<_wfinddata32_t>(handle, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 32-bit time_t, 64-bit size
extern "C" int __cdecl _findnext32i64(intptr_t const handle, _finddata32i64_t* const result)
{
    return common_find_next_narrow<_wfinddata32i64_t>(handle, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 64-bit time_t, 32-bit size
extern "C" int __cdecl _findnext64i32(intptr_t const handle, _finddata64i32_t* const result)
{
    return common_find_next_narrow<_wfinddata64i32_t>(handle, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Narrow name, 64-bit time_t, 64-bit size
extern "C" int __cdecl _findnext64(intptr_t const handle, __finddata64_t* const result)
{
    return common_find_next_narrow<_wfinddata64_t>(handle, result, __acrt_get_utf8_acp_compatibility_codepage());
}

// Wide name, 32-bit time_t, 32-bit size
extern "C" int __cdecl _wfindnext32(intptr_t const handle, _wfinddata32_t* const result)
{
    return common_find_next_wide(handle, result);
}

// Wide name, 32-bit time_t, 64-bit size
extern "C" int __cdecl _wfindnext32i64(intptr_t const handle, _wfinddata32i64_t* const result)
{
    return common_find_next_wide(handle, result);
}

// Wide name, 64-bit time_t, 32-bit size
extern "C" int __cdecl _wfindnext64i32(intptr_t const handle, _wfinddata64i32_t* const result)
{
    return common_find_next_wide(handle, result);
}

// Wide name, 64-bit time_t, 64-bit size
extern "C" int __cdecl _wfindnext64(intptr_t const handle, _wfinddata64_t* const result)
{
    return common_find_next_wide(handle, result);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// The _findclose function
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This function releases resources associated with a _findfirst/_findnext
// iteration.  It must be called exactly once for each handle returned by
// _findfirst.  Returns 0 on success; -1 on failure.
extern "C" int __cdecl _findclose(intptr_t const handle)
{
    if (!FindClose(reinterpret_cast<HANDLE>(handle)))
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}
