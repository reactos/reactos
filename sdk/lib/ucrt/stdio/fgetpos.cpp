//
// fgetpos.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fgetpos(), which gets the file position in an opaque internal format.
//
#include <corecrt_internal_stdio.h>



// Gets the file position in the internal fpos_t format.  The returned value
// should only be used in a call to fsetpos().  Our implementation happens to
// just wrap _ftelli64() and _fseeki64().  Return zero on success; returns -1
// and sets errno on failure.
extern "C" int __cdecl fgetpos(FILE* const stream, fpos_t* const position)
{
    _VALIDATE_RETURN(stream   != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(position != nullptr, EINVAL, -1);

    *position = _ftelli64(stream);
    if (*position == -1)
        return -1;

    return 0;
}
