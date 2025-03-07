//
// heapchk.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _heapchk().
//
#include <corecrt_internal.h>
#include <malloc.h>

// Performs a consistency check on the heap.  The return value is one of the
// following:
//  * _HEAPOK       The validation of the heap completed successfully
//  * _HEAPBADNODE  Some node in the heap is malformed and the heap is corrupt
extern "C" int __cdecl _heapchk()
{
    if (!HeapValidate(__acrt_heap, 0, nullptr))
        return _HEAPBADNODE;

    return _HEAPOK;
}
