#include "pch.h"
#pragma hdrstop

#include "alloc.h"
#include "except.h"


void * __cdecl operator new(
    size_t size
    )
{
    void *pv = LocalAlloc(LMEM_FIXED, size);
    if (NULL == pv)
        throw CAllocException();

    return pv;
}

void __cdecl operator delete(
    void *ptr
    )
{
    if (NULL != ptr)
        LocalFree(ptr);
}

