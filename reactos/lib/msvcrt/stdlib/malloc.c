#include <windows.h>
#include <msvcrt/stdlib.h>

extern HANDLE hHeap;

typedef void (*MSVCRT_new_handler_func)(unsigned long size);
static MSVCRT_new_handler_func MSVCRT_new_handler;

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

void* malloc(size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _size);
}

void free(void* _ptr)
{
   HeapFree(hHeap,0,_ptr);
}

void* calloc(size_t _nmemb, size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _nmemb*_size);
}

void* realloc(void* _ptr, size_t _size)
{
   return HeapReAlloc(hHeap, 0, _ptr, _size);
}
