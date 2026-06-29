//
// per_thread_data.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Per-Thread Data (PTD) used by the AppCRT.
//
#include <corecrt_internal.h>
#include <corecrt_internal_ptd_propagation.h>
#include <stddef.h>



#ifdef _CRT_GLOBAL_STATE_ISOLATION
    extern "C" DWORD __crt_global_state_mode_flsindex = FLS_OUT_OF_INDEXES;
#endif



static void WINAPI destroy_fls(void*) throw();



static unsigned long __acrt_flsindex = FLS_OUT_OF_INDEXES;



extern "C" bool __cdecl __acrt_initialize_ptd()
{
    __acrt_flsindex = __acrt_FlsAlloc(destroy_fls);
    if (__acrt_flsindex == FLS_OUT_OF_INDEXES)
    {
        return false;
    }

    if (__acrt_getptd_noexit() == nullptr)
    {
        __acrt_uninitialize_ptd(false);
        return false;
    }

    return true;
}

extern "C" bool __cdecl __acrt_uninitialize_ptd(bool)
{
    if (__acrt_flsindex != FLS_OUT_OF_INDEXES)
    {
        __acrt_FlsFree(__acrt_flsindex);
        __acrt_flsindex = FLS_OUT_OF_INDEXES;
    }

    return true;
}



static void __cdecl replace_current_thread_locale_nolock(
    __acrt_ptd*        const ptd,
    __crt_locale_data* const new_locale_info
    ) throw()
{
    if (ptd->_locale_info)
    {
        __acrt_release_locale_ref(ptd->_locale_info);
        if (ptd->_locale_info != __acrt_current_locale_data.value() &&
            ptd->_locale_info != &__acrt_initial_locale_data &&
            ptd->_locale_info->refcount == 0)
        {
            __acrt_free_locale(ptd->_locale_info);
        }
    }

    ptd->_locale_info = new_locale_info;
    if (ptd->_locale_info)
    {
        __acrt_add_locale_ref(ptd->_locale_info);
    }
}



// Constructs a single PTD object, copying the given 'locale_data' if provided.
static void __cdecl construct_ptd(
    __acrt_ptd*         const ptd,
    __crt_locale_data** const locale_data
    ) throw()
{
    ptd->_rand_state  = 1;
    ptd->_pxcptacttab = const_cast<__crt_signal_action_t*>(__acrt_exception_action_table);

    // It is necessary to always have GLOBAL_LOCALE_BIT set in perthread data
    // because when doing bitwise or, we won't get __UPDATE_LOCALE to work when
    // global per thread locale is set.
    // See _configthreadlocale() and __acrt_should_sync_with_global_locale().
    ptd->_own_locale = _GLOBAL_LOCALE_BIT;

    ptd->_multibyte_info = &__acrt_initial_multibyte_data;

    // Initialize _setloc_data. These are the only valuse that need to be
    // initialized.
    ptd->_setloc_data._cachein[0]  = L'C';
    ptd->_setloc_data._cacheout[0] = L'C';

    // Downlevel data is not initially used
    ptd->_setloc_downlevel_data = nullptr;

    __acrt_lock_and_call(__acrt_multibyte_cp_lock, [&]
    {
        _InterlockedIncrement(&ptd->_multibyte_info->refcount);
    });

    // We need to make sure that ptd->ptlocinfo in never nullptr, this saves us
    // perf counts when UPDATING locale.
    __acrt_lock_and_call(__acrt_locale_lock, [&]
    {
        replace_current_thread_locale_nolock(ptd, *locale_data);
    });
}

// Constructs each of the 'state_index_count' PTD objects in the array of PTD
// objects pointed to by 'ptd'.
static void __cdecl construct_ptd_array(__acrt_ptd* const ptd) throw()
{
    for (size_t i = 0; i != __crt_state_management::state_index_count; ++i)
    {
        construct_ptd(&ptd[i], &__acrt_current_locale_data.dangerous_get_state_array()[i]);
    }
}

// Cleans up all resources used by a single PTD; does not free the PTD structure
// itself.
static void __cdecl destroy_ptd(__acrt_ptd* const ptd) throw()
{
    if (ptd->_pxcptacttab != __acrt_exception_action_table)
    {
        _free_crt(ptd->_pxcptacttab);
    }

    _free_crt(ptd->_cvtbuf);
    _free_crt(ptd->_asctime_buffer);
    _free_crt(ptd->_wasctime_buffer);
    _free_crt(ptd->_gmtime_buffer);
    _free_crt(ptd->_tmpnam_narrow_buffer);
    _free_crt(ptd->_tmpnam_wide_buffer);
    _free_crt(ptd->_strerror_buffer);
    _free_crt(ptd->_wcserror_buffer);
    _free_crt(ptd->_beginthread_context);

    __acrt_lock_and_call(__acrt_multibyte_cp_lock, [&]
    {
        __crt_multibyte_data* const multibyte_data = ptd->_multibyte_info;
        if (!multibyte_data)
        {
            return;
        }

        if (_InterlockedDecrement(&multibyte_data->refcount) != 0)
        {
            return;
        }

        if (multibyte_data == &__acrt_initial_multibyte_data)
        {
            return;
        }

        _free_crt(multibyte_data);
    });

    __acrt_lock_and_call(__acrt_locale_lock, [&]
    {
        replace_current_thread_locale_nolock(ptd, nullptr);
    });
}

