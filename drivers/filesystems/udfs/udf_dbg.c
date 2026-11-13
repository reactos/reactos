////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
#include "udffs.h"
#if defined(UDF_DBG)

//#define TRACK_REF_COUNTERS

ULONG UdfTimeStamp = -1;

#define MAX_MEM_DEBUG_DESCRIPTORS 8192

typedef struct _MEM_DESC {
    ULONG   Length;
    PCHAR   Addr;
#ifdef TRACK_SYS_ALLOC_CALLERS
    ULONG   SrcId;
    ULONG   SrcLine;
#endif //TRACK_SYS_ALLOC_CALLERS
    POOL_TYPE Type;
} MEM_DESC, *PMEM_DESC;


MEM_DESC    MemDesc[MAX_MEM_DEBUG_DESCRIPTORS];
ULONG       cur_max = 0;
ULONG       AllocCountPaged = 0;
ULONG       AllocCountNPaged = 0;
ULONG       MemDescInited = 0;

PVOID
DebugAllocatePool(
   POOL_TYPE Type,
   ULONG size
#ifdef TRACK_SYS_ALLOC_CALLERS
 , ULONG SrcId,
   ULONG SrcLine
#endif //TRACK_SYS_ALLOC_CALLERS
) {
    ULONG i;
//    UDFPrint(("SysAllocated: %x\n",AllocCount));
    if (!MemDescInited) {
        RtlZeroMemory(&MemDesc, sizeof(MemDesc));
        MemDescInited = 1;
    }
    for (i=0;i<cur_max;i++) {
        if (MemDesc[i].Addr==NULL) {
            MemDesc[i].Addr = (PCHAR)ExAllocatePoolWithTag(Type, (size), 'Fnwd'); // dwnF

            ASSERT(MemDesc[i].Addr);

            if (MemDesc[i].Addr) {
                if (Type == PagedPool) {
                    AllocCountPaged += (size+7) & ~7;
                } else {
                    AllocCountNPaged += (size+7) & ~7;
                }
            }

            MemDesc[i].Length = size;
            MemDesc[i].Type = Type;
#ifdef TRACK_SYS_ALLOC_CALLERS
            MemDesc[i].SrcId   = SrcId;
            MemDesc[i].SrcLine = SrcLine;
#endif //TRACK_SYS_ALLOC_CALLERS
            return MemDesc[i].Addr;
        }
    }
    if (cur_max == MAX_MEM_DEBUG_DESCRIPTORS) {
        UDFPrint(("Debug memory descriptor list full\n"));
        return ExAllocatePoolWithTag(Type, (size) , 'Fnwd');
    }

    MemDesc[i].Addr = (PCHAR)ExAllocatePoolWithTag(Type, (size) , 'Fnwd');

    if (MemDesc[i].Addr) {
        if (Type == PagedPool) {
            AllocCountPaged += (size+7) & ~7;
        } else {
            AllocCountNPaged += (size+7) & ~7;
        }
    }

    MemDesc[i].Length = (size);
#ifdef TRACK_SYS_ALLOC_CALLERS
    MemDesc[i].SrcId   = SrcId;
    MemDesc[i].SrcLine = SrcLine;
#endif //TRACK_SYS_ALLOC_CALLERS
    MemDesc[i].Type = Type;
    cur_max++;
    return MemDesc[cur_max-1].Addr;

}

VOID DebugFreePool(PVOID addr) {
    ULONG i;

    ASSERT(addr);

    for (i=0;i<cur_max;i++) {
        if (MemDesc[i].Addr == addr)  {

            if (MemDesc[i].Type == PagedPool) {
                AllocCountPaged -= (MemDesc[i].Length+7) & ~7;
            } else {
                AllocCountNPaged -= (MemDesc[i].Length+7) & ~7;
            }

            MemDesc[i].Addr = NULL;
            MemDesc[i].Length = 0;
#ifdef TRACK_SYS_ALLOC_CALLERS
            MemDesc[i].SrcId   = 0;
            MemDesc[i].SrcLine = 0;
#endif //TRACK_SYS_ALLOC_CALLERS
            goto not_bug;
        }
    }
    if (i==cur_max && cur_max != MAX_MEM_DEBUG_DESCRIPTORS) {
        UDFPrint(("Buug! - Deallocating nonallocated block\n"));
        return;
    }
not_bug:
//    UDFPrint(("SysAllocated: %x\n",AllocCount));
    ExFreePoolWithTag(addr, 0);
}

#endif // UDF_DBG
