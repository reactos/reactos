/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LfsProcs.h

Abstract:

    This module defines all of the globally used procedures in the Log
    File Service.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#ifndef _LFSPROCS_
#define _LFSPROCS_

#include <ntos.h>
#include <string.h>
#include <zwapi.h>
#include <lfs.h>
#include <fsrtl.h>

#include "nodetype.h"
#include "LfsDisk.h"
#include "LfsStruc.h"
#include "LfsData.h"

//
//  Tag all of our allocations if tagging is turned on
//

#undef FsRtlAllocatePool
#undef FsRtlAllocatePoolWithQuota

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,' sfL')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,' sfL')

#define LfsAllocatePoolNoRaise(a,b)         ExAllocatePoolWithTag((a),(b),MODULE_POOL_TAG)
#define LfsAllocatePool(a,b)                ExAllocatePoolWithTag(((a) | POOL_RAISE_IF_ALLOCATION_FAILURE),(b),MODULE_POOL_TAG)
#define LfsFreePool(pv)                     ExFreePool(pv)


//
//  The following routines provide an interface with the cache package.
//  They are contained in 'CacheSup.c'.
//

NTSTATUS
LfsPinOrMapData (
    IN PLFCB Lfcb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN BOOLEAN PinData,
    IN BOOLEAN AllowErrors,
    IN BOOLEAN IgnoreUsaErrors,
    OUT PBOOLEAN UsaError,
    OUT PVOID *Buffer,
    OUT PBCB *Bcb
    );

//
//  VOID
//  LfsPreparePinWriteData (
//      IN PLFCB Lfcb,
//      IN LONGLONG FileOffset,
//      IN ULONG Length,
//      OUT PVOID *Buffer,
//      OUT PBCB *Bcb
//      );
//

#define LfsPreparePinWriteData(L,FO,LEN,BUF,B) {    \
    LONGLONG _LocalFileOffset = (FO);          \
    CcPreparePinWrite( (L)->FileObject,             \
                       (PLARGE_INTEGER)&_LocalFileOffset,           \
                       (LEN),                       \
                       FALSE,                       \
                       TRUE,                        \
                       (B),                         \
                       (BUF) );                     \
}

VOID
LfsPinOrMapLogRecordHeader (
    IN PLFCB Lfcb,
    IN LSN Lsn,
    IN BOOLEAN PinData,
    IN BOOLEAN IgnoreUsaErrors,
    OUT PBOOLEAN UsaError,
    OUT PLFS_RECORD_HEADER *RecordHeader,
    OUT PBCB *Bcb
    );

VOID
LfsCopyReadLogRecord (
    IN PLFCB Lfcb,
    IN PLFS_RECORD_HEADER RecordHeader,
    OUT PVOID Buffer
    );

VOID
LfsFlushLfcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    );

BOOLEAN
LfsReadRestart (
    IN PLFCB Lfcb,
    IN LONGLONG FileSize,
    IN BOOLEAN FirstRestart,
    OUT PLONGLONG RestartPageOffset,
    OUT PLFS_RESTART_PAGE_HEADER *RestartPage,
    OUT PBCB *RestartPageBcb,
    OUT PBOOLEAN ChkdskWasRun,
    OUT PBOOLEAN ValidPage,
    OUT PBOOLEAN UninitializedFile,
    OUT PBOOLEAN LogPacked,
    OUT PLSN LastLsn
    );


//
//  The following routines manipulate buffer control blocks.  They are
//  contained in 'LbcbSup.c'
//

VOID
LfsFlushLbcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    );

VOID
LfsFlushToLsnPriv (
    IN PLFCB Lfcb,
    IN LSN Lsn
    );

PLBCB
LfsGetLbcb (
    IN PLFCB Lfcb
    );


//
//  The following routines are in LfsData.c
//

LONG
LfsExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    );


//
//  Log page support routines.  The following routines manipulate and
//  modify log pages.  They are contained in 'LogPgSup.c'
//

//
//  VOID
//  LfsTruncateOffsetToLogPage (
//      IN PLFCB Lfcb,
//      IN LONGLONG LargeInt,
//      OUT PLONGLONG Result
//      );
//
//  ULONG
//  LfsLogPageOffset (
//      IN PLFCB Lfcb,
//      IN ULONG Integer
//      );
//

#define LfsTruncateOffsetToLogPage(LFCB,LI,OUTLI)       \
    *(OUTLI) = LI;                                      \
    *((PULONG)(OUTLI)) &= (LFCB)->SystemPageInverseMask

#define LfsLogPageOffset(LFCB,INT)                      \
    (INT & (LFCB)->LogPageMask)

