//
// eof.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _eof(), which tests whether a file is at EOF.
//
#include <corecrt_internal_lowio.h>



// Tests if a file is at EOF.  Returns:
//  *  1 if at EOF
//  *  0 if not at EOF
//  * -1 on failure (errno is set)
extern "C" int __cdecl _eof(int const fh)
{
    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN((fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN((_osfile(fh) & FOPEN), EBADF, -1);

    return __acrt_lowio_lock_fh_and_call(fh, [&]()
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _doserrno = 0;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            return -1;
        }

        __int64 const here = _lseeki64_nolock(fh, 0i64, SEEK_CUR);
        if (here == -1i64)
            return -1;


        __int64 const end = _lseeki64_nolock(fh, 0i64, SEEK_END);
        if (end == -1i64)
            return -1;

        // Now we can test if we're at the end:
        if (here == end)
            return 1;

        // If we aren't at the end, we need to reset the stream to its original
        // state before we return:
        _lseeki64_nolock(fh, here, SEEK_SET);
        return 0;
    });
}
