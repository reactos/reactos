/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS system libraries
 * FILE:           lib/crtdll/stdlib/malloc.c
 * PURPOSE:        stdc memory allocation functions
 * PROGRAMMER:     ??
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <types.h>

/* FUNCTIONS *****************************************************************/

void* malloc(size_t size)
{
   return(HeapAlloc(GetProcessHeap(), 0, size));
}

void free(void* ptr)
{
   HeapFree(GetProcessHeap(), 0, ptr);
}

void* calloc(size_t nmemb, size_t size)
{
   return(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nmemb*size));
}

void* realloc(void* ptr, size_t size)
{
   return(HeapReAlloc(GetProcessHeap(), 0, ptr, size));
}
