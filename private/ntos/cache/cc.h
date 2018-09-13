/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    cc.h

Abstract:

    This module is a header file for the Memory Management based cache
    management routines for the common Cache subsystem.

Author:

    Tom Miller      [TomM]      4-May-1990

Revision History:

--*/

#ifndef _CCh_
#define _CCh_

#include <ntos.h>
#include <NtIoLogc.h>

#ifdef MEMPRINT
#include <memprint.h>
#endif

//
// Define macros to acquire and release cache manager locks.
//

#if defined(_ALPHA_) || defined(_X86_)

#define CcAcquireMasterLock( OldIrql ) \
    *( OldIrql ) = KeAcquireQueuedSpinLock( LockQueueMasterLock )

#define CcReleaseMasterLock( OldIrql ) \
    KeReleaseQueuedSpinLock( LockQueueMasterLock, OldIrql )

#define CcAcquireMasterLockAtDpcLevel() \
    KiAcquireQueuedSpinLock( &KeGetCurrentPrcb()->LockQueue[LockQueueMasterLock] )

#define CcReleaseMasterLockFromDpcLevel() \
    KiReleaseQueuedSpinLock( &KeGetCurrentPrcb()->LockQueue[LockQueueMasterLock] )

#define CcAcquireVacbLock( OldIrql ) \
    *( OldIrql ) = KeAcquireQueuedSpinLock( LockQueueVacbLock )

#define CcReleaseVacbLock( OldIrql ) \
    KeReleaseQueuedSpinLock( LockQueueVacbLock, OldIrql )

#define CcAcquireVacbLockAtDpcLevel() \
    KiAcquireQueuedSpinLock( &KeGetCurrentPrcb()->LockQueue[LockQueueVacbLock] )

#define CcReleaseVacbLockFromDpcLevel() \
    KiReleaseQueuedSpinLock( &KeGetCurrentPrcb()->LockQueue[LockQueueVacbLock] )

#else

#define CcAcquireMasterLock( OldIrql ) \
    ExAcquireSpinLock( &CcMasterSpinLock, OldIrql )

#define CcReleaseMasterLock( OldIrql ) \
    ExReleaseSpinLock( &CcMasterSpinLock, OldIrql )

#define CcAcquireMasterLockAtDpcLevel() \
    ExAcquireSpinLockAtDpcLevel( &CcMasterSpinLock )

#define CcReleaseMasterLockFromDpcLevel() \
    ExReleaseSpinLockFromDpcLevel( &CcMasterSpinLock )

#define CcAcquireVacbLock( OldIrql ) \
    ExAcquireSpinLock( &CcVacbSpinLock, OldIrql )

#define CcReleaseVacbLock( OldIrql ) \
    ExReleaseSpinLock( &CcVacbSpinLock, OldIrql )

#define CcAcquireVacbLockAtDpcLevel() \
    ExAcquireSpinLockAtDpcLevel( &CcVacbSpinLock )

#define CcReleaseVacbLockFromDpcLevel() \
    ExReleaseSpinLockFromDpcLevel( &CcVacbSpinLock )

#endif

//
//  This turns on the Bcb list debugging in a debug system.  Set value
//  to 0 to turn off.
//
//  ****    Note it must currently be turned off because the routines in
//          pinsup.c that manipulate this list need to be changed to do the
//          right thing for Obcbs.  Right now they screw up by inserting Obcbs
//          (which may not be large enough among other things) into the global
//          list.  Ideally each place gets some code to insert the underlying
//          Bcbs into the list if they are not already there.
//

#if DBG
#define LIST_DBG 0
#endif

#include <FsRtl.h>
#ifndef _USERKDX_  // Including stdlib.h build breaks ntos\w32\ntuser\kdexts\kd (!dso)
#include <stdlib.h>
#endif
#include <string.h>
#include <limits.h>

//
//  Tag all of our allocations if tagging is turned on
//

#undef FsRtlAllocatePool
#undef FsRtlAllocatePoolWithQuota

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'  cC')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'  cC')

#undef ExAllocatePool
#undef ExAllocatePoolWithQuota

#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  cC')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  cC')


//
//  Peek at number of available pages.
//

extern PFN_COUNT MmAvailablePages;

#if DBG
//  #define MIPS_PREFILL 0
#endif

#ifdef MIPS
#ifdef MIPS_PREFILL
VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
        );
#endif
#endif

//
//  Define our node type codes.
//

#define CACHE_NTC_SHARED_CACHE_MAP       (0x2FF)
#define CACHE_NTC_PRIVATE_CACHE_MAP      (0x2FE)
#define CACHE_NTC_BCB                    (0x2FD)
#define CACHE_NTC_DEFERRED_WRITE         (0x2FC)
#define CACHE_NTC_MBCB                   (0x2FB)
#define CACHE_NTC_OBCB                   (0x2FA)
#define CACHE_NTC_MBCB_GRANDE            (0x2F9)

//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  CACHE_BUG_CHECK_ values defined below and then use CcBugCheck to bugcheck
//  the system.
//

#define CACHE_BUG_CHECK_CACHEDAT           (0x00010000)
#define CACHE_BUG_CHECK_CACHESUB           (0x00020000)
#define CACHE_BUG_CHECK_COPYSUP            (0x00030000)
#define CACHE_BUG_CHECK_FSSUP              (0x00040000)
#define CACHE_BUG_CHECK_LAZYRITE           (0x00050000)
#define CACHE_BUG_CHECK_LOGSUP             (0x00060000)
#define CACHE_BUG_CHECK_MDLSUP             (0x00070000)
#define CACHE_BUG_CHECK_PINSUP             (0x00080000)
#define CACHE_BUG_CHECK_VACBSUP            (0x00090000)

#define CcBugCheck(A,B,C) { KeBugCheckEx(CACHE_MANAGER, BugCheckFileId | __LINE__, A, B, C ); }

//
//  Define maximum View Size (These constants are currently so chosen so
//  as to be exactly a page worth of PTEs.
//

#define DEFAULT_CREATE_MODULO            ((ULONG)(0x00100000))
#define DEFAULT_EXTEND_MODULO            ((ULONG)(0x00100000))

//
//  For non FO_RANDOM_ACCESS files, define how far we go before umapping
//  views.
//

#define SEQUENTIAL_MAP_LIMIT        ((ULONG)(0x00080000))

//
//  Define some constants to drive read ahead and write behind
//

//
//  Set max read ahead.  Even though some drivers, such as AT, break up transfers >= 128kb,
//  we need to permit enough readahead to satisfy plausible cached read operation while
//  preventing denial of service attacks.
//
//  This value used to be set to 64k.  When doing cached reads in larger units (128k), we
//  would never be bringing in enough data to keep the user from blocking. 8mb is
//  arbitrarily chosen to be greater than plausible RAID bandwidth and user operation size
//  by a factor of 3-4.
//

#define MAX_READ_AHEAD                   (8 * 1024 * 1024)

//
//  Set maximum write behind / lazy write (most drivers break up transfers >= 64kb)
//

#define MAX_WRITE_BEHIND                 (MM_MAXIMUM_DISK_IO_SIZE)

