/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WHEAP.H
 *  WOW32 Heap Support (Instead of using malloc/free from CRT)
 *
 *  History:
 *  Created 13-Dec-1991 by Sudeep Bharati (sudeepb)
--*/

//
// Dynamic memory macros
//
// On checked (debug) builds, malloc_w and friends complain when they fail.
//

PVOID FASTCALL malloc_w(ULONG size);
DWORD FASTCALL size_w (PVOID pv);
PVOID FASTCALL malloc_w_zero (ULONG size);
PVOID FASTCALL realloc_w (PVOID p, ULONG size, DWORD dwFlags);
VOID  FASTCALL free_w(PVOID p);
LPSTR malloc_w_strcpy_vp16to32(VPVOID vpstr16, BOOL fMulti, INT cMax);

PVOID FASTCALL malloc_w_or_die(ULONG size);

#define INITIAL_WOW_HEAP_SIZE   32*1024   // 32k
#define GROW_HEAP_AS_NEEDED     0         // grow heap as needed


//*****************************************************************************
// Small Heap -
//*****************************************************************************
BOOL FASTCALL CreateSmallHeap(VOID);
PVOID FASTCALL malloc_w_small (ULONG size);
BOOL FASTCALL free_w_small(PVOID p);
