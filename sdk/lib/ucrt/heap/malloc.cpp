//
// malloc.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of malloc().
//
#include <corecrt_internal.h>
#include <malloc.h>



// Allocates a block of memory of size 'size' bytes in the heap.  If allocation
// fails, nullptr is returned.
//
// This function supports patching and therefore must be marked noinline.
// Both _malloc_dbg and _malloc_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to malloc
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) _CRTRESTRICT void* __cdecl malloc(size_t const size)
{
    #ifdef _DEBUG
    return _malloc_dbg(size, _NORMAL_BLOCK, nullptr, 0);
    #else
    return _malloc_base(size);
    #endif
}
