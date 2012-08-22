/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * Symbol lookup.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_compiler.h"
#include "os/os_thread.h"
#include "u_string.h"

#include "u_debug.h"
#include "u_debug_symbol.h"
#include "u_hash_table.h"

#if defined(PIPE_OS_WINDOWS) && defined(PIPE_ARCH_X86)
   
#include <windows.h>
#include <stddef.h>

#include "dbghelp.h"


static BOOL bSymInitialized = FALSE;

static HMODULE hModule_Dbghelp = NULL;


static
FARPROC WINAPI __GetProcAddress(LPCSTR lpProcName)
{
#ifdef PIPE_CC_GCC
   if (!hModule_Dbghelp) {
      /*
       * bfdhelp.dll is a dbghelp.dll look-alike replacement, which is able to
       * understand MinGW symbols using BFD library.  It is available from
       * http://people.freedesktop.org/~jrfonseca/bfdhelp/ for now.
       */
      hModule_Dbghelp = LoadLibraryA("bfdhelp.dll");
   }
#endif

   if (!hModule_Dbghelp) {
      hModule_Dbghelp = LoadLibraryA("dbghelp.dll");
      if (!hModule_Dbghelp) {
         return NULL;
      }
   }

   return GetProcAddress(hModule_Dbghelp, lpProcName);
}


typedef BOOL (WINAPI *PFNSYMINITIALIZE)(HANDLE, LPSTR, BOOL);
static PFNSYMINITIALIZE pfnSymInitialize = NULL;

static
BOOL WINAPI j_SymInitialize(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess)
{
   if(
      (pfnSymInitialize || (pfnSymInitialize = (PFNSYMINITIALIZE) __GetProcAddress("SymInitialize")))
   )
      return pfnSymInitialize(hProcess, UserSearchPath, fInvadeProcess);
   else
      return FALSE;
}

typedef DWORD (WINAPI *PFNSYMSETOPTIONS)(DWORD);
static PFNSYMSETOPTIONS pfnSymSetOptions = NULL;

static
DWORD WINAPI j_SymSetOptions(DWORD SymOptions)
{
   if(
      (pfnSymSetOptions || (pfnSymSetOptions = (PFNSYMSETOPTIONS) __GetProcAddress("SymSetOptions")))
   )
      return pfnSymSetOptions(SymOptions);
   else
      return FALSE;
}

typedef BOOL (WINAPI *PFNSYMGETSYMFROMADDR)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
static PFNSYMGETSYMFROMADDR pfnSymFromAddr = NULL;

static
BOOL WINAPI j_SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol)
{
   if(
      (pfnSymFromAddr || (pfnSymFromAddr = (PFNSYMGETSYMFROMADDR) __GetProcAddress("SymFromAddr")))
   )
      return pfnSymFromAddr(hProcess, Address, Displacement, Symbol);
   else
      return FALSE;
}


static INLINE void
debug_symbol_name_dbghelp(const void *addr, char* buf, unsigned size)
{
   HANDLE hProcess;
   BYTE symbolBuffer[1024];
   PSYMBOL_INFO pSymbol = (PSYMBOL_INFO) symbolBuffer;
   DWORD64 dwDisplacement = 0;  /* Displacement of the input address, relative to the start of the symbol */

   hProcess = GetCurrentProcess();

   memset(pSymbol, 0, sizeof *pSymbol);
   pSymbol->SizeOfStruct = sizeof(symbolBuffer);
   pSymbol->MaxNameLen = sizeof(symbolBuffer) - offsetof(SYMBOL_INFO, Name);

   if(!bSymInitialized) {
      j_SymSetOptions(/* SYMOPT_UNDNAME | */ SYMOPT_LOAD_LINES);
      if(j_SymInitialize(hProcess, NULL, TRUE))
         bSymInitialized = TRUE;
   }

   if(!j_SymFromAddr(hProcess, (DWORD64)(uintptr_t)addr, &dwDisplacement, pSymbol))
      buf[0] = 0;
   else
   {
      strncpy(buf, pSymbol->Name, size);
      buf[size - 1] = 0;
   }
}
#endif

#ifdef __GLIBC__
#include <execinfo.h>

/* This can only provide dynamic symbols, or binary offsets into a file.
 *
 * To fix this, post-process the output with tools/addr2line.sh
 */
static INLINE void
debug_symbol_name_glibc(const void *addr, char* buf, unsigned size)
{
   char** syms = backtrace_symbols((void**)&addr, 1);
   strncpy(buf, syms[0], size);
   buf[size - 1] = 0;
   free(syms);
}
#endif

void
debug_symbol_name(const void *addr, char* buf, unsigned size)
{
#if defined(PIPE_OS_WINDOWS) && defined(PIPE_ARCH_X86)
   debug_symbol_name_dbghelp(addr, buf, size);
   if(buf[0])
      return;
#endif

#ifdef __GLIBC__
   debug_symbol_name_glibc(addr, buf, size);
   if(buf[0])
      return;
#endif

   util_snprintf(buf, size, "%p", addr);
   buf[size - 1] = 0;
}

void
debug_symbol_print(const void *addr)
{
   char buf[1024];
   debug_symbol_name(addr, buf, sizeof(buf));
   debug_printf("\t%s\n", buf);
}

struct util_hash_table* symbols_hash;
pipe_static_mutex(symbols_mutex);

static unsigned hash_ptr(void* p)
{
   return (unsigned)(uintptr_t)p;
}

static int compare_ptr(void* a, void* b)
{
   if(a == b)
      return 0;
   else if(a < b)
      return -1;
   else
      return 1;
}

const char*
debug_symbol_name_cached(const void *addr)
{
   const char* name;
#ifdef PIPE_SUBSYSTEM_WINDOWS_USER
   static boolean first = TRUE;

   if (first) {
      pipe_mutex_init(symbols_mutex);
      first = FALSE;
   }
#endif

   pipe_mutex_lock(symbols_mutex);
   if(!symbols_hash)
      symbols_hash = util_hash_table_create(hash_ptr, compare_ptr);
   name = util_hash_table_get(symbols_hash, (void*)addr);
   if(!name)
   {
      char buf[1024];
      debug_symbol_name(addr, buf, sizeof(buf));
      name = strdup(buf);

      util_hash_table_set(symbols_hash, (void*)addr, (void*)name);
   }
   pipe_mutex_unlock(symbols_mutex);
   return name;
}
