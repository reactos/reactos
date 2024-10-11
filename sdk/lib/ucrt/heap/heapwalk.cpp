//
// heapwalk.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of _heapwalk().
//
#include <corecrt_internal.h>
#include <malloc.h>



// Calls HeapWalk and guards against access violations in case a bad pointer is
// passed to the CRT _heapwalk function (or in case the heap is corrupt).
static int __cdecl try_walk(PROCESS_HEAP_ENTRY* const win32_entry) throw()
{
    __try
    {
        if (HeapWalk(__acrt_heap, win32_entry))
            return _HEAPOK;

        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            return _HEAPEND;

        return _HEAPBADNODE;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
    {
        return _HEAPBADNODE;
    }
    __endtry
}

// Walks the heap, returning information on one entry at a time.  If there are
// multiple threads in the process that are allocating using the process heap,
// the caller must lock the heap for the duration of the heap walk, by calling
// the Windows API HeapLock.  See the documentation of HeapWalk for more
// information.
//
// The return value is one of the following:
//  * _HEAPOK       The call completed successfully and returned a node
//  * _HEAPBADPTR   The provided 'entry' pointer is invalid
//  * _HEAPBADBEGIN The initial node cannot be found in the heap
//  * _HEAPBADNODE  A node was malformed or the heap is corrupt
//  * _HEAPEND      The end of the heap was successfully reached
extern "C" int __cdecl _heapwalk(_HEAPINFO* const entry)
{
    // Validation section
    _VALIDATE_RETURN(entry != nullptr, EINVAL, _HEAPBADPTR);

    PROCESS_HEAP_ENTRY win32_entry = { 0 };
    win32_entry.wFlags = 0;
    win32_entry.iRegionIndex = 0;
    win32_entry.lpData = entry->_pentry;

    // If _pentry is nullptr, then we're just starting the heap walk:
    if (win32_entry.lpData == nullptr)
    {
        if (!HeapWalk(__acrt_heap, &win32_entry))
            return _HEAPBADBEGIN;
    }
    else
    {
        if (entry->_useflag == _USEDENTRY)
        {
            if (!HeapValidate(__acrt_heap, 0, entry->_pentry))
                return _HEAPBADNODE;

            win32_entry.wFlags = PROCESS_HEAP_ENTRY_BUSY;
        }

        int const status = try_walk(&win32_entry);
        if (status != _HEAPOK)
            return status;
    }

    for (;;)
    {
        if (win32_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
        {
            entry->_pentry = static_cast<int*>(win32_entry.lpData);
            entry->_size = win32_entry.cbData;
            entry->_useflag = _USEDENTRY;
            return _HEAPOK;
        }

        int const status = try_walk(&win32_entry);
        if (status != _HEAPOK)
            return status;
    }
}