VOID
LfsNextLogPageOffset (
    IN PLFCB Lfcb,
    IN LONGLONG CurrentLogPageOffset,
    OUT PLONGLONG NextLogPageOffset,
    OUT PBOOLEAN Wrapped
    );

PVOID
LfsAllocateSpanningBuffer (
    IN PLFCB Lfcb,
    IN ULONG Length
    );

VOID
LfsFreeSpanningBuffer (
    IN PVOID Buffer
    );


//
//  The following routines provide support for dealing with log records.  They
//  are contained in 'LogRcSup.c'
//

BOOLEAN
LfsWriteLogRecordIntoLogPage (
    IN PLFCB Lfcb,
    IN PLCH Lch,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN ClientUndoNextLsn OPTIONAL,
    IN LSN ClientPreviousLsn OPTIONAL,
    IN LONG UndoRequirement,
    IN BOOLEAN ForceToDisk,
    OUT PLSN Lsn
    );


//
//  Lsn support routines.  The following routines provide support for
//  manipulating Lsn values.  They are contained in 'LsnSup.c'
//

//
//  LSN
//  LfsFileOffsetToLsn (
//      IN PLFCB Lfcb,
//      IN LONGLONG FileOffset,
//      IN LONGLONG SequenceNumber
//      );
//
//  BOOLEAN
//  LfsIsLsnInFile (
//      IN PLFCB Lfcb,
//      IN LSN Lsn
//      );
//
//  LSN
//  LfsComputeLsnFromLbcb (
//      IN PLFCB Lfcb,
//      IN PLBCB Lbcb
//      );
//
//  VOID
//  LfsTruncateLsnToLogPage (
//      IN PLFCB Lfcb,
//      IN LSN Lsn,
//      OUT PLONGLONG FileOffset
//      );
//
//  LONGLONG
//  LfsLsnToFileOffset (
//      IN PLFCB Lfcb,
//      IN LSN Lsn
//      );
//
//  LONGLONG
//  LfsLsnToSeqNumber (
//      IN PLFCB Lfcb,
//      IN LSN Lsn
//      );
//
//  ULONG
//  LfsLsnToPageOffset (
//      IN PLFCB Lfcb,
//      IN LSN Lsn
//      );
//

#define LfsFileOffsetToLsn(LFCB,FO,SN) (                                        \
    (((ULONGLONG)(FO)) >> 3) + Int64ShllMod32((SN), (LFCB)->FileDataBits)                                \
)

#define LfsIsLsnInFile(LFCB,LSN)                                                \
    (/*xxGeq*/( (LSN).QuadPart >= ((LFCB)->OldestLsn).QuadPart )                                          \
     && /*xxLeq*/( (LSN).QuadPart <= ((LFCB)->RestartArea->CurrentLsn).QuadPart ))

#define LfsComputeLsnFromLbcb(LFCB,LBCB) (                                              \
    LfsFileOffsetToLsn( LFCB,                                                           \
                        (LBCB)->FileOffset + (LBCB)->BufferOffset,    \
                        (LBCB)->SeqNumber )                                    \
)

#define LfsTruncateLsnToLogPage(LFCB,LSN,FO) {                                  \
    *(FO) = LfsLsnToFileOffset( LFCB, LSN );                                    \
    *((PULONG)(FO)) &= (LFCB)->LogPageInverseMask;                                \
}

#define LfsLsnToFileOffset(LFCB,LSN)                                            \
    /*xxShr*/( ((ULONGLONG)/*xxShl*/( (LSN).QuadPart << (LFCB)->SeqNumberBits )) >> ((LFCB)->SeqNumberBits - 3) )

#define LfsLsnToSeqNumber(LFCB,LSN)                                             \
    /*xxShr*/Int64ShrlMod32( ((ULONGLONG)(LSN).QuadPart), (LFCB)->FileDataBits )

#define LfsLsnToPageOffset(LFCB,LSN)                                            \
    LfsLogPageOffset( LFCB, (LSN).LowPart << 3 )

VOID
LfsLsnFinalOffset (
    IN PLFCB Lfcb,
    IN LSN Lsn,
    IN ULONG DataLength,
    OUT PLONGLONG FinalOffset
    );

BOOLEAN
LfsFindNextLsn (
    IN PLFCB Lfcb,
    IN PLFS_RECORD_HEADER RecordHeader,
    OUT PLSN Lsn
    );


//
//  The following routines support the Lfs restart areas.  They are contained
//  in 'RstrtSup.c'
//

VOID
LfsWriteLfsRestart (
    IN PLFCB Lfcb,
    IN ULONG ThisRestartSize,
    IN BOOLEAN WaitForIo
    );

VOID
LfsFindOldestClientLsn (
    IN PLFS_RESTART_AREA RestartArea,
    IN PLFS_CLIENT_RECORD ClientArray,
    OUT PLSN OldestLsn
    );


