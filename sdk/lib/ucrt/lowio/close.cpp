//
// close.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _close(), which closes a file.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>



// Closes the underlying OS file handle, if and only if nt needs to be closed.
// Returns 0 on success; returns the OS error on failure.
static DWORD close_os_handle_nolock(int const fh) throw()
{
    // If the underlying handle is INVALID_HANDLE_VALUE, don't try to acutally
    // close it:
    if (_get_osfhandle(fh) == (intptr_t)INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    // If the file handle is stdout or stderr, and if stdout and stderr are
    // mapped to the same OS file handle, skip the CloseHandle without error.
    //
    // stdout and stderr are the only handles for which this support is
    // provided.  Other handles may be mapped to the same OS file handle only
    // at the programmer's risk.
    bool is_other_std_handle_open =
        fh == 1 && (_osfile(2) & FOPEN) ||
        fh == 2 && (_osfile(1) & FOPEN);

    if (is_other_std_handle_open && _get_osfhandle(1) == _get_osfhandle(2))
    {
        return 0;
    }

    // Otherwise, we can go ahead and close the handle:
    if (CloseHandle(reinterpret_cast<HANDLE>(_get_osfhandle(fh))))
    {
        return 0;
    }

    return GetLastError();
}



// Closes the file associated with the given file handle.  On success, returns 0.
// On failure, returns -1 and sets errno.
extern "C" int __cdecl _close_internal(int const fh, __crt_cached_ptd_host& ptd)
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (_osfile(fh) & FOPEN), EBADF, -1);

    return __acrt_lowio_lock_fh_and_call(fh, [&]()
    {
        if (_osfile(fh) & FOPEN)
        {
            return _close_nolock_internal(fh, ptd);
        }
        else
        {
            ptd.get_errno().set(EBADF);
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            return -1;
        }
    });
}

extern "C" int __cdecl _close(int const fh)
{
    __crt_cached_ptd_host ptd;
    return _close_internal(fh, ptd);
}



extern "C" int __cdecl _close_nolock_internal(int const fh, __crt_cached_ptd_host& ptd)
{
    DWORD const close_os_handle_error = close_os_handle_nolock(fh);

    _free_osfhnd(fh);
    _osfile(fh) = 0;

    if (close_os_handle_error != 0)
    {
        __acrt_errno_map_os_error_ptd(close_os_handle_error, ptd);
        return -1;
    }

    return 0;
}

extern "C" int __cdecl _close_nolock(int const fh)
{
    __crt_cached_ptd_host ptd;
    return _close_nolock_internal(fh, ptd);
}
