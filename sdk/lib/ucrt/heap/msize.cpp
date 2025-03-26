//
// msize.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of msize().
//
#include <corecrt_internal.h>
#include <malloc.h>



// This function implements the logic of _msize().  It is called only in the
// Release CRT.  The Debug CRT has its own implementation of this function.
//
// This function must be marked noinline, otherwise _msize and
// _msize_base will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because _msize
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) size_t __cdecl _msize_base(void* const block) noexcept
{
    // Validation section
    _VALIDATE_RETURN(block != nullptr, EINVAL, static_cast<size_t>(-1));

    return static_cast<size_t>(HeapSize(__acrt_heap, 0, block));
}

// Calculates the size of the specified block in the heap.  'block' must be a
// pointer to a valid block of heap-allocated memory (it must not be nullptr).
//
// This function supports patching and therefore must be marked noinline.
// Both _msize_dbg and _msize_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to _msize
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) size_t __cdecl _msize(void* const block)
{
    #ifdef _DEBUG
    return _msize_dbg(block, _NORMAL_BLOCK);
    #else
    return _msize_base(block);
    #endif
}
