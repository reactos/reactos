#include <windows.h>
#include <msvcrt/malloc.h>

/*
 * @implemented
 */
void* _expand(void* pold, size_t size)
{
  return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, pold, size);
}

/*
 * @implemented
 */
size_t _msize(void* pBlock)
{
  return HeapSize (GetProcessHeap(), 0, pBlock);
}
