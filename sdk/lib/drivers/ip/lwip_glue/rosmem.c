
#include "lwip/mem.h"

#ifndef LWIP_TAG
    #define LWIP_TAG 'PIwl'
#endif

void *
ros_malloc(size_t size)
{
    return ExAllocatePoolWithTag(NonPagedPool, size, LWIP_TAG);
}

void *
ros_calloc(size_t count, size_t size)
{
    void *mem = ros_malloc(count * size);
    
    if (!mem) return NULL;
    
    RtlZeroMemory(mem, count * size);
    
    return mem;
}

void
ros_free(void *mem)
{
    ExFreePoolWithTag(mem, LWIP_TAG);
}
