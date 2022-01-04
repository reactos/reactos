#pragma once

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/
#define MM_SEGMENT_FINALIZE (0x40000000)


#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

/* Determine what's needed to make paged pool fit in this category.
 * it seems that something more is required to satisfy arm3. */
#define BALANCER_CAN_EVICT(Consumer) \
    (((Consumer) == MC_USER) || \
     ((Consumer) == MC_CACHE))

#define SEC_CACHE                           (0x20000000)

/* We store 8 bits of location with a page association */
#define ENTRIES_PER_ELEMENT 256

extern KEVENT MmWaitPageEvent;

typedef struct _CACHE_SECTION_PAGE_TABLE
{
    LARGE_INTEGER FileOffset;
    PMM_SECTION_SEGMENT Segment;
    ULONG Refcount;
    ULONG_PTR PageEntries[ENTRIES_PER_ELEMENT];
} CACHE_SECTION_PAGE_TABLE, *PCACHE_SECTION_PAGE_TABLE;

struct _MM_REQUIRED_RESOURCES;

typedef NTSTATUS (NTAPI * AcquireResource)(
     PMMSUPPORT AddressSpace,
     struct _MEMORY_AREA *MemoryArea,
     struct _MM_REQUIRED_RESOURCES *Required);

typedef NTSTATUS (NTAPI * NotPresentFaultHandler)(
    PMMSUPPORT AddressSpace,
    struct _MEMORY_AREA *MemoryArea,
    PVOID Address,
    BOOLEAN Locked,
    struct _MM_REQUIRED_RESOURCES *Required);

typedef NTSTATUS (NTAPI * FaultHandler)(
    PMMSUPPORT AddressSpace,
    struct _MEMORY_AREA *MemoryArea,
    PVOID Address,
    struct _MM_REQUIRED_RESOURCES *Required);

typedef struct _MM_REQUIRED_RESOURCES
{
    ULONG Consumer;
    ULONG Amount;
    ULONG Offset;
    ULONG State;
    PVOID Context;
    LARGE_INTEGER FileOffset;
    AcquireResource DoAcquisition;
    PFN_NUMBER Page[2];
    PVOID Buffer[2];
    SWAPENTRY SwapEntry;
    const char *File;
    int Line;
} MM_REQUIRED_RESOURCES, *PMM_REQUIRED_RESOURCES;

NTSTATUS
NTAPI
MmCreateCacheSection(PSECTION *SectionObject,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PLARGE_INTEGER UMaximumSize,
                     ULONG SectionPageProtection,
                     ULONG AllocationAttributes,
                     PFILE_OBJECT FileObject);

PFN_NUMBER
NTAPI
MmWithdrawSectionPage(PMM_SECTION_SEGMENT Segment,
                      PLARGE_INTEGER FileOffset,
                      BOOLEAN *Dirty);

NTSTATUS
NTAPI
MmFinalizeSectionPageOut(PMM_SECTION_SEGMENT Segment,
                         PLARGE_INTEGER FileOffset,
                         PFN_NUMBER Page,
                         BOOLEAN Dirty);

/* sptab.c *******************************************************************/

VOID
NTAPI
MiInitializeSectionPageTable(PMM_SECTION_SEGMENT Segment);

typedef VOID (NTAPI *FREE_SECTION_PAGE_FUN)(
    PMM_SECTION_SEGMENT Segment,
    PLARGE_INTEGER Offset);

VOID
NTAPI
MmFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment,
                               FREE_SECTION_PAGE_FUN FreePage);

NTSTATUS
NTAPI
MmSetSectionAssociation(PFN_NUMBER Page,
                        PMM_SECTION_SEGMENT Segment,
                        PLARGE_INTEGER Offset);

VOID
NTAPI
MmDeleteSectionAssociation(PFN_NUMBER Page);

NTSTATUS
NTAPI
MmpPageOutPhysicalAddress(PFN_NUMBER Page);

/* io.c **********************************************************************/

NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File);

NTSTATUS
NTAPI
MiSimpleRead(PFILE_OBJECT FileObject,
             PLARGE_INTEGER FileOffset,
             PVOID Buffer,
             ULONG Length,
             BOOLEAN Paging,
             PIO_STATUS_BLOCK ReadStatus);

NTSTATUS
NTAPI
_MiSimpleWrite(PFILE_OBJECT FileObject,
               PLARGE_INTEGER FileOffset,
               PVOID Buffer,
               ULONG Length,
               PIO_STATUS_BLOCK ReadStatus,
               const char *file,
               int line);

#define MiSimpleWrite(F,O,B,L,R) _MiSimpleWrite(F,O,B,L,R,__FILE__,__LINE__)

NTSTATUS
NTAPI
_MiWriteBackPage(PFILE_OBJECT FileObject,
                 PLARGE_INTEGER Offset,
                 ULONG Length,
                 PFN_NUMBER Page,
                 const char *File,
                 int Line);

#define MiWriteBackPage(F,O,L,P) _MiWriteBackPage(F,O,L,P,__FILE__,__LINE__)

/* section.c *****************************************************************/

NTSTATUS
NTAPI
MmAccessFaultCacheSection(KPROCESSOR_MODE Mode,
                          ULONG_PTR Address,
                          BOOLEAN FromMdl);

