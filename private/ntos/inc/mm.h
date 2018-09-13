/*-- BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mm.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the memory management system.

Author:

    Lou Perazzoli (loup) 20-Mar-1989

Revision History:

--*/

#ifndef _MM_
#define _MM_

//
// Virtual bias applied when the kernel image was loaded.
//

extern ULONG_PTR MmVirtualBias;

#define MAX_PHYSICAL_MEMORY_FRAGMENTS 20

typedef struct _PHYSICAL_MEMORY_RUN {
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR {
    ULONG NumberOfRuns;
    PFN_NUMBER NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

//
// Physical memory blocks.
//

extern PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

//
// The allocation granularity is 64k.
//

#define MM_ALLOCATION_GRANULARITY ((ULONG)0x10000)

//
// Maximum read ahead size for cache operations.
//

#define MM_MAXIMUM_READ_CLUSTER_SIZE (15)

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
//  Indicates the system may do I/O to physical addresses above 4 GB.
//

extern PBOOLEAN Mm64BitPhysicalAddress;

// end_ntddk end_wdm end_nthal end_ntifs

#else

//
//  Indicates the system may do I/O to physical addresses above 4 GB.
//

extern BOOLEAN Mm64BitPhysicalAddress;

#endif

// begin_ntddk begin_wdm begin_nthal begin_ntifs

//
// Define maximum disk transfer size to be used by MM and Cache Manager,
// so that packet-oriented disk drivers can optimize their packet allocation
// to this size.
//

#define MM_MAXIMUM_DISK_IO_SIZE          (0x10000)

//++
//
// ULONG_PTR
// ROUND_TO_PAGES (
//     IN ULONG_PTR Size
//     )
//
// Routine Description:
//
//     The ROUND_TO_PAGES macro takes a size in bytes and rounds it up to a
//     multiple of the page size.
//
//     NOTE: This macro fails for values 0xFFFFFFFF - (PAGE_SIZE - 1).
//
// Arguments:
//
//     Size - Size in bytes to round up to a page multiple.
//
// Return Value:
//
//     Returns the size rounded up to a multiple of the page size.
//
//--

#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

//++
//
// ULONG
// BYTES_TO_PAGES (
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The BYTES_TO_PAGES macro takes the size in bytes and calculates the
//     number of pages required to contain the bytes.
//
// Arguments:
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages required to contain the specified size.
//
//--

#define BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_SHIFT) + \
                               (((ULONG)(Size) & (PAGE_SIZE - 1)) != 0))

//++
//
// ULONG
// BYTE_OFFSET (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The BYTE_OFFSET macro takes a virtual address and returns the byte offset
//     of that address within the page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the byte offset portion of the virtual address.
//
//--

#define BYTE_OFFSET(Va) ((ULONG)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))

//++
//
// PVOID
// PAGE_ALIGN (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The PAGE_ALIGN macro takes a virtual address and returns a page-aligned
//     virtual address for that page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the page aligned virtual address.
//
//--

#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

//++
//
// ULONG
// ADDRESS_AND_SIZE_TO_SPAN_PAGES (
//     IN PVOID Va,
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The ADDRESS_AND_SIZE_TO_SPAN_PAGES macro takes a virtual address and
//     size and returns the number of pages spanned by the size.
//
// Arguments:
//
//     Va - Virtual address.
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages spanned by the size.
//
//--

#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size) \
   (((((Size) - 1) >> PAGE_SHIFT) + \
   (((((ULONG)(Size-1)&(PAGE_SIZE-1)) + (PtrToUlong(Va) & (PAGE_SIZE -1)))) >> PAGE_SHIFT)) + 1L)

#define COMPUTE_PAGES_SPANNED(Va, Size) \
    ((ULONG)((((ULONG_PTR)(Va) & (PAGE_SIZE -1)) + (Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

// end_ntddk end_wdm end_nthal end_ntifs

//++
//
// BOOLEAN
// IS_SYSTEM_ADDRESS
//     IN PVOID Va,
//     )
//
// Routine Description:
//
//     This macro takes a virtual address and returns TRUE if the virtual address
//     is within system space, FALSE otherwise.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns TRUE is the address is in system space.
//
//--

#define IS_SYSTEM_ADDRESS(VA) ((VA) >= MM_SYSTEM_RANGE_START)

// begin_ntddk begin_wdm begin_nthal begin_ntifs

//++
// PPFN_NUMBER
// MmGetMdlPfnArray (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlPfnArray routine returns the virtual address of the
//     first element of the array of physical page numbers associated with
//     the MDL.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the first element of the array of
//     physical page numbers associated with the MDL.
//
//--

#define MmGetMdlPfnArray(Mdl) ((PPFN_NUMBER)(Mdl + 1))

//++
//
// PVOID
// MmGetMdlVirtualAddress (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlVirtualAddress returns the virtual address of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the buffer described by the Mdl
//
//--

#define MmGetMdlVirtualAddress(Mdl)                                     \
    ((PVOID) ((PCHAR) ((Mdl)->StartVa) + (Mdl)->ByteOffset))

//++
//
// ULONG
// MmGetMdlByteCount (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteCount returns the length in bytes of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte count of the buffer described by the Mdl
//
//--

#define MmGetMdlByteCount(Mdl)  ((Mdl)->ByteCount)

//++
//
// ULONG
// MmGetMdlByteOffset (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteOffset returns the byte offset within the page
//     of the buffer described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte offset within the page of the buffer described by the Mdl
//
//--

#define MmGetMdlByteOffset(Mdl)  ((Mdl)->ByteOffset)

//++
//
// PVOID
// MmGetMdlStartVa (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlBaseVa returns the virtual address of the buffer
//     described by the Mdl rounded down to the nearest page.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the returns the starting virtual address of the MDL.
//
//
//--

#define MmGetMdlBaseVa(Mdl)  ((Mdl)->StartVa)

// end_ntddk end_wdm end_nthal end_ntifs

//
// Section object type.
//

extern POBJECT_TYPE MmSectionObjectType;

//
// Number of pages to read in a single I/O if possible.
//

extern ULONG MmReadClusterSize;

//
// Number of colors in system.
//

extern ULONG MmNumberOfColors;

//
// Number of physical pages.
//

extern PFN_COUNT MmNumberOfPhysicalPages;


//
// Size of system cache in pages.
//

extern PFN_COUNT MmSizeOfSystemCacheInPages;

//
// System cache working set.
//

extern MMSUPPORT MmSystemCacheWs;

//
// Working set manager event.
//

extern KEVENT MmWorkingSetManagerEvent;

// begin_ntddk begin_wdm begin_nthal begin_ntifs
typedef enum _MM_SYSTEM_SIZE {
    MmSmallSystem,
    MmMediumSystem,
    MmLargeSystem
} MM_SYSTEMSIZE;

NTKERNELAPI
MM_SYSTEMSIZE
MmQuerySystemSize(
    VOID
    );

//  end_wdm

NTKERNELAPI
BOOLEAN
MmIsThisAnNtAsSystem(
    VOID
    );

//  begin_wdm

typedef enum _LOCK_OPERATION {
    IoReadAccess,
    IoWriteAccess,
    IoModifyAccess
} LOCK_OPERATION;

// end_ntddk end_wdm end_nthal end_ntifs

//
// NT product type.
//

extern ULONG MmProductType;

typedef struct _MMINFO_COUNTERS {
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
} MMINFO_COUNTERS;

typedef MMINFO_COUNTERS *PMMINFO_COUNTERS;

extern MMINFO_COUNTERS MmInfoCounters;



//
// Memory management initialization routine (for both phases).
//

BOOLEAN
MmInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock
    );

VOID
MmInitializeMemoryLimits (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludedType,
    OUT PPHYSICAL_MEMORY_DESCRIPTOR Memory
    );

VOID
MmFreeLoaderBlock (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MmEnablePAT (
    VOID
    );

PVOID
MmAllocateIndependentPages(
    IN SIZE_T NumberOfBytes
    );

BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    );

//
// Shutdown routine - flushes dirty pages, etc for system shutdown.
//

BOOLEAN
MmShutdownSystem (
    VOID
    );

//
// Routines to deal with working set and commit enforcement.
//

LOGICAL
MmAssignProcessToJob(
    IN PEPROCESS Process
    );

LOGICAL
MmEnforceWorkingSetLimit(
    IN PMMSUPPORT WsInfo,
    IN LOGICAL Enable
    );

//
// Routines to deal with session space.
//

NTSTATUS
MmSessionCreate(
    OUT PULONG SessionId
    );

NTSTATUS
MmSessionDelete(
    IN ULONG SessionId
    );

typedef
NTSTATUS
(*PKWIN32_CALLOUT) (
    IN PVOID Arg
    );

NTSTATUS
MmDispatchWin32Callout(
    IN PKWIN32_CALLOUT Function,
    IN PKWIN32_CALLOUT WorkerCallback OPTIONAL,
    IN PVOID Arg,
    IN PULONG SessionId OPTIONAL
    );

VOID
MmSessionLeader(
    IN PEPROCESS Process
    );

VOID
MmSessionSetUnloadAddress (
    IN PDRIVER_OBJECT pWin32KDevice
    );

//
// Pool support routines to allocate complete pages, not for
// general consumption, these are only used by the executive pool allocator.
//

LOGICAL
MmResourcesAvailable (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN EX_POOL_PRIORITY Priority
    );

PVOID
MiAllocatePoolPages (
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes,
    IN ULONG IsLargeSessionAllocation
    );

ULONG
MiFreePoolPages (
    IN PVOID StartingAddress
    );

PVOID
MiSessionPoolVector(
    VOID
    );

VOID
MiSessionPoolAllocated(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    );

VOID
MiSessionPoolFreed(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    );

//
// Routine for determining which pool a given address resides within.
//

POOL_TYPE
MmDeterminePoolType (
    IN PVOID VirtualAddress
    );

LOGICAL
MmIsSystemAddressLocked(
    IN PVOID VirtualAddress
    );

//
// First level fault routine.
//

NTSTATUS
MmAccessFault (
    IN BOOLEAN StoreInstruction,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    );

//
// Process Support Routines.
//

BOOLEAN
MmCreateProcessAddressSpace (
    IN ULONG MinimumWorkingSetSize,
    IN PEPROCESS NewProcess,
    OUT PULONG_PTR DirectoryTableBase
    );

NTSTATUS
MmInitializeProcessAddressSpace (
    IN PEPROCESS ProcessToInitialize,
    IN PEPROCESS ProcessToClone OPTIONAL,
    IN PVOID SectionToMap OPTIONAL,
    OUT PUNICODE_STRING * AuditName OPTIONAL
    );

VOID
MmDeleteProcessAddressSpace (
    IN PEPROCESS Process
    );

VOID
MmCleanProcessAddressSpace (
    VOID
    );

VOID
MmCleanUserProcessAddressSpace (
    VOID
    );

VOID
MmCleanVirtualAddressDescriptor (
    VOID
    );

PVOID
MmCreateKernelStack (
    BOOLEAN LargeStack
    );

VOID
MmDeleteKernelStack (
    IN PVOID PointerKernelStack,
    IN BOOLEAN LargeStack
    );

NTKERNELAPI
NTSTATUS
MmGrowKernelStack (
    IN PVOID CurrentStack
    );

#if defined(_IA64_)
NTSTATUS
MmGrowKernelBackingStore (
    IN PVOID CurrentStack
    );
#endif // defined(_IA64_)

VOID
MmOutPageKernelStack (
    IN PKTHREAD Thread
    );

VOID
MmInPageKernelStack (
    IN PKTHREAD Thread
    );

VOID
MmOutSwapProcess (
    IN PKPROCESS Process
    );

VOID
MmInSwapProcess (
    IN PKPROCESS Process
    );

PTEB
MmCreateTeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_TEB InitialTeb,
    IN PCLIENT_ID ClientId
    );

PPEB
MmCreatePeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_PEB InitialPeb
    );

VOID
MmDeleteTeb (
    IN PEPROCESS TargetProcess,
    IN PVOID TebBase
    );

VOID
MmAllowWorkingSetExpansion (
    VOID
    );

NTKERNELAPI
NTSTATUS
MmAdjustWorkingSetSize (
    IN SIZE_T WorkingSetMinimum,
    IN SIZE_T WorkingSetMaximum,
    IN ULONG SystemCache
    );

VOID
MmAdjustPageFileQuota (
    IN ULONG NewPageFileQuota
    );

VOID
MmWorkingSetManager (
    VOID
    );

VOID
MmSetMemoryPriorityProcess(
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
    );

//
// Dynamic system loading support
//

NTSTATUS
MmLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    IN BOOLEAN LoadInSessionSpace,
    OUT PVOID *Section,
    OUT PVOID *ImageBaseAddress
    );

NTSTATUS
MmLoadAndLockSystemImage (
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedBaseName OPTIONAL,
    OUT PVOID *Section,
    OUT PVOID *ImageBaseAddress
    );

VOID
MmFreeDriverInitialization (
    IN PVOID Section
    );

NTSTATUS
MmUnloadSystemImage (
    IN PVOID Section
    );

VOID
MmMakeKernelResourceSectionWritable (
    VOID
    );

VOID
VerifierFreeTrackedPool(
    IN PVOID VirtualAddress,
    IN SIZE_T ChargedBytes,
    IN LOGICAL CheckType,
    IN LOGICAL SpecialPool
    );

//
// Triage support
//

ULONG
MmSizeOfTriageInformation(
    VOID
    );

ULONG
MmSizeOfUnloadedDriverInformation(
    VOID
    );

VOID
MmWriteTriageInformation(
    IN PVOID
    );

VOID
MmWriteUnloadedDriverInformation(
    IN PVOID
    );

//
// Cache manager support
//

#if defined(_NTDDK_) || defined(_NTIFS_)

// begin_ntifs

NTKERNELAPI
BOOLEAN
MmIsRecursiveIoFault(
    VOID
    );

// end_ntifs
#else

//++
//
// BOOLEAN
// MmIsRecursiveIoFault (
//     VOID
//     );
//
// Routine Description:
//
//
// This macro examines the thread's page fault clustering information
// and determines if the current page fault is occurring during an I/O
// operation.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Returns TRUE if the fault is occurring during an I/O operation,
//     FALSE otherwise.
//
//--

#define MmIsRecursiveIoFault() \
                 ((PsGetCurrentThread()->DisablePageFaultClustering) | \
                  (PsGetCurrentThread()->ForwardClusterOnly))

#endif

//++
//
// VOID
// MmDisablePageFaultClustering
//     OUT PULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro disables page fault clustering for the current thread.
// Note, that this indicates that file system I/O is in progress
// for that thread.
//
// Arguments:
//
//     SavedState - returns previous state of page fault clustering which
//                  is guaranteed to be nonzero
//
// Return Value:
//
//     None.
//
//--

#define MmDisablePageFaultClustering(SavedState) {                                          \
                *(SavedState) = 2 + (ULONG)PsGetCurrentThread()->DisablePageFaultClustering;\
                PsGetCurrentThread()->DisablePageFaultClustering = TRUE; }


