////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
#include "udffs.h"
#if defined(UDF_DBG) || defined(PRINT_ALWAYS)

//#define TRACK_RESOURCES
//#define TRACK_REF_COUNTERS

ULONG ResCounter = 0;
ULONG AcqCounter = 0;
ULONG UdfTimeStamp = -1;

BOOLEAN
UDFDebugAcquireResourceSharedLite(
      IN PERESOURCE Resource,
      IN BOOLEAN    Wait,
      ULONG         BugCheckId,
      ULONG         Line
) {
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif

    BOOLEAN     Success;

#ifdef USE_DLD

    if (Wait) {
        DLDAcquireShared(Resource, BugCheckId, Line,FALSE);
        Success = TRUE;
    } else {
        Success = ExAcquireResourceSharedLite(Resource,Wait);
    }

#else

    Success = ExAcquireResourceSharedLite(Resource,Wait);

#endif // USE_DLD

    if(Success) {
#ifdef TRACK_RESOURCES
        KdPrint(("Res:Sha:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
            BugCheckId,Line,PsGetCurrentThread()));
#endif
        AcqCounter++;
        return Success;
    }
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha:Fail:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif
    return FALSE;
}

BOOLEAN
UDFDebugAcquireSharedStarveExclusive(
      IN PERESOURCE Resource,
      IN BOOLEAN    Wait,
      ULONG         BugCheckId,
      ULONG         Line
) {
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha*:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif

    BOOLEAN     Success;

#ifdef USE_DLD

    if (Wait) {
        DLDAcquireShared(Resource, BugCheckId, Line,FALSE);
        Success = TRUE;
    } else {
        Success = ExAcquireResourceSharedLite(Resource,Wait);
    }

#else

    Success = ExAcquireResourceSharedLite(Resource,Wait);

#endif // USE_DLD

    if(Success) {
#ifdef TRACK_RESOURCES
        KdPrint(("Res:Sha*:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
            BugCheckId,Line,PsGetCurrentThread()));
#endif
        AcqCounter++;
        return Success;
    }
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha*:Fail:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif
    return FALSE;
}

BOOLEAN
UDFDebugAcquireResourceExclusiveLite(
      IN PERESOURCE Resource,
      IN BOOLEAN    Wait,
      ULONG         BugCheckId,
      ULONG         Line
) {
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Exc:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif


    BOOLEAN     Success;

#ifdef USE_DLD

    if (Wait) {
        DLDAcquireExclusive(Resource, BugCheckId, Line);
        Success = TRUE;
    } else {
        Success = ExAcquireResourceExclusiveLite(Resource,Wait);
    }

#else

    Success = ExAcquireResourceExclusiveLite(Resource,Wait);

#endif // USE_DLD

    
    
    if(Success) {
#ifdef TRACK_RESOURCES
        KdPrint(("Res:Exc:OK:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
            BugCheckId,Line,PsGetCurrentThread()));
#endif
        AcqCounter++;
        return Success;
    }
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Exc:Fail:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif
//    BrutePoint();
    return FALSE;
    
}

VOID 
UDFDebugReleaseResourceForThreadLite(
    IN PERESOURCE  Resource,
    IN ERESOURCE_THREAD  ResourceThreadId,
    ULONG         BugCheckId,
    ULONG         Line
    )
{
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Free:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif
    ExReleaseResourceForThreadLite(Resource, ResourceThreadId);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Free:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    AcqCounter--;
}

VOID 
UDFDebugDeleteResource(
    IN PERESOURCE  Resource,
    IN ERESOURCE_THREAD  ResourceThreadId,
    ULONG         BugCheckId,
    ULONG         Line
    )
{
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Del:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    _SEH2_TRY {
        ASSERT((*((PULONG)Resource)));
        ASSERT((*(((PULONG)Resource)+1)));
        ExDeleteResourceLite(Resource);
        RtlZeroMemory(Resource, sizeof(ERESOURCE));
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Del:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    ResCounter--;
}

NTSTATUS
UDFDebugInitializeResourceLite(
    IN PERESOURCE  Resource,
    IN ERESOURCE_THREAD  ResourceThreadId,
    ULONG         BugCheckId,
    ULONG         Line
    )
{
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    NTSTATUS RC;
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Ini:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    ASSERT(!(*((PULONG)Resource)));
    ASSERT(!(*(((PULONG)Resource)+1)));
    RC = ExInitializeResourceLite(Resource);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Ini:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    if(NT_SUCCESS(RC)) {
        ResCounter++;
    }
    return RC;
}

VOID
UDFDebugConvertExclusiveToSharedLite(
    IN PERESOURCE  Resource,
    IN ERESOURCE_THREAD  ResourceThreadId,
    ULONG         BugCheckId,
    ULONG         Line
    )
{
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:2Sha:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
    ExConvertExclusiveToSharedLite(Resource);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:2Sha:Ok:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,ResourceThreadId));
#endif
}

BOOLEAN
UDFDebugAcquireSharedWaitForExclusive(
      IN PERESOURCE Resource,
      IN BOOLEAN    Wait,
      ULONG         BugCheckId,
      ULONG         Line
) {
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha*:Try:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif

    BOOLEAN     Success;

#ifdef USE_DLD

    if (Wait) {
        DLDAcquireShared(Resource, BugCheckId, Line,TRUE);
        Success = TRUE;
    } else {
        Success = ExAcquireSharedWaitForExclusive(Resource,Wait);
    }

#else

    Success = ExAcquireSharedWaitForExclusive(Resource,Wait);

#endif // USE_DLD


    if(Success) {
#ifdef TRACK_RESOURCES
        KdPrint(("Res:Sha*:OK:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
            BugCheckId,Line,PsGetCurrentThread()));
#endif
        return Success;
    }
#ifdef TRACK_RESOURCES
    KdPrint(("Res:Sha*:Fail:Resource:%x:BugCheckId:%x:Line:%d:ThId:%x\n",Resource,
        BugCheckId,Line,PsGetCurrentThread()));
#endif
//    BrutePoint();
    return FALSE;
    
}


