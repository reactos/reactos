////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifdef MY_USE_INTERNAL_MEMMANAGER

#ifdef _X86_

__inline VOID DbgTouch(IN PVOID addr)
{
    __asm {
        mov  eax,addr
        mov  al,[byte ptr eax]
    }
}

#else   // NO X86 optimization , use generic C/C++

__inline VOID DbgTouch(IN PVOID addr)
{
    UCHAR a = ((PUCHAR)addr)[0];
}

#endif // _X86_

//MEM_ALLOC_DESC Allocs[MY_HEAP_MAX_BLOCKS];

MEM_FRAME_ALLOC_DESC FrameList[MY_HEAP_MAX_FRAMES];
#ifdef MEM_LOCK_BY_SPINLOCK
KSPIN_LOCK FrameLock;
KIRQL oldIrql;
#define LockMemoryManager()        KeAcquireSpinLock(&FrameLock, &oldIrql)
#define UnlockMemoryManager()      KeReleaseSpinLock(&FrameLock, oldIrql)
__inline 
NTSTATUS
InitLockMemoryManager() {
    KeInitializeSpinLock(&FrameLock);
    return STATUS_SUCCESS;
}
#define DeinitLockMemoryManager()  {NOTHING;}
#else //MEM_LOCK_BY_SPINLOCK
ERESOURCE FrameLock;
#define LockMemoryManager()        ExAcquireResourceExclusiveLite(&FrameLock, TRUE)
#define UnlockMemoryManager()      ExReleaseResourceForThreadLite(&FrameLock, ExGetCurrentResourceThread())
#define InitLockMemoryManager()    ExInitializeResourceLite(&FrameLock)
#define DeinitLockMemoryManager()  ExDeleteResourceLite(&FrameLock)
#endif //MEM_LOCK_BY_SPINLOCK
ULONG FrameCount;
ULONG LastFrame;
BOOLEAN MyMemInitialized = FALSE;

#define MyAllocIsFrameFree(FrameList, i) \
    (!(FrameList[i].LastUsed || FrameList[i].FirstFree))

#ifdef UDF_DBG
ULONG MemTotalAllocated;
PCHAR BreakAddr;

VOID
MyAllocDumpDescr(
    PMEM_ALLOC_DESC Allocs,
    ULONG i
    )
{
    BOOLEAN Used;

    Used = (Allocs[i].Len & MY_HEAP_FLAG_USED) ? TRUE : FALSE;
    KdPrint(("block %x \t%s addr %x len %x  \t", i, Used ? "used" : "free", Allocs[i].Addr, (Allocs[i].Len) & MY_HEAP_FLAG_LEN_MASK));
#ifdef MY_HEAP_TRACK_OWNERS
    KdPrint(("src %x   \t line %d     \t", Allocs[i].Src, Allocs[i].Line));
#endif
#ifdef MY_HEAP_TRACK_REF
    KdPrint(("%s%s", Used ? " " : "-", Allocs[i].Tag ? Allocs[i].Tag : ""));
#endif
    KdPrint(("\n"));
}

//#define CHECK_ALLOC_FRAMES

#define DUMP_MEM_FRAMES

#ifdef DUMP_MEM_FRAMES
ULONG MyDumpMem = FALSE;
#endif //DUMP_MEM_FRAMES

#define DUMP_MEM_FRAMES2

//#ifdef CHECK_ALLOC_FRAMES
VOID
MyAllocDumpFrame(
    ULONG Frame
    )
{
    ULONG i;
    PMEM_ALLOC_DESC Allocs;
    Allocs = FrameList[Frame].Frame;
    ULONG k=0;
    BOOLEAN Used;
#ifdef DUMP_MEM_FRAMES
    if(!MyDumpMem)
#endif //DUMP_MEM_FRAMES
        return;

    KdPrint(("Dumping frame %x\n",Frame));
    KdPrint(("FirstFree %x   LastUsed %x  ", FrameList[Frame].FirstFree, FrameList[Frame].LastUsed));
    KdPrint(("Type %x\n", FrameList[Frame].Type));
    if(Allocs) {
        for(i=0;i< (MY_HEAP_MAX_BLOCKS/*-1*/);i++) {
            Used = (Allocs[i].Len & MY_HEAP_FLAG_USED) ? TRUE : FALSE;
            KdPrint(("block %x \t%s addr %x len %x  \t", i, Used ? "used" : "free", Allocs[i].Addr, (Allocs[i].Len) & MY_HEAP_FLAG_LEN_MASK));
#ifdef MY_HEAP_TRACK_OWNERS
            KdPrint(("src %x   \t line %d     \t", Allocs[i].Src, Allocs[i].Line));
#endif
#ifdef MY_HEAP_TRACK_REF
            KdPrint(("%s%s", Used ? " " : "-", Allocs[i].Tag ? Allocs[i].Tag : ""));
#endif
            KdPrint(("\n"));
            if(!(Allocs[i].Len) && !(Allocs[i].Addr)) {
                break;
            }
            if(Allocs[i].Len & MY_HEAP_FLAG_USED)
                k += ((Allocs[i].Len) & MY_HEAP_FLAG_LEN_MASK);
        }
    }
    KdPrint(("    Wasted %x bytes from %x\n", MY_HEAP_FRAME_SIZE - k, MY_HEAP_FRAME_SIZE));
} // end MyAllocDumpFrame()