//++
//
// VOID
// MmEnablePageFaultClustering
//     IN ULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro enables page fault clustering for the current thread.
// Note, that this indicates that no file system I/O is in progress for
// that thread.
//
// Arguments:
//
//     SavedState - supplies previous state of page fault clustering
//
// Return Value:
//
//     None.
//
//--

#define MmEnablePageFaultClustering(SavedState) {                                               \
                PsGetCurrentThread()->DisablePageFaultClustering = (BOOLEAN)(SavedState - 2); }

//++
//
// VOID
// MmSavePageFaultReadAhead
//     IN PETHREAD Thread,
//     OUT PULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro saves the page fault read ahead value for the specified
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     SavedState - returns previous state of page fault read ahead
//
// Return Value:
//
//     None.
//
//--


#define MmSavePageFaultReadAhead(Thread,SavedState) {               \
                *(SavedState) = (Thread)->ReadClusterSize * 2 +     \
                                (Thread)->ForwardClusterOnly; }

//++
//
// VOID
// MmSetPageFaultReadAhead
//     IN PETHREAD Thread,
//     IN ULONG ReadAhead
//     );
//
// Routine Description:
//
//
// This macro sets the page fault read ahead value for the specified
// thread, and indicates that file system I/O is in progress for that
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     ReadAhead - Supplies the number of pages to read in addition to
//                 the page the fault is taken on.  A value of 0
//                 reads only the faulting page, a value of 1 reads in
//                 the faulting page and the following page, etc.
//
// Return Value:
//
//     None.
//
//--


