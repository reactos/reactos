/*
 * msvcrt.dll thread functions
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
#include <process.h>
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/********************************************************************/

typedef struct {
  HANDLE thread;
  union {
      _beginthread_start_routine_t start_address;
      _beginthreadex_start_routine_t start_address_ex;
  };
  void *arglist;
#if _MSVCR_VER >= 140
  HMODULE module;
#endif
} _beginthread_trampoline_t;

/*********************************************************************
 *		msvcrt_get_thread_data
 *
 * Return the thread local storage structure.
 */
thread_data_t *CDECL msvcrt_get_thread_data(void)
{
    thread_data_t *ptr;
    DWORD err = GetLastError();  /* need to preserve last error */

    if (!(ptr = TlsGetValue( msvcrt_tls_index )))
    {
        if (!(ptr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ptr) )))
            _amsg_exit( _RT_THREAD );
        if (!TlsSetValue( msvcrt_tls_index, ptr )) _amsg_exit( _RT_THREAD );
        ptr->tid = GetCurrentThreadId();
        ptr->handle = INVALID_HANDLE_VALUE;
        ptr->random_seed = 1;
        ptr->locinfo = MSVCRT_locale->locinfo;
        ptr->mbcinfo = MSVCRT_locale->mbcinfo;
        ptr->cached_locale[0] = 'C';
        ptr->cached_locale[1] = 0;
#if _MSVCR_VER >= 140
        ptr->module = NULL;
#endif
    }
    SetLastError( err );
    return ptr;
}

/*********************************************************************
 *		_endthread (MSVCRT.@)
 */
void CDECL _endthread(void)
{
  thread_data_t *tls;

  TRACE("(void)\n");

  tls = TlsGetValue(msvcrt_tls_index);
  if (tls && tls->handle != INVALID_HANDLE_VALUE)
  {
      CloseHandle(tls->handle);
      tls->handle = INVALID_HANDLE_VALUE;
  } else
      WARN("tls=%p tls->handle=%p\n", tls, tls ? tls->handle : INVALID_HANDLE_VALUE);

  _endthreadex(0);
}

/*********************************************************************
 *		_endthreadex (MSVCRT.@)
 */
void CDECL _endthreadex(
  unsigned int retval) /* [in] Thread exit code */
{
  TRACE("(%d)\n", retval);

#if _MSVCR_VER >= 140
  {
      thread_data_t *tls = TlsGetValue(msvcrt_tls_index);

      if (tls && tls->module != NULL)
          FreeLibraryAndExitThread(tls->module, retval);
      else
          WARN("tls=%p tls->module=%p\n", tls, tls ? tls->module : NULL);
  }
#endif

  ExitThread(retval);
}

/*********************************************************************
 *		_beginthread_trampoline
 */
static DWORD CALLBACK _beginthread_trampoline(LPVOID arg)
{
    _beginthread_trampoline_t local_trampoline;
    thread_data_t *data = msvcrt_get_thread_data();

    memcpy(&local_trampoline,arg,sizeof(local_trampoline));
    free(arg);
    data->handle = local_trampoline.thread;
#if _MSVCR_VER >= 140
    data->module = local_trampoline.module;
#endif

    local_trampoline.start_address(local_trampoline.arglist);
    _endthread();
}

/*********************************************************************
 *		_beginthread (MSVCRT.@)
 */
uintptr_t CDECL _beginthread(
  _beginthread_start_routine_t start_address, /* [in] Start address of routine that begins execution of new thread */
  unsigned int stack_size, /* [in] Stack size for new thread or 0 */
  void *arglist)           /* [in] Argument list to be passed to new thread or NULL */
{
  _beginthread_trampoline_t* trampoline;
  HANDLE thread;

  TRACE("(%p, %d, %p)\n", start_address, stack_size, arglist);

  if (!MSVCRT_CHECK_PMT(start_address)) return -1;

  trampoline = malloc(sizeof(*trampoline));
  if(!trampoline) {
      *_errno() = EAGAIN;
      return -1;
  }

  thread = CreateThread(NULL, stack_size, _beginthread_trampoline,
          trampoline, CREATE_SUSPENDED, NULL);
  if(!thread) {
      free(trampoline);
      msvcrt_set_errno(GetLastError());
      return -1;
  }

  trampoline->thread = thread;
  trampoline->start_address = start_address;
  trampoline->arglist = arglist;

#if _MSVCR_VER >= 140
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
              (void*)start_address, &trampoline->module))
  {
      trampoline->module = NULL;
      WARN("failed to get module for the start_address: %lu\n", GetLastError());
  }
#endif

  if(ResumeThread(thread) == -1) {
#if _MSVCR_VER >= 140
      FreeLibrary(trampoline->module);
#endif
      free(trampoline);
      *_errno() = EAGAIN;
      return -1;
  }

  return (uintptr_t)thread;
}

/*********************************************************************
 *		_beginthreadex_trampoline
 */
static DWORD CALLBACK _beginthreadex_trampoline(LPVOID arg)
{
    unsigned int retval;
    _beginthread_trampoline_t local_trampoline;
    thread_data_t *data = msvcrt_get_thread_data();

    memcpy(&local_trampoline, arg, sizeof(local_trampoline));
    free(arg);
    data->handle = local_trampoline.thread;
#if _MSVCR_VER >= 140
    data->module = local_trampoline.module;
#endif

    retval = local_trampoline.start_address_ex(local_trampoline.arglist);
    _endthreadex(retval);
}
/*********************************************************************
 *		_beginthreadex (MSVCRT.@)
 */
uintptr_t CDECL _beginthreadex(
  void *security,          /* [in] Security descriptor for new thread; must be NULL for Windows 9x applications */
  unsigned int stack_size, /* [in] Stack size for new thread or 0 */
  _beginthreadex_start_routine_t start_address, /* [in] Start address of routine that begins execution of new thread */
  void *arglist,           /* [in] Argument list to be passed to new thread or NULL */
  unsigned int initflag,   /* [in] Initial state of new thread (0 for running or CREATE_SUSPEND for suspended) */
  unsigned int *thrdaddr)  /* [out] Points to a 32-bit variable that receives the thread identifier */
{
  _beginthread_trampoline_t* trampoline;
  HANDLE thread;

  TRACE("(%p, %d, %p, %p, %d, %p)\n", security, stack_size, start_address, arglist, initflag, thrdaddr);

  /* FIXME: may use different errno / return values */
  if (!MSVCRT_CHECK_PMT(start_address)) return 0;

  if (!(trampoline = malloc(sizeof(*trampoline))))
      return 0;

  trampoline->thread = INVALID_HANDLE_VALUE;
  trampoline->start_address_ex = start_address;
  trampoline->arglist = arglist;

#if _MSVCR_VER >= 140
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
              (void*)start_address, &trampoline->module))
  {
     trampoline->module = NULL;
     WARN("failed to get module for the start_address: %lu\n", GetLastError());
  }
#endif

  thread = CreateThread(security, stack_size, _beginthreadex_trampoline,
          trampoline, initflag, (DWORD *)thrdaddr);
  if(!thread) {
#if _MSVCR_VER >= 140
      FreeLibrary(trampoline->module);
#endif
      free(trampoline);
      msvcrt_set_errno(GetLastError());
      return 0;
  }

  return (uintptr_t)thread;
}

#if _MSVCR_VER>=80
/*********************************************************************
 *		_getptd (MSVCR80.@)
 */
thread_data_t* CDECL _getptd(void)
{
    FIXME("returns undocumented/not fully filled data\n");
    return msvcrt_get_thread_data();
}
#endif
