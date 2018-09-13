#include <cache.hxx>

#ifdef TEST

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "hndlmgr.hxx"

#define ALLOCATE_FIXED_MEMORY(size) malloc(size)
#define REALLOCATE_MEMORY(ptr, size, flags) realloc(ptr, size)
#define FREE_MEMORY(ptr) free(ptr)

int main ()
{
    HNDLMGR HandleMgr;
    HANDLE h[10];
    HANDLE hBad = (HANDLE) 5150;

    // Test alloc and realloc of handle heap.
    for (int i=9; i>=0; i--)
    {
        h[i] = HandleMgr.Alloc (54);
        printf ("Alloc h[%d] = %d\n", i, (DWORD) h[i]);
    }

    // Test invalid, valid, and double free.
    printf ("Free(%d) returns %d\n", NULL, HandleMgr.Free(NULL));
    printf ("Free(%d) returns %d\n", hBad, HandleMgr.Free(hBad));
    printf ("Free(%d) returns %d\n", h[3], HandleMgr.Free(h[3]));
    printf ("Free(%d) returns %d\n", h[3], HandleMgr.Free(h[3]));
    printf ("Free(%d) returns %d\n", h[9], HandleMgr.Free(h[9]));

    // Test mapping of invalid, free, and valid handles.
    printf ("Map(%d) = %d\n", NULL, HandleMgr.Map(NULL));
    printf ("Map(%d) = %d\n", hBad, HandleMgr.Map(hBad));
    printf ("Map(%d) = %d\n", h[3], HandleMgr.Map(h[3]));
    printf ("Map(%d) = %d\n", h[5], HandleMgr.Map(h[5]));

    // Test recycling of handles from free list.
    i = 3;
    h[i] = HandleMgr.Alloc (42);
    printf ("Alloc h[%d] = %d\n", i, (DWORD) h[i]);
    i = 9;
    h[i] = HandleMgr.Alloc (42);
    printf ("Alloc h[%d] = %d\n", i, (DWORD) h[i]);

    return 1;
}

#endif // TEST

#define INC_GROW 8

//=========================================================================
void HNDLMGR::Destroy (void)
{
    if (pHeap)
    {
        for (DWORD iHandle=0; iHandle < pHeap->dwNumHandles; iHandle++)
        {
            if ((DWORD_PTR) pHeap->pvHandles[iHandle] >= pHeap->dwMaxHandles)
                FREE_MEMORY (pHeap->pvHandles[iHandle]);
        }
        FREE_MEMORY (pHeap);
    }        
}

//=========================================================================
BOOL HNDLMGR::IsValidOffset (DWORD_PTR dwp)
{
    return (pHeap && (dwp < pHeap->dwNumHandles) &&
        ((DWORD_PTR) pHeap->pvHandles[dwp]) >= pHeap->dwMaxHandles);
}

//=========================================================================
HANDLE HNDLMGR::Alloc (DWORD cbAlloc)
{
    PVOID pTemp;
    
    if (!pHeap)
    {
        // Allocate the heap.
        pHeap = (HNDLHEAP*) ALLOCATE_FIXED_MEMORY
            (sizeof(HNDLHEAP) + INC_GROW * sizeof(LPVOID));
        if (!pHeap)
            return NULL;

        // Initialize the heap.
        pHeap->dwNumHandles = 0;
        pHeap->dwNumInUse = 0;
        pHeap->dwMaxHandles = 0xFFFFFFFF;
        pHeap->dwFirstFree = 0;
        for (DWORD iHandle = 0; iHandle < INC_GROW; iHandle++)
        {
            pHeap->pvHandles[pHeap->dwNumHandles] =
                (HANDLE) (pHeap->dwNumHandles + 1);
            pHeap->dwNumHandles++;
        }
    }

    else if (pHeap->dwFirstFree == pHeap->dwNumHandles)
    {
        // Reallocate the heap.
        if (pHeap->dwNumHandles + INC_GROW >= pHeap->dwMaxHandles)
        {
            // Uh oh, heap is hit the lower bound set by the allocator.
            return NULL; 
        }
        pTemp = REALLOCATE_MEMORY (pHeap, sizeof(HNDLHEAP)
            + (pHeap->dwNumHandles + INC_GROW) * sizeof(LPVOID), LMEM_MOVEABLE);
        if (!pTemp)
            return NULL;
        pHeap = (HNDLHEAP*) pTemp;

        // Extend the free list.
        for (DWORD iHandle = 0; iHandle < INC_GROW; iHandle++)
        {
            pHeap->pvHandles[pHeap->dwNumHandles] =
                (HANDLE) (pHeap->dwNumHandles + 1);
            pHeap->dwNumHandles++;
        }
    }

    // Allocate a handle.
    pTemp = ALLOCATE_FIXED_MEMORY (cbAlloc);
    if (!pTemp)
        return NULL;
    if ((DWORD_PTR) pTemp < pHeap->dwNumHandles)
    {
        // Uh oh, allocator returned a low value!
        FREE_MEMORY (pTemp);
        return NULL;
    }
    if (pHeap->dwMaxHandles >= ((DWORD_PTR) pTemp))
        pHeap->dwMaxHandles = ((DWORD_PTR) pTemp);

    // Pop the handle off the top of the free list.
    DWORD_PTR dwOffset = pHeap->dwFirstFree;
    pHeap->dwFirstFree = (DWORD_PTR) pHeap->pvHandles[pHeap->dwFirstFree];
    pHeap->pvHandles[dwOffset] = pTemp;
    pHeap->dwNumInUse++;
    return (HANDLE) (dwOffset + 1);
}

//=========================================================================
LPVOID HNDLMGR::Map (HANDLE h)
{
    // Subtract one from handle to get offset.
    DWORD_PTR dwOffset = (DWORD_PTR)h - 1;
    if (!IsValidOffset (dwOffset))
        return NULL;
    else
        return pHeap->pvHandles[dwOffset];
}

//=========================================================================
BOOL HNDLMGR::Free (HANDLE h)
{
    // Subtract one from handle to get offset.
    DWORD_PTR dwOffset = (DWORD_PTR)h - 1;
    if (!IsValidOffset (dwOffset))
        return FALSE;
        
    // Push the handle on the top of the free list.
    FREE_MEMORY (pHeap->pvHandles[dwOffset]);
    pHeap->pvHandles[dwOffset] = (LPVOID) pHeap->dwFirstFree;
    pHeap->dwFirstFree = dwOffset;
    pHeap->dwNumInUse--;
    return TRUE;
}