#define MmSetPageFaultReadAhead(Thread,ReadAhead) {                          \
                (Thread)->ForwardClusterOnly = TRUE;                         \
                if ((ReadAhead) > MM_MAXIMUM_READ_CLUSTER_SIZE) {            \
                    (Thread)->ReadClusterSize = MM_MAXIMUM_READ_CLUSTER_SIZE;\
                } else {                                                     \
                    (Thread)->ReadClusterSize = (ReadAhead);                 \
                } }

//++
//
// VOID
// MmResetPageFaultReadAhead
//     IN PETHREAD Thread,
//     IN ULONG SavedState
//     );
//
// Routine Description:
//
//
// This macro resets the default page fault read ahead value for the specified
// thread, and indicates that file system I/O is not in progress for that
// thread.
//
// Arguments:
//
//     Thread - Supplies a pointer to the current thread.
//
//     SavedState - supplies previous state of page fault read ahead
//
// Return Value:
//
//     None.
//
//--

#define MmResetPageFaultReadAhead(Thread, SavedState) {                     \
                (Thread)->ForwardClusterOnly = (BOOLEAN)((SavedState) & 1); \
                (Thread)->ReadClusterSize = (SavedState) / 2; }

//
// The order of this list is important, the zeroed, free and standby
// must occur before the modified or bad so comparisons can be
// made when pages are added to a list.
//
// NOTE: This field is limited to 8 elements.
//

