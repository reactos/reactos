#include "lwip/mem.h"

#ifndef LWIP_TAG
    #define LWIP_TAG 'PIwl'
#endif

void *
ros_malloc(mem_size_t size)
{
    return ExAllocatePoolWithTag(NonPagedPool, size, LWIP_TAG);
}

void *
ros_calloc(mem_size_t count, mem_size_t size)
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

/* This is only used to trim in lwIP */
void *
ros_realloc(void *mem, size_t size)
{
    void* new_mem;

    /* realloc() with a NULL mem pointer acts like a call to malloc() */
    if (mem == NULL) {
        return ros_malloc(size);
    }

    /* realloc() with a size 0 acts like a call to free() */
    if (size == 0) {
        ros_free(mem);
        return NULL;
    }

    /* Allocate the new buffer first */
    new_mem = ros_malloc(size);
    if (new_mem == NULL) {
        /* The old buffer is still intact */
        return NULL;
    }

    /* Copy the data over */
    RtlCopyMemory(new_mem, mem, size);

    /* Deallocate the old buffer */
    ros_free(mem);

    /* Return the newly allocated block */
    return new_mem;
}