//
//  Set a throttle for charging a given write against the total number of dirty
//  pages in the system, for the purpose of seeing when we should invoke write
//  throttling.
//
//  This must be the same as the throttle used for seeing when we must flush
//  temporary files in the lazy writer.  On the back of the envelope, here
//  is why:
//
//      RDP = Regular File Dirty Pages
//      TDP = Temporary File Dirty Pages
//      CWT = Charged Write Throttle
//          -> the maximum we will charge a user with when we see if
//              he should be throttled
//      TWT = Temporary Write Throttle
//          -> if we can't write this many pages, we must write temp data
//      DPT = Dirty Page Threshold
//          -> the limit when write throttling kicks in
//
//      PTD = Pages To Dirty
//      CDP = Charged Dirty Pages
//
//      Now, CDP = Min( PTD, CWT).
//
//      Excluding other effects, we throttle when:
//          #0  (RDP + TDP) + CPD >= DPT
//
//      To write temporary data, we must cause:
//          #1  (RDP + TDP) + TWT >= DPT
//
//      To release the throttle, we must eventually cause:
//          #2  (RDP + TDP) + CDP < DPT
//
//      Now, imagine TDP >> RDP (perhaps RDP == 0) and CDP == CWT for a particular
//      throttled write.
//
//      If CWT > TWT, as we drive RDP to zero (we never defer writing regular
//      data except for hotspots or other very temporary conditions), it is clear
//      that we may never trigger the writing of temporary data (#1) but also
//      never release the throttle (#2).  Simply, we would be willing to charge
//      for more dirty pages than we would be willing to guarantee are avaliable
//      to dirty.  Hence, potential deadlock.
//
//      CWT < TWT I leave aside for the moment.  This would mean we try not to
//      allow temporary data to accumulate to the point that writes throttle as
//      a result.  Perhaps this would even be better than CWT == TWT.
//
//  It is legitimate to ask if throttling temporary data writes should be relaxed
//  if we see a large amount of dirty temp data accumulate (and it would be very
//  easy to keep track of this).  I don't claim to know the best answer to this,
//  but for now the attempt to avoid temporary data writes at all costs still
//  fits the reasonable operation mix, and we will only penalize the outside
//  oddcase with a little more throttle/release.
//

#define WRITE_CHARGE_THRESHOLD          (64 * PAGE_SIZE)

//
//  Define constants to control zeroing of file data: one constant to control
//  how much data we will actually zero ahead in the cache, and another to
//  control what the maximum transfer size is that we will use to write zeros.
//

#define MAX_ZERO_TRANSFER               (PAGE_SIZE * 128)
#define MIN_ZERO_TRANSFER               (0x10000)
#define MAX_ZEROS_IN_CACHE              (0x10000)

//
//  Definitions for multi-level Vacb structure.  The primary definition is the
//  VACB_LEVEL_SHIFT.  In a multi-level Vacb structure, level in the tree of
//  pointers has 2 ** VACB_LEVEL_SHIFT pointers.
//
//  For test, this value may be set as low as 4 (no lower), a value of 10 corresponds
//  to a convenient block size of 4KB.  (If set to 2, CcExtendVacbArray will try to
//  "push" the Vacb array allocated within the SharedCacheMap, and later someone will
//  try to deallocate the middle of the SharedCacheMap.  At 3, the MBCB_BITMAP_BLOCK_SIZE
//  is larger than MBCB_BITMAP_BLOCK_SIZE)
//
//  There is a bit of a trick as we make the jump to the multilevel structure in that
//  we need a real fixed reference count.
//

#define VACB_LEVEL_SHIFT                  (7)

//
//  This is how many bytes of pointers are at each level.  This is the size for both
//  the Vacb array and (optional) Bcb listheads.  It does not include the reference
//  block.
//

#define VACB_LEVEL_BLOCK_SIZE             ((1 << VACB_LEVEL_SHIFT) * sizeof(PVOID))

//
//  This is the last index for a level.
//

#define VACB_LAST_INDEX_FOR_LEVEL         ((1 << VACB_LEVEL_SHIFT) - 1)

//
//  This is the size of file which can be handled in a single level.
//

#define VACB_SIZE_OF_FIRST_LEVEL         (1 << (VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT))

//
//  This is the maximum number of levels it takes to support 63-bits.  It is
//  used for routines that must remember a path.
//

#define VACB_NUMBER_OF_LEVELS            (((63 - VACB_OFFSET_SHIFT)/VACB_LEVEL_SHIFT) + 1)

//
//  Define the reference structure for multilevel Vacb trees.
//

typedef struct _VACB_LEVEL_REFERENCE {

    LONG Reference;
    LONG SpecialReference;

} VACB_LEVEL_REFERENCE, *PVACB_LEVEL_REFERENCE;

//
//  Define the size of a bitmap allocated for a bitmap range, in bytes.
//

#define MBCB_BITMAP_BLOCK_SIZE           (VACB_LEVEL_BLOCK_SIZE)

//
//  Define how many bytes of a file are covered by an Mbcb bitmap range,
//  at a bit for each page.
//

#define MBCB_BITMAP_RANGE                (MBCB_BITMAP_BLOCK_SIZE * 8 * PAGE_SIZE)

//
//  Define the initial size of the Mbcb bitmap that is self-contained in the Mbcb.
//

#define MBCB_BITMAP_INITIAL_SIZE         (2 * sizeof(BITMAP_RANGE))

//
//  Define constants controlling when the Bcb list is broken into a
//  pendaflex-style array of listheads, and how the correct listhead
//  is found.  Begin when file size exceeds 2MB, and cover 512KB per
//  listhead.  At 512KB per listhead, the BcbListArray is the same
//  size as the Vacb array, i.e., it doubles the size.
//
//  The code handling these Bcb lists in the Vacb package contains
//  assumptions that the size is the same as that of the Vacb pointers.
//  Future work could undo this, but until then the size and shift
//  below cannot change.  There really isn't a good reason to want to
//  anyway.
//
//  Note that by definition a flat vacb array cannot fail to find an
//  exact match when searching for the listhead - this is only a
//  complication of the sparse structure.
//


#define BEGIN_BCB_LIST_ARRAY             (0x200000)
#define SIZE_PER_BCB_LIST                (VACB_MAPPING_GRANULARITY * 2)
#define BCB_LIST_SHIFT                   (VACB_OFFSET_SHIFT + 1)

#define GetBcbListHead(SCM,OFF,FAILSUCC) (                                                         \
  (((SCM)->SectionSize.QuadPart > BEGIN_BCB_LIST_ARRAY) &&                                         \
   FlagOn((SCM)->Flags, MODIFIED_WRITE_DISABLED)) ?                                                \
   (((SCM)->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) ?                                     \
    CcGetBcbListHeadLargeOffset((SCM),(OFF),(FAILSUCC)) :                                          \
    (((OFF) >= (SCM)->SectionSize.QuadPart) ? &(SCM)->BcbList :                                    \
     ((PLIST_ENTRY)((SCM)->Vacbs) + (((SCM)->SectionSize.QuadPart + (OFF)) >> BCB_LIST_SHIFT)))) : \
   &(SCM)->BcbList                                                                                 \
)

//
//  Macros to lock/unlock a Vacb level as Bcbs are inserted/deleted
//

#define CcLockVacbLevel(SCM,OFF) {                                                               \
    if (((SCM)->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) &&                              \
        FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED)) {                                \
    CcAdjustVacbLevelLockCount((SCM),(OFF), +1);}                                                \
}

