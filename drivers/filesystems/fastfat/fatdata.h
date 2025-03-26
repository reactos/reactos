/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FatData.c

Abstract:

    This module declares the global data used by the Fat file system.


--*/

#ifndef _FATDATA_
#define _FATDATA_

//
//  The global fsd data record, and a global zero large integer
//

extern FAT_DATA FatData;

extern IO_STATUS_BLOCK FatGarbageIosb;

extern NPAGED_LOOKASIDE_LIST FatIrpContextLookasideList;
extern NPAGED_LOOKASIDE_LIST FatNonPagedFcbLookasideList;
extern NPAGED_LOOKASIDE_LIST FatEResourceLookasideList;

extern SLIST_HEADER FatCloseContextSList;
extern FAST_MUTEX FatCloseQueueMutex;

extern PDEVICE_OBJECT FatDiskFileSystemDeviceObject;
extern PDEVICE_OBJECT FatCdromFileSystemDeviceObject;

extern LARGE_INTEGER FatLargeZero;
extern LARGE_INTEGER FatMaxLarge;
extern LARGE_INTEGER Fat30Milliseconds;
extern LARGE_INTEGER Fat100Milliseconds;
extern LARGE_INTEGER FatOneSecond;
extern LARGE_INTEGER FatOneDay;
extern LARGE_INTEGER FatJanOne1980;
extern LARGE_INTEGER FatDecThirtyOne1979;

extern FAT_TIME_STAMP FatTimeJanOne1980;

extern LARGE_INTEGER FatMagic10000;
#define FAT_SHIFT10000 13

extern LARGE_INTEGER FatMagic86400000;
#define FAT_SHIFT86400000 26

#define FatConvert100nsToMilliseconds(LARGE_INTEGER) (                      \
    RtlExtendedMagicDivide( (LARGE_INTEGER), FatMagic10000, FAT_SHIFT10000 )\
    )

#define FatConvertMillisecondsToDays(LARGE_INTEGER) (                       \
    RtlExtendedMagicDivide( (LARGE_INTEGER), FatMagic86400000, FAT_SHIFT86400000 ) \
    )

#define FatConvertDaysToMilliseconds(DAYS) (                                \
    Int32x32To64( (DAYS), 86400000 )                                        \
    )

//
//  Reserve MDL for paging file io forward progress.
//

#define FAT_RESERVE_MDL_SIZE    16

#ifndef __REACTOS__
__volatile extern PMDL FatReserveMdl;
#else
extern __volatile PMDL FatReserveMdl;
#endif
extern KEVENT FatReserveEvent;

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH FatFastIoDispatch;

//
//  Global to store disk IO accounting Enabled/Disabled state
//

extern LOGICAL FatDiskAccountingEnabled;

//
// Read ahead amount used for normal data files
//

#define READ_AHEAD_GRANULARITY           (0x10000)

//
//  Define maximum number of parallel Reads or Writes that will be generated
//  per one request.
//

#define FAT_MAX_IO_RUNS_ON_STACK        ((ULONG) 5)

//
//  Define the maximum number of delayed closes.
//

#define FAT_MAX_DELAYED_CLOSES          ((ULONG)16)

extern ULONG FatMaxDelayedCloseCount;

//
//  The maximum chunk size we use when defragmenting files.
//

#define FAT_DEFAULT_DEFRAG_CHUNK_IN_BYTES       (0x10000)

//
// Define constant for time rounding.
//

#define TenMSec (10*1000*10)
#define TwoSeconds (2*1000*1000*10)
#define AlmostTenMSec (TenMSec - 1)
#define AlmostTwoSeconds (TwoSeconds - 1)

// too big #define HighPartPerDay (24*60*60*1000*1000*10 >> 32)

#define HighPartPerDay (52734375 >> 18)

//
//  The global Fat debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001      Error conditions
//      0x00000002      Debug hooks
//      0x00000004      Catch exceptions before completing Irp
//      0x00000008
//
//      0x00000010
//      0x00000020
//      0x00000040
//      0x00000080
//
//      0x00000100
//      0x00000200
//      0x00000400
//      0x00000800
//
//      0x00001000
//      0x00002000
//      0x00004000
//      0x00008000
//
//      0x00010000
//      0x00020000
//      0x00040000
//      0x00080000
//
//      0x00100000
//      0x00200000
//      0x00400000
//      0x00800000
//
//      0x01000000
//      0x02000000
//      0x04000000
//      0x08000000
//
//      0x10000000
//      0x20000000
//      0x40000000
//      0x80000000
//


#ifdef FASTFATDBG

