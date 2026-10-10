#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

void* __cdecl recalloc(void *mem, size_t num, size_t size)
{
    size_t old_size;
    void *ret;

    if(!mem)
        return calloc(num, size);

    size = num*size;
    old_size = _msize(mem);

    ret = realloc(mem, size);
    if(!ret) {
        *_errno() = ENOMEM;
        return NULL;
    }

    if(size>old_size)
        memset((unsigned char*)ret+old_size, 0, size-old_size);
    return ret;
}

#ifdef _WIN64
void* __imp__recalloc = recalloc;
#else
void* _imp___recalloc = recalloc;
#endif