#define CcUnlockVacbLevel(SCM,OFF) {                                                             \
    if (((SCM)->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) &&                              \
        FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED)) {                                \
    CcAdjustVacbLevelLockCount((SCM),(OFF), -1);}                                                \
}

//
//  NOISE_BITS defines how many bits are masked off when testing for
//  sequential reads.  This allows the reader to skip up to 7 bytes
//  for alignment purposes, and we still consider the next read to be
//  sequential.  Starting and ending addresses are masked by this pattern
//  before comparison.
//

#define NOISE_BITS                       (0x7)

//
//  Define some constants to drive the Lazy Writer
//

#define LAZY_WRITER_IDLE_DELAY           ((LONG)(10000000))
#define LAZY_WRITER_COLLISION_DELAY      ((LONG)(1000000))

//
//  The following target should best be a power of 2
//

#define LAZY_WRITER_MAX_AGE_TARGET       ((ULONG)(8))

//
//  Requeue information hint for the lazy writer.
//

#define CC_REQUEUE                       35422

//
//  The global Cache Manager debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001      FsSup
//      0x00000002      CacheSub
//      0x00000004      CopySup
//      0x00000008      PinSup
//
//      0x00000010      MdlSup
//      0x00000020      LazyRite
//      0x00000040
//      0x00000080
//
//      0x00000100      Trace all Mm calls
//

#define mm (0x100)

//
//  Miscellaneous support macros.
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
//      ULONG
//      QuadAlign (
//          IN ULONG Pointer
//          );
//

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(F,SF) { \
    (F) |= (SF);        \
}

#define ClearFlag(F,SF) { \
    (F) &= ~(SF);         \
}

#define QuadAlign(P) (             \
    ((((P)) + 7) & (-8)) \
)

//
//  Turn on pseudo-asserts if CC_FREE_ASSERTS is defined.
//

