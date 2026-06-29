//
// locks.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Critical sections used for synchronization in the CoreCRT.
//
#include <corecrt_internal.h>



// This table holds the locks used by the CoreCRT.  It is indexed using the
// enumerators of the __acrt_lock_id enumeration.
static CRITICAL_SECTION __acrt_lock_table[__acrt_lock_count];

// This variable stores the number of locks that have been successfully
// initialized.  Locks are initialized in order and are destroyed in reverse
// order.  The startup and exit code must ensure that initialization and
// destruction is synchronized:  these functions should never be executed
// concurrently.
static unsigned __acrt_locks_initialized;



extern "C" bool __cdecl __acrt_initialize_locks()
{
    for (unsigned i = 0; i < __acrt_lock_count; ++i)
    {
        if (!__acrt_InitializeCriticalSectionEx(&__acrt_lock_table[i], _CORECRT_SPINCOUNT, 0))
        {
            __acrt_uninitialize_locks(false);
            return false;
        }

        ++__acrt_locks_initialized;
    }

    return true;
}

extern "C" bool __cdecl __acrt_uninitialize_locks(bool const /* terminating */)
{
    for (unsigned i = __acrt_locks_initialized; i > 0; --i)
    {
        DeleteCriticalSection(&__acrt_lock_table[i - 1]);
        --__acrt_locks_initialized;
    }

    return true;
}

extern "C" void __cdecl __acrt_lock(_In_ __acrt_lock_id _Lock)
{
    EnterCriticalSection(&__acrt_lock_table[_Lock]);
}

extern "C" void __cdecl __acrt_unlock(_In_ __acrt_lock_id _Lock)
{
    LeaveCriticalSection(&__acrt_lock_table[_Lock]);
}

extern "C" void __cdecl _lock_locales()
{
	__acrt_eagerly_load_locale_apis();
    __acrt_lock(__acrt_locale_lock);
}

extern "C" void __cdecl _unlock_locales()
{
    __acrt_unlock(__acrt_locale_lock);
}
