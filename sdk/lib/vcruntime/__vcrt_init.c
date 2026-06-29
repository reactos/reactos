//
// __vcrt_init.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of vcruntime initialization and termination functions.
//
// SPDX-License-Identifier: MIT
//

#include <internal_shared.h>

void msvcrt_init_exception(HINSTANCE hinstDLL);
BOOL msvcrt_init_tls(void);
void msvcrt_free_tls_mem(void);
BOOL msvcrt_free_tls(void);

__vcrt_bool __cdecl __vcrt_initialize(void)
{
    msvcrt_init_exception(GetModuleHandle(NULL));

    if (!msvcrt_init_tls())
        return FALSE;

    return TRUE;
}

__vcrt_bool __cdecl __vcrt_uninitialize(_In_ __vcrt_bool _Terminating)
{
    if (!msvcrt_free_tls())
        return FALSE;

    return TRUE;
}

__vcrt_bool __cdecl __vcrt_uninitialize_critical(void)
{
    return TRUE;
}

__vcrt_bool __cdecl __vcrt_thread_attach(void)
{
    // Not called by UCRT
    return TRUE;
}

__vcrt_bool __cdecl __vcrt_thread_detach(void)
{
    // Not called by UCRT
    return TRUE;
}

// UCRT doesn't have a thread detach callback for the vcruntime TLS, because
// the native vcruntime uses FlsAlloc and provides a cleanup callback there.
// Since we don't have that, we use TLS callbacks.
const IMAGE_TLS_DIRECTORY* __p_tls_used = &_tls_used;

static
VOID
WINAPI
wine_tls_cleanup_callback(PVOID hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // For the last thread, only DLL_PROCESS_DETACH is called
    if ((fdwReason == DLL_THREAD_DETACH) ||
        (fdwReason == DLL_PROCESS_DETACH))
    {
        msvcrt_free_tls_mem();
    }
}

_CRTALLOC(".CRT$XLD") PIMAGE_TLS_CALLBACK wine_tls_cleanup_ptr = wine_tls_cleanup_callback;
