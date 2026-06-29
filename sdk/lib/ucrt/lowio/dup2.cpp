//
// dup2.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _dup2() and _dup2_nolock, which duplicate lowio file handles
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>

static int __cdecl _dup2_nolock_internal(int const source_fh, int const target_fh, __crt_cached_ptd_host& ptd) throw()
{
    if ((_osfile(source_fh) & FOPEN) == 0)
    {
        // If the source handle is not open, return an error.  Noe that the
        // DuplicateHandle API will not detect this error, because it implies
        // that _osfhnd(source_fh) == INVALID_HANDLE_VALUE, and this is a
        // legal HANDLE value to be duplicated.
        ptd.get_errno().set(EBADF);
        ptd.get_doserrno().set(0);
        _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
        return -1;
    }

    // Duplicate the source file onto the target file:
    intptr_t new_osfhandle;

    BOOL const result = DuplicateHandle(
        GetCurrentProcess(),
        reinterpret_cast<HANDLE>(_get_osfhandle(source_fh)),
        GetCurrentProcess(),
        &reinterpret_cast<HANDLE&>(new_osfhandle),
        0,
        TRUE,
        DUPLICATE_SAME_ACCESS);

    if (!result)
    {
        __acrt_errno_map_os_error_ptd(GetLastError(), ptd);
        return -1;
    }

    // If the target is open, close it once we know the OS handle was duplicated successfully.
    // We ignore the possibility of an error here: an error simply means that the OS handle
    // value may remain bound for the duration of the process.
    if (_osfile(target_fh) & FOPEN)
    {
        _close_nolock_internal(target_fh, ptd);
    }

    __acrt_lowio_set_os_handle(target_fh, new_osfhandle);

    // Copy the _osfile information, with the FNOINHERIT bit cleared:
    _osfile(target_fh) = _osfile(source_fh) & ~FNOINHERIT;
    _textmode(target_fh) = _textmode(source_fh);
    _tm_unicode(target_fh) = _tm_unicode(source_fh);

    return 0;
}

static int __cdecl _dup2_internal(int const source_fh, int const target_fh, __crt_cached_ptd_host& ptd) throw()
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, source_fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (source_fh >= 0 && (unsigned)source_fh < (unsigned)_nhandle), EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (_osfile(source_fh) & FOPEN), EBADF, -1);
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, target_fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, ((unsigned)target_fh < _NHANDLE_), EBADF, -1);

    // Make sure there is an __crt_lowio_handle_data struct corresponding to the target_fh:
    if (target_fh >= _nhandle && __acrt_lowio_ensure_fh_exists(target_fh) != 0)
    {
        return -1;
    }

    // If the source and target are the same, return success (we've already
    // verified that the file handle is open, above).  This is for conformance
    // with the POSIX specification for dup2.
    if (source_fh == target_fh)
    {
        return 0;
    }

    // Obtain the two file handle locks.  In order to prevent deadlock, we
    // always obtain the lock for the lower-numbered file handle first:
    if (source_fh < target_fh)
    {
        __acrt_lowio_lock_fh(source_fh);
        __acrt_lowio_lock_fh(target_fh);
    }
    else if (source_fh > target_fh)
    {
        __acrt_lowio_lock_fh(target_fh);
        __acrt_lowio_lock_fh(source_fh);
    }

    int result = 0;

    __try
    {
        result = _dup2_nolock_internal(source_fh, target_fh, ptd);
    }
    __finally
    {
        // The order in which we unlock the file handles does not matter:
        __acrt_lowio_unlock_fh(source_fh);
        __acrt_lowio_unlock_fh(target_fh);
    }
    __endtry
    return result;
}

// _dup2() makes the target file handle a duplicate of the source file handle,
// so that both handles refer to the same file.  If the target handle is open
// upon entry, it is closed so that it is not leaked.
//
// Returns 0 if successful; returns -1 and sets errno on failure.
extern "C" int __cdecl _dup2(int const source_fh, int const target_fh)
{
    __crt_cached_ptd_host ptd;
    return _dup2_internal(source_fh, target_fh, ptd);
}
