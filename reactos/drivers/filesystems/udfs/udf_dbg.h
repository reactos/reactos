////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/* 
    Module name: 

   Udf_dbg.h

    Abstract:

   This file contains small set of debug macroses. 
   It is used by the UDF project.

*/

#ifndef _UDF_DEBUG_H_
#define _UDF_DEBUG_H_

//======================================

//#define ALWAYS_CHECK_WAIT_TIMEOUT
//#define PRINT_ALWAYS

#ifdef UDF_DBG

//#define CHECK_ALLOC_FRAMES

//#define TRACK_SYS_ALLOCS
//#define TRACK_SYS_ALLOC_CALLERS

// Use internal deadlock detector
//#define USE_DLD

#endif //UDF_DBG

#define PROTECTED_MEM_RTL

//#define UDF_SIMULATE_WRITES

#define USE_PERF_PRINT

#define USE_KD_PRINT
#define USE_MM_PRINT
#define USE_AD_PRINT
#define UDF_DUMP_EXTENT
//#define USE_TH_PRINT
//#define USE_TIME_PRINT
//#define CHECK_REF_COUNTS

//======================================

#if defined UDF_DBG || defined PRINT_ALWAYS
  
  ULONG
  _cdecl
  DbgPrint(
      PCH Format,
      ...
      );

  
  #ifdef KdPrint
    #undef KdPrint
  #endif

  #ifdef USE_KD_PRINT
    #define KdPrint(_x_) DbgPrint _x_
  #else
    #define KdPrint(a)  {NOTHING;}
  #endif //USE_KD_PRINT

  #ifdef USE_MM_PRINT
    #define MmPrint(_x_) DbgPrint _x_
  #else
    #define MmPrint(_x_) {NOTHING;}
  #endif //USE_MM_PRINT

  #ifdef USE_TIME_PRINT
    extern ULONG UdfTimeStamp;
    #define TmPrint(_x_) {UdfTimeStamp++;KdPrint(("TM:%d: ",UdfTimeStamp));KdPrint(_x_);}
  #else
    #define TmPrint KdPrint
  #endif //USE_MM_PRINT

  #ifdef USE_PERF_PRINT
    #define PerfPrint(_x_) DbgPrint _x_
  #else
    #define PerfPrint(_x_) {NOTHING;}
  #endif //USE_MM_PRINT

  #ifdef USE_AD_PRINT
    #define AdPrint(_x_) {DbgPrint("Thrd:%x:",PsGetCurrentThread());DbgPrint _x_;}
  #else 
    #define AdPrint(_x_) {NOTHING;}
  #endif

  #ifdef USE_TH_PRINT
    #define ThPrint(_x_) {DbgPrint("Thrd:%x:",PsGetCurrentThread());DbgPrint _x_;}
  #else
    #define ThPrint(_x_) {NOTHING;}
  #endif

  #ifdef UDF_DUMP_EXTENT
    #define ExtPrint(_x_)  KdPrint(_x_)
  #else
    #define ExtPrint(_x_)  {NOTHING;}
  #endif

#else // defined UDF_DBG || defined PRINT_ALWAYS

  #define MmPrint(_x_)   {NOTHING;}
  #define TmPrint(_x_)   {NOTHING;}
  #define PerfPrint(_x_) {NOTHING;}
  #define AdPrint(_x_)   {NOTHING;}
  #define ThPrint(_x_)   {NOTHING;}
  #define ExtPrint(_x_)  {NOTHING;}

#endif // defined UDF_DBG || defined PRINT_ALWAYS

