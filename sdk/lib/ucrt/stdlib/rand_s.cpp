//
// rand_s.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The implementation of the rand_s() function, which generates random numbers.
//
#include <corecrt_internal.h>
#include <stdlib.h>



extern "C" errno_t __cdecl rand_s(unsigned int* const result)
{
    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);
    *result = 0;

    if (!__acrt_RtlGenRandom(result, static_cast<ULONG>(sizeof(*result))))
    {
        errno = ENOMEM;
        return errno;
    }

    return 0;
}
