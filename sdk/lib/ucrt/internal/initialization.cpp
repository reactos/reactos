//
// initialization.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This file defines the main initialization and uninitialization routines for
// the AppCRT, shared by both the static and dynamic AppCRT libraries.  In the
// dynamic AppCRT library, these are called by DllMain.  In the static AppCRT
// library, these are called by the initialization code.
//
#include <corecrt_internal.h>
#include <corecrt_internal_stdio.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {



extern _onexit_table_t __acrt_atexit_table;
extern _onexit_table_t __acrt_at_quick_exit_table;
extern void*           __acrt_stdout_buffer;
extern void*           __acrt_stderr_buffer;



static bool __cdecl initialize_global_variables()
{
    __acrt_current_locale_data.initialize(&__acrt_initial_locale_data);
    return true;
}



#ifdef CRTDLL

    static bool __cdecl initialize_c()
    {
        _initialize_onexit_table(&__acrt_atexit_table);
        _initialize_onexit_table(&__acrt_at_quick_exit_table);

        // Do C initialization:
        if (_initterm_e(__xi_a, __xi_z) != 0)
        {
            return false;
        }

        // Do C++ initialization:
        _initterm(__xc_a, __xc_z);
        return true;
    }

    static bool __cdecl uninitialize_c(bool)
    {
        // Do pre-termination:
        _initterm(__xp_a, __xp_z);

        // Do termination:
        _initterm(__xt_a, __xt_z);
        return true;
    }

// C4505: unreferenced local function
#pragma warning( suppress: 4505 )
    static bool __cdecl initialize_environment()
    {
        if (_initialize_narrow_environment() < 0)
        {
            return false;
        }

        if (!_get_initial_narrow_environment())
        {
            return false;
        }

        return true;
    }

#else

    static bool __cdecl initialize_c()
    {
        _initialize_onexit_table(&__acrt_atexit_table);
        _initialize_onexit_table(&__acrt_at_quick_exit_table);
        return true;
    }

    static bool __cdecl uninitialize_c(bool)
    {
        return true;
    }

// C4505: unreferenced local function
#pragma warning( suppress: 4505 )
    static bool __cdecl initialize_environment()
    {
        return true;
    }

#endif

// C4505: unreferenced local function
#pragma warning( suppress: 4505 )
static bool __cdecl uninitialize_environment(bool const terminating)
{
    UNREFERENCED_PARAMETER(terminating);

    #ifdef _DEBUG
    if (terminating)
    {
        return true;
    }
    #endif

    __dcrt_uninitialize_environments_nolock();
    return true;
}

#ifdef _CRT_GLOBAL_STATE_ISOLATION

    static bool __cdecl initialize_global_state_isolation()
    {
        // Configure CRT's per-thread global state mode data
        return __crt_state_management::initialize_global_state_isolation();
    }

    static bool __cdecl uninitialize_global_state_isolation(bool const terminating)
    {
        // Configure CRT's per-thread global state mode data
        __crt_state_management::uninitialize_global_state_isolation(terminating);
        return true;
    }

#else

    static bool __cdecl initialize_global_state_isolation()
    {
        return true;
    }

    static bool __cdecl uninitialize_global_state_isolation(bool const /* terminating */)
    {
        return true;
    }

#endif



static bool __cdecl initialize_pointers()
{
    void* const encoded_null = __crt_fast_encode_pointer(nullptr);
    __acrt_initialize_invalid_parameter_handler(encoded_null);
    __acrt_initialize_new_handler(encoded_null);
    __acrt_initialize_signal_handlers(encoded_null);
    __acrt_initialize_user_matherr(encoded_null);
    __acrt_initialize_thread_local_exit_callback(encoded_null);
    return true;
}

static bool __cdecl uninitialize_vcruntime(const bool /* terminating */)
{
    return __vcrt_uninitialize(false);
}

static bool __cdecl uninitialize_allocated_memory(bool const /* terminating */)
{
    __acrt_current_multibyte_data.uninitialize([](__crt_multibyte_data*& multibyte_data)
    {
        if (_InterlockedDecrement(&multibyte_data->refcount) == 0 &&
            multibyte_data != &__acrt_initial_multibyte_data)
        {
            _free_crt(multibyte_data);
            multibyte_data = &__acrt_initial_multibyte_data;
        }
    });

    return true;
}

// C4505: unreferenced local function
#pragma warning( suppress: 4505 )
static bool __cdecl uninitialize_allocated_io_buffers(bool const /* terminating */)
{
    _free_crt(__acrt_stdout_buffer);
    __acrt_stdout_buffer = nullptr;

    _free_crt(__acrt_stderr_buffer);
    __acrt_stderr_buffer = nullptr;

    _free_crt(__argv);
    __argv = nullptr;

    _free_crt(__wargv);
    __wargv = nullptr;

    return true;
}

