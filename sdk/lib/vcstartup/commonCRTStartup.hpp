//
// commonCRTStartup.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of common executable startup code.
//
// SPDX-License-Identifier: MIT
//

#include <corecrt_startup.h>
#include <internal_shared.h>
#include <excpt.h>
#include <stdlib.h>
#include <intrin.h>
#include <pseh/pseh2.h>

// Defined in winnt.h
#define FAST_FAIL_FATAL_APP_EXIT 7

extern "C" int __cdecl main(int, char**, char**);
extern "C" int __cdecl wmain(int, wchar_t**, wchar_t**);

template<typename Tmain>
static int call_main();

template<>
int call_main<decltype(main)>()
{
    _configure_narrow_argv(_crt_argv_unexpanded_arguments);

    return main(*__p___argc(), *__p___argv(), _get_initial_narrow_environment());
}

template<>
int call_main<decltype(wmain)>()
{
    _configure_wide_argv(_crt_argv_unexpanded_arguments);

    return wmain(*__p___argc(), *__p___wargv(), _get_initial_wide_environment());
}

static bool __scrt_initialize()
{
    __isa_available_init();

    if (!__vcrt_initialize())
    {
        return false;
    }

    if (!__acrt_initialize())
    {
        __vcrt_uninitialize(false);
        return false;
    }

    if (_initterm_e(__xi_a, __xi_z) != 0)
    {
        return false;
    }

    _initterm(__xc_a, __xc_z);

    return true;
}

template<typename Tmain>
static __declspec(noinline) int __cdecl __commonCRTStartup()
{
    int exitCode;

    if (!__scrt_initialize())
    {
        __fastfail(FAST_FAIL_FATAL_APP_EXIT);
    }

    __try
    {
        exitCode = call_main<Tmain>();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        exitCode = GetExceptionCode();
    }
    __endtry

    exit(exitCode);
}
