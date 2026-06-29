//
// debug_heap_hook.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of the default debug heap allocation hook.  This is in its own
// object so that it may be replaced by a client hook at link time when the
// static CRT is used.
//
#ifndef _DEBUG
    #error This file is supported only in debug builds
#endif

#include <corecrt_internal.h>



// A default heap allocation hook that permits all allocations
extern "C" int __cdecl _CrtDefaultAllocHook(
    int                  const allocation_type,
    void*                const data,
    size_t               const size,
    int                  const block_use,
    long                 const request,
    unsigned char const* const file_name,
    int                  const line_number
    )
{
    UNREFERENCED_PARAMETER(allocation_type);
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(block_use);
    UNREFERENCED_PARAMETER(request);
    UNREFERENCED_PARAMETER(file_name);
    UNREFERENCED_PARAMETER(line_number);

    return 1; // Allow all heap operations
}

extern "C" { _CRT_ALLOC_HOOK _pfnAllocHook = _CrtDefaultAllocHook; }
