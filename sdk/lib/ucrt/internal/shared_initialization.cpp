//
// shared_initialization.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Shared initialization logic used by both the AppCRT and the DesktopCRT.
//
#include <corecrt_internal.h>

extern "C" bool __cdecl __acrt_execute_initializers(
    __acrt_initializer const* const first,
    __acrt_initializer const* const last
    )
{
    if (first == last)
        return true;

    // Execute the initializers in [first, last), in order:
    __acrt_initializer const* it = first;
    for (; it != last; ++it)
    {
        if (it->_initialize == nullptr)
            continue;

        if (!(it->_initialize)())
            break;
    }

    // If we reached the end, all initializers completed successfully:
    if (it == last)
        return true;

    // Otherwise, the initializer pointed to by it failed.  We need to roll back
    // the initialization by executing the uninitializers corresponding to each
    // of the initializers that completed successfully:
    for (; it != first; --it)
    {
        // During initialization roll back, we do not execute uninitializers
        // that have no corresponding initializer:
        if ((it - 1)->_initialize == nullptr || (it - 1)->_uninitialize == nullptr)
            continue;

        (it - 1)->_uninitialize(false);
    }

    return false;
}

extern "C" bool __cdecl __acrt_execute_uninitializers(
    __acrt_initializer const* const first,
    __acrt_initializer const* const last
    )
{
    if (first == last)
        return true;

    // Execute the uninitializers in [first, last), in reverse order:
    for (__acrt_initializer const* it = last; it != first; --it)
    {
        if ((it - 1)->_uninitialize == nullptr)
            continue;

        (it - 1)->_uninitialize(false);
    }

    return true;
}
