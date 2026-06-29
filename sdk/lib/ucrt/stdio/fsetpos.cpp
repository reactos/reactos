//
// fsetpos.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines fsetpos(), which sets the file position using a position previously
// returned by fgetpos().
//
#include <corecrt_internal_stdio.h>



// Sets the file position to the given position.  The position value must have
// been returned by a prior call to fgetpos().  Returns 0 on success; returns
// nonzero on failure.
extern "C" int __cdecl fsetpos(FILE* const stream, fpos_t const* const position)
{
    _VALIDATE_RETURN(stream   != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(position != nullptr, EINVAL, -1);

    return _fseeki64(stream, *position, SEEK_SET);
}
