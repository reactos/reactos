//
// dup.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _dup() and _dup_nolock, which duplicate lowio file handles
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>

static int __cdecl duplicate_osfhnd(int const fh, int const new_fh, __crt_cached_ptd_host& ptd) throw()
{
    // Duplicate the file handle:
    intptr_t new_osfhandle;

    BOOL const result = DuplicateHandle(
        GetCurrentProcess(),
        reinterpret_cast<HANDLE>(_get_osfhandle(fh)),
        GetCurrentProcess(),
        &reinterpret_cast<HANDLE&>(new_osfhandle),
        0L,
        TRUE,
        DUPLICATE_SAME_ACCESS);

    if (!result)
    {
        __acrt_errno_map_os_error_ptd(GetLastError(), ptd);
        return -1;
    }

    // Duplicate the handle state:
    __acrt_lowio_set_os_handle(new_fh, new_osfhandle);
    _osfile(new_fh) = _osfile(fh) & ~FNOINHERIT;
    _textmode(new_fh) = _textmode(fh);
    _tm_unicode(new_fh) = _tm_unicode(fh);
    return new_fh;
}

static int __cdecl _dup_nolock_internal(int const fh, __crt_cached_ptd_host& ptd) throw()
{
    if ((_osfile(fh) & FOPEN) == 0)
    {
        ptd.get_errno().set(EBADF);
        ptd.get_doserrno().set(0);
        _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread", 0));
        return -1;
    }

    // Allocate a duplicate handle
    int const new_fh = _alloc_osfhnd();
    if (new_fh == -1)
    {
        ptd.get_errno().set(EMFILE);
        ptd.get_doserrno().set(0);
        return -1;
    }

    int return_value = -1;
    __try
    {
        return_value = duplicate_osfhnd(fh, new_fh, ptd);
    }
    __finally
    {
        // The handle returned by _alloc_osfhnd is both open and locked.  If we
        // failed to duplicate the handle, we need to abandon the handle by
        // unsetting the open flag.  We always need to unlock the handle:
        if (return_value == -1)
        {
            _osfile(new_fh) &= ~FOPEN;
        }

        __acrt_lowio_unlock_fh(new_fh);
    }
    __endtry
    return return_value;
}

static int __cdecl _dup_internal(int const fh, __crt_cached_ptd_host& ptd)
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (_osfile(fh) & FOPEN), EBADF, -1);

    return __acrt_lowio_lock_fh_and_call(fh, [&](){
        return _dup_nolock_internal(fh, ptd);
    });
}

// _dup() duplicates a file handle and returns the duplicate.  If the function
// fails, -1 is returned and errno is set.
extern "C" int __cdecl _dup(int const fh)
{
    __crt_cached_ptd_host ptd;
    return _dup_internal(fh, ptd);
}
