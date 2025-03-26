//
// free.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of free().
//
#include <corecrt_internal.h>
#include <malloc.h>

// Frees a block in the heap.  The 'block' pointer must either be a null pointer
// or must point to a valid block in the heap.
//
// This function supports patching and therefore must be marked noinline.
// Both _free_dbg and _free_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to free
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) void __cdecl free(void* const block)
{
    // Some libraries that hook memory allocation routines (such as libtcmalloc)
    // look for an appropriate place inside free to place a patch to its version
    // of free. Without these extra instructions padding the call to _free_base,
    // some libraries may choose to insert this patch in _free_base instead.
    volatile int extra_instructions_for_patching_libraries = 0;
    (void) extra_instructions_for_patching_libraries;

    #ifdef _DEBUG
    _free_dbg(block, _NORMAL_BLOCK);
    #else
    _free_base(block);
    #endif
}
