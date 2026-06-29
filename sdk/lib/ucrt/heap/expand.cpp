//
// expand.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _expand().
//
#include <corecrt_internal.h>
#include <malloc.h>



// Enclaves have a different heap implementation.

#ifdef _UCRT_ENCLAVE_BUILD

// Tests whether the allocation contraction is possible
__inline static bool is_contraction_possible(size_t const) throw()
{
    return FALSE;
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

// Tests whether the allocation contraction is possible
__inline static bool is_contraction_possible(size_t const old_size) throw()
{
    // Check if object allocated on low fragmentation heap.
    //The LFH can only allocate blocks up to 16KB in size.
    if (old_size <= 0x4000)
    {
        LONG heap_type = -1;
        if (!HeapQueryInformation(
            __acrt_heap,
            HeapCompatibilityInformation,
            &heap_type,
            sizeof(heap_type),
            nullptr))
        {
            return FALSE;
        }
        return heap_type != 2;
    }
    // Contraction possible for objects not on the LFH
    return TRUE;
}

#endif /* _UCRT_ENCLAVE_BUILD */


// This function implements the logic of expand().  It is called directly by the
// _expand() function in the Release CRT, and called by the debug heap in the
// Debug CRT.
//
// This function must be marked noinline, otherwise _expand and
// _expand_base will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because _expand
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) void* __cdecl _expand_base(void* const block, size_t const size)
{
    // Validation section
    _VALIDATE_RETURN      (block != nullptr,     EINVAL, nullptr);
    _VALIDATE_RETURN_NOEXC(size <= _HEAP_MAXREQ, ENOMEM, nullptr);

    size_t const old_size = static_cast<size_t>(HeapSize(__acrt_heap, 0, block));
    size_t const new_size = size == 0 ? 1 : size;

    void* new_block = HeapReAlloc(__acrt_heap, HEAP_REALLOC_IN_PLACE_ONLY, block, new_size);
    if (new_block != nullptr)
        return new_block;

    // If a failure to contract was caused by platform limitations, just
    // return the original block.
    if (new_size <= old_size && !is_contraction_possible(old_size))
        return block;

    errno = __acrt_errno_from_os_error(GetLastError());
    return nullptr;
}

// Expands or contracts a block of memory in the heap.
//
// This function resizes a block of memory in the heap to 'size' bytes.  The
// new size may be either greater (expansion) or less (contraction) than the
// original size of the block.  This function never moves the block.  In the
// case of expansion, if the block cannot be expanded to 'size', it is expanded
// as much as possible.
//
// This function supports patching and therefore must be marked noinline.
// Both _expand_dbg and _expand_base must also be marked noinline
// to prevent identical COMDAT folding from substituting calls to _expand
// with either other function or vice versa.
extern "C" _CRT_HYBRIDPATCHABLE __declspec(noinline) void* __cdecl _expand(void* const block, size_t const size)
{
    #ifdef _DEBUG
    return _expand_dbg(block, size, _NORMAL_BLOCK, nullptr, 0);
    #else
    return _expand_base(block, size);
    #endif
}
