#include <windows.h>
#include <msvcrt/stdlib.h>


/*
 * @implemented
 */
void* malloc(size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),0,_size));
}

/*
 * @implemented
 */
void free(void* _ptr)
{
   HeapFree(GetProcessHeap(),0,_ptr);
}

/*
 * @implemented
 */
void* calloc(size_t _nmemb, size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, _nmemb*_size));
}

/*
 * @implemented
 */
void* realloc(void* _ptr, size_t _size)
{
   return(HeapReAlloc(GetProcessHeap(),0,_ptr,_size));
}
