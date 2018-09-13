/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_BASE_MPHEAP
#define _CORE_BASE_MPHEAP

// 
// Valid heap creation options 
// 
#define MPHEAP_GROWABLE HEAP_GROWABLE 
#define MPHEAP_REALLOC_IN_PLACE_ONLY HEAP_REALLOC_IN_PLACE_ONLY 
#define MPHEAP_TAIL_CHECKING_ENABLED HEAP_TAIL_CHECKING_ENABLED 
#define MPHEAP_FREE_CHECKING_ENABLED HEAP_FREE_CHECKING_ENABLED 
#define MPHEAP_DISABLE_COALESCE_ON_FREE HEAP_DISABLE_COALESCE_ON_FREE 
#define MPHEAP_ZERO_MEMORY HEAP_ZERO_MEMORY 
#define MPHEAP_COLLECT_STATS 0x10000000 
#define MPHEAP_OBJECT 0x20000000
 
HANDLE 
WINAPI 
MpHeapCreate( 
    DWORD flOptions, 
    DWORD dwInitialSize, 
    DWORD dwParallelism 
    ); 
 
BOOL 
WINAPI 
MpHeapDestroy( 
    HANDLE hMpHeap 
    ); 
 
#if DBG == 1
BOOL 
WINAPI 
MpHeapValidate( 
    HANDLE hMpHeap, 
    LPVOID lpMem 
    ); 
#endif
/* 
UINT 
WINAPI 
MpHeapCompact( 
    HANDLE hMpHeap 
    ); 
*/ 
LPVOID 
WINAPI 
MpHeapAlloc( 
    HANDLE hMpHeap, 
    DWORD flOptions, 
    DWORD dwBytes 
    ); 
/* 
LPVOID 
WINAPI 
MpHeapReAlloc( 
    HANDLE hMpHeap, 
    LPVOID lpMem, 
    DWORD dwBytes 
    ); 
*/ 
BOOL 
WINAPI 
MpHeapFree( 
    HANDLE hMpHeap, 
    LPVOID lpMem 
    ); 
 
// 
// Statistics structure 
// 
typedef struct _MPHEAP_STATISTICS { 
    DWORD Contention; 
    DWORD TotalAllocates; 
    DWORD TotalFrees; 
    DWORD LookasideAllocates; 
    DWORD LookasideFrees; 
    DWORD DelayedFrees; 
} MPHEAP_STATISTICS, *LPMPHEAP_STATISTICS; 
 
DWORD 
MpHeapGetStatistics( 
    HANDLE hMpHeap, 
    LPDWORD lpdwSize, 
    MPHEAP_STATISTICS Statistics[] 
    ); 

#endif _CORE_BASE_MPHEAP