VOID
MyAllocDumpFrames(
    VOID
    )
{
    ULONG i;

    for(i=0;i<MY_HEAP_MAX_FRAMES; i++) {
        if(FrameList[i].Frame) {
            MyAllocDumpFrame(i);
        }
    }

    KdPrint(("\n"));

    for(i=0;i<MY_HEAP_MAX_FRAMES; i++) {
        if(FrameList[i].Frame) {
            KdPrint(("Addr %x   ", FrameList[i].Frame));
            KdPrint(("Type %x\n" , FrameList[i].Type));
        }
    }

} // end MyAllocDumpFrame()

VOID
MyAllocCheck(
    ULONG Frame
    )
{
    ULONG i, j;
    PMEM_ALLOC_DESC Allocs;
    Allocs = FrameList[Frame].Frame;
    ULONG len, addr;

    for(i=0;i< (MY_HEAP_MAX_BLOCKS-1);i++) {
        len = (Allocs[i].Len & MY_HEAP_FLAG_LEN_MASK);
        addr = Allocs[i].Addr;
        if( len != (Allocs[i+1].Addr - addr) ) {
            if(Allocs[i+1].Addr) {
                KdPrint(("ERROR! Memory block aliasing\n"));
                KdPrint(("block %x, frame %x\n", i, Frame));
                KdPrint(("block descriptor %x\n", &(Allocs[i]) ));
                BrutePoint();
                MyAllocDumpFrame(Frame);
            }
        }
#ifdef MY_HEAP_CHECK_BOUNDS
        if(*((PULONG)(addr+len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) != 0xBAADF00D) {
            MyAllocDumpDescr(Allocs, i);
        }
#endif //MY_HEAP_CHECK_BOUNDS
    }
} // end MyAllocCheck()

//#endif //CHECK_ALLOC_FRAMES
#else

#define MyAllocDumpFrame(a) {}
#define MyAllocCheck(a) {}
#define MyAllocDumpFrames() {}

#endif // UDF_DBG

PCHAR
#ifndef MY_HEAP_TRACK_OWNERS
__fastcall
#endif
MyAllocatePoolInFrame(
    ULONG Frame,
    ULONG size
#ifdef MY_HEAP_TRACK_OWNERS
   ,USHORT Src,
    USHORT Line
#endif
#ifdef MY_HEAP_TRACK_REF
   ,PCHAR Tag
#endif //MY_HEAP_TRACK_REF
    )
{
    ULONG addr;
    ULONG i;
    ULONG min_len;
    ULONG best_i;
    PMEM_ALLOC_DESC Allocs;
    PMEM_ALLOC_DESC Allocs0;
    ULONG LastUsed, FirstFree;
    ULONG l;

#ifdef CHECK_ALLOC_FRAMES
    MyAllocCheck(Frame);
#endif

    if(!size) return NULL;
#ifdef MY_HEAP_CHECK_BOUNDS
    size+=MY_HEAP_CHECK_BOUNDS_BSZ;
#endif

/*    if(size == 0x70) {
        BrutePoint();
    }*/
    // lock frame
    Allocs0 = FrameList[Frame].Frame;
    if(!Allocs0) return NULL;
    best_i = MY_HEAP_MAX_BLOCKS;
    min_len = 0;
    LastUsed = FrameList[Frame].LastUsed;
    FirstFree = FrameList[Frame].FirstFree;

    if(LastUsed >= (MY_HEAP_MAX_BLOCKS-1))
        return NULL;

    for(i=FirstFree, Allocs = &(Allocs0[i]);i<=LastUsed;i++, Allocs++) {
        if( !((l = Allocs->Len) & MY_HEAP_FLAG_USED) &&
             ((l &= MY_HEAP_FLAG_LEN_MASK) >= size) ) {
            // check if minimal
            // check for first occurence
            if(l < min_len || !min_len) {
                min_len = l;
                best_i = i;
            }
            if(l == size)
                break;
        }
    }
    // not enough resources
    if(best_i >= MY_HEAP_MAX_BLOCKS) return NULL;
    // mark as used
    Allocs = Allocs0+best_i;
    addr = Allocs->Addr;
    // create entry for unallocated tail
    if(Allocs->Len != size) {     // this element is always FREE
        if(Allocs[1].Len) {
            if(Allocs0[MY_HEAP_MAX_BLOCKS-1].Len) return NULL;
/*            for(i=MY_HEAP_MAX_BLOCKS-1;i>best_i;i--) {
                Allocs[i] = Allocs[i-1];
            }*/
            RtlMoveMemory(&(Allocs[1]), &(Allocs[0]), (LastUsed-best_i+1)*sizeof(MEM_ALLOC_DESC));
        }
        Allocs[1].Addr = Allocs->Addr + size;
        if(Allocs[1].Len) {
            Allocs[1].Len -= size;
        } else {
            Allocs[1].Len = MY_HEAP_FRAME_SIZE - (addr - Allocs0[0].Addr) - size;
        }
//        Allocs[best_i+1].Used = FALSE;   // this had been done by prev. ops.
        FrameList[Frame].LastUsed++;
    }
    // update FirstFree pointer
    if(FirstFree == best_i) {
        for(i=best_i+1, Allocs++; (i<=LastUsed) && (Allocs->Len & MY_HEAP_FLAG_USED);i++, Allocs++) {
            // do nothing but scan
        }
        FrameList[Frame].FirstFree = i;
        Allocs = Allocs0+best_i;
    }
    Allocs->Len = size | MY_HEAP_FLAG_USED;
#ifdef MY_HEAP_TRACK_OWNERS
    Allocs->Src = Src;
    Allocs->Line = Line;
#endif
#ifdef MY_HEAP_TRACK_REF
    Allocs->Tag = Tag;
#endif //MY_HEAP_TRACK_REF

//    KdPrint(( "Mem: Allocated %x at addr %x\n", size, (ULONG)addr ));
    // this will set IntegrityTag to zero
    *((PULONG)addr) = 0x00000000;
#ifdef MY_HEAP_CHECK_BOUNDS
    for(i=0; i<MY_HEAP_CHECK_BOUNDS_SZ; i++) {
        *((PULONG)(addr+size+(i*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) = 0xBAADF00D;
    }
#endif //MY_HEAP_CHECK_BOUNDS

#ifdef UDF_DBG
    MemTotalAllocated += size;
#endif
    return (PCHAR)addr;
} // end MyAllocatePoolInFrame()

LONG
__fastcall
MyFindMemDescByAddr(
    ULONG Frame,
    PCHAR addr
    )
{
    ULONG i;
    ULONG left;
    ULONG right;
    PMEM_ALLOC_DESC Allocs;

    Allocs = FrameList[Frame].Frame;
//    i = FrameList[Frame].LastUsed >> 1;
//    KdPrint(("Mem: Freeing %x\n", (ULONG)addr)); DEADDA7A
//    for(i=0;i<MY_HEAP_MAX_BLOCKS;i++) {
    left = 0;
    right = FrameList[Frame].LastUsed;
    if(!right && FrameList[Frame].FirstFree)
        right = 1;
    while(left != right) {
        i = (right + left) >> 1;
        if( (Allocs[i].Len & MY_HEAP_FLAG_USED) && (Allocs[i].Addr == (ULONG)addr) ) {
FIF_Found:
            return i;
        }
        if(right - left == 1) {
            if( (Allocs[i+1].Len & MY_HEAP_FLAG_USED) && (Allocs[i+1].Addr == (ULONG)addr) ) {
                i++;
                goto FIF_Found;
            }
            break;
        }
        if(Allocs[i].Addr && (Allocs[i].Addr < (ULONG)addr)) {
            left = i;
        } else {
            right = i;
        }
    }
    return -1;
} // end MyFindMemDescByAddr()

VOID
__fastcall
MyFreePoolInFrame(
    ULONG Frame,
    PCHAR addr
    )
{
    LONG i, j;
    ULONG pc;
    ULONG len, len2;
    PMEM_ALLOC_DESC Allocs;

    Allocs = FrameList[Frame].Frame;
    pc = 0;
    i = MyFindMemDescByAddr(Frame, addr);
    if(i < 0) {
        KdPrint(("Mem: <<<*** WARNING ***>>> Double deallocation at %x !!!   ;( \n", addr));
        MyAllocDumpFrame(Frame);
        BrutePoint();
        return;
    }
    Allocs[i].Len &= ~MY_HEAP_FLAG_USED;
    len = Allocs[i].Len;  // USED bit is already cleared

#ifdef MY_HEAP_CHECK_BOUNDS
    for(j=0; j<MY_HEAP_CHECK_BOUNDS_SZ; j++) {
        ASSERT(*((PULONG)(addr+len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) == 0xBAADF00D);
        if(*((PULONG)(addr+len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) != 0xBAADF00D) {
            MyAllocDumpDescr(Allocs, i);
        }
    }
#endif //MY_HEAP_CHECK_BOUNDS

#ifdef UDF_DBG
    // this is a marker of deallocated blocks
    // some structures have DWORD IntegrityTag as a first member
    // so, if IntegrityTag is equal to 0xDEADDA7A we shall return
    // a <<<*** BIG ERROR MESSAGE ***>>> when somebody try to use it
    *((PULONG)addr) = 0xDEADDA7A;
    MemTotalAllocated -= len;
#endif
    if((i<MY_HEAP_MAX_BLOCKS-1) && !((len2 = Allocs[i+1].Len) & MY_HEAP_FLAG_USED)) {
        // pack up
        if((len2 &= MY_HEAP_FLAG_LEN_MASK)) {
            len += len2;
        } else {
            len = MY_HEAP_FRAME_SIZE - (Allocs[i].Addr - Allocs[0].Addr);
        }
        pc++;
    }
    if((i>0) && !((len2 = Allocs[i-1].Len) & MY_HEAP_FLAG_USED)) {
        // pack down
        len += (len2 & MY_HEAP_FLAG_LEN_MASK);
        pc++;
        i--;
    }
    if(pc) {
        // pack

        Allocs[i+pc].Addr = Allocs[i].Addr;
        Allocs[i+pc].Len = len;
/*                for(;i<MY_HEAP_MAX_BLOCKS-pc;i++) {
            Allocs[i] = Allocs[i+pc];
        }*/
        RtlMoveMemory(&(Allocs[i]), &(Allocs[i+pc]), (MY_HEAP_MAX_BLOCKS-pc-i)*sizeof(MEM_ALLOC_DESC) );
/*                for(i=MY_HEAP_MAX_BLOCKS-pc;i<MY_HEAP_MAX_BLOCKS;i++) {
            Allocs[i].Addr =
            Allocs[i].Len =
            Allocs[i].Used = 0;
        }*/
        RtlZeroMemory(&(Allocs[MY_HEAP_MAX_BLOCKS-pc]), pc*sizeof(MEM_ALLOC_DESC));
    }
    if(FrameList[Frame].FirstFree > (ULONG)i)
        FrameList[Frame].FirstFree = (ULONG)i;
    //ASSERT(FrameList[Frame].LastUsed >= pc);
    if(FrameList[Frame].LastUsed < pc) {
        FrameList[Frame].LastUsed = 0;
    } else {
        FrameList[Frame].LastUsed -= pc;
    }
    return;
} // end MyFreePoolInFrame()

BOOLEAN
__fastcall
MyResizePoolInFrame(
    ULONG Frame,
    PCHAR addr,
    ULONG new_len
#ifdef MY_HEAP_TRACK_REF
   ,PCHAR* Tag
#endif //MY_HEAP_TRACK_REF
    )
{
    LONG i, j;
    ULONG len, len2;
    PMEM_ALLOC_DESC Allocs;

    if(FrameList[Frame].LastUsed >= (MY_HEAP_MAX_BLOCKS-1))
        return FALSE;
    Allocs = FrameList[Frame].Frame;
    i = MyFindMemDescByAddr(Frame, addr);
    if(i < 0) {
        KdPrint(("Mem: <<<*** WARNING ***>>> Double deallocation at %x !!!   ;( \n", addr));
        MyAllocDumpFrame(Frame);
        BrutePoint();
        return FALSE;
    }
    if(i>=(MY_HEAP_MAX_BLOCKS-2))
        return FALSE;

#ifdef MY_HEAP_TRACK_REF
    *Tag = Allocs[i].Tag;
#endif //MY_HEAP_TRACK_REF

    len = (Allocs[i].Len & MY_HEAP_FLAG_LEN_MASK);

#ifdef MY_HEAP_CHECK_BOUNDS
    new_len += MY_HEAP_CHECK_BOUNDS_BSZ;
    for(j=0; j<MY_HEAP_CHECK_BOUNDS_SZ; j++) {
        ASSERT(*((PULONG)(addr+len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) == 0xBAADF00D);
        if(*((PULONG)(addr+len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) != 0xBAADF00D) {
            MyAllocDumpDescr(Allocs, i);
        }
    }
#endif //MY_HEAP_CHECK_BOUNDS
    
    if(new_len > len ) {
        if(Allocs[i+1].Len & MY_HEAP_FLAG_USED)
            return FALSE;
        if(len + (Allocs[i+1].Len & MY_HEAP_FLAG_LEN_MASK) < new_len)
            return FALSE;
        Allocs[i].Len += (len2 = (new_len - len));
        Allocs[i+1].Len -= len2;
        Allocs[i+1].Addr += len2;

#ifdef MY_HEAP_CHECK_BOUNDS
        for(j=0; j<MY_HEAP_CHECK_BOUNDS_SZ; j++) {
            *((PULONG)(addr+new_len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) = 0xBAADF00D;
        }
#endif //MY_HEAP_CHECK_BOUNDS
        
        if(!Allocs[i+1].Len) {
            i++;
            RtlMoveMemory(&(Allocs[i]), &(Allocs[i+1]), (MY_HEAP_MAX_BLOCKS-1-i)*sizeof(MEM_ALLOC_DESC) );
            RtlZeroMemory(&(Allocs[MY_HEAP_MAX_BLOCKS-1]), sizeof(MEM_ALLOC_DESC));
            if((ULONG)i<FrameList[Frame].LastUsed)
                FrameList[Frame].LastUsed--;
            if(FrameList[Frame].FirstFree == (ULONG)i) {
                for(;i<MY_HEAP_MAX_BLOCKS;i++) {
                    if(!(Allocs[i].Len & MY_HEAP_FLAG_USED))
                        break;
                }
                FrameList[Frame].FirstFree = i;
            }
        }
#ifdef UDF_DBG
        MemTotalAllocated += len;
#endif
    } else {

        len2 = len - new_len;
        if(!len2) return TRUE;

#ifdef MY_HEAP_CHECK_BOUNDS
        for(j=0; j<MY_HEAP_CHECK_BOUNDS_SZ; j++) {
            *((PULONG)(addr+new_len+(j*sizeof(ULONG))-MY_HEAP_CHECK_BOUNDS_BSZ)) = 0xBAADF00D;
        }
#endif //MY_HEAP_CHECK_BOUNDS

        Allocs[i].Len -= len2;
        if(Allocs[i+1].Len & MY_HEAP_FLAG_USED) {
            i++;
            RtlMoveMemory(&(Allocs[i+1]), &(Allocs[i]), (MY_HEAP_MAX_BLOCKS-i-1)*sizeof(MEM_ALLOC_DESC) );

            Allocs[i].Len = len2;
            Allocs[i].Addr = Allocs[i-1].Addr + new_len;

            if(FrameList[Frame].FirstFree > (ULONG)i)
                FrameList[Frame].FirstFree = i;
            FrameList[Frame].LastUsed++;

        } else {
            Allocs[i+1].Len += len2;
            Allocs[i+1].Addr -= len2;
        }
#ifdef UDF_DBG
        MemTotalAllocated -= len2;
#endif
    }

    return TRUE;
} // end MyResizePoolInFrame()

VOID
__fastcall
MyAllocInitFrame(
    ULONG Type,
    ULONG Frame
    )
{
    PMEM_ALLOC_DESC Allocs;

    Allocs = (PMEM_ALLOC_DESC)DbgAllocatePool(NonPagedPool, sizeof(MEM_ALLOC_DESC)*(MY_HEAP_MAX_BLOCKS+1));
    if(!Allocs) {
        KdPrint(("Insufficient resources to allocate frame descriptor\n"));
        FrameList[Frame].Frame = NULL;
        MyAllocDumpFrames();
        BrutePoint();
        return;
    }
    RtlZeroMemory(Allocs, sizeof(MEM_ALLOC_DESC)*(MY_HEAP_MAX_BLOCKS+1));
    // alloc heap
    Allocs[0].Addr = (ULONG)DbgAllocatePool((POOL_TYPE)Type, MY_HEAP_FRAME_SIZE);
    if(!Allocs[0].Addr) {
        KdPrint(("Insufficient resources to allocate frame\n"));
        DbgFreePool(Allocs);
        FrameList[Frame].Frame = NULL;
        MyAllocDumpFrames();
        BrutePoint();
        return;
    }
    Allocs[0].Len = MY_HEAP_FRAME_SIZE;
//    Allocs[0].Used = FALSE;
    FrameList[Frame].Frame = Allocs;
    FrameList[Frame].LastUsed =
    FrameList[Frame].FirstFree = 0;
    FrameList[Frame].Type = Type;
    FrameCount++;
    if(LastFrame < Frame)
        LastFrame = Frame;
} // end MyAllocInitFrame()

VOID
__fastcall
MyAllocFreeFrame(
    ULONG Frame
    )
{
    // check if already deinitialized
    if(!FrameList[Frame].Frame) {
        BrutePoint();
        return;
    }
    DbgFreePool((PVOID)(FrameList[Frame].Frame)[0].Addr);
    DbgFreePool((PVOID)(FrameList[Frame].Frame));
    FrameList[Frame].Frame = NULL;
    FrameCount--;
    if(LastFrame == Frame) {
        LONG i;
        for(i=LastFrame; i>0; i--) {
            if(FrameList[i].Frame)
                break;
        }
        LastFrame = i;
    }
} // end MyAllocFreeFrame()

PCHAR
#ifndef MY_HEAP_TRACK_OWNERS
__fastcall
#endif
MyAllocatePool(
    ULONG type,
    ULONG size
#ifdef MY_HEAP_TRACK_OWNERS
   ,USHORT Src,
    USHORT Line
#endif
#ifdef MY_HEAP_TRACK_REF
   ,PCHAR Tag
#endif //MY_HEAP_TRACK_REF
    )
{
    ULONG i;
    ULONG addr;

//    KdPrint(("MemFrames: %x\n",FrameCount));

    if(!size || (size > MY_HEAP_FRAME_SIZE)) return NULL;

#ifdef DUMP_MEM_FRAMES2
    if(MyDumpMem)
        MyAllocDumpFrames();
#endif

    LockMemoryManager();
    for(i=0;i<MY_HEAP_MAX_FRAMES; i++) {
        if( FrameList[i].Frame &&
           (FrameList[i].Type == type) &&
           (addr = (ULONG)MyAllocatePoolInFrame(i,size
#ifdef MY_HEAP_TRACK_OWNERS
                                                      ,Src,Line
#endif
#ifdef MY_HEAP_TRACK_REF
                                                               ,Tag
#endif //MY_HEAP_TRACK_REF
                                                                )) ) {

#ifdef UDF_DBG
//            if(addr >= (ULONG)BreakAddr && addr < sizeof(UDF_FILE_INFO) + (ULONG)BreakAddr) {
//            if(addr<=(ULONG)BreakAddr && addr+sizeof(UDF_FILE_INFO) > (ULONG)BreakAddr) {
//                KdPrint(("ERROR !!! Allocating in examined block\n"));
//                KdPrint(("addr %x\n", addr));
//                MyAllocDumpFrame(i);
//                BrutePoint();
//            }
#endif //UDF_DBG

            UnlockMemoryManager();
            DbgTouch((PVOID)addr);
            return (PCHAR)addr;
        }
    }
#ifdef DUMP_MEM_FRAMES2
    MyAllocDumpFrames();
#endif
    addr = 0;
    for(i=0;i<MY_HEAP_MAX_FRAMES; i++) {
//        MyAllocDumpFrame(i);
        if(!(FrameList[i].Frame)) {
            MyAllocInitFrame(type, i);
            if(FrameList[i].Frame &&
               (addr = (ULONG)MyAllocatePoolInFrame(i,size
#ifdef MY_HEAP_TRACK_OWNERS
                                                           ,Src,Line
#endif
#ifdef MY_HEAP_TRACK_REF
                                                                    ,Tag
#endif //MY_HEAP_TRACK_REF
                                                                     )) ) {

#ifdef UDF_DBG
//                if(addr >= (ULONG)BreakAddr && addr < sizeof(UDF_FILE_INFO) + (ULONG)BreakAddr) {
//                if(addr<=(ULONG)BreakAddr && addr+sizeof(UDF_FILE_INFO) > (ULONG)BreakAddr) {
//                    KdPrint(("ERROR !!! Allocating in examined block\n"));
//                    KdPrint(("addr %x\n", addr));
//                    MyAllocDumpFrame(i);
//                    BrutePoint();
//                }
//            } else {
//                addr = 0;
#endif //UDF_DBG
            }
#ifdef DUMP_MEM_FRAMES2
            MyAllocDumpFrames();
#endif
            break;
        }
    }
    UnlockMemoryManager();
    return (PCHAR)addr;
} // end MyAllocatePool()

LONG
__fastcall
MyFindFrameByAddr(
    PCHAR addr
    )
{
    ULONG i;
//    ULONG j;
    PMEM_ALLOC_DESC Allocs;

    for(i=0;i<=LastFrame; i++) {
        if( (Allocs = FrameList[i].Frame) &&
            (Allocs[0].Addr <= (ULONG)addr) &&
            (Allocs[0].Addr + MY_HEAP_FRAME_SIZE > (ULONG)addr) ) {
            return i;
        }
    }
    return -1;
}

VOID
__fastcall
MyFreePool(
    PCHAR addr
    )
{
    LONG i;

//    KdPrint(("MemFrames: %x\n",FrameCount));

    LockMemoryManager();
    i = MyFindFrameByAddr(addr);
    if(i < 0) {
        UnlockMemoryManager();
        KdPrint(("Mem: <<<*** WARNING ***>>> Double deallocation at %x !!!   ;( \n", addr));
        BrutePoint();
        return;
    }

#ifdef UDF_DBG
            // BreakAddr <= addr < BreakAddr + sizeof(UDF_FILE_INFO)
//            if((ULONG)addr >= (ULONG)BreakAddr && (ULONG)addr < sizeof(UDF_FILE_INFO) + (ULONG)BreakAddr) {
//                KdPrint(("Deallocating in examined block\n"));
//                KdPrint(("addr %x\n", addr));
//                MyAllocDumpFrame(i);
//                BrutePoint();
//                BreakAddr = NULL;
//            }
#endif //UDF_DBG

    MyFreePoolInFrame(i,addr);
/*            for(j=0;j<MY_HEAP_MAX_BLOCKS; j++) {
        if((Allocs[j].Len & MY_HEAP_FLAG_USED) || (FrameCount<=1)) {
            return;
        }
    }*/
    if(MyAllocIsFrameFree(FrameList, i)) {
        MyAllocFreeFrame(i);
    }
    UnlockMemoryManager();
    return;
} // end MyFreePool()

ULONG
#ifndef MY_HEAP_TRACK_OWNERS
__fastcall
#endif
MyReallocPool(
    IN PCHAR addr,
    IN ULONG OldLength,
    OUT PCHAR* NewBuff,
    IN ULONG NewLength
#ifdef MY_HEAP_TRACK_OWNERS
   ,USHORT Src,
    USHORT Line
#endif
    )
{
    ULONG i;
    PCHAR new_buff;
#ifdef MY_HEAP_TRACK_REF
    PCHAR Tag;
#endif

//    KdPrint(("MemFrames: %x\n",FrameCount));
    (*NewBuff) = addr;
    if(OldLength == NewLength) return OldLength;

    if(!NewLength) {
        BrutePoint();
        return 0;
    }

    LockMemoryManager();
    i = MyFindFrameByAddr(addr);
    if(i < 0) {
        UnlockMemoryManager();
        KdPrint(("Mem: <<<*** WARNING ***>>> Double deallocation at %x !!!   ;( \n", addr));
        BrutePoint();
        return 0;
    }

    if(MyResizePoolInFrame(i,addr,NewLength
#ifdef MY_HEAP_TRACK_REF
                                           , &Tag
#endif
                                                  )) {
#ifdef CHECK_ALLOC_FRAMES
MyAllocCheck(i);
#endif

        (*NewBuff) = addr;
        DbgTouch((PVOID)addr);
        UnlockMemoryManager();
        return NewLength;
    }

    new_buff = MyAllocatePool(FrameList[i].Type, MyAlignSize__(NewLength)
#ifdef MY_HEAP_TRACK_OWNERS
                                                                         ,Src,Line
#endif
#ifdef MY_HEAP_TRACK_REF
                                                                                  ,Tag
#endif //MY_HEAP_TRACK_REF
                                                                                   );
    if(!new_buff) {
        UnlockMemoryManager();
        return 0;
    }

    if(OldLength > NewLength) OldLength = NewLength;
    RtlCopyMemory(new_buff, addr, OldLength);

    MyFreePoolInFrame(i,addr);

    if(MyAllocIsFrameFree(FrameList, i)) {
        MyAllocFreeFrame(i);
    }
    UnlockMemoryManager();

    DbgTouch((PVOID)new_buff);
    (*NewBuff) = new_buff;
    return OldLength;

} // end MyReallocPool()

#ifdef UDF_DBG
LONG
MyFindMemDescByRangeInFrame(
    ULONG Frame,
    PCHAR addr
    )
{
    ULONG i;
    ULONG left;
    ULONG right;
    PMEM_ALLOC_DESC Allocs;
    ULONG curaddr;
    ULONG curlen;

    Allocs = FrameList[Frame].Frame;
//    i = FrameList[Frame].LastUsed >> 1;
//    KdPrint(("Mem: Freeing %x\n", (ULONG)addr)); DEADDA7A
//    for(i=0;i<MY_HEAP_MAX_BLOCKS;i++) {
    left = 0;
    right = FrameList[Frame].LastUsed;
    if(!right && FrameList[Frame].FirstFree)
        right = 1;
    while(left != right) {
        i = (right + left) >> 1;
        curaddr = Allocs[i].Addr;
        curlen = Allocs[i].Len;
        if( (curlen & MY_HEAP_FLAG_USED) &&
            (curaddr <= (ULONG)addr) &&
            ((curaddr+(curlen & MY_HEAP_FLAG_LEN_MASK)) > (ULONG)addr) ) {
FIF_Found:
            return i;
        }
        if(right - left == 1) {
            if( (Allocs[i+1].Len & MY_HEAP_FLAG_USED) && (Allocs[i+1].Addr == (ULONG)addr) ) {
                i++;
                goto FIF_Found;
            }
            break;
        }
        if(Allocs[i].Addr && (Allocs[i].Addr < (ULONG)addr)) {
            left = i;
        } else {
            right = i;
        }
    }
    return -1;
} // end MyFindMemDescByRangeInFrame()

LONG
MyFindMemBaseByAddr(
    PCHAR addr
    )
{
    ULONG Frame, Base, i;

    LockMemoryManager();
    Frame = MyFindFrameByAddr(addr);
    if(Frame < 0) {
        UnlockMemoryManager();
        KdPrint(("Mem: <<<*** WARNING ***>>> Unknown base for %x !!!   ;( \n", addr));
        BrutePoint();
        return -1;
    }
    i = MyFindMemDescByRangeInFrame(Frame, addr);
    Base = FrameList[Frame].Frame[i].Addr;
    UnlockMemoryManager();
    return Base;
} // end MyFindMemBaseByAddr()
#endif //UDF_DBG

BOOLEAN
MyAllocInit(VOID)
{
    RtlZeroMemory(&FrameList, sizeof(FrameList));
    if(!OS_SUCCESS(InitLockMemoryManager())) {
       return FALSE;
    }
    MyAllocInitFrame(NonPagedPool, 0);
    LastFrame = 0;
    return (MyMemInitialized = TRUE);
} // end MyAllocInit()

VOID
MyAllocRelease(VOID)
{
    ULONG i;
    PMEM_ALLOC_DESC Allocs;

    if(!MyMemInitialized)
        return;
    LockMemoryManager();
    for(i=0;i<MY_HEAP_MAX_FRAMES; i++) {
        if(Allocs = FrameList[i].Frame) {
            MyAllocFreeFrame(i);
        }
    }
    RtlZeroMemory(&FrameList, sizeof(FrameList));
    UnlockMemoryManager();
    DeinitLockMemoryManager();
    MyMemInitialized = FALSE;
} // end MyAllocRelease()

#endif //MY_USE_INTERNAL_MEMMANAGER
