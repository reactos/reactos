//
// swab.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the _swab function, which copies a source buffer into a destination
// buffer, swapping the odd and even bytes of each word as it does so.
//
#include <corecrt_internal.h>
#include <stdlib.h>



extern "C" void __cdecl _swab(
    char* source,
    char* destination,
    int   bytes
    )
{
    _VALIDATE_RETURN_VOID(source      != nullptr, EINVAL);
    _VALIDATE_RETURN_VOID(destination != nullptr, EINVAL);
    _VALIDATE_RETURN_VOID(bytes >= 0,             EINVAL);

    while (bytes > 1)
    {
        char const b1 = *source++;
        char const b2 = *source++;

        *destination++ = b2;
        *destination++ = b1;

        bytes -= 2;
    }
}
