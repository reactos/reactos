//
// isatty.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _isatty(), which tests whether a file refers to a character device.
//
#include <corecrt_internal_lowio.h>



// Tests if the given file refers to a character device (e.g. terminal, console,
// printer, serial port, etc.).  Returns nonzero if so; zero if not.
extern "C" int __cdecl _isatty(int const fh)
{
    _CHECK_FH_RETURN(fh, EBADF, 0);
    _VALIDATE_RETURN((fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, 0);

    return static_cast<int>(_osfile(fh) & FDEV);
}
