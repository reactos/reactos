/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef MSXMLPRFCOUNTERS_H
#define MSXMLPRFCOUNTERS_H

#ifdef PRFDATA
// Global C functions for managing perf counters.

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

void PrfInitCounters();
void PrfCleanupCounters();

void PrfCountGC( long delta );
void PrfCountObjects( long delta );
void PrfCountCommittedPages( long delta );
void PrfCountHeapSize( long delta );
void PrfCountOOM( long delta );

LPVOID PrfHeapAlloc(
  HANDLE hHeap,  // handle to the private heap block
  DWORD dwFlags, // heap allocation control flags
  DWORD dwBytes  // number of bytes to allocate
);
BOOL PrfHeapFree(
  HANDLE hHeap,  // handle to the heap
  DWORD dwFlags, // heap freeing flags
  LPVOID lpMem   // pointer to the memory to free
);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#else

#define PrfHeapAlloc HeapAlloc
#define PrfHeapFree  HeapFree

#endif

#endif
