#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"

#ifndef LWIP_TAG
    #define LWIP_TAG 'PIwl'
#endif

void *
malloc(mem_size_t size)
{
    return ExAllocatePoolWithTag(NonPagedPool, size, LWIP_TAG);
}

void *
calloc(mem_size_t count, mem_size_t size)
{
    void *mem = malloc(count * size);
    
    if (!mem) return NULL;
    
    RtlZeroMemory(mem, count * size);
    
    return mem;
}

void
free(void *mem)
{
    ExFreePoolWithTag(mem, LWIP_TAG);
}

void *
realloc(void *mem, size_t size)
{
    free(mem);
    return malloc(size);
}