//
//  The following routines are used for managing the structures allocated
//  by us.  They are contained in 'StrucSup.c'
//

PLFCB
LfsAllocateLfcb (
    );


VOID
LfsDeallocateLfcb (
    IN PLFCB Lfcb,
    IN BOOLEAN CompleteTeardown
    );

VOID
LfsAllocateLbcb (
    IN PLFCB Lfcb,
    OUT PLBCB *Lbcb
    );

VOID
LfsDeallocateLbcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    );

VOID
LfsAllocateLcb (
    IN PLFCB Lfcb,
    OUT PLCB *NewLcb
    );

VOID
LfsDeallocateLcb (
    IN PLFCB Lfcb,
    IN PLCB Lcb
    );

//
//  VOID
//  LfsInitializeLcb (
//      IN PLCB Lcb,
//      IN LFS_CLIENT_ID ClientId,
//      IN LFS_CONTEXT_MODE ContextMode
//      );
//
//
//  VOID
//  LfsAllocateLch (
//      OUT PLCH *Lch
//      );
//
//  VOID
//  LfsDeallocateLch (
//      IN PLCH Lch
//      );
//
//  VOID
//  LfsAllocateRestartArea (
//      OUT PLFS_RESTART_AREA *RestartArea,
//      ULONG Size
//      );
//
//  VOID
//  LfsDeallocateRestartArea (
//      IN PLFS_RESTART_AREA RestartArea
//      );
//
//  BOOLEAN
//  LfsLbcbIsRestart (
//      IN PLBCB Lbcb
//      );
//

#define LfsInitializeLcb(LCB,ID,MODE)                           \
    (LCB)->ClientId = ID;                                       \
    (LCB)->ContextMode = MODE


#define LfsAllocateLch(NEW)     {                               \
    *(NEW) = FsRtlAllocatePool( PagedPool, sizeof( LCH ));      \
    RtlZeroMemory( (*NEW), sizeof( LCH ));                      \
    (*(NEW))->NodeTypeCode = LFS_NTC_LCH;                       \
    (*(NEW))->NodeByteSize = sizeof( LCH );                     \
}

#define LfsDeallocateLch(LCH)                                   \
    ExFreePool( LCH )

#define LfsAllocateRestartArea(RS,SIZE)                         \
    *(RS) = FsRtlAllocatePool( PagedPool, (SIZE) );             \
    RtlZeroMemory( *(RS), (SIZE) )

#define LfsDeallocateRestartArea(RS)                            \
    ExFreePool( RS )

#define LfsLbcbIsRestart(LBCB)                                  \
    (FlagOn( (LBCB)->LbcbFlags, LBCB_RESTART_LBCB ))


//
//  The following routines provide synchronization support for the Lfs
//  shared structures.  They are contained in 'SyncSup.c'
//

//
//  VOID
//  LfsAcquireLfsData (
//      );
//
//  VOID
//  LfsReleaseLfsData (
//      );
//
//  VOID
//  LfsAcquireLfcb (
//      IN PLFCB Lfcb
//      );
//
//  VOID
//  LfsReleaseLfcb (
//      IN PLFCB Lfcb
//      );
//
//  VOID
//  LfsAcquireLch (
//      IN PLCH Lch
//      );
//
//  VOID
//  LfsReleaseLfcb (
//      IN PLCH Lch
//      );
//

#define LfsAcquireLfsData()                                 \
    ExAcquireFastMutex( &LfsData.LfsDataLock )

#define LfsReleaseLfsData()                                 \
    ExReleaseFastMutex( &LfsData.LfsDataLock )

#define LfsAcquireBufferLock()                              \
    ExAcquireFastMutex( &LfsData.BufferLock )

#define LfsReleaseBufferLock()                              \
    ExReleaseFastMutex( &LfsData.BufferLock )

#define LfsWaitForBufferNotification()                      \
    KeWaitForSingleObject( &LfsData.BufferNotification,     \
                           Executive,                       \
                           KernelMode,                      \
                           FALSE,                           \
                           NULL )                          

#define LfsNotifyBufferWaiters()                            \
    KeSetEvent( &LfsData.BufferNotification, 0, FALSE )

#define LfsBlockBufferWaiters()                             \
    KeClearEvent( &LfsData.BufferNotification )

#define LfsAcquireLfcb(LFCB)                                                            \
    ExAcquireResourceExclusive( &(LFCB)->Sync->Resource, TRUE )

#define LfsReleaseLfcb(LFCB)                                                            \
    if ((LFCB)->Sync->Resource.OwnerThreads[0].OwnerThread == ExGetCurrentResourceThread()) {\
        ExReleaseResource( &(LFCB)->Sync->Resource );                                   \
    }

