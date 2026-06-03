//
// __vcrt_init_stubs.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Stubs for vcruntime initialization and termination in vcstartup.
//
// SPDX-License-Identifier: MIT
//

#include <vcruntime_startup.h>

__vcrt_bool __cdecl __vcrt_initialize(void)
{
    return 1;
}

__vcrt_bool __cdecl __vcrt_uninitialize(_In_ __vcrt_bool _Terminating)
{
    return 1;
}

__vcrt_bool __cdecl __vcrt_uninitialize_critical(void)
{
    return 1;
}

__vcrt_bool __cdecl __vcrt_thread_attach(void)
{
    return 1;
}

__vcrt_bool __cdecl __vcrt_thread_detach(void)
{
    return 1;
}
