////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __MY_MEM_TOOLS_H__
#define __MY_MEM_TOOLS_H__

#define MY_HEAP_FLAG_USED       0x00000001
#define MY_HEAP_FLAG_LEN_MASK   0xfffffffe

#define MyFreeMemoryAndPointer(ptr) \
    if(ptr) {                   \
        MyFreePool__(ptr);      \
        ptr = NULL;             \
    }

#define MY_USE_ALIGN
//#define MY_MEM_BOUNDS_CHECK

typedef struct _MEM_ALLOC_DESC {
    ULONG Addr;
    ULONG Len;
#ifdef MY_HEAP_TRACK_OWNERS
    USHORT Src;
    USHORT Line;
#endif
#ifdef MY_HEAP_TRACK_REF
//    PCHAR Ref;
    PCHAR Tag;
#endif 
} MEM_ALLOC_DESC, *PMEM_ALLOC_DESC;

typedef struct _MEM_FRAME_ALLOC_DESC {
    PMEM_ALLOC_DESC Frame;
    ULONG LastUsed;
    ULONG FirstFree;
    ULONG Type;
} MEM_FRAME_ALLOC_DESC, *PMEM_FRAME_ALLOC_DESC;

extern PCHAR BreakAddr;
extern ULONG MemTotalAllocated;

#define MY_HEAP_FRAME_SIZE          (256*1024)
#define MY_HEAP_MAX_FRAMES          512
#define MY_HEAP_MAX_BLOCKS          4*1024  // blocks per frame

#ifdef USE_THREAD_HEAPS
//extern HANDLE MemLock;
extern "C" VOID ExInitThreadPools();
extern "C" VOID ExDeInitThreadPools();
extern "C" VOID ExFreeThreadPool();
#endif //USE_THREAD_HEAPS

// Mem
BOOLEAN MyAllocInit(VOID);
VOID MyAllocRelease(VOID);
#ifdef MY_HEAP_TRACK_OWNERS
PCHAR MyAllocatePool(ULONG Type, ULONG size, USHORT Src, USHORT Line
  #ifdef MY_HEAP_TRACK_REF
                    ,PCHAR Tag
  #endif //MY_HEAP_TRACK_REF
      );
ULONG MyReallocPool( PCHAR addr, ULONG OldLength, PCHAR* NewBuff, ULONG NewLength, USHORT Src, USHORT Line);
#else
PCHAR __fastcall MyAllocatePool(ULONG Type, ULONG size
  #ifdef MY_HEAP_TRACK_REF
                    ,PCHAR Tag
  #endif //MY_HEAP_TRACK_REF
      );
ULONG __fastcall MyReallocPool( PCHAR addr, ULONG OldLength, PCHAR* NewBuff, ULONG NewLength);
#endif
VOID __fastcall MyFreePool(PCHAR addr);

#ifdef MY_HEAP_CHECK_BOUNDS
  #define MY_HEAP_ALIGN             63
#else
  #define MY_HEAP_ALIGN             63
#endif
#define PAGE_SIZE_ALIGN             (PAGE_SIZE - 1)

#define AlignToPageSize(size) (((size)+PAGE_SIZE_ALIGN)&(~PAGE_SIZE_ALIGN))
#define MyAlignSize__(size) (((size)+MY_HEAP_ALIGN)&(~MY_HEAP_ALIGN))
#ifdef MY_HEAP_FORCE_NONPAGED
#define MyFixMemType(type)          NonPagedPool
#else //MY_HEAP_FORCE_NONPAGED
#define MyFixMemType(type)          (type)
#endif //MY_HEAP_FORCE_NONPAGED

#ifdef MY_USE_INTERNAL_MEMMANAGER

#ifdef MY_HEAP_TRACK_OWNERS
  #ifdef MY_HEAP_TRACK_REF
    #define MyAllocatePool__(type,size) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__, NULL)
    #define MyAllocatePoolTag__(type,size,tag) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__, (PCHAR)(tag))
  #else
    #define MyAllocatePool__(type,size) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
    #define MyAllocatePoolTag__(type,size,tag) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
  #endif //MY_HEAP_TRACK_REF
#else //MY_HEAP_TRACK_OWNERS
  #ifdef MY_HEAP_TRACK_REF
    #define MyAllocatePool__(type,size) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), NULL)
    #define MyAllocatePoolTag__(type,size,tag) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size), (PCHAR)(tag))
  #else
    #define MyAllocatePool__(type,size) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size))
    #define MyAllocatePoolTag__(type,size,tag) MyAllocatePool(MyFixMemType(type), MyAlignSize__(size))
  #endif //MY_HEAP_TRACK_REF