#define NUMBER_OF_PAGE_LISTS 8

typedef enum _MMLISTS {
    ZeroedPageList,
    FreePageList,
    StandbyPageList,  //this list and before make up available pages.
    ModifiedPageList,
    ModifiedNoWritePageList,
    BadPageList,
    ActiveAndValid,
    TransitionPage
} MMLISTS;

typedef struct _MMPFNLIST {
    PFN_NUMBER Total;
    MMLISTS ListName;
    PFN_NUMBER Flink;
    PFN_NUMBER Blink;
} MMPFNLIST;

typedef MMPFNLIST *PMMPFNLIST;

extern MMPFNLIST MmModifiedPageListHead;

extern PFN_NUMBER MmThrottleTop;
extern PFN_NUMBER MmThrottleBottom;

//++
//
// BOOLEAN
// MmEnoughMemoryForWrite (
//     VOID
//     );
//
// Routine Description:
//
//
// This macro checks the modified pages and available pages to determine
// to allow the cache manager to throttle write operations.
//
// For NTAS:
// Writes are blocked if there are less than 127 available pages OR
// there are more than 1000 modified pages AND less than 450 available pages.
//
// For DeskTop:
// Writes are blocked if there are less than 30 available pages OR
// there are more than 1000 modified pages AND less than 250 available pages.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     TRUE if ample memory exists and the write should proceed.
//
//--

#define MmEnoughMemoryForWrite()                         \
            ((MmAvailablePages > MmThrottleTop)          \
                        ||                               \
             (((MmModifiedPageListHead.Total < 1000)) && \
               (MmAvailablePages > MmThrottleBottom)))


NTKERNELAPI
NTSTATUS
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
    );


NTKERNELAPI
NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
     );

// begin_ntifs

BOOLEAN
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN DelayClose
    );

// end_ntifs

NTSTATUS
MmGetFileNameForSection (
    IN HANDLE Section,
    OUT PSTRING FileName
    );

NTSTATUS
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    );

NTSTATUS
MmSetVerifierInformation (
    IN OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength
    );

NTSTATUS
MmGetVerifierInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
MmGetPageFileInformation(
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG Length
    );

NTSTATUS
MmExtendSection (
    IN PVOID SectionToExtend,
    IN OUT PLARGE_INTEGER NewSectionSize,
    IN ULONG IgnoreFileSizeChecking
    );

NTSTATUS
MmFlushVirtualMemory (
    IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
MmMapViewInSystemCache (
    IN PVOID SectionToMap,
    OUT PVOID *CapturedBase,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize
    );

VOID
MmUnmapViewInSystemCache (
    IN PVOID BaseAddress,
    IN PVOID SectionToUnmap,
    IN ULONG AddToFront
    );

BOOLEAN
MmPurgeSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER Offset OPTIONAL,
    IN SIZE_T RegionSize,
    IN ULONG IgnoreCacheViews
    );

NTSTATUS
MmFlushSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER Offset OPTIONAL,
    IN SIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN ULONG AcquireFile
    );

NTSTATUS
MmGetCrashDumpInformation (
    IN PSYSTEM_CRASH_DUMP_INFORMATION CrashInfo
    );

NTSTATUS
MmGetCrashDumpStateInformation (
    IN PSYSTEM_CRASH_STATE_INFORMATION CrashInfo
    );

