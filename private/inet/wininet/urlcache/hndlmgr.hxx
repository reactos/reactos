/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: hndlmgr.hxx

Abstract:

    Interfaces to manage a heap of handles with a free list.
    All methods assume caller has serialized concurrent access.

Author:

    Rajeev Dujari (rajeevd) 08-Nov-96

Revision History:

--*/

struct HNDLHEAP
{
    DWORD_PTR   dwNumHandles;  // current number of handles in array
    DWORD       dwNumInUse;    // number of handles in use
    DWORD_PTR   dwMaxHandles;  // lowest value returned by allocator
    DWORD_PTR   dwFirstFree;   // index of first free element in array
    LPVOID      pvHandles[1];  // array of handle values
};

class HNDLMGR
{
    HNDLHEAP* pHeap;

    BOOL IsValidOffset (DWORD_PTR dwp);
    
public:
    HNDLMGR() {pHeap = NULL;}
    void Destroy (void);
    BOOL InUse(void) {return pHeap && pHeap->dwNumInUse;}
    HANDLE Alloc (DWORD cbAlloc);
    LPVOID Map (HANDLE h);
    BOOL Free (HANDLE h);
};


