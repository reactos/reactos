//
// tell.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _tell(), which gets the current position of the file pointer.
//
#include <corecrt_internal_lowio.h>
#include <stdio.h>



// Gets the current position of the file pointer, without adjustment for
// buffering.  Returns -1 on error.
extern "C" long __cdecl _tell(int const fh)
{
    return _lseek(fh, 0, SEEK_CUR);
}
