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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file isnt really the main file in ReactOS msvcrt.
 * -sedwards
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wine/winternl.h"
#include "wine/exception.h"
#include "winnt.h"
#include "excpt.h"
#include "wine/debug.h"
#include "msvcrt/malloc.h"
#include "msvcrt/stdlib.h"
#include "msvcrt/locale.h"
#include "msvcrt/stdio.h"

#include "msvcrt.h"
#include "cppexcept.h"
#include "mtdll.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* Index to TLS */
DWORD MSVCRT_tls_index;

static inline BOOL msvcrt_init_tls(void);
static inline BOOL msvcrt_free_tls(void);
//const char* msvcrt_get_reason(DWORD reason) WINE_UNUSED;

typedef void* (*MSVCRT_malloc_func)(MSVCRT_size_t);

char* MSVCRT___unDName(char *,const char*,int,MSVCRT_malloc_func,MSVCRT_free_func,unsigned short int);
char* MSVCRT___unDNameEx(char *,const char*,int,MSVCRT_malloc_func,MSVCRT_free_func,void *,unsigned short int);

#if 0 /* __REACTOS__ */

/*********************************************************************
 *                  Init
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  MSVCRT_thread_data *tls;

  TRACE("(%p, %s, %p) pid(%lx), tid(%lx), tls(%ld)\n",
        hinstDLL, msvcrt_get_reason(fdwReason), lpvReserved,
        GetCurrentProcessId(), GetCurrentThreadId(),
        (long)MSVCRT_tls_index);

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    if (!msvcrt_init_tls())
      return FALSE;
    msvcrt_init_mt_locks();
    msvcrt_init_io();
    msvcrt_init_console();
    msvcrt_init_args();
    MSVCRT_setlocale(0, "C");
    TRACE("finished process init\n");
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    msvcrt_free_mt_locks();
    msvcrt_free_io();
    msvcrt_free_console();
    msvcrt_free_args();
    if (!msvcrt_free_tls())
      return FALSE;
    TRACE("finished process free\n");
    break;
  case DLL_THREAD_DETACH:
    /* Free TLS */
    tls = TlsGetValue(MSVCRT_tls_index);
    if (tls) HeapFree(GetProcessHeap(), 0, tls);
    TRACE("finished thread free\n");
    break;
  }
  return TRUE;
}

#endif /* __REACTOS__ */

static inline BOOL msvcrt_init_tls(void)
{
  MSVCRT_tls_index = TlsAlloc();

  if (MSVCRT_tls_index == TLS_OUT_OF_INDEXES)
  {
    ERR("TlsAlloc() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline BOOL msvcrt_free_tls(void)
{
  if (!TlsFree(MSVCRT_tls_index))
  {
    ERR("TlsFree() failed!\n");
    return FALSE;
  }
  return TRUE;
}

const char* msvcrt_get_reason(DWORD reason)
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


/*********************************************************************
 *		$I10_OUTPUT (MSVCRT.@)
 * Function not really understood but needed to make the DLL work
 */
void MSVCRT_I10_OUTPUT(void)
{
  /* FIXME: This is probably data, not a function */
}

/*********************************************************************
 *		__unDNameEx (MSVCRT.@)
 *
 * Demangle a C++ identifier.
 *
 * PARAMS
 *  OutStr   [O] If not NULL, the place to put the demangled string
 *  mangled  [I] Mangled name of the function
 *  OutStrLen[I] Length of OutStr
 *  memget   [I] Function to allocate memory with
 *  memfree  [I] Function to free memory with
 *  unknown  [?] Unknown, possibly a call back
 *  flags    [I] Flags determining demangled format
 *
 * RETURNS
 *  Success: A string pointing to the unmangled name, allocated with memget.
 *  Failure: NULL.
 */
char* MSVCRT___unDNameEx(char * OutStr, const char* mangled, int OutStrLen,
                       MSVCRT_malloc_func memget,
                       MSVCRT_free_func memfree,
                       void * unknown,
                       unsigned short int flags)
{
  FIXME("(%p,%s,%d,%p,%p,%p,%x) stub!\n",
          OutStr, mangled, OutStrLen, memget, memfree, unknown, flags);

  /* FIXME: The code in tools/winebuild/msmangle.c is pretty complete and
   * could be used here.
   */

  /* Experimentation reveals the following flag meanings when set:
   * 0x0001 - Don't show __ in calling convention
   * 0x0002 - Don't show calling convention at all
   * 0x0004 - Don't show function/method return value
   * 0x0010 - Same as 0x1
   * 0x0080 - Don't show access specifier (public/protected/private)
   * 0x0200 - Don't show static specifier
   * 0x0800 - Unknown, passed by type_info::name()
   * 0x1000 - Only report the variable/class name
   * 0x2000 - Unknown, passed by type_info::name()
   */
  /* Duplicate the mangled name; for comparisons it doesn't matter anyway */
  if( OutStr == NULL) {
      OutStrLen = strlen(mangled) + 1;
      OutStr = memget( OutStrLen);
  }
  strncpy( OutStr, mangled, OutStrLen);
  return OutStr;
}


/*********************************************************************
 *		__unDName (MSVCRT.@)
 */
char* MSVCRT___unDName(char * OutStr, const char* mangled, int OutStrLen,
                       MSVCRT_malloc_func memget,
                       MSVCRT_free_func memfree,
                       unsigned short int flags)
{
   return MSVCRT___unDNameEx( OutStr, mangled, OutStrLen, memget, memfree,
           NULL, flags);
}