LONG
UDFDebugInterlockedIncrement(
    IN PLONG      addr,
    ULONG         BugCheckId,
    ULONG         Line)
{
#ifdef TRACK_REF_COUNTERS
    LONG a;
    a = InterlockedIncrement(addr);
    KdPrint(("ThId:%x:Ilck:Inc:FileId:%x:Line:%d:Ref:%x:Val:%x:%x\n",
        PsGetCurrentThread(),BugCheckId,Line,addr,a-1,a));
    return a;
#else
    return InterlockedIncrement(addr);
#endif
}

LONG
UDFDebugInterlockedDecrement(
    IN PLONG      addr,
    ULONG         BugCheckId,
    ULONG         Line)
{
#ifdef TRACK_REF_COUNTERS
    LONG a;
    a = InterlockedDecrement(addr);
    KdPrint(("ThId:%x:Ilck:Dec:FileId:%x:Line:%d:Ref:%x:Val:%x:%x\n",
        PsGetCurrentThread(),BugCheckId,Line,addr,a+1,a));
    return a;
#else
    return InterlockedDecrement(addr);
#endif
}

LONG
UDFDebugInterlockedExchangeAdd(
    IN PLONG      addr,
    IN LONG       i,
    ULONG         BugCheckId,
    ULONG         Line)
{
#ifdef TRACK_REF_COUNTERS
    LONG a;
    a = InterlockedExchangeAdd(addr,i);
    KdPrint(("ThId:%x:Ilck:Add:FileId:%x:Line:%d:Ref:%x:Val:%x:%x\n",
        PsGetCurrentThread(),BugCheckId,Line,addr,a,a+i));
    return a;
#else
    return InterlockedExchangeAdd(addr,i);
#endif
}

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
//    KdPrint(("SysAllocated: %x\n",AllocCount));
    if(!MemDescInited) {
        RtlZeroMemory(&MemDesc, sizeof(MemDesc));
        MemDescInited = 1;
    }
    for (i=0;i<cur_max;i++) {
        if (MemDesc[i].Addr==NULL) {
            MemDesc[i].Addr = (PCHAR)ExAllocatePoolWithTag(Type, (size), 'Fnwd'); // dwnF

            ASSERT(MemDesc[i].Addr);

            if(MemDesc[i].Addr) {
                if(Type == PagedPool) {
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
    if(cur_max == MAX_MEM_DEBUG_DESCRIPTORS) {
        KdPrint(("Debug memory descriptor list full\n"));
        return ExAllocatePoolWithTag(Type, (size) , 'Fnwd');
    }

    MemDesc[i].Addr = (PCHAR)ExAllocatePoolWithTag(Type, (size) , 'Fnwd');

    if(MemDesc[i].Addr) {
        if(Type == PagedPool) {
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

            if(MemDesc[i].Type == PagedPool) {
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
        KdPrint(("Buug! - Deallocating nonallocated block\n"));
        return;
    }
not_bug:
//    KdPrint(("SysAllocated: %x\n",AllocCount));
    ExFreePool(addr);
}

NTSTATUS
UDFWaitForSingleObject(
    IN PLONG Object,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    KdPrint(("UDFWaitForSingleObject\n"));
    LARGE_INTEGER LocalTimeout;
    LARGE_INTEGER delay;
    delay.QuadPart = -(WAIT_FOR_XXX_EMU_DELAY);

    if(Timeout && (Timeout->QuadPart)) LocalTimeout = *Timeout;
    else LocalTimeout.QuadPart = 0x7FFFFFFFFFFFFFFFLL;

    KdPrint(("SignalState %x\n", *Object));
    if(!Object) return STATUS_INVALID_PARAMETER;
    if((*Object)) return STATUS_SUCCESS;
    while(LocalTimeout.QuadPart>0 && !(*Object) ) {
        KdPrint(("SignalState %x\n", *Object));
        // Stall for a while.
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
        LocalTimeout.QuadPart -= WAIT_FOR_XXX_EMU_DELAY;
    }
    return STATUS_SUCCESS;
} // end UDFWaitForSingleObject()

NTSTATUS
DbgWaitForSingleObject_(
    IN PVOID Object,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    PLARGE_INTEGER to;
    LARGE_INTEGER dto;
    LARGE_INTEGER cto;
    NTSTATUS RC;
    ULONG c = 20;

    dto.QuadPart = -5LL*1000000LL*10LL; // 5 sec
    cto.QuadPart = Timeout->QuadPart;
    if(Timeout) {
        if(dto.QuadPart > Timeout->QuadPart) {
            to = Timeout;
        } else {
            to = &dto;
        }
    } else {
        to = &dto;
    }

    for(; c--; c) {
        RC = KeWaitForSingleObject(Object, Executive, KernelMode, FALSE, to);
        if(RC == STATUS_SUCCESS)
            break;
        KdPrint(("No response ?\n"));
        if(c<2)
            BrutePoint();
    }
    return RC;
}
#endif // UDF_DBG
