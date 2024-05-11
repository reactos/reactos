//
// calloc_base.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _calloc_base().  This is defined in a different source file
// from the calloc() function to allow calloc() to be replaced by the user.
//
#include <corecrt_internal.h>
#include <malloc.h>
#include <new.h>

// This function implements the logic of calloc().
//
// This function must be marked noinline, otherwise calloc and
// _calloc_base will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because calloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) _CRTRESTRICT void* __cdecl _calloc_base(
    size_t const count,
    size_t const size
    )
{
    // Ensure that (count * size) does not overflow
    _VALIDATE_RETURN_NOEXC(count == 0 || (_HEAP_MAXREQ / count) >= size, ENOMEM, nullptr);

    // Ensure that we allocate a nonzero block size:
    size_t const requested_block_size = count * size;
    size_t const actual_block_size = requested_block_size == 0
        ? 1
        : requested_block_size;

    for (;;)
    {
        void* const block = HeapAlloc(__acrt_heap, HEAP_ZERO_MEMORY, actual_block_size);

        // If allocation succeeded, return the pointer to the new block:
        if (block)
        {
            return block;
        }

        // Otherwise, see if we need to call the new handler, and if so call it.
        // If the new handler fails, just return nullptr:
        if (_query_new_mode() == 0 || !_callnewh(actual_block_size))
        {
            errno = ENOMEM;
            return nullptr;
        }

        // The new handler was successful; try to allocate aagain...
    }
}
