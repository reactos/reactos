#include <windows.h>
#include <stdlib.h>
//#include <ddk/ntddk.h>

void* malloc(size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),0,_size));
}

void free(void* _ptr)
{
   HeapFree(GetProcessHeap(),0,_ptr);
}

void* calloc(size_t _nmemb, size_t _size)
{
   return(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,_nmemb*_size));
}

void* realloc(void* _ptr, size_t _size)
{
   return(HeapReAlloc(GetProcessHeap(),0, _ptr,  _size));
}
