/*
 * PROJECT:     ReactOS msvcrt
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     TLS support code for use with ucrtbase
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <process.h>
#include <assert.h>
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

DWORD msvcrt_tls_index;

BOOL msvcrt_init_tls(void)
{
    msvcrt_tls_index = TlsAlloc();
    if (msvcrt_tls_index == TLS_OUT_OF_INDEXES)
    {
        ERR("TlsAlloc() failed!\n");
        return FALSE;
    }

    return TRUE;
}

BOOL msvcrt_free_tls(void)
{
    if (!TlsFree(msvcrt_tls_index))
    {
        ERR("TlsFree() failed!\n");
        return FALSE;
    }

    return TRUE;
}

void msvcrt_free_tls_mem(void)
{
    thread_data_t *tls = TlsGetValue(msvcrt_tls_index);
    if (tls)
    {
        free(tls->efcvt_buffer);
        assert(tls->asctime_buffer == NULL);
        assert(tls->wasctime_buffer == NULL);
        assert(tls->strerror_buffer == NULL);
        assert(tls->wcserror_buffer == NULL);
        assert(tls->time_buffer == NULL);
        assert(tls->tmpnam_buffer == NULL);
        assert(tls->wtmpnam_buffer == NULL);
        assert(tls->locinfo == NULL);
        assert(tls->mbcinfo == NULL);
        HeapFree(GetProcessHeap(), 0, tls);
    }
}

thread_data_t *CDECL msvcrt_get_thread_data(void)
{
    thread_data_t *ptr;
    DWORD err = GetLastError();  /* need to preserve last error */

    ptr = TlsGetValue(msvcrt_tls_index);
    if (ptr == NULL)
    {
        ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ptr));
        if (ptr == NULL)
            exit(_RT_THREAD);

        if (!TlsSetValue(msvcrt_tls_index, ptr))
            exit(_RT_THREAD);

        ptr->tid = GetCurrentThreadId();
        ptr->handle = INVALID_HANDLE_VALUE;
        ptr->random_seed = 1;
    }

    SetLastError(err);
    return ptr;
}