#if (!DBG && defined( CC_FREE_ASSERTS ))
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)                                             \
    ((exp) ? TRUE :                                             \
             (DbgPrint( "%s:%d %s\n",__FILE__,__LINE__,#exp ),  \
              DbgBreakPoint(),                                  \
              TRUE))
#define ASSERTMSG(msg,exp)                                              \
    ((exp) ? TRUE :                                                     \
             (DbgPrint( "%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp ),   \
              DbgBreakPoint(),                                          \
              TRUE))
#endif


#if DANLO
typedef struct _CC_LOG_ENTRY {
    ULONG Action;
    ULONG Reason;
} CC_LOG_ENTRY;

typedef struct _CC_LOG {
    USHORT Current;
    USHORT Size;
    CC_LOG_ENTRY Log[48];
} CC_LOG;

#define CcAddToLog( LOG, ACTION, REASON ) {         \
    (LOG)->Current += 1;                            \
    if ((LOG)->Current == (LOG)->Size) {            \
        (LOG)->Current = 0;                         \
    }                                               \
    (LOG)->Log[(LOG)->Current].Action = (ACTION);   \
    (LOG)->Log[(LOG)->Current].Reason = (REASON);   \
}
#else
#define CcAddToLog( LOG, ACTION, REASON )
#endif


//
//  Define the Virtual Address Control Block, which controls all mapping
//  performed by the Cache Manager.
//

//
//  First some constants
//

#define PREALLOCATED_VACBS               (4)

//
//  Virtual Address Control Block
//

typedef struct _VACB {

    //
    //  Base Address for this control block.
    //

    PVOID BaseAddress;

    //
    //  Pointer to the Shared Cache Map using this Vacb.
    //

    struct _SHARED_CACHE_MAP *SharedCacheMap;

    //
    //  Overlay for remembering mapped offset within the Shared Cache Map,
    //  and the count of the number of times this Vacb is in use.
    //

    union {

        //
        //  File Offset within Shared Cache Map
        //

        LARGE_INTEGER FileOffset;

        //
        //  Count of number of times this Vacb is in use.  The size of this
        //  count is calculated to be adequate, while never large enough to
        //  overwrite nonzero bits of the FileOffset, which is a multiple
        //  of VACB_MAPPING_GRANULARITY.
        //

        USHORT ActiveCount;

    } Overlay;

    //
    //  Entry for the VACB reuse list
    //

    LIST_ENTRY LruList;

} VACB, *PVACB;

//
//  These define special flag values that are overloaded as PVACB.  They cause
//  certain special behavior, currently only in the case of multilevel structures.
//

#define VACB_SPECIAL_REFERENCE           ((PVACB) ~0)
#define VACB_SPECIAL_DEREFERENCE         ((PVACB) ~1)

#define VACB_SPECIAL_FIRST_VALID         VACB_SPECIAL_DEREFERENCE


//
//  The Private Cache Map is a structure pointed to by the File Object, whenever
//  a file is opened with caching enabled (default).
//

typedef struct _PRIVATE_CACHE_MAP {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Pointer to FileObject for this PrivateCacheMap.
    //

    PFILE_OBJECT FileObject;

    //
    //  READ AHEAD CONTROL
    //
    //  Read ahead history for determining when read ahead might be
    //  beneficial.
    //

    LARGE_INTEGER FileOffset1;
    LARGE_INTEGER BeyondLastByte1;

    LARGE_INTEGER FileOffset2;
    LARGE_INTEGER BeyondLastByte2;

    //
    //  Current read ahead requirements.
    //
    //  Array element 0 is optionally used for recording remaining bytes
    //  required for satisfying a large Mdl read.
    //
    //  Array element 1 is used for predicted read ahead.
    //

    LARGE_INTEGER ReadAheadOffset[2];
    ULONG ReadAheadLength[2];

    //
    //  SpinLock controlling access to following fields
    //

    KSPIN_LOCK ReadAheadSpinLock;

    //
    //  Read Ahead mask formed from Read Ahead granularity - 1
    //

    ULONG ReadAheadMask;

    //
    // Links for list of all PrivateCacheMaps linked to the same
    // SharedCacheMap.
    //

    LIST_ENTRY PrivateLinks;

    //
    //  This flag says read ahead is currently active, which means either
    //  a file system call to CcReadAhead is still determining if the
    //  desired data is already resident, or else a request to do read ahead
    //  has been queued to a worker thread.
    //

    BOOLEAN ReadAheadActive;

    //
    //  Flag to say whether read ahead is currently enabled for this
    //  FileObject/PrivateCacheMap.  On read misses it is enabled on
    //  read ahead hits it will be disabled.  Initially disabled.
    //

    BOOLEAN ReadAheadEnabled;

} PRIVATE_CACHE_MAP;

typedef PRIVATE_CACHE_MAP *PPRIVATE_CACHE_MAP;


//
//  The Shared Cache Map is a per-file structure pointed to indirectly by
//  each File Object.  The File Object points to a pointer in a single
//  FS-private structure for the file (Fcb).  The SharedCacheMap maps the
//  first part of the file for common access by all callers.
//

typedef struct _SHARED_CACHE_MAP {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Number of times this file has been opened cached.
    //

    ULONG OpenCount;

    //
    //  Actual size of file, primarily for restricting Read Ahead.  Initialized
    //  on creation and maintained by extend and truncate operations.
    //
    //  NOTE:   This field may never be moved, thanks to the late DavidGoe,
    //          who should have written this comment himself :-(   cache.h
    //          exports a macro which "knows" that FileSize is the second
    //          longword in the Cache Map!
    //

    LARGE_INTEGER FileSize;

    //
    //  Bcb Listhead.  The BcbList is ordered by descending
    //  FileOffsets, to optimize misses in the sequential I/O case.
    //  Synchronized by the BcbSpinLock.
    //

    LIST_ENTRY BcbList;

    //
    //  Size of section created.
    //

    LARGE_INTEGER SectionSize;

    //
    //  ValidDataLength for file, as currently stored by the file system.
    //  Synchronized by the BcbSpinLock or exclusive access by FileSystem.
    //

    LARGE_INTEGER ValidDataLength;

    //
    //  Goal for ValidDataLength, when current dirty data is written.
    //  Synchronized by the BcbSpinLock or exclusive access by FileSystem.
    //

    LARGE_INTEGER ValidDataGoal;

    //
    //  Pointer to a contiguous array of Vacb pointers which control mapping
    //  to this file, along with Vacbs (currently) for a 1MB file.
    //  Synchronized by CcVacbSpinLock.
    //

    PVACB InitialVacbs[PREALLOCATED_VACBS];
    PVACB * Vacbs;

    //
    //  Referenced pointer to original File Object on which the SharedCacheMap
    //  was created.
    //

    PFILE_OBJECT FileObject;

    //
    //  Describe Active Vacb and Page for copysup optimizations.
    //

    volatile PVACB ActiveVacb;
    ULONG ActivePage;

    //
    //  Virtual address needing zero to end of page
    //

    volatile PVOID NeedToZero;
    ULONG NeedToZeroPage;

    //
    //  Fields for synchronizing on active requests.
    //

    KSPIN_LOCK ActiveVacbSpinLock;
    ULONG VacbActiveCount;

    //
    //  THE NEXT TWO FIELDS MUST BE ADJACENT, TO SUPPORT
    //  SHARED_CACHE_MAP_LIST_CURSOR!
    //
    //  Links for Global SharedCacheMap List
    //

    LIST_ENTRY SharedCacheMapLinks;

    //
    //  Shared Cache Map flags (defined below)
    //

    ULONG Flags;

    //
    //  Mask Bcb for this SharedCacheMap, if there is one.
    //  Synchronized by the BcbSpinLock.
    //

    struct _MBCB *Mbcb;

    //
    //  Number of dirty pages in this SharedCacheMap.  Used to trigger
    //  write behind.  Synchronized by CcMasterSpinLock.
    //

    ULONG DirtyPages;

    //
    //  Pointer to the common Section Object used by the file system.
    //

    PVOID Section;

    //
    //  Status variable set by creator of SharedCacheMap
    //

    NTSTATUS Status;

    //
    //  This event pointer is used to handle creation collisions.
    //  If a second thread tries to call CcInitializeCacheMap for the
    //  same file, while BeingCreated (below) is TRUE, then that thread
    //  will allocate an event store it here (if not already allocated),
    //  and wait on it.  The first creator will set this event when it
    //  is done.  The event is not deleted until CcUninitializedCacheMap
    //  is called, to avoid possible race conditions.  (Note that normally
    //  the event never has to be allocated.
    //

    PKEVENT CreateEvent;

    //
    //  This points to an event used to wait for active count to go to zero
    //

    PKEVENT WaitOnActiveCount;

    //
    //  These two fields control the writing of large metadata
    //  streams.  The first field gives a target for the current
    //  flush interval, and the second field stores the end of
    //  the last flush that occurred on this file.
    //

    ULONG PagesToWrite;
    LONGLONG BeyondLastFlush;

#if 0
    //
    //  This records where the last view miss occured in the file.
    //

    LARGE_INTEGER LastViewMiss;
#endif
    
    //
    //  Pointer to structure of routines used by the Lazy Writer to Acquire
    //  and Release the file for Lazy Write and Close, to avoid deadlocks,
    //  and the context to call them with.
    //

    PCACHE_MANAGER_CALLBACKS Callbacks;

    PVOID LazyWriteContext;

    //
    //  Listhead of all PrivateCacheMaps linked to this SharedCacheMap.
    //

    LIST_ENTRY PrivateList;

    //
    //  Log handle specified for this shared cache map, for support of routines
    //  in logsup.c
    //

    PVOID LogHandle;

    //
    //  Callback routine specified for flushing to Lsn.
    //

    PFLUSH_TO_LSN FlushToLsnRoutine;

    //
    //  Dirty Page Threshold for this stream
    //

    ULONG DirtyPageThreshold;

    //
    //  Lazy Writer pass count.  Used by the Lazy Writer for
    //  no modified write streams, which are not serviced on
    //  every pass in order to avoid contention with foreground
    //  activity.
    //

    ULONG LazyWritePassCount;

    //
    //  This event pointer is used to allow a file system to be notified when
    //  the deletion of a shared cache map.
    //
    //  This has to be provided here because the cache manager may decide to
    //  "Lazy Delete" the shared cache map, and some network file systems
    //  will want to know when the lazy delete completes.
    //

    PCACHE_UNINITIALIZE_EVENT UninitializeEvent;

    //
    //  This Vacb pointer is needed for keeping the NeedToZero virtual address
    //  valid.
    //

    PVACB NeedToZeroVacb;

    //
    //  Spinlock for synchronizing the Mbcb and Bcb lists - must be acquired
    //  before CcMasterSpinLock.  This spinlock also synchronizes ValidDataGoal
    //  and ValidDataLength, as described above.
    //

    KSPIN_LOCK BcbSpinLock;

    //
    //  This is a scratch event which can be used either for
    //  a CreateEvent or a WaitOnActiveCount event.  It is
    //  difficult to share this event, because of the very
    //  careful semantics by which they are cleared.  On the
    //  other hand, both events are relatively rarely used
    //  (especially the CreateEvent), so it will be rare that
    //  we will actually use both for the same file, and have
    //  to allocate one.
    //

    PKEVENT LocalEvent;
    KEVENT Event;

    //
    //  Preallocate on PrivateCacheMap to reduce pool allocations.
    //

    PRIVATE_CACHE_MAP PrivateCacheMap;

#if DANLO
    //
    //  Instrument reasons for OpenCount
    //

    CC_LOG OpenCountLog;
#endif

} SHARED_CACHE_MAP;

typedef SHARED_CACHE_MAP *PSHARED_CACHE_MAP;

//
//  OpenCount log Reasons/Actions
//

#define CcIncrementOpenCount( SCM, REASON ) {           \
    (SCM)->OpenCount += 1;                              \
    if (REASON != 0) {                                  \
        CcAddToLog( &(SCM)->OpenCountLog, REASON, 1 );  \
    }                                                   \
}

#define CcDecrementOpenCount( SCM, REASON ) {           \
    (SCM)->OpenCount -= 1;                              \
    if (REASON != 0) {                                  \
        CcAddToLog( &(SCM)->OpenCountLog, REASON, -1 ); \
    }                                                   \
}

//
//  Shared Cache Map Flags
//

//
//  Read ahead has been disabled on this file.
//

#define DISABLE_READ_AHEAD               0x0001

//
//  Write behind has been disabled on this file.
//

#define DISABLE_WRITE_BEHIND             0x0002

//
//  This flag indicates whether CcInitializeCacheMap was called with
//  PinAccess = TRUE.
//

#define PIN_ACCESS                       0x0004

//
//  This flag indicates that a truncate is required when OpenCount
//  goes to 0.
//

#define TRUNCATE_REQUIRED                0x0010

//
//  This flag indicates that a LazyWrite request is queued.
//

#define WRITE_QUEUED                     0x0020

//
//  This flag indicates that we have never seen anyone cache
//  the file except for with FO_SEQUENTIAL_ONLY, so we should
//  tell MM to quickly dump pages when we unmap.
//

#define ONLY_SEQUENTIAL_ONLY_SEEN        0x0040

//
//  Active Page is locked
//

#define ACTIVE_PAGE_IS_DIRTY             0x0080

//
//  Flag to say that a create is in progress.
//

#define BEING_CREATED                    0x0100

//
//  Flag to say that modified write was disabled on the section.
//

#define MODIFIED_WRITE_DISABLED          0x0200

//
//  Flag that indicates if a lazy write ever occurred on this file.
//

#define LAZY_WRITE_OCCURRED              0x0400

//
//  Flag that indicates this structure is only a cursor, only the
//  SharedCacheMapLinks and Flags are valid!
//

#define IS_CURSOR                        0x0800

//
//  Flag that indicates that we have seen someone cache this file
//  and specify FO_RANDOM_ACCESS.  This will deactivate our cache
//  working set trim assist.
//

#define RANDOM_ACCESS_SEEN               0x1000

//
//  Cursor structure for traversing the SharedCacheMap lists.  Anyone
//  scanning these lists must verify that the IS_CURSOR flag is clear
//  before looking at other SharedCacheMap fields.
//


typedef struct _SHARED_CACHE_MAP_LIST_CURSOR {

    //
    //  Links for Global SharedCacheMap List
    //

    LIST_ENTRY SharedCacheMapLinks;

    //
    //  Shared Cache Map flags, IS_CURSOR must be set.
    //

    ULONG Flags;

} SHARED_CACHE_MAP_LIST_CURSOR, *PSHARED_CACHE_MAP_LIST_CURSOR;



#ifndef KDEXT
//
//  Bitmap Range structure.  For small files there is just one embedded in the
//  Mbcb.  For large files there may be many of these linked to the Mbcb.
//

typedef struct _BITMAP_RANGE {

    //
    //  Links for the list of bitmap ranges off the Mbcb.
    //

    LIST_ENTRY Links;

    //
    //  Base page (FileOffset / PAGE_SIZE) represented by this range.
    //  (Size is a fixed maximum.)
    //

    LONGLONG BasePage;

    //
    //  First and Last dirty pages relative to the BasePage.
    //

    ULONG FirstDirtyPage;
    ULONG LastDirtyPage;

    //
    //  Number of dirty pages in this range.
    //

    ULONG DirtyPages;

    //
    //  Pointer to the bitmap for this range.
    //

    PULONG Bitmap;

} BITMAP_RANGE, *PBITMAP_RANGE;
#endif

//
//  This structure is a "mask" Bcb.  For fast simple write operations,
//  a mask Bcb is used so that we basically only have to set bits to remember
//  where the dirty data is.
//

typedef struct _MBCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeIsInZone;

    //
    //  This field is used as a scratch area for the Lazy Writer to
    //  guide how much he will write each time he wakes up.
    //

    ULONG PagesToWrite;

    //
    //  Number of dirty pages (set bits) in the bitmap below.
    //

    ULONG DirtyPages;

    //
    //  Reserved for alignment.
    //

    ULONG Reserved;

    //
    //  ListHead of Bitmap ranges.
    //

    LIST_ENTRY BitmapRanges;

    //
    //  This is a hint on where to resume writing, since we will not
    //  always write all of the dirty data at once.
    //

    LONGLONG ResumeWritePage;

    //
    //  Initial three embedded Bitmap ranges.  For a file up to 2MB, only the
    //  first range is used, and the rest of the Mbcb contains bits for 2MB of
    //  dirty pages (4MB on Alpha).  For larger files, all three ranges may
    //  be used to describe external bitmaps.
    //

    BITMAP_RANGE BitmapRange1;
    BITMAP_RANGE BitmapRange2;
    BITMAP_RANGE BitmapRange3;

} MBCB;

