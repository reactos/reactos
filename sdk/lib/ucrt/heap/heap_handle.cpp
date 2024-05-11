//
// heap_handle.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the global CRT heap handle and initialization/termination functions.
//
#include <corecrt_internal.h>
#include <malloc.h>



// The CRT heap handle.  This global variable is modified only during CRT
// startup and CRT shutdown.
extern "C" HANDLE __acrt_heap = nullptr;



// Initializes the heap.  This function must be called during CRT startup, and
// must be called before any user code that might use the heap is executed.
extern "C" bool __cdecl __acrt_initialize_heap()
{
    __acrt_heap = GetProcessHeap();
    if (__acrt_heap == nullptr)
        return false;

    return true;
}

// Uninitializes the heap.  This function should be called during CRT shutdown,
// after any user code that might use the heap has stopped running.
extern "C" bool __cdecl __acrt_uninitialize_heap(bool const /* terminating */)
{
    __acrt_heap = nullptr;
    return true;
}

// Gets the HANDLE of the CRT heap.  Because the CRT always uses the process
// heap, this function always returns the same thing as GetProcessHeap().
extern "C" intptr_t __cdecl _get_heap_handle()
{
    _ASSERTE(__acrt_heap != nullptr);
    return reinterpret_cast<intptr_t>(__acrt_heap);
}

// Internal CRT function to get the HANDLE of the CRT heap, that returns a
// HANDLE instead of an intptr_t.
extern "C" HANDLE __acrt_getheap()
{
    _ASSERTE(__acrt_heap != nullptr);
    return __acrt_heap;
}
