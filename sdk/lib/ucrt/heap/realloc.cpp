//
// realloc.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of realloc().
//
#include <corecrt_internal.h>
#include <malloc.h>

// Reallocates a block of memory in the heap.
//
// This function reallocates the block pointed to by 'block' such that it is
// 'size' bytes in size.  The new size may be either greater or less than the
// original size of the block.  The reallocation may result in the block being
// moved to a new location in memory.  If the block is moved, the contents of
// the original block are copied.
//
// Standard behavior notes:
// [1] realloc(nullptr, new_size) is equivalent to malloc(new_size)
// [2] realloc(p, 0) is equivalent to free(p), and nullptr is returned
// [3] If reallocation fails, the original block is left unchanged
//
// If 'block' is non-null, it must point to a valid block of memory allocated in
// the heap.
//
// This function supports patching and therefore must be marked noinline.
// Both _realloc_dbg and _realloc_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to realloc
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) _CRTRESTRICT void* __cdecl realloc(
    void*  const block,
    size_t const size
    )
{
    #ifdef _DEBUG
    return _realloc_dbg(block, size, _NORMAL_BLOCK, nullptr, 0);
    #else
    return _realloc_base(block, size);
    #endif
}