// begin_ntifs

typedef enum _MMFLUSH_TYPE {
    MmFlushForDelete,
    MmFlushForWrite
} MMFLUSH_TYPE;


BOOLEAN
MmFlushImageSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN MMFLUSH_TYPE FlushType
    );

BOOLEAN
MmCanFileBeTruncated (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN PLARGE_INTEGER NewFileSize
    );


// end_ntifs

BOOLEAN
MmDisableModifiedWriteOfSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    );

VOID
MmPurgeWorkingSet (
     IN PEPROCESS Process,
     IN PVOID BaseAddress,
     IN SIZE_T RegionSize
     );

BOOLEAN                                     // ntifs
MmSetAddressRangeModified (                 // ntifs
    IN PVOID Address,                       // ntifs
    IN SIZE_T Length                        // ntifs
    );                                      // ntifs

BOOLEAN
MmCheckCachedPageState (
    IN PVOID Address,
    IN BOOLEAN SetToZero
    );

NTSTATUS
MmCopyToCachedPage (
    IN PVOID Address,
    IN PVOID UserBuffer,
    IN ULONG Offset,
    IN SIZE_T CountInBytes,
    IN BOOLEAN DontZero
    );

VOID
MmUnlockCachedPage (
    IN PVOID AddressInCache
    );

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    );

PVOID
MmDbgWriteCheck (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    );

VOID
MmDbgReleaseAddress (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    );

PVOID64
MmDbgReadCheck64 (
    IN PVOID64 VirtualAddress
    );

PVOID64
MmDbgWriteCheck64 (
    IN PVOID64 VirtualAddress
    );

PVOID64
MmDbgTranslatePhysicalAddress64 (
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

VOID
MmHibernateInformation (
    IN PVOID MemoryMap,
    OUT PULONG_PTR HiberVa,
    OUT PPHYSICAL_ADDRESS HiberPte
    );

// begin_ntddk begin_ntifs begin_wdm

NTKERNELAPI
VOID
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );


// begin_nthal
//
// I/O support routines.
//

NTKERNELAPI
VOID
MmProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );


NTKERNELAPI
VOID
MmUnlockPages (
    IN PMDL MemoryDescriptorList
    );

NTKERNELAPI
VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    );

NTKERNELAPI
PVOID
MmMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
PVOID
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    );

// end_wdm

NTKERNELAPI
NTSTATUS
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    );

// begin_wdm

//
// _MM_PAGE_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPagePriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//

typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;

//
// Note: This function is not available in WDM 1.0
//
NTKERNELAPI
PVOID
MmMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     );

NTKERNELAPI
VOID
MmUnmapLockedPages (
    IN PVOID BaseAddress,
    IN PMDL MemoryDescriptorList
    );

// end_wdm

typedef struct _PHYSICAL_MEMORY_RANGE {
    PHYSICAL_ADDRESS BaseAddress;
    LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

NTKERNELAPI
NTSTATUS
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    );

NTKERNELAPI
PMDL
MmAllocatePagesForMdl (
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes
    );

NTKERNELAPI
VOID
MmFreePagesFromMdl (
    IN PMDL MemoryDescriptorList
    );

// begin_wdm

NTKERNELAPI
PVOID
MmMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmUnmapIoSpace (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

//  end_wdm end_ntddk end_ntifs

NTKERNELAPI
VOID
MmProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

//  begin_ntddk begin_ntifs

NTKERNELAPI
PVOID
MmMapVideoDisplay (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
     );

NTKERNELAPI
VOID
MmUnmapVideoDisplay (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     );

NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress (
    IN PVOID BaseAddress
    );

NTKERNELAPI
PVOID
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemorySpecifyCache (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS LowestAcceptableAddress,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    );

NTKERNELAPI
VOID
MmFreeContiguousMemorySpecifyCache (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

// end_ntddk end_ntifs end_nthal

NTKERNELAPI
ULONG
MmGatherMemoryForHibernate (
    IN PMDL Mdl,
    IN BOOLEAN Wait
    );

NTKERNELAPI
VOID
MmReturnMemoryForHibernate (
    IN PMDL Mdl
    );

VOID
MmReleaseDumpAddresses (
    IN PFN_NUMBER Pages
    );

// begin_ntddk begin_ntifs begin_nthal

NTKERNELAPI
PVOID
MmAllocateNonCachedMemory (
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
BOOLEAN
MmIsAddressValid (
    IN PVOID VirtualAddress
    );

NTKERNELAPI
BOOLEAN
MmIsNonPagedSystemAddressValid (
    IN PVOID VirtualAddress
    );

//  begin_wdm

NTKERNELAPI
SIZE_T
MmSizeOfMdl(
    IN PVOID Base,
    IN SIZE_T Length
    );

NTKERNELAPI
PMDL
MmCreateMdl(
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN SIZE_T Length
    );

NTKERNELAPI
PVOID
MmLockPagableDataSection(
    IN PVOID AddressWithinSection
    );

//  end_wdm

NTKERNELAPI
VOID
MmLockPagableSectionByHandle (
    IN PVOID ImageSectionHandle
    );

// end_ntddk end_ntifs
NTKERNELAPI
VOID
MmLockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

NTKERNELAPI
VOID
MmUnlockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

// begin_wdm begin_ntddk begin_ntifs
NTKERNELAPI
VOID
MmResetDriverPaging (
    IN PVOID AddressWithinSection
    );


NTKERNELAPI
PVOID
MmPageEntireDriver (
    IN PVOID AddressWithinSection
    );

NTKERNELAPI
VOID
MmUnlockPagableImageSection(
    IN PVOID ImageSectionHandle
    );

// end_wdm

NTKERNELAPI
HANDLE
MmSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode
    );

NTKERNELAPI
VOID
MmUnsecureVirtualMemory (
    IN HANDLE SecureHandle
    );

NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    );

NTKERNELAPI
NTSTATUS
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    );