#define DEBUG_TRACE_ERROR                (0x00000001)
#define DEBUG_TRACE_DEBUG_HOOKS          (0x00000002)
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00000004)
#define DEBUG_TRACE_UNWIND               (0x00000008)
#define DEBUG_TRACE_CLEANUP              (0x00000010)
#define DEBUG_TRACE_CLOSE                (0x00000020)
#define DEBUG_TRACE_CREATE               (0x00000040)
#define DEBUG_TRACE_DIRCTRL              (0x00000080)
#define DEBUG_TRACE_EA                   (0x00000100)
#define DEBUG_TRACE_FILEINFO             (0x00000200)
#define DEBUG_TRACE_FSCTRL               (0x00000400)
#define DEBUG_TRACE_LOCKCTRL             (0x00000800)
#define DEBUG_TRACE_READ                 (0x00001000)
#define DEBUG_TRACE_VOLINFO              (0x00002000)
#define DEBUG_TRACE_WRITE                (0x00004000)
#define DEBUG_TRACE_FLUSH                (0x00008000)
#define DEBUG_TRACE_DEVCTRL              (0x00010000)
#define DEBUG_TRACE_SHUTDOWN             (0x00020000)
#define DEBUG_TRACE_FATDATA              (0x00040000)
#define DEBUG_TRACE_PNP                  (0x00080000)
#define DEBUG_TRACE_ACCHKSUP             (0x00100000)
#define DEBUG_TRACE_ALLOCSUP             (0x00200000)
#define DEBUG_TRACE_DIRSUP               (0x00400000)
#define DEBUG_TRACE_FILOBSUP             (0x00800000)
#define DEBUG_TRACE_NAMESUP              (0x01000000)
#define DEBUG_TRACE_VERFYSUP             (0x02000000)
#define DEBUG_TRACE_CACHESUP             (0x04000000)
#define DEBUG_TRACE_SPLAYSUP             (0x08000000)
#define DEBUG_TRACE_DEVIOSUP             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)
#define DEBUG_TRACE_FSP_DISPATCHER       (0x40000000)
#define DEBUG_TRACE_FSP_DUMP             (0x80000000)

extern LONG FatDebugTraceLevel;
extern LONG FatDebugTraceIndent;

#define DebugTrace(INDENT,LEVEL,X,Y) {                      \
    LONG _i;                                                \
    if (((LEVEL) == 0) || (FatDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        if ((INDENT) < 0) {                                 \
            FatDebugTraceIndent += (INDENT);                \
        }                                                   \
        if (FatDebugTraceIndent < 0) {                      \
            FatDebugTraceIndent = 0;                        \
        }                                                   \
        for (_i = 0; _i < FatDebugTraceIndent; _i += 1) {   \
            DbgPrint(" ");                                  \
        }                                                   \
        DbgPrint(X,Y);                                      \
        if ((INDENT) > 0) {                                 \
            FatDebugTraceIndent += (INDENT);                \
        }                                                   \
    }                                                       \
}

#if !defined(__REACTOS__) || defined(_MSC_VER)
#define DebugDump(STR,LEVEL,PTR) {                          \
    __pragma(warning(push))                                 \
    __pragma(warning(disable:4210))                         \
    ULONG _i;                                               \
    VOID FatDump(IN PVOID Ptr);                             \
    if (((LEVEL) == 0) || (FatDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        DbgPrint(STR);                                      \
        if (PTR != NULL) {FatDump(PTR);}                    \
        NT_ASSERT(FALSE);                                   \
    }                                                       \
    __pragma(warning(pop))                                  \
}
#else
#define DebugDump(STR,LEVEL,PTR) {                          \
    ULONG _i;                                               \
    VOID FatDump(IN PVOID Ptr);                             \
    if (((LEVEL) == 0) || (FatDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        DbgPrint(STR);                                      \
        if (PTR != NULL) {FatDump(PTR);}                    \
        NT_ASSERT(FALSE);                                   \
    }                                                       \
}
#endif

#define DebugUnwind(X) {                                                      \
    if (_SEH2_AbnormalTermination()) {                                        \
        DebugTrace(0, DEBUG_TRACE_UNWIND, #X ", Abnormal termination.\n", 0); \
    }                                                                         \
}

//
//  The following variables are used to keep track of the total amount
//  of requests processed by the file system, and the number of requests
//  that end up being processed by the Fsp thread.  The first variable
//  is incremented whenever an Irp context is created (which is always
//  at the start of an Fsd entry point) and the second is incremented
//  by read request.
//

extern ULONG FatFsdEntryCount;
extern ULONG FatFspEntryCount;
extern ULONG FatIoCallDriverCount;
extern ULONG FatTotalTicks[];

#define DebugDoit(X)                     {X;}

extern LONG FatPerformanceTimerLevel;

#define TimerStart(LEVEL) {                     \
    LARGE_INTEGER TStart, TEnd;                 \
    LARGE_INTEGER TElapsed;                     \
    TStart = KeQueryPerformanceCounter( NULL ); \

#define TimerStop(LEVEL,s)                                    \
    TEnd = KeQueryPerformanceCounter( NULL );                 \
    TElapsed.QuadPart = TEnd.QuadPart - TStart.QuadPart;      \
    FatTotalTicks[FatLogOf(LEVEL)] += TElapsed.LowPart;       \
    if (FlagOn( FatPerformanceTimerLevel, (LEVEL))) {         \
        DbgPrint("Time of %s %ld\n", (s), TElapsed.LowPart ); \
    }                                                         \
}

//
//  I need this because C can't support conditional compilation within
//  a macro.
//

extern PVOID FatNull;

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define DebugDoit(X)                     {NOTHING;}

#define TimerStart(LEVEL)
#define TimerStop(LEVEL,s)

#define FatNull NULL

#endif // FASTFATDBG

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just fastfat dbg users
//

#if DBG

#define DbgDoit(X)                       {X;}

#else

#define DbgDoit(X)                       {NOTHING;}

#endif // DBG

#if DBG

extern NTSTATUS FatBreakOnInterestingIoCompletion;
extern NTSTATUS FatBreakOnInterestingExceptionStatus;
extern NTSTATUS FatBreakOnInterestingIrpCompletion;

extern BOOLEAN FatTestRaisedStatus;

#endif

#endif // _FATDATA_


