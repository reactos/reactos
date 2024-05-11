//
// filelength.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _filelength(), which computes the length of a file.
//
#include <corecrt_internal_lowio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>



// Computes the length, in bytes, of the given file. Returns -1 on failure.
template <typename Integer>
_Success_(return != -1)
static Integer __cdecl common_filelength(int const fh) throw()
{
    typedef __crt_integer_traits<Integer> traits;

    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN,                          EBADF, -1);

    __acrt_lowio_lock_fh(fh);
    Integer end = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _doserrno = 0;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread", 0));
            __leave;
        }

        // Seek to the end to get the length:
        Integer const here = traits::lseek_nolock(fh, 0, SEEK_CUR);
        if (here == -1)
            __leave;

        end = traits::lseek_nolock(fh, 0, SEEK_END);
        if (here != end)
            traits::lseek_nolock(fh, here, SEEK_SET);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    return end;
}

extern "C" long __cdecl _filelength(int const fh)
{
    return common_filelength<long>(fh);
}

extern "C" __int64 __cdecl _filelengthi64(int const fh)
{
    return common_filelength<__int64>(fh);
}