// begin_wdm

//++
//
// VOID
// MmInitializeMdl (
//     IN PMDL MemoryDescriptorList,
//     IN PVOID BaseVa,
//     IN SIZE_T Length
//     )
//
// Routine Description:
//
//     This routine initializes the header of a Memory Descriptor List (MDL).
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to initialize.
//
//     BaseVa - Base virtual address mapped by the MDL.
//
//     Length - Length, in bytes, of the buffer mapped by the MDL.
//
// Return Value:
//
//     None.
//
//--

#define MmInitializeMdl(MemoryDescriptorList, BaseVa, Length) { \
    (MemoryDescriptorList)->Next = (PMDL) NULL; \
    (MemoryDescriptorList)->Size = (CSHORT)(sizeof(MDL) +  \
            (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES((BaseVa), (Length)))); \
    (MemoryDescriptorList)->MdlFlags = 0; \
    (MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN((BaseVa)); \
    (MemoryDescriptorList)->ByteOffset = BYTE_OFFSET((BaseVa)); \
    (MemoryDescriptorList)->ByteCount = (ULONG)(Length); \
    }

//++
//
// PVOID
// MmGetSystemAddressForMdlSafe (
//     IN PMDL MDL,
//     IN MM_PAGE_PRIORITY PRIORITY
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL. If the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
//     Priority - Supplies an indication as to how important it is that this
//                request succeed under low available PTE conditions.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//     Unlike MmGetSystemAddressForMdl, Safe guarantees that it will always
//     return NULL on failure instead of bugchecking the system.
//
//     This macro is not usable by WDM 1.0 drivers as 1.0 did not include
//     MmMapLockedPagesSpecifyCache.  The solution for WDM 1.0 drivers is to
//     provide synchronization and set/reset the MDL_MAPPING_CAN_FAIL bit.
//
//--

#define MmGetSystemAddressForMdlSafe(MDL, PRIORITY)                    \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPagesSpecifyCache((MDL),      \
                                                           KernelMode, \
                                                           MmCached,   \
                                                           NULL,       \
                                                           FALSE,      \
                                                           (PRIORITY))))

//++
//
// PVOID
// MmGetSystemAddressForMdl (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL, if the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//--

//#define MmGetSystemAddressForMdl(MDL)
//     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA)) ?
//                             ((MDL)->MappedSystemVa) :
//                ((((MDL)->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)) ?
//                      ((PVOID)((ULONG)(MDL)->StartVa | (MDL)->ByteOffset)) :
//                            (MmMapLockedPages((MDL),KernelMode)))))

#define MmGetSystemAddressForMdl(MDL)                                  \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPages((MDL),KernelMode)))

//++
//
// VOID
// MmPrepareMdlForReuse (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine will take all of the steps necessary to allow an MDL to be
//     re-used.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL that will be re-used.
//
// Return Value:
//
//     None.
//
//--

#define MmPrepareMdlForReuse(MDL)                                       \
    if (((MDL)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) {         \
        ASSERT(((MDL)->MdlFlags & MDL_PARTIAL) != 0);                   \
        MmUnmapLockedPages( (MDL)->MappedSystemVa, (MDL) );             \
    } else if (((MDL)->MdlFlags & MDL_PARTIAL) == 0) {                  \
        ASSERT(((MDL)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);       \
    }

typedef NTSTATUS (*PMM_DLL_INITIALIZE)(
    IN PUNICODE_STRING RegistryPath
    );

typedef NTSTATUS (*PMM_DLL_UNLOAD)(
    VOID
    );


// end_ntddk end_wdm end_nthal end_ntifs

#if DBG || (i386 && !FPO)
typedef NTSTATUS (*PMM_SNAPSHOT_POOL_PAGE)(
    IN PVOID Address,
    IN ULONG Size,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTSTATUS
MmSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PMM_SNAPSHOT_POOL_PAGE SnapShotPoolPage,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );
#endif // DBG || (i386 && !FPO)

PVOID
MmAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE Type,
    IN ULONG SpecialPoolType
    );

VOID
MmFreeSpecialPool (
    IN PVOID P
    );

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    );

LOGICAL
MmProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    );

LOGICAL
MmIsSpecialPoolAddressFree (
    IN PVOID VirtualAddress
    );

SIZE_T
MmQuerySpecialPoolBlockSize (
    IN PVOID P
    );

extern ULONG MmSpecialPoolTag;
extern PVOID MmSpecialPoolStart;
extern PVOID MmSpecialPoolEnd;

LOGICAL
MmIsHydraAddress (
    IN PVOID VirtualAddress
    );

PUNICODE_STRING
MmLocateUnloadedDriver (
    IN PVOID VirtualAddress
    );

// begin_ntddk

//
// Define an empty typedef for the _DRIVER_OBJECT structure so it may be
// referenced by function types before it is actually defined.
//
struct _DRIVER_OBJECT;

