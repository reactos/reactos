/*
 * msvcrt.dll initialisation functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <locale.h>
#include "msvcrt.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* Index to TLS */
DWORD msvcrt_tls_index;

static const char* msvcrt_get_reason(DWORD reason)
{
  switch (reason)
  {
  case DLL_PROCESS_ATTACH: return "DLL_PROCESS_ATTACH";
  case DLL_PROCESS_DETACH: return "DLL_PROCESS_DETACH";
  case DLL_THREAD_ATTACH:  return "DLL_THREAD_ATTACH";
  case DLL_THREAD_DETACH:  return "DLL_THREAD_DETACH";
  }
  return "UNKNOWN";
}

static inline BOOL msvcrt_init_tls(void)
{
  msvcrt_tls_index = TlsAlloc();

  if (msvcrt_tls_index == TLS_OUT_OF_INDEXES)
  {
    ERR("TlsAlloc() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline BOOL msvcrt_free_tls(void)
{
  if (!TlsFree(msvcrt_tls_index))
  {
    ERR("TlsFree() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline void msvcrt_free_tls_mem(void)
{
  thread_data_t *tls = TlsGetValue(msvcrt_tls_index);

  if (tls)
  {
    free(tls->efcvt_buffer);
    free(tls->asctime_buffer);
    free(tls->wasctime_buffer);
    free(tls->strerror_buffer);
    free(tls->wcserror_buffer);
    free(tls->time_buffer);
    free(tls->tmpnam_buffer);
    free(tls->wtmpnam_buffer);
    if(tls->locale_flags & LOCALE_FREE) {
        free_locinfo(tls->locinfo);
        free_mbcinfo(tls->mbcinfo);
    }
  }
  HeapFree(GetProcessHeap(), 0, tls);
}

/*********************************************************************
 *                  Init
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("(%p, %s, %p) pid(%lx), tid(%lx), tls(%lu)\n",
        hinstDLL, msvcrt_get_reason(fdwReason), lpvReserved,
        GetCurrentProcessId(), GetCurrentThreadId(),
        msvcrt_tls_index);

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    msvcrt_init_exception(hinstDLL);
    if(!msvcrt_init_heap())
        return FALSE;
    if(!msvcrt_init_tls()) {
      msvcrt_destroy_heap();
      return FALSE;
    }
    msvcrt_init_mt_locks();
    if(!msvcrt_init_locale()) {
        msvcrt_free_locks();
        msvcrt_free_tls_mem();
        msvcrt_destroy_heap();
        return FALSE;
    }
#if defined(__x86_64__) && _MSVCR_VER>=140
    if(!msvcrt_init_handler4()) {
        msvcrt_free_locks();
        msvcrt_free_tls_mem();
        msvcrt_destroy_heap();
        _free_locale(MSVCRT_locale);
        return FALSE;
    }
#endif
    msvcrt_init_math(hinstDLL);
    msvcrt_init_io();
    msvcrt_init_args();
    msvcrt_init_signals();
#if _MSVCR_VER >= 100 && _MSVCR_VER <= 120
    msvcrt_init_concurrency(hinstDLL);
#endif
#if _MSVCR_VER == 0
    /* don't allow unloading msvcrt, we can't setup file handles twice */
    LdrAddRefDll( LDR_ADDREF_DLL_PIN, hinstDLL );
#elif _MSVCR_VER >= 80
    _set_printf_count_output(0);
#endif
    msvcrt_init_clock();
    TRACE("finished process init\n");
    break;
  case DLL_THREAD_ATTACH:
#if defined(__x86_64__) && _MSVCR_VER>=140
    msvcrt_attach_handler4();
#endif
    break;
  case DLL_PROCESS_DETACH:
    msvcrt_free_io();
    if (lpvReserved) break;
    msvcrt_free_popen_data();
    msvcrt_free_locks();
    msvcrt_free_console();
    msvcrt_free_args();
    msvcrt_free_signals();
    msvcrt_free_tls_mem();
    if (!msvcrt_free_tls())
      return FALSE;
#if defined(__x86_64__) && _MSVCR_VER>=140
    msvcrt_free_handler4();
#endif
    _free_locale(MSVCRT_locale);
#if _MSVCR_VER >= 100 && _MSVCR_VER <= 120
    msvcrt_free_scheduler_thread();
    msvcrt_free_concurrency();
#endif
    msvcrt_destroy_heap();
    TRACE("finished process free\n");
    break;
  case DLL_THREAD_DETACH:
    msvcrt_free_tls_mem();
#if _MSVCR_VER >= 100 && _MSVCR_VER <= 120
    msvcrt_free_scheduler_thread();
#endif
    TRACE("finished thread free\n");
    break;
  }
  return TRUE;
}
