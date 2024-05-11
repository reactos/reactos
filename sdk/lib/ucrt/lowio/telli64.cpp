//
// telli64.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _telli64(), which gets the current position of the file pointer.
//
#include <corecrt_internal_lowio.h>
#include <stdio.h>



// Gets the current position of the file pointer, without adjustment for
// buffering.  Returns -1 on error.
extern "C" __int64 __cdecl _telli64(int const fh)
{
    return _lseeki64(fh, 0, SEEK_CUR);
}
