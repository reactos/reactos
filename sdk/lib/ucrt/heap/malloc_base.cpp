//
// malloc_base.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _malloc_base().  This is defined in a different source file
// from the malloc() function to allow malloc() to be replaced by the user.
//
#include <corecrt_internal.h>
#include <malloc.h>
#include <new.h>



// This function implements the logic of malloc().  It is called directly by the
// malloc() function in the Release CRT and is called by the debug heap in the
// Debug CRT.
//
// This function must be marked noinline, otherwise malloc and
// _malloc_base will have identical COMDATs, and the linker will fold
// them when calling one from the CRT. This is necessary because malloc
// needs to support users patching in custom implementations.
extern "C" __declspec(noinline) _CRTRESTRICT void* __cdecl _malloc_base(size_t const size)
{
    // Ensure that the requested size is not too large:
    _VALIDATE_RETURN_NOEXC(_HEAP_MAXREQ >= size, ENOMEM, nullptr);

    // Ensure we request an allocation of at least one byte:
    size_t const actual_size = size == 0 ? 1 : size;

    for (;;)
    {
        void* const block = HeapAlloc(__acrt_heap, 0, actual_size);
        if (block)
            return block;

        // Otherwise, see if we need to call the new handler, and if so call it.
        // If the new handler fails, just return nullptr:
        if (_query_new_mode() == 0 || !_callnewh(actual_size))
        {
            errno = ENOMEM;
            return nullptr;
        }

        // The new handler was successful; try to allocate again...
    }
}
