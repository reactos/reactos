//
// calloc.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of calloc().  Note that _calloc_base is defined in its own
// source file to resolve various issues when linking.
//
#include <corecrt_internal.h>
#include <malloc.h>

// Allocates a block of memory of size 'count * size' in the heap.  The newly
// allocated block is zero-initialized.  If allocation fails, nullptr is
// returned.
//
// This function supports patching and therefore must be marked noinline.
// Both _calloc_dbg and _calloc_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to calloc
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) _CRTRESTRICT void* __cdecl calloc(
    size_t const count,
    size_t const size
    )
{
    #ifdef _DEBUG
    return _calloc_dbg(count, size, _NORMAL_BLOCK, nullptr, 0);
    #else
    return _calloc_base(count, size);
    #endif
}