NTSTATUS
DbgWaitForSingleObject_(
    IN PVOID Object,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

#if defined ALWAYS_CHECK_WAIT_TIMEOUT
  #define DbgWaitForSingleObject(o, to)   DbgWaitForSingleObject_(o, to)
#else
  #define DbgWaitForSingleObject(o, to)   KeWaitForSingleObject(o, Executive, KernelMode, FALSE, to);
#endif

#ifdef UDF_DBG

#ifdef _X86_
// This is an illegal use of INT3
#define UDFBreakPoint() { __asm int 3 }
#else // _X86_

#define UDFBreakPoint() DbgBreakPoint()
#endif // _X86_

#ifdef BRUTE
#define BrutePoint() UDFBreakPoint()
#else
#define BrutePoint() {}
#endif // BRUTE

#ifdef CHECK_REF_COUNTS
#define ASSERT_REF(_a_) ASSERT(_a_)
#else
#define ASSERT_REF(_a_) {NOTHING;}
#endif //CHECK_REF_COUNTS

#ifdef TRACK_SYS_ALLOCS

PVOID DebugAllocatePool(POOL_TYPE Type,ULONG size
#ifdef TRACK_SYS_ALLOC_CALLERS
, ULONG SrcId, ULONG SrcLine
#endif //TRACK_SYS_ALLOC_CALLERS
);
VOID DebugFreePool(PVOID addr);

#ifdef TRACK_SYS_ALLOC_CALLERS
  #define DbgAllocatePoolWithTag(a,b,c) DebugAllocatePool(a,b,UDF_BUG_CHECK_ID,__LINE__)
  #define DbgAllocatePool(x,y) DebugAllocatePool(x,y,UDF_BUG_CHECK_ID,__LINE__)
#else //TRACK_SYS_ALLOC_CALLERS
  #define DbgAllocatePoolWithTag(a,b,c) DebugAllocatePool(a,b)
  #define DbgAllocatePool(x,y) DebugAllocatePool(x,y)
#endif //TRACK_SYS_ALLOC_CALLERS
#define DbgFreePool(x) DebugFreePool(x)

#else //TRACK_SYS_ALLOCS

#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)
#define DbgAllocatePool(x,y) ExAllocatePoolWithTag(x,y,'Fnwd')
#define DbgFreePool(x) ExFreePool(x)

#endif //TRACK_SYS_ALLOCS


#ifdef PROTECTED_MEM_RTL

#define DbgMoveMemory(d, s, l)   \
_SEH2_TRY {                               \
    RtlMoveMemory(d, s, l);               \
} _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {  \
    BrutePoint();                         \
} _SEH2_END;

#define DbgCopyMemory(d, s, l)   \
_SEH2_TRY {                               \
    RtlCopyMemory(d, s, l);               \
} _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {  \
    BrutePoint();                         \
} _SEH2_END;

__inline
ULONG
DbgCompareMemory(PVOID d, PVOID s, ULONG l) {
    _SEH2_TRY {
        return RtlCompareMemory(d, s, l);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
    return -1;
}

#else //PROTECTED_MEM_RTL

#define DbgMoveMemory(d, s, l)     RtlMoveMemory(d, s, l)
#define DbgCopyMemory(d, s, l)     RtlCopyMemory(d, s, l)
#define DbgCompareMemory(d, s, l)  RtlCompareMemory(d, s, l)

#endif //PROTECTED_MEM_RTL

//#define KdPrint(_x_)

#ifdef VALIDATE_STRUCTURES
#define ValidateFileInfo(fi)            \
{    /* validate FileInfo */            \
    if(!fi || (fi)->IntegrityTag) {            \
        KdPrint(("UDF: ERROR! Using deallocated structure !!!\n"));\
        BrutePoint();                   \
    }                                   \
    if(fi && !fi->Dloc) {               \
        KdPrint(("UDF: ERROR! FI without Dloc !!!\n"));\
        BrutePoint();                   \
    }                                   \
}

#else
#define ValidateFileInfo(fi)  {}
#endif

//#ifdef _X86_
#ifdef _MSC_VER

__inline VOID UDFTouch(IN PVOID addr)
{
    __asm {
        mov  eax,addr
        mov  al,[byte ptr eax]
    }
}

#else   // NO X86 optimization , use generic C/C++

__inline VOID UDFTouch(IN PVOID addr)
{
    UCHAR a = ((PUCHAR)addr)[0];
    a = a;
}

#endif // _X86_

#else // UDF_DBG

#define DbgAllocatePool(x,y) ExAllocatePoolWithTag(x,y,'Fnwd')
#define DbgFreePool(x) ExFreePool(x)
#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)

#define DbgMoveMemory(d, s, l)     RtlMoveMemory(d, s, l)
#define DbgCopyMemory(d, s, l)     RtlCopyMemory(d, s, l)
#define DbgCompareMemory(d, s, l)  RtlCompareMemory(d, s, l)

#define ASSERT_REF(_a_) {NOTHING;}

#define UDFBreakPoint() {}
#define BrutePoint() {}
#define ValidateFileInfo(fi)  {}

#define UDFTouch(addr) {}

#endif // UDF_DBG

#if defined UDF_DBG || defined PRINT_ALWAYS

#define KdDump(a,b)                         \
if((a)!=NULL) {                             \
    ULONG i;                                \
    for(i=0; i<(b); i++) {                  \
        ULONG c;                            \
        c = (ULONG)(*(((PUCHAR)(a))+i));    \
        KdPrint(("%2.2x ",c));              \
        if ((i & 0x0f) == 0x0f) KdPrint(("\n"));   \
    }                                       \
    KdPrint(("\n"));                        \
}

#else

#define KdDump(a,b) {}

#endif // UDF_DBG

#define UserPrint  KdPrint

#endif  // _UDF_DEBUG_H_