#endif //MY_HEAP_TRACK_OWNERS

#define MyFreePool__(addr) MyFreePool((PCHAR)(addr))

#ifdef MY_HEAP_TRACK_OWNERS
#define MyReallocPool__(addr, len, pnewaddr, newlen) MyReallocPool((PCHAR)(addr), MyAlignSize__(len), pnewaddr, MyAlignSize__(newlen), UDF_BUG_CHECK_ID, __LINE__)
#else
#define MyReallocPool__(addr, len, pnewaddr, newlen) MyReallocPool((PCHAR)(addr), MyAlignSize__(len), pnewaddr, MyAlignSize__(newlen))
#endif

#ifdef UDF_DBG
LONG
MyFindMemBaseByAddr(
    PCHAR addr
    );

#define MyCheckArray(base, index) \
    ASSERT(MyFindMemBaseByAddr((PCHAR)(base)) == MyFindMemBaseByAddr((PCHAR)(base+(index))))

#else
#define MyCheckArray(base, index)
#endif //UDF_DBG


#else //MY_USE_INTERNAL_MEMMANAGER

#ifndef MY_USE_ALIGN
#undef  MyAlignSize__
#define MyAlignSize__(size) (size)
#endif

BOOLEAN inline MyAllocInit(VOID) {return TRUE;}
#define MyAllocRelease()

#ifndef MY_MEM_BOUNDS_CHECK

  #ifdef TRACK_SYS_ALLOC_CALLERS
    #define MyAllocatePool__(type,size) DebugAllocatePool(NonPagedPool,MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
    #define MyAllocatePoolTag__(type,size,tag) DebugAllocatePool(NonPagedPool,MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
  #else //TRACK_SYS_ALLOC_CALLERS
    #define MyAllocatePool__(type,size) DbgAllocatePoolWithTag(NonPagedPool,MyAlignSize__(size), 'fNWD')
    #define MyAllocatePoolTag__(type,size,tag) DbgAllocatePoolWithTag(NonPagedPool,MyAlignSize__(size), tag)
  #endif //TRACK_SYS_ALLOC_CALLERS
  #define MyFreePool__(addr) DbgFreePool((PCHAR)(addr))

#else //MY_MEM_BOUNDS_CHECK

  #ifdef TRACK_SYS_ALLOC_CALLERS
    #define MyAllocatePool_(type,size) DebugAllocatePool(NonPagedPool,MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
    #define MyAllocatePoolTag_(type,size,tag) DebugAllocatePool(NonPagedPool,MyAlignSize__(size), UDF_BUG_CHECK_ID, __LINE__)
  #else //TRACK_SYS_ALLOC_CALLERS
    #define MyAllocatePool_(type,size) DbgAllocatePoolWithTag(NonPagedPool,MyAlignSize__(size), 'mNWD')
    #define MyAllocatePoolTag_(type,size,tag) DbgAllocatePoolWithTag(NonPagedPool,MyAlignSize__(size), tag)
  #endif //TRACK_SYS_ALLOC_CALLERS
  #define MyFreePool_(addr) DbgFreePool((PCHAR)(addr))

#define MyAllocatePool__(type,size) MyAllocatePoolTag__(type,size,'mNWD')

/*
PVOID inline MyAllocatePool__(ULONG type, ULONG len) {
    PCHAR newaddr;
    ULONG i;
//    newaddr = (PCHAR)MyAllocatePool_(type, len+MY_HEAP_ALIGN+1);
#ifdef TRACK_SYS_ALLOC_CALLERS
    newaddr = (PCHAR)DebugAllocatePool(type,len+MY_HEAP_ALIGN+1, 0x202, __LINE__);
#else //TRACK_SYS_ALLOC_CALLERS
    newaddr = (PCHAR)MyAllocatePool_(type,len+MY_HEAP_ALIGN+1); 
#endif //TRACK_SYS_ALLOC_CALLERS
    if(!newaddr)
        return NULL;
    for(i=0; i<MY_HEAP_ALIGN+1; i++) {
        newaddr[len+i] = (UCHAR)('A'+i);
    }
    return newaddr;
}
*/