typedef MBCB *PMBCB;


//
//  This is the Buffer Control Block structure for representing data which
//  is "pinned" in memory by one or more active requests and/or dirty.  This
//  structure is created the first time that a call to CcPinFileData specifies
//  a particular integral range of pages.  It is deallocated whenever the Pin
//  Count reaches 0 and the Bcb is not Dirty.
//
//  NOTE: The first four fields must be the same as the PUBLIC_BCB.
//

typedef struct _BCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeIsInZone;

    //
    //  Byte FileOffset and and length of entire buffer
    //

    ULONG  ByteLength;
    LARGE_INTEGER FileOffset;

    //
    //  Links for BcbList in SharedCacheMap
    //

    LIST_ENTRY BcbLinks;

    //
    //  Byte FileOffset of last byte in buffer (used for searching)
    //

    LARGE_INTEGER BeyondLastByte;

    //
    //  Oldest Lsn (if specified) when this buffer was set dirty.
    //

    LARGE_INTEGER OldestLsn;

    //
    //  Most recent Lsn specified when this buffer was set dirty.
    //  The FlushToLsnRoutine is called with this Lsn.
    //

    LARGE_INTEGER NewestLsn;

    //
    //  Pointer to Vacb via which this Bcb is mapped.
    //

    PVACB Vacb;

    //
    //  Links and caller addresses for the global Bcb list (for debug only)
    //

#if LIST_DBG
    LIST_ENTRY CcBcbLinks;
    PVOID CallerAddress;
    PVOID CallersCallerAddress;
#endif

    //
    //  Count of threads actively using this Bcb to process a request.
    //  This must be manipulated under protection of the BcbListSpinLock
    //  in the SharedCacheMap.
    //

    ULONG PinCount;

    //
    //  Resource to synchronize buffer access.  Pinning Readers and all Writers
    //  of the described buffer take out shared access (synchronization of
    //  buffer modifications is strictly up to the caller).  Note that pinning
    //  readers do not declare if they are going to modify the buffer or not.
    //  Anyone writing to disk takes out exclusive access, to prevent the buffer
    //  from changing while it is being written out.
    //

    ERESOURCE Resource;

    //
    //  Pointer to SharedCacheMap for this Bcb.
    //

    PSHARED_CACHE_MAP SharedCacheMap;

    //
    //  This is the Base Address at which the buffer can be seen in
    //  system space.  All access to buffer data should go through this
    //  address.
    //

    PVOID BaseAddress;

    //
    //  Flags
    //

    BOOLEAN Dirty;

} BCB;

#ifndef KDEXT
typedef BCB *PBCB;
#endif

//
//  This is the Overlap Buffer Control Block structure for representing data which
//  is "pinned" in memory and must be represented by multiple Bcbs due to overlaps.
//
//  NOTE: The first four fields must be the same as the PUBLIC_BCB.
//

typedef struct _OBCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Byte FileOffset and and length of entire buffer
    //

    ULONG  ByteLength;
    LARGE_INTEGER FileOffset;

    //
    //  Vector of Bcb pointers.
    //

    PBCB Bcbs[ANYSIZE_ARRAY];

} OBCB;

