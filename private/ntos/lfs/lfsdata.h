/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LfsData.c

Abstract:

    This module declares the global data used by the Log File Service.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#ifndef _LFSDATA_
#define _LFSDATA_

//
//  The global Lfs data record
//

extern LFS_DATA LfsData;

//
//  Various large integer constants.
//

#define LfsMaximumFileSize (0x0000000100000000)

extern LARGE_INTEGER LfsLi0;
extern LARGE_INTEGER LfsLi1;

//
//  The following Lsn is used as a starting point in the file.
//

extern LSN LfsStartingLsn;

//
//  Turn on pseudo-asserts if NTFS_FREE_ASSERTS is defined.
//

#if !DBG
#ifdef NTFS_FREE_ASSERTS
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)        if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s\n",__FILE__,__LINE__,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#define ASSERTMSG(msg,exp) if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#endif
#endif

//
//  The global Lfs debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001      Error conditions
//      0x00000002      Debug hooks
//      0x00000004      Catch exceptions before completing Irp
//      0x00000008      Unwinding during error conditions
//
//      0x00000010      Lfs initialization
//      0x00000020      Lfs query log records
//      0x00000040      Lfs write log records
//      0x00000080      Lfs registry routines
//
//      0x00000100      Lfs worker thread routines
//      0x00000200
//      0x00000400
//      0x00000800
//
//      0x00001000      Log page support routines
//      0x00002000      Lsn support routines
//      0x00004000      Miscellaneous support routines
//      0x00008000      Support routines for cache operations
//
//      0x00010000      Structure support routines
//      0x00020000      Verify/validate support routines
//      0x00040000      Synchronization routines
//      0x00080000      Log buffer support routines
//
//      0x00100000      Support routines for manipulating log records
//      0x00200000      Support routines for manipulation lfs restart areas
//      0x00400000      Support routines for client restart operations
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

#ifdef LFSDBG

#define DEBUG_TRACE_ERROR                (0x00000001)
#define DEBUG_TRACE_DEBUG_HOOKS          (0x00000002)
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00000004)
#define DEBUG_TRACE_UNWIND               (0x00000008)
#define DEBUG_TRACE_INITIALIZATION       (0x00000010)
#define DEBUG_TRACE_QUERY                (0x00000020)
#define DEBUG_TRACE_WRITE                (0x00000040)
#define DEBUG_TRACE_RESTART              (0x00000080)
#define DEBUG_TRACE_REGISTRY             (0x00000100)
#define DEBUG_TRACE_WORKER               (0x00000200)
#define DEBUG_TRACE_0x00000400           (0x00000400)
#define DEBUG_TRACE_0x00000800           (0x00000800)
#define DEBUG_TRACE_LOG_PAGE_SUP         (0x00001000)
#define DEBUG_TRACE_LSN_SUP              (0x00002000)
#define DEBUG_TRACE_MISC_SUP             (0x00004000)
#define DEBUG_TRACE_CACHE_SUP            (0x00008000)
#define DEBUG_TRACE_STRUC_SUP            (0x00010000)
#define DEBUG_TRACE_VERIFY_SUP           (0x00020000)
#define DEBUG_TRACE_SYNCH_SUP            (0x00040000)
#define DEBUG_TRACE_LBCB_SUP             (0x00080000)
#define DEBUG_TRACE_LOG_RECORD_SUP       (0x00100000)
#define DEBUG_TRACE_RESTART_SUP          (0x00200000)
#define DEBUG_TRACE_0x00400000           (0x00400000)
#define DEBUG_TRACE_0x00800000           (0x00800000)
#define DEBUG_TRACE_0x01000000           (0x01000000)
#define DEBUG_TRACE_0x02000000           (0x02000000)
#define DEBUG_TRACE_0x04000000           (0x04000000)
#define DEBUG_TRACE_0x08000000           (0x08000000)
#define DEBUG_TRACE_0x10000000           (0x10000000)
#define DEBUG_TRACE_0x20000000           (0x20000000)
#define DEBUG_TRACE_0x40000000           (0x40000000)
#define DEBUG_TRACE_0x80000000           (0x80000000)

extern LONG LfsDebugTraceLevel;
extern LONG LfsDebugTraceIndent;

#define DebugTrace(INDENT,LEVEL,X,Y) {                      \
    LONG _i;                                                \
    if (((LEVEL) == 0) || (LfsDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        if ((INDENT) < 0) {                                 \
            LfsDebugTraceIndent += (INDENT);                \
        }                                                   \
        if (LfsDebugTraceIndent < 0) {                      \
            LfsDebugTraceIndent = 0;                        \
        }                                                   \
        for (_i = 0; _i < LfsDebugTraceIndent; _i += 1) {   \
            DbgPrint(" ");                                  \
        }                                                   \
        DbgPrint(X,Y);                                      \
        if ((INDENT) > 0) {                                 \
            LfsDebugTraceIndent += (INDENT);                \
        }                                                   \
    }                                                       \
}

#define DebugDump(STR,LEVEL,PTR) {                          \
    ULONG _i;                                               \
    VOID LfsDump();                                         \
    if (((LEVEL) == 0) || (LfsDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        DbgPrint(STR);                                      \
        if (PTR != NULL) {LfsDump(PTR);}                    \
        DbgBreakPoint();                                    \
    }                                                       \
}

#define DebugUnwind(X) {                                                      \
    if (AbnormalTermination()) {                                             \
        DebugTrace(0, DEBUG_TRACE_UNWIND, #X ", Abnormal termination.\n", 0); \
    }                                                                         \
}

#define DebugDoit(X)                     {X;}

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define DebugDoit(X)                     {NOTHING;}

#endif // LFSDBG

#endif // _LFSDATA_

