//
// difftime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The difftime() family of functions, which compute the difference between time
// values.
//
#include <corecrt_internal_time.h>



// Computes the difference between two times (b - a).  Returns a double with the
// difference in seconds between the two times.  Returns zero if the input is
// invalid.
template <typename TimeType>
static double __cdecl common_difftime(TimeType const b, TimeType const a) throw()
{
    _VALIDATE_RETURN_NOEXC(a >= 0 && b >= 0, EINVAL, 0);

    return static_cast<double>(b - a);
}

extern "C" double __cdecl _difftime32(__time32_t const b, __time32_t const a)
{
    return common_difftime(b, a);
}

extern "C" double __cdecl _difftime64(__time64_t const b, __time64_t const a)
{
    return common_difftime(b, a);
}
