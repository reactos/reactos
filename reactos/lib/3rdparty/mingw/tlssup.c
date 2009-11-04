/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifdef CRTDLL
#undef CRTDLL
#endif

#include <internal.h>
#include <sect_attribs.h>
#include <windows.h>
#include <malloc.h>
#include <crtdbg.h>

extern BOOL __mingw_TLScallback (HANDLE hDllHandle, DWORD reason, LPVOID reserved);

#define FUNCS_PER_NODE 30

typedef struct TlsDtorNode {
  int count;
  struct TlsDtorNode *next;
  _PVFV funcs[FUNCS_PER_NODE];
} TlsDtorNode;

ULONG _tls_index = 0;

/* TLS raw template data start and end. */
_CRTALLOC(".tls$AAA") char _tls_start = 0;
_CRTALLOC(".tls$ZZZ") char _tls_end = 0;

_CRTALLOC(".CRT$XLA") PIMAGE_TLS_CALLBACK __xl_a = 0;
_CRTALLOC(".CRT$XLZ") PIMAGE_TLS_CALLBACK __xl_z = 0;

#ifdef _WIN64
_CRTALLOC(".tls") const IMAGE_TLS_DIRECTORY64 _tls_used = {
  (ULONGLONG) &_tls_start+1, (ULONGLONG) &_tls_end, (ULONGLONG) &_tls_index,
  (ULONGLONG) (&__xl_a+1), (ULONG) 0, (ULONG) 0
};
#else
_CRTALLOC(".tls") const IMAGE_TLS_DIRECTORY _tls_used = {
  (ULONG)(ULONG_PTR) &_tls_start+1, (ULONG)(ULONG_PTR) &_tls_end,
  (ULONG)(ULONG_PTR) &_tls_index, (ULONG)(ULONG_PTR) (&__xl_a+1),
  (ULONG) 0, (ULONG) 0
};
#endif

#ifndef __CRT_THREAD
#ifdef HAVE_ATTRIBUTE_THREAD
#define __CRT_THREAD	__declspec(thread)
#else
#define __CRT_THREAD    __thread
#endif
#endif

#define DISABLE_MS_TLS 1

static _CRTALLOC(".CRT$XDA") _PVFV __xd_a = 0;
static _CRTALLOC(".CRT$XDZ") _PVFV __xd_z = 0;

#if !defined (DISABLE_MS_TLS)
static __CRT_THREAD TlsDtorNode *dtor_list;
static __CRT_THREAD TlsDtorNode dtor_list_head;
#endif

extern int _CRT_MT;

BOOL WINAPI __dyn_tls_init (HANDLE, DWORD, LPVOID);

BOOL WINAPI
__dyn_tls_init (HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
  _PVFV *pfunc;

  /* We don't let us trick here.  */
  if (_CRT_MT != 2)
   _CRT_MT = 2;

  if (dwReason != DLL_THREAD_ATTACH)
    {
      if (dwReason == DLL_PROCESS_ATTACH)
        __mingw_TLScallback (hDllHandle, dwReason, lpreserved);
      return TRUE;
    }

  for (pfunc = &__xd_a + 1; pfunc != &__xd_z; ++pfunc)
    {
      if (*pfunc != NULL)
	(*pfunc)();
    }
  return TRUE;
}

const PIMAGE_TLS_CALLBACK __dyn_tls_init_callback = (const PIMAGE_TLS_CALLBACK) __dyn_tls_init;
_CRTALLOC(".CRT$XLC") PIMAGE_TLS_CALLBACK __xl_c = (PIMAGE_TLS_CALLBACK) __dyn_tls_init;

int __cdecl __tlregdtor (_PVFV);

int __cdecl
__tlregdtor (_PVFV func)
{
  if (!func)
    return 0;
#if !defined (DISABLE_MS_TLS)
  if (dtor_list == NULL)
    {
      dtor_list = &dtor_list_head;
      dtor_list_head.count = 0;
    }
    else if (dtor_list->count == FUNCS_PER_NODE)
    {
      TlsDtorNode *pnode = (TlsDtorNode *) malloc (sizeof (TlsDtorNode));
      if (pnode == NULL)
	return -1;
      pnode->count = 0;
      pnode->next = dtor_list;
      dtor_list = pnode;

      dtor_list->count = 0;
    }
  dtor_list->funcs[dtor_list->count++] = func;
#endif
  return 0;
}

static BOOL WINAPI
__dyn_tls_dtor (HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
#if !defined (DISABLE_MS_TLS)
  TlsDtorNode *pnode, *pnext;
  int i;
#endif

  if (dwReason != DLL_THREAD_DETACH && dwReason != DLL_PROCESS_DETACH)
    return TRUE;
  /* As TLS variables are detroyed already by DLL_THREAD_DETACH
     call, we have to avoid access on the possible DLL_PROCESS_DETACH
     call the already destroyed TLS vars.
     TODO: The used local thread based variables have to be handled
     manually, so that we can control their lifetime here.  */
#if !defined (DISABLE_MS_TLS)
  if (dwReason != DLL_PROCESS_DETACH)
    {
      for (pnode = dtor_list; pnode != NULL; pnode = pnext)
        {
          for (i = pnode->count - 1; i >= 0; --i)
	    {
	      if (pnode->funcs[i] != NULL)
	        (*pnode->funcs[i])();
	    }
          pnext = pnode->next;
          if (pnext != NULL)
	    free ((void *) pnode);
        }
    }
#endif
  __mingw_TLScallback (hDllHandle, dwReason, lpreserved);
  return TRUE;
}

_CRTALLOC(".CRT$XLD") PIMAGE_TLS_CALLBACK __xl_d = (PIMAGE_TLS_CALLBACK) __dyn_tls_dtor;


int mingw_initltsdrot_force = 0;
int mingw_initltsdyn_force=0;
int mingw_initltssuo_force = 0;