NTSTATUS
NTAPI
MiReadFilePage(PMMSUPPORT AddressSpace,
               PMEMORY_AREA MemoryArea,
               PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiGetOnePage(PMMSUPPORT AddressSpace,
             PMEMORY_AREA MemoryArea,
             PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiSwapInPage(PMMSUPPORT AddressSpace,
             PMEMORY_AREA MemoryArea,
             PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiWriteSwapPage(PMMSUPPORT AddressSpace,
                PMEMORY_AREA MemoryArea,
                PMM_REQUIRED_RESOURCES Resources);

NTSTATUS
NTAPI
MiWriteFilePage(PMMSUPPORT AddressSpace,
                PMEMORY_AREA MemoryArea,
                PMM_REQUIRED_RESOURCES Resources);

VOID
NTAPI
MiFreeSegmentPage(PMM_SECTION_SEGMENT Segment,
                  PLARGE_INTEGER FileOffset);

_Success_(1)
_When_(return==STATUS_MORE_PROCESSING_REQUIRED, _At_(Required->DoAcquisition, _Post_notnull_))
NTSTATUS
NTAPI
MiCowCacheSectionPage (
    _In_ PMMSUPPORT AddressSpace,
    _In_ PMEMORY_AREA MemoryArea,
    _In_ PVOID Address,
    _In_ BOOLEAN Locked,
    _Inout_ PMM_REQUIRED_RESOURCES Required);

VOID
MmPageOutDeleteMapping(PVOID Context,
                       PEPROCESS Process,
                       PVOID Address);

VOID
MmFreeCacheSectionPage(PVOID Context,
                       MEMORY_AREA* MemoryArea,
                       PVOID Address,
                       PFN_NUMBER Page,
                       SWAPENTRY SwapEntry,
                       BOOLEAN Dirty);

NTSTATUS
NTAPI
_MiFlushMappedSection(PVOID BaseAddress,
                      PLARGE_INTEGER BaseOffset,
                      PLARGE_INTEGER FileSize,
                      BOOLEAN Dirty,
                      const char *File,
                      int Line);

#define MiFlushMappedSection(A,O,S,D) _MiFlushMappedSection(A,O,S,D,__FILE__,__LINE__)

VOID
NTAPI
MmFinalizeSegment(PMM_SECTION_SEGMENT Segment);

VOID
NTAPI
MmFreeSectionSegments(PFILE_OBJECT FileObject);

NTSTATUS
NTAPI
MmMapCacheViewInSystemSpaceAtOffset(IN PMM_SECTION_SEGMENT Segment,
                                    OUT PVOID* MappedBase,
                                    IN PLARGE_INTEGER ViewOffset,
                                    IN OUT PULONG ViewSize);

NTSTATUS
NTAPI
_MiMapViewOfSegment(PMMSUPPORT AddressSpace,
                    PMM_SECTION_SEGMENT Segment,
                    PVOID* BaseAddress,
                    SIZE_T ViewSize,
                    ULONG Protect,
                    PLARGE_INTEGER ViewOffset,
                    ULONG AllocationType,
                    const char *file,
                    int line);

#define MiMapViewOfSegment(AddressSpace,Segment,BaseAddress,ViewSize,Protect,ViewOffset,AllocationType) \
    _MiMapViewOfSegment(AddressSpace,Segment,BaseAddress,ViewSize,Protect,ViewOffset,AllocationType,__FILE__,__LINE__)

NTSTATUS
NTAPI
MmUnmapViewOfCacheSegment(PMMSUPPORT AddressSpace,
                          PVOID BaseAddress);

NTSTATUS
NTAPI
MmUnmapCacheViewInSystemSpace(PVOID Address);

_Success_(1)
_When_(return==STATUS_MORE_PROCESSING_REQUIRED, _At_(Required->DoAcquisition, _Post_notnull_))
NTSTATUS
NTAPI
MmNotPresentFaultCachePage (
    _In_ PMMSUPPORT AddressSpace,
    _In_ MEMORY_AREA* MemoryArea,
    _In_ PVOID Address,
    _In_ BOOLEAN Locked,
    _Inout_ PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MmPageOutPageFileView(PMMSUPPORT AddressSpace,
                      PMEMORY_AREA MemoryArea,
                      PVOID Address,
                      PMM_REQUIRED_RESOURCES Required);

FORCEINLINE
BOOLEAN
_MmTryToLockAddressSpace(IN PMMSUPPORT AddressSpace,
                         const char *file,
                         int line)
{
    BOOLEAN Result = KeTryToAcquireGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
    //DbgPrint("(%s:%d) Try Lock Address Space %x -> %s\n", file, line, AddressSpace, Result ? "true" : "false");
    return Result;
}

#define MmTryToLockAddressSpace(x) _MmTryToLockAddressSpace(x,__FILE__,__LINE__)

NTSTATUS
NTAPI
MiWidenSegment(PMMSUPPORT AddressSpace,
               PMEMORY_AREA MemoryArea,
               PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiSwapInSectionPage(PMMSUPPORT AddressSpace,
                    PMEMORY_AREA MemoryArea,
                    PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MmExtendCacheSection(PSECTION Section,
                     PLARGE_INTEGER NewSize,
                     BOOLEAN ExtendFile);

NTSTATUS
NTAPI
_MiFlushMappedSection(PVOID BaseAddress,
                      PLARGE_INTEGER BaseOffset,
                      PLARGE_INTEGER FileSize,
                      BOOLEAN Dirty,
                      const char *File,
                      int Line);

#define MiFlushMappedSection(A,O,S,D) _MiFlushMappedSection(A,O,S,D,__FILE__,__LINE__)

PVOID
NTAPI
MmGetSegmentRmap(PFN_NUMBER Page,
                 PULONG RawOffset);

NTSTATUS
NTAPI
MmNotPresentFaultCacheSection(KPROCESSOR_MODE Mode,
                              ULONG_PTR Address,
                              BOOLEAN FromMdl);

ULONG
NTAPI
MiCacheEvictPages(PMM_SECTION_SEGMENT Segment,
                  ULONG Target);

NTSTATUS
MiRosTrimCache(ULONG Target,
               ULONG Priority,
               PULONG NrFreed);
