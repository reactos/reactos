//
// heapmin.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _heapmin().
//
#include <corecrt_internal.h>
#include <malloc.h>

// Minimizes the heap, freeing as much memory as possible back to the OS.
// Returns 0 on success, -1 on failure.
extern "C" int __cdecl _heapmin()
{
    if (!HeapCompact(__acrt_heap, 0))
        return -1;

    return 0;
}