typedef OBCB *POBCB;


//
//  Struct for remembering deferred writes for later posting.
//

typedef struct _DEFERRED_WRITE {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  The file to be written.
    //

    PFILE_OBJECT FileObject;

    //
    //  Number of bytes the caller intends to write
    //

    ULONG BytesToWrite;

    //
    //  Links for the deferred write queue.
    //

    LIST_ENTRY DeferredWriteLinks;

    //
    //  If this event pointer is not NULL, then this event will
    //  be signalled when the write is ok, rather than calling
    //  the PostRoutine below.
    //

    PKEVENT Event;

    //
    //  The posting routine and its parameters
    //

    PCC_POST_DEFERRED_WRITE PostRoutine;
    PVOID Context1;
    PVOID Context2;

    BOOLEAN LimitModifiedPages;

} DEFERRED_WRITE, *PDEFERRED_WRITE;


//
//  Struct controlling the Lazy Writer algorithms
//

typedef struct _LAZY_WRITER {

    //
    //  A few Mm routines still require a process.
    //

    PEPROCESS OurProcess;

    //
    //  Work queue.
    //

    LIST_ENTRY WorkQueue;

    //
    //  Zone for Bcbs.
    //

    ZONE_HEADER BcbZone;

    //
    //  Dpc and Timer Structures used for activating periodic scan when active.
    //

    KDPC ScanDpc;
    KTIMER ScanTimer;

    //
    //  Boolean to say whether Lazy Writer scan is active or not.
    //

    BOOLEAN ScanActive;

    //
    //  Boolean indicating if there is any other reason for Lazy Writer to
    //  wake up.
    //

    BOOLEAN OtherWork;

} LAZY_WRITER;


#ifndef KDEXT
//
//  Work queue entry for the worker threads, with an enumerated
//  function code.
//

typedef enum _WORKER_FUNCTION {
    Noop = 0,
    ReadAhead,
    WriteBehind,
    LazyWriteScan,
    EventSet
    } WORKER_FUNCTION;
#endif

typedef struct _WORK_QUEUE_ENTRY {

    //
    //  List entry for our work queues.
    //

    LIST_ENTRY WorkQueueLinks;

    //
    //  Define a union to contain function-specific parameters.
    //

    union {

        //
        //  Read parameters (for read ahead)
        //

        struct {
            PFILE_OBJECT FileObject;
        } Read;

        //
        //  Write parameters (for write behind)
        //

        struct {
            PSHARED_CACHE_MAP SharedCacheMap;
        } Write;
        
        //
        //  Set event parameters (for queue checks)
        //

        struct {
            PKEVENT Event;
        } Event;

    } Parameters;

    //
    //  Function code for this entry:
    //

    UCHAR Function;

} WORK_QUEUE_ENTRY, *PWORK_QUEUE_ENTRY;

//
//  This is a structure apended to the end of an MDL
//

typedef struct _MDL_WRITE {

    //
    //  This field is for the use of the Server to stash anything interesting
    //

    PVOID ServerContext;

    //
    //  This is the resource to release when the write is complete.
    //

    PERESOURCE Resource;

    //
    //  This is thread caller's thread, and the thread that must release
    //  the resource.
    //

    ERESOURCE_THREAD Thread;

    //
    //  This links all the pending MDLs through the shared cache map.
    //

    LIST_ENTRY MdlLinks;

} MDL_WRITE, *PMDL_WRITE;


//
//  Common Private routine definitions for the Cache Manager
//

#define GetActiveVacb(SCM,IRQ,V,P,D) {                                  \
    ExAcquireFastLock(&(SCM)->ActiveVacbSpinLock, &(IRQ));              \
    (V) = (SCM)->ActiveVacb;                                            \
    if ((V) != NULL) {                                                  \
        (P) = (SCM)->ActivePage;                                        \
        (SCM)->ActiveVacb = NULL;                                       \
        (D) = (SCM)->Flags & ACTIVE_PAGE_IS_DIRTY;                      \
    }                                                                   \
    ExReleaseFastLock(&(SCM)->ActiveVacbSpinLock, (IRQ));               \
}

#define GetActiveVacbAtDpcLevel(SCM,V,P,D) {                            \
    ExAcquireSpinLockAtDpcLevel(&(SCM)->ActiveVacbSpinLock);            \
    (V) = (SCM)->ActiveVacb;                                            \
    if ((V) != NULL) {                                                  \
        (P) = (SCM)->ActivePage;                                        \
        (SCM)->ActiveVacb = NULL;                                       \
        (D) = (SCM)->Flags & ACTIVE_PAGE_IS_DIRTY;                      \
    }                                                                   \
    ExReleaseSpinLockFromDpcLevel(&(SCM)->ActiveVacbSpinLock);          \
}

//
//  When setting dirty, when we set ACTIVE_PAGE_IS_DIRTY the first time,
//  we increment the dirty counts, and they never get decremented until
//  CcFreeActiveVacb.  If we are trying to set and there is already an
//  active Vacb *or* we are trying to set a clean one and the flag above
//  is set, we do not allow it, and we just free the vacb (we only want
//  to handle the clean transition in one place).
//
//  MP & UP cases are separately defined, because I do not trust the compiler
//  to otherwise generate the optimal UP code.
//


//
//  In the MP case, we test if we are setting the page dirty, because then
//  we must acquire CcMasterSpinLock to diddle CcDirtyPages.
//

#if !defined(NT_UP)                                                                     \