// Destroys each of the 'state_index_count' PTD objects in the array of PTD
// objects pointed to by 'ptd'.
static void __cdecl destroy_ptd_array(__acrt_ptd* const ptd) throw()
{
    for (size_t i = 0; i != __crt_state_management::state_index_count; ++i)
    {
        destroy_ptd(&ptd[i]);
    }
}

// This function is called by the operating system when a thread is being
// destroyed, to allow us the opportunity to clean up.
static void WINAPI destroy_fls(void* const pfd) throw()
{
    if (!pfd)
    {
        return;
    }

    destroy_ptd_array(static_cast<__acrt_ptd*>(pfd));
    _free_crt(pfd);
}

static __forceinline __acrt_ptd* try_get_ptd_head() throw()
{
    // If we haven't allocated per-thread data for this module, return failure:
    if (__acrt_flsindex == FLS_OUT_OF_INDEXES)
    {
        return nullptr;
    }

    __acrt_ptd* const ptd_head = static_cast<__acrt_ptd*>(__acrt_FlsGetValue(__acrt_flsindex));
    if (!ptd_head)
    {
        return nullptr;
    }

    return ptd_head;
}

_Success_(return != nullptr)
static __forceinline __acrt_ptd* internal_get_ptd_head() throw()
{
    // We use the CRT heap to allocate the PTD.  If the CRT heap fails to
    // allocate the requested memory, it will attempt to set errno to ENOMEM,
    // which will in turn attempt to acquire the PTD, resulting in infinite
    // recursion that causes a stack overflow.
    //
    // We set the PTD to this sentinel value for the duration of the allocation
    // in order to detect this case.
    static void* const reentrancy_sentinel = reinterpret_cast<void*>(SIZE_MAX);

    __acrt_ptd* const existing_ptd_head = try_get_ptd_head();
    if (existing_ptd_head == reentrancy_sentinel)
    {
        return nullptr;
    }
    else if (existing_ptd_head != nullptr)
    {
        return existing_ptd_head;
    }

    if (!__acrt_FlsSetValue(__acrt_flsindex, reentrancy_sentinel))
    {
        return nullptr;
    }

    __crt_unique_heap_ptr<__acrt_ptd> new_ptd_head(_calloc_crt_t(__acrt_ptd, __crt_state_management::state_index_count));
    if (!new_ptd_head)
    {
        __acrt_FlsSetValue(__acrt_flsindex, nullptr);
        return nullptr;
    }

    if (!__acrt_FlsSetValue(__acrt_flsindex, new_ptd_head.get()))
    {
        __acrt_FlsSetValue(__acrt_flsindex, nullptr);
        return nullptr;
    }

    construct_ptd_array(new_ptd_head.get());
    return new_ptd_head.detach();
}

// This functionality has been split out of __acrt_getptd_noexit so that we can
// force it to be inlined into both __acrt_getptd_noexit and __acrt_getptd.  These
// functions are performance critical and this change has substantially improved
// __acrt_getptd performance.
static __forceinline __acrt_ptd* __cdecl internal_getptd_noexit(
    __crt_scoped_get_last_error_reset const& last_error_reset,
    size_t                            const  global_state_index
    ) throw()
{
    UNREFERENCED_PARAMETER(last_error_reset);
    __acrt_ptd* const ptd_head = internal_get_ptd_head();
    if (!ptd_head)
    {
        return nullptr;
    }

    return ptd_head + global_state_index;
}

static __forceinline __acrt_ptd* __cdecl internal_getptd_noexit() throw()
{
    __crt_scoped_get_last_error_reset const last_error_reset;
    return internal_getptd_noexit(last_error_reset, __crt_state_management::get_current_state_index(last_error_reset));
}

__acrt_ptd* __cdecl __acrt_getptd_noexit_explicit(__crt_scoped_get_last_error_reset const& last_error_reset, size_t const global_state_index)
{   // An extra function to grab the PTD while a GetLastError() reset guard is already in place
    // and the global state index is already known.

    return internal_getptd_noexit(last_error_reset, global_state_index);
}

extern "C" __acrt_ptd* __cdecl __acrt_getptd_noexit()
{
    return internal_getptd_noexit();
}

extern "C" __acrt_ptd* __cdecl __acrt_getptd()
{
    __acrt_ptd* const ptd = internal_getptd_noexit();
    if (!ptd)
    {
        abort();
    }

    return ptd;
}

extern "C" __acrt_ptd* __cdecl __acrt_getptd_head()
{
    __acrt_ptd* const ptd_head = internal_get_ptd_head();
    if (!ptd_head)
    {
        abort();
    }

    return ptd_head;
}



extern "C" void __cdecl __acrt_freeptd()
{
    __acrt_ptd* const ptd_head = try_get_ptd_head();
    if (!ptd_head)
    {
        return;
    }

    __acrt_FlsSetValue(__acrt_flsindex, nullptr);
    destroy_fls(ptd_head);
}



// These functions are simply wrappers around the Windows API functions.
extern "C" unsigned long __cdecl __threadid()
{
    return GetCurrentThreadId();
}

extern "C" uintptr_t __cdecl __threadhandle()
{
    return reinterpret_cast<uintptr_t>(GetCurrentThread());
}