NTKERNELAPI
LOGICAL
MmIsDriverVerifying (
    IN struct _DRIVER_OBJECT *DriverObject
    );

// end_ntddk

LOGICAL
MmTrimAllSystemPagableMemory (
    IN LOGICAL PurgeTransition
    );

#define MMNONPAGED_QUOTA_INCREASE (64*1024)

#define MMPAGED_QUOTA_INCREASE (512*1024)

#define MMNONPAGED_QUOTA_CHECK (256*1024)

#define MMPAGED_QUOTA_CHECK (4*1024*1024)

BOOLEAN
MmRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T OldQuotaLimit,
    OUT PSIZE_T NewQuotaLimit
    );

VOID
MmReturnPoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T ReturnedQuota
    );

//
// Zero page thread routine.
//

VOID
MmZeroPageThread (
    VOID
    );

NTSTATUS
MmCopyVirtualMemory(
    IN PEPROCESS FromProcess,
    IN PVOID FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN ULONG BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PULONG NumberOfBytesCopied
    );

NTSTATUS
MmGetSectionRange(
    IN PVOID AddressWithinSection,
    OUT PVOID *StartingSectionAddress,
    OUT PULONG SizeofSection
    );

VOID
MmMapMemoryDumpMdl(
    IN OUT PMDL MemoryDumpMdl
    );


// begin_ntminiport

//
// Graphics support routines.
//

typedef
VOID
(*PBANKED_SECTION_ROUTINE) (
    IN ULONG ReadBank,
    IN ULONG WriteBank,
    IN PVOID Context
    );

// end_ntminiport

NTSTATUS
MmSetBankedSection(
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG BankLength,
    IN BOOLEAN ReadWriteBank,
    IN PBANKED_SECTION_ROUTINE BankRoutine,
    IN PVOID Context);


NTKERNELAPI
BOOLEAN
MmIsSystemAddressAccessable (
    IN PVOID VirtualAddress
    );

BOOLEAN
MmVerifyImageIsOkForMpUse(
    IN PVOID BaseAddress
    );

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG Length
    );

typedef
VOID
(FASTCALL *PPAGE_FAULT_NOTIFY_ROUTINE)(
    IN NTSTATUS Status,
    IN PVOID VirtualAddress,
    IN PVOID TrapInformation
    );

typedef
VOID
(FASTCALL *PHARD_FAULT_NOTIFY_ROUTINE)(
    IN HANDLE FileObject,
    IN PVOID VirtualAddress
    );

NTKERNELAPI
VOID
FASTCALL
MmSetPageFaultNotifyRoutine(
    IN PPAGE_FAULT_NOTIFY_ROUTINE NotifyRoutine
    );

NTKERNELAPI
VOID
FASTCALL
MmSetHardFaultNotifyRoutine(
    IN PHARD_FAULT_NOTIFY_ROUTINE NotifyRoutine
    );

