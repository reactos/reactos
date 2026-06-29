//
// initterm.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// _initterm and _initterm_e functions used during dynamic initialization.
//
#include <corecrt_internal.h>



// Calls each function in [first, last).  [first, last) must be a valid range of
// function pointers.  Each function is called, in order.
extern "C" void __cdecl _initterm(_PVFV* const first, _PVFV* const last)
{
    for (_PVFV* it = first; it != last; ++it)
    {
        if (*it == nullptr)
            continue;

        (**it)();
    }
}

// Calls each function in [first, last).  [first, last) must be a valid range of
// function pointers.  Each function must return zero on success, nonzero on
// failure.  If any function returns nonzero, iteration stops immediately and
// the nonzero value is returned.  Otherwise all functions are called and zero
// is returned.
//
// If a nonzero value is returned, it is expected to be one of the runtime error
// values (_RT_{NAME}, defined in the internal header files).
extern "C" int __cdecl _initterm_e(_PIFV* const first, _PIFV* const last)
{
    for (_PIFV* it = first; it != last; ++it)
    {
        if (*it == nullptr)
            continue;

        int const result = (**it)();
        if (result != 0)
            return result;
    }

    return 0;
}
