//
// locking.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _locking(), which locks and unlocks regions of a file.
//
#include <corecrt_internal_lowio.h>
#include <sys/locking.h>



static int __cdecl locking_nolock(int const fh, int const locking_mode, long const number_of_bytes) throw()
{
    __int64 const lock_offset = _lseeki64_nolock(fh, 0L, SEEK_CUR);
    if (lock_offset == -1)
        return -1;

    OVERLAPPED overlapped = { 0 };
    overlapped.Offset     = static_cast<DWORD>(lock_offset);
    overlapped.OffsetHigh = static_cast<DWORD>((lock_offset >> 32) & 0xffffffff);

    // Set the retry count, based on the mode:
    bool const allow_retry = locking_mode == _LK_LOCK || locking_mode == _LK_RLCK;
    int  const retry_count = allow_retry ? 10 : 1;

    // Ask the OS to lock the file either until the request succeeds or the
    // retry count is reached, whichever comes first.  Note that the only error
    // possible is a locking violation, since an invalid handle would have
    // already failed above.
    bool succeeded = false;
    for (int i = 0; i != retry_count; ++i)
    {
        if (locking_mode == _LK_UNLCK)
        {
            succeeded = UnlockFileEx(
                reinterpret_cast<HANDLE>(_get_osfhandle(fh)),
                0,
                number_of_bytes,
                0,
                &overlapped) == TRUE;
        }
        else
        {
            // Ensure exclusive lock access, and return immediately if lock
            // acquisition fails:
            succeeded = LockFileEx(
                reinterpret_cast<HANDLE>(_get_osfhandle(fh)),
                LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                0,
                number_of_bytes,
                0,
                &overlapped) == TRUE;
        }

        if (succeeded)
        {
            break;
        }

        // Doesnt sleep on the last try
        if (i != retry_count - 1)
        {
            Sleep(1000);
        }
    }

    // If an OS error occurred (e.g., if the file was already locked), return
    // EDEADLOCK if this was ablocking call; otherwise map the error noramlly:
    if (!succeeded)
    {
        __acrt_errno_map_os_error(GetLastError());
        if (locking_mode == _LK_LOCK || locking_mode == _LK_RLCK)
        {
            errno = EDEADLOCK;
        }

        return -1;
    }

    return 0;
}



// Locks or unlocks the requested number of bytes in the specified file.
//
// Note that this function acquires the lock for the specified file and holds
// this lock for the entire duration of the call, even during the one second
// delays between calls into the operating system.  This is to prevent other
// threads from changing the file during the call.
//
// Returns 0 on success; returns -1 and sets errno on failure.
extern "C" int __cdecl _locking(int const fh, int const locking_mode, long const number_of_bytes)
{
    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(number_of_bytes >= 0, EINVAL, -1);

    __acrt_lowio_lock_fh(fh);
    int result = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _doserrno = 0;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            __leave;
        }

        result = locking_nolock(fh, locking_mode, number_of_bytes);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return result;
}
