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

#ifdef UDF_DBG

//#define TRACK_SYS_ALLOCS
//#define TRACK_SYS_ALLOC_CALLERS

#endif //UDF_DBG

//#define USE_KD_PRINT
//#define USE_MM_PRINT
//#define USE_AD_PRINT
//#define UDF_DUMP_EXTENT
//#define USE_TH_PRINT
//#define USE_TIME_PRINT

//======================================

#if defined UDF_DBG

  ULONG
  _cdecl
  DbgPrint(
      const char* Format,
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
#define DbgFreePool(x) ExFreePoolWithTag(x, 0)

#endif //TRACK_SYS_ALLOCS

#define DbgMoveMemory(d, s, l)     RtlMoveMemory(d, s, l)
#define DbgCopyMemory(d, s, l)     RtlCopyMemory(d, s, l)
#define DbgCompareMemory(d, s, l)  RtlCompareMemory(d, s, l)

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

#else // UDF_DBG

#define DbgAllocatePool(x,y) ExAllocatePoolWithTag(x,y,'Fnwd')
#define DbgFreePool(x) ExFreePoolWithTag(x, 0)
#define DbgAllocatePoolWithTag(a,b,c) ExAllocatePoolWithTag(a,b,c)

#define DbgMoveMemory(d, s, l)     RtlMoveMemory(d, s, l)
#define DbgCopyMemory(d, s, l)     RtlCopyMemory(d, s, l)
#define DbgCompareMemory(d, s, l)  RtlCompareMemory(d, s, l)

#define ASSERT_REF(_a_) {NOTHING;}

#define UDFBreakPoint() {}
#define BrutePoint() {}
#define ValidateFileInfo(fi)  {}

#endif // UDF_DBG

#if defined UDF_DBG || defined PRINT_ALWAYS

#define KdDump(a,b)                         \
if ((a)!=NULL) {                            \
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
