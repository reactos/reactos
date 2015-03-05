#include "precomp.h"

void *operator new(size_t size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}

void operator delete(void *p)
{
    LocalFree(p);
}