PVOID inline MyAllocatePoolTag__(ULONG type, ULONG len, /*PCHAR*/ULONG tag) {
    PCHAR newaddr;
    ULONG i;
//    newaddr = (PCHAR)MyAllocatePoolTag_(type, len+MY_HEAP_ALIGN+1, tag);
#ifdef TRACK_SYS_ALLOC_CALLERS
    newaddr = (PCHAR)DebugAllocatePool(type,len+MY_HEAP_ALIGN+1, 0x202, __LINE__);
#else //TRACK_SYS_ALLOC_CALLERS
    newaddr = (PCHAR)MyAllocatePoolTag_(type,len+MY_HEAP_ALIGN+1, tag); 
#endif //TRACK_SYS_ALLOC_CALLERS
    if(!newaddr)
        return NULL;
    for(i=0; i<MY_HEAP_ALIGN+1; i++) {
        newaddr[len+i] = (UCHAR)('A'+i);
    }
    return newaddr;
}

VOID inline MyFreePool__(PVOID addr) {
    PCHAR newaddr;
//    ULONG i;
    newaddr = (PCHAR)addr;
    if(!newaddr) {
        __asm int 3;
        return;
    }
/*
    for(i=0; i<MY_HEAP_ALIGN+1; i++) {
        if(newaddr[len+i] != (UCHAR)('A'+i)) {
            __asm int 3;
            break;
        }
    }
*/
    MyFreePool_(newaddr);
}

#endif //MY_MEM_BOUNDS_CHECK

ULONG inline MyReallocPool__(PCHAR addr, ULONG len, PCHAR *pnewaddr, ULONG newlen) {
    ULONG _len, _newlen;
    _newlen = MyAlignSize__(newlen);
    _len = MyAlignSize__(len);
    PCHAR newaddr;

    ASSERT(len && newlen);

#ifdef MY_MEM_BOUNDS_CHECK
    ULONG i;

    for(i=0; i<MY_HEAP_ALIGN+1; i++) {
        if((UCHAR)(addr[len+i]) != (UCHAR)('A'+i)) {
            __asm int 3;
            break;
        }
    }
#endif //MY_MEM_BOUNDS_CHECK
    
    if ((_newlen != _len)
#ifdef MY_MEM_BOUNDS_CHECK
        || TRUE
#endif //MY_MEM_BOUNDS_CHECK
    ) { 
#ifdef TRACK_SYS_ALLOC_CALLERS
        newaddr = (PCHAR)DebugAllocatePool(NonPagedPool,_newlen, 0x202, __LINE__);
#else //TRACK_SYS_ALLOC_CALLERS
        newaddr = (PCHAR)MyAllocatePool__(NonPagedPool,_newlen); 
#endif //TRACK_SYS_ALLOC_CALLERS
        if (!newaddr) {
            __debugbreak();
            *pnewaddr = addr;
            return 0;
        }
#ifdef MY_MEM_BOUNDS_CHECK
        for(i=0; i<MY_HEAP_ALIGN+1; i++) {
            newaddr[newlen+i] = (UCHAR)('A'+i);
        }
#endif //MY_MEM_BOUNDS_CHECK
        *pnewaddr = newaddr;
        if(_newlen <= _len) {
            RtlCopyMemory(newaddr, addr, newlen);
        } else {
            RtlCopyMemory(newaddr, addr, len);
            RtlZeroMemory(newaddr+len, newlen - len);
        }
#ifdef MY_MEM_BOUNDS_CHECK
        for(i=0; i<MY_HEAP_ALIGN+1; i++) {
            if((UCHAR)(newaddr[newlen+i]) != (UCHAR)('A'+i)) {
                __asm int 3;
                break;
            }
        }
#endif //MY_MEM_BOUNDS_CHECK

        MyFreePool__(addr); 
    } else {
        *pnewaddr = addr;
    }
    if(newlen > len) {
        //RtlZeroMemory(newaddr+len, newlen - len);
    }
/*
#ifdef MY_MEM_BOUNDS_CHECK
    for(i=0; i<MY_HEAP_ALIGN+1; i++) {
        newaddr[newlen+i] = (UCHAR)('A'+i);
    }
#endif //MY_MEM_BOUNDS_CHECK
*/
    return newlen;
}

#ifndef MY_USE_ALIGN
#undef  MyAlignSize__
#define MyAlignSize__(size) (((size)+MY_HEAP_ALIGN)&(~MY_HEAP_ALIGN))
#endif

#define MyCheckArray(base, index)

#endif // MY_USE_INTERNAL_MEMMANAGER

#endif // __MY_MEM_TOOLS_H__
