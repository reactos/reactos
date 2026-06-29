//
// lseek.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _lseek(), which seeks the file pointer of a file.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>
#include <stdlib.h>

#if SEEK_SET != FILE_BEGIN || SEEK_CUR != FILE_CURRENT || SEEK_END != FILE_END
    #error Xenix and Win32 seek constants not compatible
#endif



// These functions actually seek the underlying operating system file handle.
// The logic is different for 32-bit and 64-bit seek values because a seek may
// end up moving out of range of 32-bit values.
static long __cdecl common_lseek_do_seek_nolock(HANDLE const os_handle, long const offset, int const origin, __crt_cached_ptd_host& ptd) throw()
{
    LARGE_INTEGER const origin_pos = { 0 };

    LARGE_INTEGER saved_pos;
    if (!SetFilePointerEx(os_handle, origin_pos, &saved_pos, FILE_CURRENT))
    {
        __acrt_errno_map_os_error_ptd(GetLastError(), ptd);
        return -1;
    }

    LARGE_INTEGER seek_pos = { 0 };
    seek_pos.QuadPart = offset;

    LARGE_INTEGER new_pos = { 0 };
    if (!SetFilePointerEx(os_handle, seek_pos, &new_pos, origin))
    {
        __acrt_errno_map_os_error_ptd(GetLastError(), ptd);
        return -1;
    }

    // The call succeeded, but the new file pointer location is too large for
    // the return type or is a negative value.  So, restore the file pointer
    // to the saved location and return an error:
    if (new_pos.QuadPart > LONG_MAX)
    {
        SetFilePointerEx(os_handle, saved_pos, nullptr, FILE_BEGIN);
        ptd.get_errno().set(EINVAL);
        return -1;
    }

    return static_cast<long>(new_pos.LowPart);
}

static __int64 __cdecl common_lseek_do_seek_nolock(HANDLE const os_handle, __int64 const offset, int const origin, __crt_cached_ptd_host& ptd) throw()
{
    LARGE_INTEGER new_pos;
    if (!SetFilePointerEx(os_handle, *reinterpret_cast<LARGE_INTEGER const*>(&offset), &new_pos, origin))
    {
        __acrt_errno_map_os_error_ptd(GetLastError(), ptd);
        return -1;
    }

    return new_pos.QuadPart;
}


template <typename Integer>
static Integer __cdecl common_lseek_nolock(int const fh, Integer const offset, int const origin, __crt_cached_ptd_host& ptd) throw()
{
    HANDLE const os_handle = reinterpret_cast<HANDLE>(_get_osfhandle(fh));
    if (os_handle == reinterpret_cast<HANDLE>(-1))
    {
        ptd.get_errno().set(EBADF);
        _ASSERTE(("Invalid file descriptor",0));
        return -1;
    }

    Integer const new_position = common_lseek_do_seek_nolock(os_handle, offset, origin, ptd);
    if (new_position == -1)
    {
        return -1;
    }

    // The call succeeded, so return success:
    _osfile(fh) &= ~FEOFLAG; // Clear the Ctrl-Z flag
    return new_position;
}

// Moves the file pointer associated with the given file to a new position.  The
// new position is 'offset' bytes (the offset may be negative) away from the
// origin specified by 'origin'.
//
//     If origin == SEEK_SET, the origin in the beginning of file
//     If origin == SEEK_CUR, the origin is the current file pointer position
//     If origin == SEEK_END, the origin is the end of the file
//
// Returns the offset, in bytes, of the new position, from the beginning of the
// file.  Returns -1 and sets errno on failure.  Note that seeking beyond the
// end of the file is not an error, but seeking before the beginning is.
template <typename Integer>
static Integer __cdecl common_lseek(int const fh, Integer const offset, int const origin, __crt_cached_ptd_host& ptd) throw()
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, _osfile(fh) & FOPEN, EBADF, -1);

    __acrt_lowio_lock_fh(fh);
    Integer result = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            ptd.get_errno().set(EBADF);
            ptd.get_doserrno().set(0);
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            __leave;
        }

        result = common_lseek_nolock(fh, offset, origin, ptd);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return result;
}

extern "C" long __cdecl _lseek_internal(int const fh, long const offset, int const origin, __crt_cached_ptd_host& ptd)
{
    return common_lseek(fh, offset, origin, ptd);
}

extern "C" __int64 __cdecl _lseeki64_internal(int const fh, __int64 const offset, int const origin, __crt_cached_ptd_host& ptd)
{
    return common_lseek(fh, offset, origin, ptd);
}

extern "C" long __cdecl _lseek_nolock_internal(int const fh, long const offset, int const origin, __crt_cached_ptd_host& ptd)
{
    return common_lseek_nolock(fh, offset, origin, ptd);
}

extern "C" __int64 __cdecl _lseeki64_nolock_internal(int const fh, __int64 const offset, int const origin, __crt_cached_ptd_host& ptd)
{
    return common_lseek_nolock(fh, offset, origin, ptd);
}

extern "C" long __cdecl _lseek(int const fh, long const offset, int const origin)
{
    __crt_cached_ptd_host ptd;
    return common_lseek(fh, offset, origin, ptd);
}

extern "C" __int64 __cdecl _lseeki64(int const fh, __int64 const offset, int const origin)
{
    __crt_cached_ptd_host ptd;
    return common_lseek(fh, offset, origin, ptd);
}

extern "C" long __cdecl _lseek_nolock(int const fh, long const offset, int const origin)
{
    __crt_cached_ptd_host ptd;
    return common_lseek_nolock(fh, offset, origin, ptd);
}

extern "C" __int64 __cdecl _lseeki64_nolock(int const fh, __int64 const offset, int const origin)
{
    __crt_cached_ptd_host ptd;
    return common_lseek_nolock(fh, offset, origin, ptd);
}