NTSTATUS
MmCallDllInitialize(
    IN  PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

// Crash dump only
// Called to initialize the kernel memory to include in a summary dump
VOID
MmSetKernelDumpRange(
    IN PVOID DumpContext
    );

#ifdef NTPERF
#include "perfinfokrn.h"
#else
#define PERFINFO_ADDPOOLPAGE(CheckType, PoolIndex, Addr, PoolDesc)
#define PERFINFO_ADDTOWS(PageFrame, Address, Pid)
#define PERFINFO_BIGPOOLALLOC(Type, PTag, NumBytes, Addr)
#define PERFINFO_CM_CHECKCELLTYPE(Map)
#define PERFINFO_CM_CHECKCELLTYPE(Map)
#define PERFINFO_CM_HIVECELL_REFERENCE_FLAT(Hive, pcell, Cell)
#define PERFINFO_CM_HIVECELL_REFERENCE_PAGED(Hive, pcell, Cell, Type, Map)
#define PERFINFO_CONVERT_TO_GUI_THREAD(EThread)
#define PERFINFO_DECREFCNT(PageFrame, Flag, Type)
#define PERFINFO_DELETE_STACK(PointerPte, NumberOfPtes)
#define PERFINFO_DISPATCHFAULT_DECL()
#define PERFINFO_DRIVER_COMPLETIONROUTINE_CALL(irp, irpsp)
#define PERFINFO_DRIVER_COMPLETIONROUTINE_RETURN(irp, irpsp)
#define PERFINFO_DRIVER_INIT(pdo)
#define PERFINFO_DRIVER_INIT_COMPLETE(pdo)
#define PERFINFO_DRIVER_MAJORFUNCTION_CALL(irp, irpsp, pdo)
#define PERFINFO_DRIVER_MAJORFUNCTION_RETURN(irp, irpsp, pdo)
#define PERFINFO_EXALLOCATEPOOLWITHTAG_DECL()
#define PERFINFO_EXFREEPOOLWITHTAG_DECL()
#define PERFINFO_FREEPOOL(Addr)
#define PERFINFO_FREEPOOLPAGE(CheckType, PoolIndex, Addr, PoolDesc)
#define PERFINFO_GET_PAGE_INFO(PointerPte)
#define PERFINFO_GET_PAGE_INFO_REPLACEMENT(PointerPte)
#define PERFINFO_GET_PAGE_INFO_WITH_DECL(PointerPte)
#define PERFINFO_GROW_STACK(EThread)
#define PERFINFO_HARDFAULT(Address, InpageSupport)
#define PERFINFO_HARDFAULT_INFO(ProtoPte)
#define PERFINFO_HARDFAULT_IOTIME()
#define PERFINFO_HIVECELL_REFERENCE_FLAT(Hive, pcell, Cell)
#define PERFINFO_HIVECELL_REFERENCE_PAGED(Hive, pcell, Cell, Type, Map)
#define PERFINFO_IMAGE_LOAD(LdrDataTableEntry)
#define PERFINFO_IMAGE_UNLOAD(Address)
#define PERFINFO_INIT_POOLRANGE(PoolStart, PoolPages)
#define PERFINFO_INIT_PERFMEMTABLE(LoaderBlock)
#define PERFINFO_INIT_TRACEFLAGS(OptionString, SpecificOption)
#define PERFINFO_INSERTINLIST(Page, ListHead)
#define PERFINFO_INSERT_FRONT_STANDBY(Page)
#define PERFINFO_LOG_MARK(PMARK)
#define PERFINFO_LOG_MARK_SPRINTF(PMARK, VARIABLE)
#define PERFINFO_LOG_WMI_TRACE_EVENT( PData, xLength)
#define PERFINFO_LOG_WMI_TRACE_KERNEL_EVENT(GroupType, PData, xLength, Thread)
#define PERFINFO_LOG_WMI_TRACE_LONG_EVENT(GroupType, PData, xCount, Thread)
#define PERFINFO_LOG_WS_REMOVAL(Type, WsInfo)
#define PERFINFO_LOG_WS_REPLACEMENT(WsInfo)
#define PERFINFO_MIH_DECL
#define PERFINFO_MMINIT_DECL
#define PERFINFO_MMINIT_START()
#define PERFINFO_MOD_PAGE_WRITER3()
#define PERFINFO_PAGE_INFO_DECL()
#define PERFINFO_PAGE_INFO_REPLACEMENT_DECL()
#define PERFINFO_POOLALLOC(Type, PTag, NumBytes)
#define PERFINFO_POOLALLOC_ADDR(Addr)
#define PERFINFO_POOL_ALLOC_COMMON(Type, PTag, NumBytes)
#define PERFINFO_PRIVATE_COPY_ON_WRITE(CopyFrom, PAGE_SIZE)
#define PERFINFO_PRIVATE_PAGE_DEMAND_ZERO(VirtualAddress)
#define PERFINFO_PROCESS_CREATE(EProcess)
#define PERFINFO_PROCESS_DELETE(EProcess)
#define PERFINFO_DELETE_PAGE(ppfn)
#define PERFINFO_REMOVEPAGE(PageIndex, LogType)
#define PERFINFO_SECTION_CREATE(ControlArea)
#define PERFINFO_SEGMENT_DELETE(FileName)
#define PERFINFO_SOFTFAULT(PageFrame, Address, Type)
#define PERFINFO_THREAD_CREATE(EThread, ITeb)
#define PERFINFO_THREAD_DELETE(EThread)
#define PERFINFO_UNLINKFREEPAGE(Index, Location)
#define PERFINFO_UNLINKPAGE(Index, Location)
#define PERFINFO_WSMANAGE_ACTUALTRIM(Trim)
#define PERFINFO_WSMANAGE_DECL()
#define PERFINFO_WSMANAGE_DUMPENTRIES()
#define PERFINFO_WSMANAGE_DUMPENTRIES_CLAIMS()
#define PERFINFO_WSMANAGE_DUMPENTRIES_FAULTS()
#define PERFINFO_WSMANAGE_DUMPWS(VmSupport, SampledAgeCounts)
#define PERFINFO_WSMANAGE_FINALACTION(TrimAction)
#define PERFINFO_WSMANAGE_GLOBAL_DECL
#define PERFINFO_WSMANAGE_LOGINFO_CLAIMS(TrimAction)
#define PERFINFO_WSMANAGE_LOGINFO_FAULTS(TrimAction)
#define PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport)
#define PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport)
#define PERFINFO_WSMANAGE_STARTLOG()
#define PERFINFO_WSMANAGE_STARTLOG_CLAIMS()
#define PERFINFO_WSMANAGE_STARTLOG_FAULTS()
#define PERFINFO_WSMANAGE_TOTRIM(Trim)
#define PERFINFO_WSMANAGE_TRIMACTION(TrimAction)
#define PERFINFO_WSMANAGE_TRIMEND_CLAIMS(Criteria)
#define PERFINFO_WSMANAGE_TRIMEND_FAULTS(Criteria)
#define PERFINFO_WSMANAGE_TRIMWS(Process, SessionSpace, VmSupport)
#define PERFINFO_WSMANAGE_TRIMWS_CLAIMINFO(VmSupport)
#define PERFINFO_WSMANAGE_TRIMWS_CLAIMINFO(VmSupport)
#define PERFINFO_WSMANAGE_WAITFORWRITER_CLAIMS()
#define PERFINFO_WSMANAGE_WAITFORWRITER_FAULTS()
#define PERFINFO_WSMANAGE_WILLTRIM(ReductionGoal, FreeGoal)
#define PERFINFO_WSMANAGE_WILLTRIM_CLAIMS(Criteria)
#define PERFINFO_WSMANAGE_WILLTRIM_FAULTS(Criteria)

#define PERFINFO_DO_PAGEFAULT_CLUSTERING() 1
#endif // NTPERF

#endif  // MM
