#include <windows.h>
#include <msvcrt/stdlib.h>

extern HANDLE hHeap;

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
