//
// new_mode.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of _set_new_mode and _query_new_mode, which provide access to the
// _newmode global flag.
//
#include <corecrt_internal.h>
#include <new.h>



static __crt_state_management::dual_state_global<long> __acrt_global_new_mode;



// Sets the _newmode flag to the new value 'mode' and return the old mode.
extern "C" int __cdecl _set_new_mode(int const mode)
{
    // The only valid values of _newmode are 0 and 1:
    _VALIDATE_RETURN(mode == 0 || mode == 1, EINVAL, -1);

    return static_cast<int>(_InterlockedExchange(&__acrt_global_new_mode.value(), mode));
}

// Gets the current value of the _newmode flag.
extern "C" int __cdecl _query_new_mode()
{
    return static_cast<int>(__crt_interlocked_read(&__acrt_global_new_mode.value()));
}