#define SetActiveVacb(SCM,IRQ,V,P,D) {                                                  \
    if (D) {                                                                            \
        CcAcquireMasterLock(&(IRQ));                                                    \
        ExAcquireSpinLockAtDpcLevel(&(SCM)->ActiveVacbSpinLock);                        \
    } else {                                                                            \
        ExAcquireSpinLock(&(SCM)->ActiveVacbSpinLock, &(IRQ));                          \
    }                                                                                   \
    do {                                                                                \
        if ((SCM)->ActiveVacb == NULL) {                                                \
            if (((SCM)->Flags & ACTIVE_PAGE_IS_DIRTY) != (D)) {                         \
                if (D) {                                                                \
                    (SCM)->ActiveVacb = (V);                                            \
                    (SCM)->ActivePage = (P);                                            \
                    (V) = NULL;                                                         \
                    SetFlag((SCM)->Flags, ACTIVE_PAGE_IS_DIRTY);                        \
                    CcTotalDirtyPages += 1;                                             \
                    (SCM)->DirtyPages += 1;                                             \
                    if ((SCM)->DirtyPages == 1) {                                       \
                        PLIST_ENTRY Blink;                                              \
                        PLIST_ENTRY Entry;                                              \
                        PLIST_ENTRY Flink;                                              \
                        PLIST_ENTRY Head;                                               \
                        Entry = &(SCM)->SharedCacheMapLinks;                            \
                        Blink = Entry->Blink;                                           \
                        Flink = Entry->Flink;                                           \
                        Blink->Flink = Flink;                                           \
                        Flink->Blink = Blink;                                           \
                        Head = &CcDirtySharedCacheMapList.SharedCacheMapLinks;          \
                        Blink = Head->Blink;                                            \
                        Entry->Flink = Head;                                            \
                        Entry->Blink = Blink;                                           \
                        Blink->Flink = Entry;                                           \
                        Head->Blink = Entry;                                            \
                        if (!LazyWriter.ScanActive) {                                   \
                            LazyWriter.ScanActive = TRUE;                               \
                            ExReleaseSpinLockFromDpcLevel(&(SCM)->ActiveVacbSpinLock);  \
                            CcReleaseMasterLock((IRQ));                                 \
                            KeSetTimer( &LazyWriter.ScanTimer,                          \
                                        CcFirstDelay,                                   \
                                        &LazyWriter.ScanDpc );                          \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                }                                                                       \
            } else {                                                                    \
                (SCM)->ActiveVacb = (V);                                                \
                (SCM)->ActivePage = (P);                                                \
                (V) = NULL;                                                             \
            }                                                                           \
        }                                                                               \
        if (D) {                                                                        \
            ExReleaseSpinLockFromDpcLevel(&(SCM)->ActiveVacbSpinLock);                  \
            CcReleaseMasterLock((IRQ));                                                 \
        } else {                                                                        \
            ExReleaseSpinLock(&(SCM)->ActiveVacbSpinLock, (IRQ));                       \
        }                                                                               \
        if ((V) != NULL) {                                                              \
            CcFreeActiveVacb( (SCM), (V), (P), (D));                                    \
        }                                                                               \
    } while (FALSE);                                                                    \
}

//
//  In the UP case, any FastLock will do, so we just use the ActiveVacb lock, and do not
//  explicitly acquire CcMasterSpinLock.
//

#else

#define SetActiveVacb(SCM,IRQ,V,P,D) {                                                  \
    ExAcquireFastLock(&(SCM)->ActiveVacbSpinLock, &(IRQ));                              \
    do {                                                                                \
        if ((SCM)->ActiveVacb == NULL) {                                                \
            if (((SCM)->Flags & ACTIVE_PAGE_IS_DIRTY) != (D)) {                         \
                if (D) {                                                                \
                    (SCM)->ActiveVacb = (V);                                            \
                    (SCM)->ActivePage = (P);                                            \
                    (V) = NULL;                                                         \
                    SetFlag((SCM)->Flags, ACTIVE_PAGE_IS_DIRTY);                        \
                    CcTotalDirtyPages += 1;                                             \
                    (SCM)->DirtyPages += 1;                                             \
                    if ((SCM)->DirtyPages == 1) {                                       \
                        PLIST_ENTRY Blink;                                              \
                        PLIST_ENTRY Entry;                                              \
                        PLIST_ENTRY Flink;                                              \
                        PLIST_ENTRY Head;                                               \
                        Entry = &(SCM)->SharedCacheMapLinks;                            \
                        Blink = Entry->Blink;                                           \
                        Flink = Entry->Flink;                                           \
                        Blink->Flink = Flink;                                           \
                        Flink->Blink = Blink;                                           \
                        Head = &CcDirtySharedCacheMapList.SharedCacheMapLinks;          \
                        Blink = Head->Blink;                                            \
                        Entry->Flink = Head;                                            \
                        Entry->Blink = Blink;                                           \
                        Blink->Flink = Entry;                                           \
                        Head->Blink = Entry;                                            \
                        if (!LazyWriter.ScanActive) {                                   \
                            LazyWriter.ScanActive = TRUE;                               \
                            ExReleaseFastLock(&(SCM)->ActiveVacbSpinLock, (IRQ));       \
                            KeSetTimer( &LazyWriter.ScanTimer,                          \
                                        CcFirstDelay,                                   \
                                        &LazyWriter.ScanDpc );                          \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                }                                                                       \
            } else {                                                                    \
                (SCM)->ActiveVacb = (V);                                                \
                (SCM)->ActivePage = (P);                                                \
                (V) = NULL;                                                             \
            }                                                                           \
        }                                                                               \
        ExReleaseFastLock(&(SCM)->ActiveVacbSpinLock, (IRQ));                           \
        if ((V) != NULL) {                                                              \
            CcFreeActiveVacb( (SCM), (V), (P), (D));                                    \
        }                                                                               \
    } while (FALSE);                                                                    \
}

#endif

VOID
CcPostDeferredWrites (
    );

BOOLEAN
CcPinFileData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN WriteOnly,
    IN ULONG Flags,
    OUT PBCB *Bcb,
    OUT PVOID *BaseAddress,
    OUT PLARGE_INTEGER BeyondLastByte
    );

typedef enum {
    UNPIN,
    UNREF,
    SET_CLEAN
} UNMAP_ACTIONS;

VOID
FASTCALL
CcUnpinFileData (
    IN OUT PBCB Bcb,
    IN BOOLEAN ReadOnly,
    IN UNMAP_ACTIONS UnmapAction
    );

VOID
FASTCALL
CcDeallocateBcb (
    IN PBCB Bcb
    );

VOID
FASTCALL
CcPerformReadAhead (
    IN PFILE_OBJECT FileObject
    );

VOID
CcSetDirtyInMask (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length
    );

VOID
FASTCALL
CcWriteBehind (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PIO_STATUS_BLOCK IoStatus
    );

#define ZERO_FIRST_PAGE                  1
#define ZERO_MIDDLE_PAGES                2
#define ZERO_LAST_PAGE                   4

BOOLEAN
CcMapAndRead(
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG ZeroFlags,
    IN BOOLEAN Wait,
    IN PVOID BaseAddress
    );

VOID
CcFreeActiveVacb (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB ActiveVacb OPTIONAL,
    IN ULONG ActivePage,
    IN ULONG PageIsDirty
    );

VOID
CcMapAndCopy(
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVOID UserBuffer,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG ZeroFlags,
    IN BOOLEAN WriteThrough
    );

VOID
CcScanDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
CcScheduleLazyWriteScan (
    );

VOID
CcStartLazyWriter (
    IN PVOID NotUsed
    );

#define CcAllocateWorkQueueEntry() \
    (PWORK_QUEUE_ENTRY)ExAllocateFromPPNPagedLookasideList(LookasideTwilightList)

#define CcFreeWorkQueueEntry(_entry_)         \
    ExFreeToPPNPagedLookasideList(LookasideTwilightList, (_entry_))

VOID
FASTCALL
CcPostWorkQueue (
    IN PWORK_QUEUE_ENTRY WorkQueueEntry,
    IN PLIST_ENTRY WorkQueue
    );

VOID
CcWorkerThread (
    PVOID ExWorkQueueItem
    );

VOID
FASTCALL
CcDeleteSharedCacheMap (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN KIRQL ListIrql,
    IN ULONG ReleaseFile
    );

//
//  This exception filter handles STATUS_IN_PAGE_ERROR correctly
//

LONG
CcCopyReadExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN PNTSTATUS ExceptionCode
    );

//
//  Exception filter for Worker Threads in lazyrite.c
//

LONG
CcExceptionFilter (
    IN NTSTATUS ExceptionCode
    );

#ifdef CCDBG
VOID
CcDump (
    IN PVOID Ptr
    );
#endif

//
//  Vacb routines
//

VOID
CcInitializeVacbs(
    );

PVOID
CcGetVirtualAddressIfMapped (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    OUT PVACB *Vacb,
    OUT PULONG ReceivedLength
    );

