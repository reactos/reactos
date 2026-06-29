//
// commit.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _commit(), which flushes a file buffer to disk.
//
#include <corecrt_internal_lowio.h>



// Flushes the buffer for the given file to disk.
//
// On success, 0 is returned.  On failure, -1 is returned and errno is set.
extern "C" int __cdecl _commit(int const fh)
{
    _CHECK_FH_RETURN(fh, EBADF, -1);
    _VALIDATE_RETURN((fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _VALIDATE_RETURN((_osfile(fh) & FOPEN), EBADF, -1);

    return __acrt_lowio_lock_fh_and_call(fh, [&]()
    {
        if (_osfile(fh) & FOPEN)
        {
            if (FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(fh))))
                return 0; // Success

            _doserrno = GetLastError();
        }

        errno = EBADF;

        _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
        return -1;
    });
}