#define LfsAcquireLch(LCH)                                                              \
    ExAcquireResourceExclusive( &(LCH)->Sync->Resource, TRUE )

#define LfsReleaseLch(LCH)                                                              \
    if ((LCH)->Sync->Resource.OwnerThreads[0].OwnerThread == ExGetCurrentResourceThread()) { \
        ExReleaseResource( &(LCH)->Sync->Resource );                                    \
    }


//
//  The following routines are used to check various structures for validity
//  and comparability.  They are contained in 'VerfySup.c'.
//

VOID
LfsCurrentAvailSpace (
    IN PLFCB Lfcb,
    OUT PLONGLONG CurrentAvailSpace,
    OUT PULONG CurrentPageBytes
    );

BOOLEAN
LfsVerifyLogSpaceAvail (
    IN PLFCB Lfcb,
    IN PLCH Lch,
    IN ULONG RemainingLogBytes,
    IN LONG UndoRequirement,
    IN BOOLEAN ForceToDisk
    );

VOID
LfsFindCurrentAvail (
    IN PLFCB Lfcb
    );

//
//  VOID
//  LfsValidateLch (
//      IN PLCH Lch
//      );
//
//  VOID
//  LfsValidateClientId (
//      IN PLFCB Lfcb,
//      IN PLCH Lch
//      );
//
//  BOOLEAN
//  LfsVerifyClientLsnInRange (
//      IN PLFCB Lfcb,
//      IN PLFS_CLIENT_RECORD ClientRecord,
//      IN LSN Lsn
//      );
//
//  BOOLEAN
//  LfsClientIdMatch (
//      IN PLFS_CLIENT_ID ClientA,
//      IN PLFS_CLIENT_ID ClientB
//      )
//
//  VOID
//  LfsValidateLcb (
//      IN PLFS_CONTEXT_BLOCK Lcb,
//      IN PLCH Lch
//      )
//

#define LfsValidateLch(LCH)                                     \
    if ((LCH) == NULL                                           \
        || (LCH)->NodeTypeCode != LFS_NTC_LCH                   \
        || ((LCH)->Lfcb != NULL                                 \
            && (LCH)->Lfcb->NodeTypeCode != LFS_NTC_LFCB)) {    \
                                                                \
        ExRaiseStatus( STATUS_ACCESS_DENIED );                  \
    }

#define LfsValidateClientId(LFCB,LCH)                                   \
    if ((LCH)->ClientId.ClientIndex >= (LFCB)->RestartArea->LogClients  \
        || (LCH)->ClientId.SeqNumber                                    \
           != Add2Ptr( Lfcb->ClientArray,                               \
                       (LCH)->ClientArrayByteOffset,                    \
                       PLFS_CLIENT_RECORD )->SeqNumber) {               \
        ExRaiseStatus( STATUS_ACCESS_DENIED );                          \
    }

#define LfsVerifyClientLsnInRange(LFCB,CLIENT,LSN)                      \
    (/*xxGeq*/( (LSN).QuadPart >= ((CLIENT)->OldestLsn).QuadPart )                                  \
     && /*xxLeq*/( (LSN).QuadPart <= ((LFCB)->RestartArea->CurrentLsn).QuadPart )                   \
     && /*xxNeqZero*/( (LSN).QuadPart != 0 ))

#define LfsClientIdMatch(CLIENT_A,CLIENT_B)                             \
    ((BOOLEAN) ((CLIENT_A)->SeqNumber == (CLIENT_B)->SeqNumber          \
                && (CLIENT_A)->ClientIndex == (CLIENT_B)->ClientIndex))

#define LfsValidateLcb(LCB,LCH)                                         \
    if (LCB == NULL                                                     \
        || (LCB)->NodeTypeCode != LFS_NTC_LCB                           \
        || !LfsClientIdMatch( &(LCB)->ClientId, &(LCH)->ClientId )) {   \
        ExRaiseStatus( STATUS_ACCESS_DENIED );                          \
    }


//
//  Miscellaneous support routines
//

//
//      ULONG
//      FlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      BOOLEAN
//      BooleanFlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      SetFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      ClearFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}

//
//  This macro takes a pointer (or ulong) and returns its rounded up word
//  value
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up longword
//  value
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  This macro will up a 64 bit value to the next quad align boundary.
//

#define LiQuadAlign(LI,OUT)   {         \
    *(OUT) = /*xxAdd*/( (LI) + 7 );       \
    *((PULONG)(OUT)) &= 0xfffffff8;       \
}

//
//      CAST
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          IN (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG_PTR)(OFFSET) - (ULONG_PTR)(BASE)))


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }

#endif // _LFSPROCS_