PVOID
CcGetVirtualAddress (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset,
    OUT PVACB *Vacb,
    OUT PULONG ReceivedLength
    );

VOID
FASTCALL
CcFreeVirtualAddress (
    IN PVACB Vacb
    );

VOID
CcReferenceFileOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset
    );

VOID
CcDereferenceFileOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset
    );

VOID
CcWaitOnActiveCount (
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

VOID
FASTCALL
CcCreateVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    );

VOID
CcExtendVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    );

BOOLEAN
FASTCALL
CcUnmapVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN UnmapBehind
    );

VOID
CcAdjustVacbLevelLockCount (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN LONG Adjustment
    );

PLIST_ENTRY
CcGetBcbListHeadLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN BOOLEAN FailToSuccessor
    );

ULONG
CcPrefillVacbLevelZone (
    IN ULONG NumberNeeded,
    OUT PKIRQL OldIrql,
    IN ULONG NeedBcbListHeads
    );

VOID
CcDrainVacbLevelZone (
    );

//
//  Define references to global data
//

extern KSPIN_LOCK CcMasterSpinLock;
extern KSPIN_LOCK CcBcbSpinLock;
extern LIST_ENTRY CcCleanSharedCacheMapList;
extern SHARED_CACHE_MAP_LIST_CURSOR CcDirtySharedCacheMapList;
extern SHARED_CACHE_MAP_LIST_CURSOR CcLazyWriterCursor;
extern NPAGED_LOOKASIDE_LIST CcTwilightLookasideList;
extern KSPIN_LOCK CcWorkQueueSpinlock;
extern ULONG CcNumberWorkerThreads;
extern ULONG CcNumberActiveWorkerThreads;
extern LIST_ENTRY CcIdleWorkerThreadList;
extern LIST_ENTRY CcExpressWorkQueue;
extern LIST_ENTRY CcRegularWorkQueue;
extern LIST_ENTRY CcPostTickWorkQueue;
extern BOOLEAN CcQueueThrottle;
extern ULONG CcIdleDelayTick;
extern LARGE_INTEGER CcNoDelay;
extern LARGE_INTEGER CcFirstDelay;
extern LARGE_INTEGER CcIdleDelay;
extern LARGE_INTEGER CcCollisionDelay;
extern LARGE_INTEGER CcTargetCleanDelay;
extern LAZY_WRITER LazyWriter;
extern KSPIN_LOCK CcVacbSpinLock;
extern ULONG CcNumberVacbs;
extern PVACB CcVacbs;
extern PVACB CcBeyondVacbs;
extern LIST_ENTRY CcVacbLru;
extern KSPIN_LOCK CcDeferredWriteSpinLock;
extern LIST_ENTRY CcDeferredWrites;
extern ULONG CcDirtyPageThreshold;
extern ULONG CcDirtyPageTarget;
extern ULONG CcDirtyPagesLastScan;
extern ULONG CcPagesYetToWrite;
extern ULONG CcPagesWrittenLastTime;
extern ULONG CcAvailablePagesThreshold;
extern ULONG CcTotalDirtyPages;
extern ULONG CcTune;
extern LONG CcAggressiveZeroCount;
extern LONG CcAggressiveZeroThreshold;
extern ULONG CcLazyWriteHotSpots;
extern MM_SYSTEMSIZE CcCapturedSystemSize;
extern ULONG CcMaxVacbLevelsSeen;
extern ULONG CcVacbLevelEntries;
extern PVACB *CcVacbLevelFreeList;
extern ULONG CcVacbLevelWithBcbsEntries;
extern PVACB *CcVacbLevelWithBcbsFreeList;

//
//  Macros for allocating and deallocating Vacb levels - CcVacbSpinLock must
//  be acquired.
//

_inline PVACB *CcAllocateVacbLevel (
    IN BOOLEAN AllocatingBcbListHeads
    )

{
    PVACB *ReturnEntry;

    if (AllocatingBcbListHeads) {
        ReturnEntry = CcVacbLevelWithBcbsFreeList;
        CcVacbLevelWithBcbsFreeList = (PVACB *)*ReturnEntry;
        CcVacbLevelWithBcbsEntries -= 1;
    } else {
        ReturnEntry = CcVacbLevelFreeList;
        CcVacbLevelFreeList = (PVACB *)*ReturnEntry;
        CcVacbLevelEntries -= 1;
    }
    *ReturnEntry = NULL;
    ASSERT(RtlCompareMemory(ReturnEntry, ReturnEntry + 1, VACB_LEVEL_BLOCK_SIZE - sizeof(PVACB)) ==
                                                          (VACB_LEVEL_BLOCK_SIZE - sizeof(PVACB)));
    return ReturnEntry;
}

_inline VOID CcDeallocateVacbLevel (
    IN PVACB *Entry,
    IN BOOLEAN DeallocatingBcbListHeads
    )

{
    if (DeallocatingBcbListHeads) {
        *Entry = (PVACB)CcVacbLevelWithBcbsFreeList;
        CcVacbLevelWithBcbsFreeList = Entry;
        CcVacbLevelWithBcbsEntries += 1;
    } else {
        *Entry = (PVACB)CcVacbLevelFreeList;
        CcVacbLevelFreeList = Entry;
        CcVacbLevelEntries += 1;
    }
}

//
//  Export the macros for inspecting the reference counts for
//  the multilevel Vacb array.
//

_inline
PVACB_LEVEL_REFERENCE
VacbLevelReference (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB *VacbArray,
    IN ULONG Level
    )
{
    return (PVACB_LEVEL_REFERENCE)
           ((PCHAR)VacbArray +
            VACB_LEVEL_BLOCK_SIZE +
            (Level != 0?
             0 : (FlagOn( SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED )?
                  VACB_LEVEL_BLOCK_SIZE : 0)));
}

_inline
ULONG
IsVacbLevelReferenced (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB *VacbArray,
    IN ULONG Level
    )
{
    PVACB_LEVEL_REFERENCE VacbReference = VacbLevelReference( SharedCacheMap, VacbArray, Level );

    return VacbReference->Reference | VacbReference->SpecialReference;
}


//
//  Here is a page of macros stolen directly from Pinball...
//

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

#ifdef CCDBG

extern LONG CcDebugTraceLevel;
extern LONG CcDebugTraceIndent;

#ifndef CCDBG_LOCK

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {                  \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y,Z);                                   \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    LONG _i;                                               \
    VOID CcDump();                                         \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {CcDump(PTR);}                    \
        DbgBreakPoint();                                   \
    }                                                      \
}

#else //  ndef CCDBG_LOCK

extern KSPIN_LOCK CcDebugTraceLock;

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {                  \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y,Z);                                   \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
      ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    VOID CcDump();                                         \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
      ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {CcDump(PTR);}                    \
        DbgBreakPoint();                                   \
      ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#endif //  else ndef CCDBG_LOCK

#else

#undef CCDBG_LOCK

#define DebugTrace(INDENT,LEVEL,X,Y) {NOTHING;}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {NOTHING;}

#define DebugDump(STR,LEVEL,PTR) {NOTHING;}

#endif //  CCDBG

//
//  Global list of pinned Bcbs which may be examined for debug purposes
//

#if DBG

extern ULONG CcBcbCount;
extern LIST_ENTRY CcBcbList;

#endif

#endif  //  _CCh_
