#include <windows.h>
#include <msvcrt/stdlib.h>

extern HANDLE hHeap;

typedef void (*MSVCRT_new_handler_func)(unsigned long size);
static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/* ??2@YAPAXI@Z (MSVCRT.@) */
void* MSVCRT_operator_new(unsigned long size)
{
  void *retval = HeapAlloc(GetProcessHeap(), 0, size);

/*  FIXME: LOCK_HEAP; */
  if(!retval && MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
/*  FIXME: UNLOCK_HEAP; */

  return retval;
}

/* ??3@YAXPAX@Z (MSVCRT.@) */
void MSVCRT_operator_delete(void *mem)
{
  HeapFree(GetProcessHeap(), 0, mem);
}

/* ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z (MSVCRT.@) */
MSVCRT_new_handler_func MSVCRT__set_new_handler(MSVCRT_new_handler_func func)
{
  MSVCRT_new_handler_func old_handler;
/*  FIXME:  LOCK_HEAP; */
  old_handler = MSVCRT_new_handler;
  MSVCRT_new_handler = func;
/*  FIXME:  UNLOCK_HEAP; */
  return old_handler;
}

/*
 * @implemented
 */
void* malloc(size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _size);
}

/*
 * @implemented
 */
void free(void* _ptr)
{
   HeapFree(hHeap,0,_ptr);
}

/*
 * @implemented
 */
void* calloc(size_t _nmemb, size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _nmemb*_size);
}

/*
 * @implemented
 */
void* realloc(void* _ptr, size_t _size)
{
   return HeapReAlloc(hHeap, 0, _ptr, _size);
}

/* ?_set_new_mode@@YAHH@Z (MSVCRT.@) */
int MSVCRT__set_new_mode(int mode)
{
  int old_mode;
/*  FIXME:  LOCK_HEAP; */
  old_mode = MSVCRT_new_mode;
  MSVCRT_new_mode = mode;
/*  FIXME:  UNLOCK_HEAP; */
  return old_mode;
}
