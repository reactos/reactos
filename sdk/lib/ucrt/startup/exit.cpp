//
// exit.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The exit() implementation
//
#include <corecrt_internal.h>
#include <eh.h>
#include <process.h>

static long c_termination_complete = FALSE;

extern "C" _onexit_table_t __acrt_atexit_table;
extern "C" _onexit_table_t __acrt_at_quick_exit_table;

// thread_local atexit dtor handling. The APPCRT exports a function to set the
// callback function. The exe main function will call this to set the callback
// function so exit() can invoke destructors for thread-storage objects owned
// by the main thread.
static _tls_callback_type thread_local_exit_callback_func;


// CRT_REFACTOR TODO This needs to be declared somewhere more accessible and we
// need to clean up the static CRT exit coordination.
#if !defined CRTDLL && defined _DEBUG
    extern "C" bool __cdecl __scrt_uninitialize_crt(bool is_terminating, bool from_exit);
#endif


// Enclaves have no support for managed apps
#ifdef _UCRT_ENCLAVE_BUILD

static bool __cdecl is_managed_app() throw() { return false; }

static void __cdecl try_cor_exit_process(UINT const) throw() { }

// This function never returns.  It causes the process to exit.
static void __cdecl exit_or_terminate_process(UINT const return_code) throw()
{
    TerminateProcess(GetCurrentProcess(), return_code);
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

typedef void (WINAPI* exit_process_pft)(UINT);

// CRT_REFACTOR TODO This is duplicated in the VCStartup utility_desktop.cpp.
static bool __cdecl is_managed_app() throw()
{
    PIMAGE_DOS_HEADER const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleHandleW(nullptr));
    if (dos_header == nullptr)
        return false;

    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    PIMAGE_NT_HEADERS const pe_header = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<BYTE*>(dos_header) + dos_header->e_lfanew);

    if (pe_header->Signature != IMAGE_NT_SIGNATURE)
        return false;

    if (pe_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        return false;

    // prefast assumes we are overrunning __ImageBase
    #pragma warning(push)
    #pragma warning(disable: 26000)

    if (pe_header->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
        return false;

    #pragma warning(pop)

    if (pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress == 0)
        return false;

    return true;
}

static void __cdecl try_cor_exit_process(UINT const return_code) throw()
{
    __crt_unique_hmodule mscoree;
    if (!GetModuleHandleExW(0, L"mscoree.dll", mscoree.get_address_of()))
        return;

    auto const cor_exit_process = __crt_get_proc_address<exit_process_pft>(mscoree.get(), "CorExitProcess");
    if (!cor_exit_process)
        return;

    cor_exit_process(return_code);
}



// For Windows Store apps, starting in Windows 10, we use TerminateProcess
// instead of ExitProcess.  ExitProcess will allow threads to run during
// termination in an unordered fashion. This has lead to problems in various
// apps.  TerminateProcess does not allow threads to run while the process is
// being torn down.  See also CoreApplication::Run, which does the same.
//
// For non-Windows Store app processes, we continue to call ExitProcess, for
// compatibility with the legacy runtimes.
static bool __cdecl should_call_terminate_process() throw()
{
    if (__acrt_get_process_end_policy() == process_end_policy_exit_process)
    {
        return false;
    }

    // If application verifier is running, we still want to call ExitProcess,
    // to enable tools that require DLLs to be unloaded cleanly at process exit
    // to do their work.
    if (__acrt_app_verifier_enabled())
    {
        return false;
    }

    return true;
}



// This function never returns.  It causes the process to exit.
static void __cdecl exit_or_terminate_process(UINT const return_code) throw()
{
    if (should_call_terminate_process())
    {
        TerminateProcess(GetCurrentProcess(), return_code);
    }

    try_cor_exit_process(return_code);

    // If that returned, then the exe for this process is not managed or we
    // failed to exit via a call to CorExitProcess.  Exit the normal way:
    ExitProcess(return_code);
}

#endif /* _UCRT_ENCLAVE_BUILD */


static int __cdecl atexit_exception_filter(unsigned long const _exception_code) throw()
{
    if (_exception_code == ('msc' | 0xE0000000))
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}



extern "C" void __cdecl __acrt_initialize_thread_local_exit_callback(void * encoded_null)
{
    thread_local_exit_callback_func = reinterpret_cast<_tls_callback_type>(encoded_null);
}

// Register the dynamic TLS dtor callback. Called from the vcstartup library
// as part of the EXE common_main to allow the acrt to call back into the scrt.
extern "C" void __cdecl _register_thread_local_exe_atexit_callback(_In_ _tls_callback_type const _Callback)
{
    // Can only set the callback once.
    if (thread_local_exit_callback_func != __crt_fast_encode_pointer(nullptr))
    {
        terminate();
    }

    thread_local_exit_callback_func = __crt_fast_encode_pointer(_Callback);
}



static void __cdecl common_exit(
    int                    const return_code,
    _crt_exit_cleanup_mode const cleanup_mode,
    _crt_exit_return_mode  const return_mode
    ) throw()
{
    // First, check to see if we're loaded in a managed app.  If we are, try to
    // call CorExitProcess to let the CLR handle the process termination.  If
    // the call to CorExitProcess is successful, then it will call back through
    // this function with a return mode of _crt_exit_return_to_caller, at which
    // point we will run the C termination routines.  It will then terminate the
    // process itself and not return.
    if (return_mode == _crt_exit_terminate_process && is_managed_app())
    {
        try_cor_exit_process(return_code);
    }

    // Run the C termination:
    bool crt_uninitialization_required = false;

    __acrt_lock_and_call(__acrt_select_exit_lock(), [&]
    {
        static bool c_exit_complete = false;
        if (c_exit_complete)
        {
            return;
        }

        _InterlockedExchange(&c_termination_complete, TRUE);

        __try
        {
            if (cleanup_mode == _crt_exit_full_cleanup)
            {

                // If this module has any dynamically initialized
                // __declspec(thread) variables, then we invoke their
                // destruction for the primary thread. All thread_local
                // destructors are sequenced before any atexit calls or static
                // object destructors (3.6.3/1)
                if (thread_local_exit_callback_func != __crt_fast_encode_pointer(nullptr))
                {
                    (__crt_fast_decode_pointer(thread_local_exit_callback_func))(nullptr, DLL_PROCESS_DETACH, nullptr);
                }

                _execute_onexit_table(&__acrt_atexit_table);
            }
            else if (cleanup_mode == _crt_exit_quick_cleanup)
            {
                _execute_onexit_table(&__acrt_at_quick_exit_table);
            }
        }
        __except (atexit_exception_filter(GetExceptionCode()))
        {
            terminate();
        }
        __endtry

        #ifndef CRTDLL
        // When the CRT is statically linked, we are responsible for executing
        // the terminators here, because the CRT code is present in this module.
        // When the CRT DLLs are used, the terminators will be executed when
        // the CRT DLLs are unloaded, after the call to ExitProcess.
        if (cleanup_mode == _crt_exit_full_cleanup)
        {
            _initterm(__xp_a, __xp_z);
        }

        _initterm(__xt_a, __xt_z);
        #endif // CRTDLL

        if (return_mode == _crt_exit_terminate_process)
        {
            c_exit_complete = true;
            crt_uninitialization_required = true;
        }
    });

    // Do NOT try to uninitialize the CRT while holding one of its locks.
    if (crt_uninitialization_required)
    {
        // If we are about to terminate the process, if the debug CRT is linked
        // statically into this module and this module is an EXE, we need to
        // ensure that we fully and correctly uninitialize the CRT so that the
        // debug on-exit() checks (e.g. debug heap leak detection) have a chance
        // to run.
        //
        // We do not need to uninitialize the CRT when it is statically linked
        // into a DLL because its DllMain will be called for DLL_PROCESS_DETACH
        // and we can uninitialize the CRT there.
        //
        // We never need to uninitialize the retail CRT during exit() because
        // the process is about to terminate.
        #if !CRTDLL && _DEBUG
        __scrt_uninitialize_crt(true, true);
        #endif
    }

    if (return_mode == _crt_exit_terminate_process)
    {
        exit_or_terminate_process(return_code);
    }
}

extern "C" int __cdecl _is_c_termination_complete()
{
    return static_cast<int>(__crt_interlocked_read(&c_termination_complete));
}



extern "C" void __cdecl exit(int const return_code)
{
    common_exit(return_code, _crt_exit_full_cleanup, _crt_exit_terminate_process);
    UNREACHABLE;
}

extern "C" void __cdecl _exit(int const return_code)
{
    common_exit(return_code, _crt_exit_no_cleanup, _crt_exit_terminate_process);
    UNREACHABLE;
}

extern "C" void __cdecl _Exit(int const return_code)
{
    common_exit(return_code, _crt_exit_no_cleanup, _crt_exit_terminate_process);
    UNREACHABLE;
}

extern "C" void __cdecl quick_exit(int const return_code)
{
    common_exit(return_code, _crt_exit_quick_cleanup, _crt_exit_terminate_process);
    UNREACHABLE;
}

extern "C" void __cdecl _cexit()
{
    common_exit(0, _crt_exit_full_cleanup, _crt_exit_return_to_caller);
}

extern "C" void __cdecl _c_exit()
{
    common_exit(0, _crt_exit_no_cleanup, _crt_exit_return_to_caller);
}