static bool __cdecl report_memory_leaks(bool const /* terminating */)
{
    #ifdef _DEBUG
    if (_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & _CRTDBG_LEAK_CHECK_DF)
    {
        _CrtSetDumpClient(nullptr);
        _CrtDumpMemoryLeaks();
    }
    #endif

    return true;
}



// This is the table of initializer/uninitializer pairs that is used to perform
// AppCRT initialization.  Initializers are run first-to-last during AppCRT
// initialization, and uninitializers are run last-to-first during termination.
static __acrt_initializer const __acrt_initializers[] =
{
    // Init globals that can't be set at compile time because they have c'tors
    { initialize_global_variables,             nullptr                                  },

    // Global pointers are stored in encoded form; they must be dynamically
    // initialized to the encoded nullptr value before they are used by the CRT.
    { initialize_pointers,                     nullptr                                  },
    // Enclaves only require initializers for supported features.
#ifndef _UCRT_ENCLAVE_BUILD
    { __acrt_initialize_winapi_thunks,         __acrt_uninitialize_winapi_thunks        },
#endif

    // Configure CRT's global state isolation system.  This system calls FlsAlloc
    // and thus must occur after the initialize_pointers initialization, otherwise
    // it will fall back to call TlsAlloc, then try to use the allocated TLS slot
    // with the FLS functions.  This does not turn out well.  By running this
    // initialization after the initialize_pointers step, we ensure that it can
    // call FlsAlloc.
    { initialize_global_state_isolation,       uninitialize_global_state_isolation      },

    // The heap and locks must be initialized before most other initialization
    // takes place, as other initialization steps rely on the heap and locks:
    { __acrt_initialize_locks,                 __acrt_uninitialize_locks                },
    { __acrt_initialize_heap,                  __acrt_uninitialize_heap                 },

    // During uninitialization, before the heap is uninitialized, the AppCRT
    // needs to notify all VCRuntime instances in the process to allow them to
    // release any memory that they allocated via the AppCRT heap.
    //
    // First, we notify all modules that registered for shutdown notification.
    // This only occurs in the AppCRT DLL, because the static CRT is only ever
    // used by a single module, so this notification is never required.
    //
    // Then, we notify our own VCRuntime instance.  Note that after this point
    // during uninitialization, no exception handling may take place in any
    // CRT module.
    { nullptr,                                 uninitialize_vcruntime                   },

    { __acrt_initialize_ptd,                   __acrt_uninitialize_ptd                  },
    // Enclaves only require initializers for supported features.
#ifndef _UCRT_ENCLAVE_BUILD
    { __acrt_initialize_lowio,                 __acrt_uninitialize_lowio                },
    { __acrt_initialize_command_line,          __acrt_uninitialize_command_line         },
#endif
    { __acrt_initialize_multibyte,             nullptr                                  },
    { nullptr,                                 report_memory_leaks                      },
    // Enclaves only require initializers for supported features.
#ifndef _UCRT_ENCLAVE_BUILD
    { nullptr,                                 uninitialize_allocated_io_buffers        },
#endif
    { nullptr,                                 uninitialize_allocated_memory            },
    // Enclaves only require initializers for supported features.
#ifndef _UCRT_ENCLAVE_BUILD
    { initialize_environment,                  uninitialize_environment                 },
#endif
    { initialize_c,                            uninitialize_c                           },
};




__crt_bool __cdecl __acrt_initialize()
{
    #if defined CRTDLL
    __isa_available_init();
    #endif

    return __acrt_execute_initializers(
        __acrt_initializers,
        __acrt_initializers + _countof(__acrt_initializers)
        );
}

__crt_bool __cdecl __acrt_uninitialize(__crt_bool const terminating)
{
    UNREFERENCED_PARAMETER(terminating);

    // If the process is terminating, there's no point in cleaning up, except
    // in debug builds.
    #ifndef _DEBUG
    if (terminating) {
        #ifndef _UCRT_ENCLAVE_BUILD
        if (__acrt_stdio_is_initialized()) {
            _flushall();
        }
        #endif
        return TRUE;
    }
    #endif

    return __acrt_execute_uninitializers(
        __acrt_initializers,
        __acrt_initializers + _countof(__acrt_initializers)
        );
}

__crt_bool __cdecl __acrt_uninitialize_critical(__crt_bool const terminating)
{
    __acrt_uninitialize_ptd(terminating);

    #ifdef _CRT_GLOBAL_STATE_ISOLATION
    uninitialize_global_state_isolation(terminating);
    #endif

    return true;
}

__crt_bool __cdecl __acrt_thread_attach()
{
    // Create a per-thread data structure for this thread (getptd will attempt
    // to create a new per-thread data structure if one does not already exist
    // for this thread):
    if (__acrt_getptd_noexit() == nullptr)
        return false;

    return true;
}

__crt_bool __cdecl __acrt_thread_detach()
{
    // Free the per-thread data structure for this thread:
    __acrt_freeptd();
    return true;
}

}
