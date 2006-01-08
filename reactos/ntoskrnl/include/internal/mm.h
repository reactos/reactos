#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

extern ULONG MiFreeSwapPages;
extern ULONG MiUsedSwapPages;
extern ULONG MmPagedPoolSize;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;
extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

struct _KTRAP_FRAME;
struct _EPROCESS;
struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP;
typedef ULONG SWAPENTRY;
typedef ULONG PFN_TYPE, *PPFN_TYPE;

#define MEMORY_AREA_INVALID                 (0)
#define MEMORY_AREA_SECTION_VIEW            (1)
#define MEMORY_AREA_CONTINUOUS_MEMORY       (2)
#define MEMORY_AREA_NO_CACHE                (3)
#define MEMORY_AREA_IO_MAPPING              (4)
#define MEMORY_AREA_SYSTEM                  (5)
#define MEMORY_AREA_MDL_MAPPING             (7)
#define MEMORY_AREA_VIRTUAL_MEMORY          (8)
#define MEMORY_AREA_CACHE_SEGMENT           (9)
#define MEMORY_AREA_SHARED_DATA             (10)
#define MEMORY_AREA_KERNEL_STACK            (11)
#define MEMORY_AREA_PAGED_POOL              (12)
#define MEMORY_AREA_NO_ACCESS               (13)
#define MEMORY_AREA_PEB_OR_TEB              (14)

#define MM_PHYSICAL_PAGE_MPW_PENDING        (0x8)

#define MM_CORE_DUMP_TYPE_NONE              (0x0)
#define MM_CORE_DUMP_TYPE_MINIMAL           (0x1)
#define MM_CORE_DUMP_TYPE_FULL              (0x2)

#define MM_PAGEOP_PAGEIN                    (1)
#define MM_PAGEOP_PAGEOUT                   (2)
#define MM_PAGEOP_PAGESYNCH                 (3)
#define MM_PAGEOP_ACCESSFAULT               (4)

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             1024

#define TEB_BASE                            0x7FFDE000
#define KPCR_BASE                           0xFF000000

/* Although Microsoft says this isn't hardcoded anymore,
   they won't be able to change it. Stuff depends on it */
#define MM_VIRTMEM_GRANULARITY              (64 * 1024) 

#define STATUS_MM_RESTART_OPERATION         ((NTSTATUS)0xD0000001)

/*
 * Additional flags for protection attributes
 */
#define PAGE_WRITETHROUGH                   (1024)
#define PAGE_SYSTEM                         (2048)

#define SEC_PHYSICALMEMORY                  (0x80000000)

#define MM_PAGEFILE_SEGMENT                 (0x1)
#define MM_DATAFILE_SEGMENT                 (0x2)

#define MC_CACHE                            (0)
#define MC_USER                             (1)
#define MC_PPOOL                            (2)
#define MC_NPPOOL                           (3)
#define MC_MAXIMUM                          (4)

#define PAGED_POOL_MASK                     1
#define MUST_SUCCEED_POOL_MASK              2
#define CACHE_ALIGNED_POOL_MASK             4
#define QUOTA_POOL_MASK                     8
#define SESSION_POOL_MASK                   32
#define VERIFIER_POOL_MASK                  64

#define MM_PAGED_POOL_SIZE                  (100*1024*1024)
#define MM_NONPAGED_POOL_SIZE               (100*1024*1024)

/*
 * Paged and non-paged pools are 8-byte aligned
 */
#define MM_POOL_ALIGNMENT                   8

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define MM_KERNEL_MAP_SIZE                  (16*1024*1024)
#define MM_KERNEL_MAP_BASE                  (0xf0c00000)

/*
 * FIXME - different architectures have different cache line sizes...
 */
#define MM_CACHE_LINE_SIZE                  32

#define MM_ROUND_UP(x,s)                    \
    ((PVOID)(((ULONG_PTR)(x)+(s)-1) & ~((ULONG_PTR)(s)-1)))

#define MM_ROUND_DOWN(x,s)                  \
    ((PVOID)(((ULONG_PTR)(x)) & ~((ULONG_PTR)(s)-1)))

#define PAGE_FLAGS_VALID_FROM_USER_MODE     \
    (PAGE_READONLY | \
    PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY | \
    PAGE_GUARD | \
    PAGE_NOACCESS | \
    PAGE_NOCACHE)

#define PAGE_IS_READABLE                    \
    (PAGE_READONLY | \
    PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITABLE                    \
    (PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_EXECUTABLE                  \
    (PAGE_EXECUTE | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITECOPY                   \
    (PAGE_WRITECOPY | \
    PAGE_EXECUTE_WRITECOPY)

typedef struct
{
    ULONG Entry[NR_SECTION_PAGE_ENTRIES];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

typedef struct
{
    PSECTION_PAGE_TABLE PageTables[NR_SECTION_PAGE_TABLES];
} SECTION_PAGE_DIRECTORY, *PSECTION_PAGE_DIRECTORY;

typedef struct _MM_SECTION_SEGMENT
{
    LONGLONG FileOffset;
    ULONG_PTR VirtualAddress;
    ULONG RawLength;
    ULONG Length;
    ULONG Protection;
    FAST_MUTEX Lock;
    ULONG ReferenceCount;
    SECTION_PAGE_DIRECTORY PageDirectory;
    ULONG Flags;
    ULONG Characteristics;
    BOOLEAN WriteCopy;
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct _MM_IMAGE_SECTION_OBJECT
{
    ULONG_PTR ImageBase;
    ULONG_PTR StackReserve;
    ULONG_PTR StackCommit;
    ULONG_PTR EntryPoint;
    ULONG Subsystem;
    ULONG ImageCharacteristics;
    USHORT MinorSubsystemVersion;
    USHORT MajorSubsystemVersion;
    USHORT Machine;
    BOOLEAN Executable;
    ULONG NrSegments;
    PMM_SECTION_SEGMENT Segments;
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

typedef struct _SECTION_OBJECT
{
    CSHORT Type;
    CSHORT Size;
    LARGE_INTEGER MaximumSize;
    ULONG SectionPageProtection;
    ULONG AllocationAttributes;
    PFILE_OBJECT FileObject;
    union
    {
        PMM_IMAGE_SECTION_OBJECT ImageSection;
        PMM_SECTION_SEGMENT Segment;
    };
} SECTION_OBJECT, *PSECTION_OBJECT;

typedef struct _MEMORY_AREA
{
    PVOID StartingAddress;
    PVOID EndingAddress;
    struct _MEMORY_AREA *Parent;
    struct _MEMORY_AREA *LeftChild;
    struct _MEMORY_AREA *RightChild;
    ULONG Type;
    ULONG Protect;
    ULONG Flags;
    ULONG LockCount;
    BOOLEAN DeleteInProgress;
    ULONG PageOpCount;
    union
    {
        struct
        {
            SECTION_OBJECT* Section;
            ULONG ViewOffset;
            PMM_SECTION_SEGMENT Segment;
            BOOLEAN WriteCopyView;
            LIST_ENTRY RegionListHead;
        } SectionData;
        struct
        {
            LIST_ENTRY RegionListHead;
        } VirtualMemoryData;
    } Data;
} MEMORY_AREA, *PMEMORY_AREA;

#ifndef _MMTYPES_H
typedef struct _MADDRESS_SPACE
{
    PMEMORY_AREA MemoryAreaRoot;
    FAST_MUTEX Lock;
    PVOID LowestAddress;
    struct _EPROCESS* Process;
    PUSHORT PageTableRefCountTable;
    ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;
#endif

typedef struct
{
    ULONG NrTotalPages;
    ULONG NrSystemPages;
    ULONG NrReservedPages;
    ULONG NrUserPages;
    ULONG NrFreePages;
    ULONG NrDirtyPages;
    ULONG NrLockedPages;
    ULONG PagingRequestsInLastMinute;
    ULONG PagingRequestsInLastFiveMinutes;
    ULONG PagingRequestsInLastFifteenMinutes;
} MM_STATS;

extern MM_STATS MmStats;

typedef struct _MM_PAGEOP
{
  /* Type of operation. */
  ULONG OpType;
  /* Number of threads interested in this operation. */
  ULONG ReferenceCount;
  /* Event that will be set when the operation is completed. */
  KEVENT CompletionEvent;
  /* Status of the operation once it is completed. */
  NTSTATUS Status;
  /* TRUE if the operation was abandoned. */
  BOOLEAN Abandoned;
  /* The memory area to be affected by the operation. */
  PMEMORY_AREA MArea;
  ULONG Hash;
  struct _MM_PAGEOP* Next;
  struct _ETHREAD* Thread;
  /*
   * These fields are used to identify the operation if it is against a
   * virtual memory area.
   */
  HANDLE Pid;
  PVOID Address;
  /*
   * These fields are used to identify the operation if it is against a
   * section mapping.
   */
  PMM_SECTION_SEGMENT Segment;
  ULONG Offset;
} MM_PAGEOP, *PMM_PAGEOP;

typedef struct _MM_MEMORY_CONSUMER
{
    ULONG PagesUsed;
    ULONG PagesTarget;
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

typedef struct _MM_REGION
{
    ULONG Type;
    ULONG Protect;
    ULONG Length;
    LIST_ENTRY RegionListEntry;
} MM_REGION, *PMM_REGION;

extern MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];

typedef VOID
(*PMM_ALTER_REGION_FUNC)(
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress,
    ULONG Length,
    ULONG OldType,
    ULONG OldProtect,
    ULONG NewType,
    ULONG NewProtect
);

typedef VOID
(*PMM_FREE_PAGE_FUNC)(
    PVOID Context,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PFN_TYPE Page,
    SWAPENTRY SwapEntry,
    BOOLEAN Dirty
);


/* FUNCTIONS */

/* aspace.c ******************************************************************/

VOID 
NTAPI
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace);

VOID
NTAPI
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace);

VOID
NTAPI
MmInitializeKernelAddressSpace(VOID);

PMADDRESS_SPACE
NTAPI
MmGetCurrentAddressSpace(VOID);

PMADDRESS_SPACE
NTAPI
MmGetKernelAddressSpace(VOID);

NTSTATUS
NTAPI
MmInitializeAddressSpace(
    struct _EPROCESS* Process,
    PMADDRESS_SPACE AddressSpace);

NTSTATUS
NTAPI
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace);

/* marea.c *******************************************************************/

NTSTATUS
NTAPI
MmInitMemoryAreas(VOID);

NTSTATUS
STDCALL
MmCreateMemoryArea(
    PMADDRESS_SPACE AddressSpace,
    ULONG Type,
    PVOID *BaseAddress,
    ULONG_PTR Length,
    ULONG Protection,
    PMEMORY_AREA *Result,
    BOOLEAN FixedAddress,
    ULONG AllocationFlags,
    PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL
);

PMEMORY_AREA
STDCALL
MmLocateMemoryAreaByAddress(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address
);

ULONG_PTR
STDCALL
MmFindGapAtAddress(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address
);

NTSTATUS
STDCALL
MmFreeMemoryArea(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

NTSTATUS
STDCALL
MmFreeMemoryAreaByPtr(
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

VOID
STDCALL
MmDumpMemoryAreas(PMADDRESS_SPACE AddressSpace);

PMEMORY_AREA
STDCALL
MmLocateMemoryAreaByRegion(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address,
    ULONG_PTR Length
);

PVOID
STDCALL
MmFindGap(
    PMADDRESS_SPACE AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown
);

VOID
STDCALL
MmReleaseMemoryAreaIfDecommitted(
    PEPROCESS Process,
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress
);

/* npool.c *******************************************************************/

VOID
NTAPI
MiDebugDumpNonPagedPool(BOOLEAN NewOnly);

VOID
NTAPI
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);

VOID
NTAPI
MiInitializeNonPagedPool(VOID);

PVOID
NTAPI
MmGetMdlPageAddress(
    PMDL Mdl,
    PVOID Offset
);

/* pool.c *******************************************************************/

PVOID
STDCALL
ExAllocateNonPagedPoolWithTag(
    POOL_TYPE type,
    ULONG size,
    ULONG Tag,
    PVOID Caller
);

PVOID
STDCALL
ExAllocatePagedPoolWithTag(
    POOL_TYPE Type,
    ULONG size,
    ULONG Tag
);

VOID
STDCALL
ExFreeNonPagedPool(PVOID block);

VOID 
STDCALL
ExFreePagedPool(IN PVOID Block);

VOID 
NTAPI
MmInitializePagedPool(VOID);

PVOID
STDCALL
MiAllocateSpecialPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN ULONG Underrun
);

BOOLEAN
STDCALL
MiRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN ULONG CurrentMaxQuota,
    OUT PULONG NewMaxQuota
);

/* mdl.c *********************************************************************/

VOID
NTAPI
MmBuildMdlFromPages(
    PMDL Mdl,
    PULONG Pages
);

/* mminit.c ******************************************************************/

VOID
NTAPI
MiShutdownMemoryManager(VOID);

VOID
NTAPI
MmInit1(
    ULONG_PTR FirstKernelPhysAddress,
    ULONG_PTR LastKernelPhysAddress,
    ULONG_PTR LastKernelAddress,
    PADDRESS_RANGE BIOSMemoryMap,
    ULONG AddressRangeCount,
    ULONG MaxMemInMeg
);

VOID
NTAPI
MmInit2(VOID);

VOID
NTAPI
MmInit3(VOID);

VOID
NTAPI
MiFreeInitMemory(VOID);

VOID
NTAPI
MmInitializeMdlImplementation(VOID);

/* pagefile.c ****************************************************************/

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID);

VOID
NTAPI
MmDereserveSwapPages(ULONG Nr);

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry);

VOID
NTAPI
MmInitPagingFile(VOID);

NTSTATUS
NTAPI
MmReadFromSwapPage(
    SWAPENTRY SwapEntry,
    PFN_TYPE Page
);

BOOLEAN
NTAPI
MmReserveSwapPages(ULONG Nr);

NTSTATUS
NTAPI
MmWriteToSwapPage(
    SWAPENTRY SwapEntry,
    PFN_TYPE Page
);

NTSTATUS
STDCALL
MmDumpToPagingFile(
    ULONG BugCode,
    ULONG BugCodeParameter1,
    ULONG BugCodeParameter2,
    ULONG BugCodeParameter3,
    ULONG BugCodeParameter4,
    struct _KTRAP_FRAME* TrapFrame
);

BOOLEAN
NTAPI
MmIsAvailableSwapPage(VOID);

VOID
NTAPI
MmShowOutOfSpaceMessagePagingFile(VOID);

/* process.c ****************************************************************/

NTSTATUS
STDCALL
MmCreateProcessAddressSpace(
    IN struct _EPROCESS* Process,
    IN PSECTION_OBJECT Section OPTIONAL
);

NTSTATUS
STDCALL
MmCreatePeb(PEPROCESS Process);

struct _TEB*
STDCALL
MmCreateTeb(
    PEPROCESS Process,
    PCLIENT_ID ClientId,
    PINITIAL_TEB InitialTeb
);

VOID
STDCALL
MmDeleteTeb(
    PEPROCESS Process,
    struct _TEB* Teb
);

/* i386/pfault.c *************************************************************/

NTSTATUS
NTAPI
MmPageFault(
    ULONG Cs,
    PULONG Eip,
    PULONG Eax,
    ULONG Cr2,
    ULONG ErrorCode
);

/* mm.c **********************************************************************/

NTSTATUS
NTAPI
MmAccessFault(
    KPROCESSOR_MODE Mode,
    ULONG_PTR Address,
    BOOLEAN FromMdl
);

NTSTATUS
NTAPI
MmNotPresentFault(
    KPROCESSOR_MODE Mode,
    ULONG_PTR Address,
    BOOLEAN FromMdl
);

/* anonmem.c *****************************************************************/

NTSTATUS
NTAPI
MmNotPresentFaultVirtualMemory(
    PMADDRESS_SPACE AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

NTSTATUS
NTAPI
MmPageOutVirtualMemory(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    struct _MM_PAGEOP* PageOp
);

NTSTATUS
STDCALL
MmQueryAnonMem(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PULONG ResultLength
);

VOID
NTAPI
MmFreeVirtualMemory(
    struct _EPROCESS* Process,
    PMEMORY_AREA MemoryArea
);

NTSTATUS
NTAPI
MmProtectAnonMem(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    ULONG Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmWritePageVirtualMemory(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MArea,
    PVOID Address,
    PMM_PAGEOP PageOp
);

/* kmap.c ********************************************************************/

PVOID
NTAPI
ExAllocatePage(VOID);

VOID
NTAPI
ExUnmapPage(PVOID Addr);

PVOID
NTAPI
ExAllocatePageWithPhysPage(PFN_TYPE Page);

NTSTATUS
NTAPI
MiCopyFromUserPage(
    PFN_TYPE Page,
    PVOID SourceAddress
);

NTSTATUS
NTAPI
MiZeroPage(PFN_TYPE Page);

/* memsafe.s *****************************************************************/

PVOID
FASTCALL
MmSafeReadPtr(PVOID Source);

/* pageop.c ******************************************************************/

VOID
NTAPI
MmReleasePageOp(PMM_PAGEOP PageOp);

PMM_PAGEOP
NTAPI
MmGetPageOp(
    PMEMORY_AREA MArea,
    HANDLE Pid,
    PVOID Address,
    PMM_SECTION_SEGMENT Segment,
    ULONG Offset,
    ULONG OpType,
    BOOLEAN First
);

PMM_PAGEOP
NTAPI
MmCheckForPageOp(
    PMEMORY_AREA MArea,
    HANDLE Pid,
    PVOID Address,
    PMM_SECTION_SEGMENT Segment,
    ULONG Offset
);

VOID
NTAPI
MmInitializePageOp(VOID);

/* process.c *****************************************************************/

PVOID
STDCALL
MmCreateKernelStack(BOOLEAN GuiStack);

VOID
STDCALL
MmDeleteKernelStack(PVOID Stack,
                    BOOLEAN GuiStack);

/* balace.c ******************************************************************/

VOID
NTAPI
MmInitializeMemoryConsumer(
    ULONG Consumer,
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed)
);

VOID 
NTAPI
MmInitializeBalancer(
    ULONG NrAvailablePages,
    ULONG NrSystemPages
);

NTSTATUS 
NTAPI
MmReleasePageMemoryConsumer(
    ULONG Consumer,
    PFN_TYPE Page
);

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(
    ULONG Consumer,
    BOOLEAN MyWait,
    PPFN_TYPE AllocatedPage
);

VOID
NTAPI
MiInitBalancerThread(VOID);

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID);

/* rmap.c **************************************************************/

VOID
NTAPI
MmSetRmapListHeadPage(
    PFN_TYPE Page,
    struct _MM_RMAP_ENTRY* ListHead
);

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Page);

VOID
NTAPI
MmInsertRmap(
    PFN_TYPE Page,
    PEPROCESS Process,
    PVOID Address
);

VOID
NTAPI
MmDeleteAllRmaps(
    PFN_TYPE Page,
    PVOID Context,
    VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process, PVOID Address)
);

VOID
NTAPI
MmDeleteRmap(
    PFN_TYPE Page,
    PEPROCESS Process,
    PVOID Address
);

VOID
NTAPI
MmInitializeRmapList(VOID);

VOID
NTAPI
MmSetCleanAllRmaps(PFN_TYPE Page);

VOID
NTAPI
MmSetDirtyAllRmaps(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_TYPE Page);

NTSTATUS
NTAPI
MmWritePagePhysicalAddress(PFN_TYPE Page);

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_TYPE Page);

/* freelist.c **********************************************************/

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPage);

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID);

VOID
NTAPI
MmSetLRULastPage(PFN_TYPE Page);

VOID
NTAPI
MmLockPage(PFN_TYPE Page);

VOID
NTAPI
MmLockPageUnsafe(PFN_TYPE Page);

VOID
NTAPI
MmUnlockPage(PFN_TYPE Page);

ULONG
NTAPI
MmGetLockCountPage(PFN_TYPE Page);

PVOID
NTAPI
MmInitializePageList(
    ULONG_PTR FirstPhysKernelAddress,
    ULONG_PTR LastPhysKernelAddress,
    ULONG MemorySizeInPages,
    ULONG_PTR LastKernelBase,
    PADDRESS_RANGE BIOSMemoryMap,
    ULONG AddressRangeCount
);

PFN_TYPE
NTAPI
MmGetContinuousPages(
    ULONG NumberOfBytes,
    PHYSICAL_ADDRESS LowestAcceptableAddress,
    PHYSICAL_ADDRESS HighestAcceptableAddress,
    PHYSICAL_ADDRESS BoundaryAddressMultiple
);

NTSTATUS
NTAPI
MmInitZeroPageThread(VOID);

/* i386/page.c *********************************************************/

PVOID
NTAPI
MmCreateHyperspaceMapping(PFN_TYPE Page);

PFN_TYPE
NTAPI
MmChangeHyperspaceMapping(
    PVOID Address,
    PFN_TYPE Page
);

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(PVOID Address);

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

NTSTATUS
NTAPI
MmCommitPagedPoolAddress(
    PVOID Address,
    BOOLEAN Locked
);

NTSTATUS
NTAPI
MmCreateVirtualMapping(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

ULONG
NTAPI
MmGetPageProtect(
    struct _EPROCESS* Process,
    PVOID Address);

VOID
NTAPI
MmSetPageProtect(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect
);

BOOLEAN
NTAPI
MmIsPagePresent(
    struct _EPROCESS* Process,
    PVOID Address
);

VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID);

VOID
NTAPI
MmDisableVirtualMapping(
    PEPROCESS Process,
    PVOID Address,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

VOID
NTAPI
MmEnableVirtualMapping(
    PEPROCESS Process,
    PVOID Address
);

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address);

VOID
NTAPI
MmDeletePageFileMapping(
    PEPROCESS Process,
    PVOID Address,
    SWAPENTRY* SwapEntry
);

NTSTATUS
NTAPI
MmCreatePageFileMapping(
    PEPROCESS Process,
    PVOID Address,
    SWAPENTRY SwapEntry
);

BOOLEAN
NTAPI
MmIsPageSwapEntry(
    PEPROCESS Process,
    PVOID Address
);

VOID
NTAPI
MmTransferOwnershipPage(
    PFN_TYPE Page,
    ULONG NewConsumer
);

VOID
NTAPI
MmSetDirtyPage(
    PEPROCESS Process,
    PVOID Address
);

PFN_TYPE
NTAPI
MmAllocPage(
    ULONG Consumer,
    SWAPENTRY SavedSwapEntry
);

LONG
NTAPI
MmAllocPagesSpecifyRange(
    ULONG Consumer,
    PHYSICAL_ADDRESS LowestAddress,
    PHYSICAL_ADDRESS HighestAddress,
    ULONG NumberOfPages,
    PPFN_TYPE Pages
);

VOID
NTAPI
MmDereferencePage(PFN_TYPE Page);

VOID
NTAPI
MmReferencePage(PFN_TYPE Page);

VOID
NTAPI
MmReferencePageUnsafe(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsAccessedAndResetAccessPage(
    PEPROCESS Process,
    PVOID Address
);

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsUsablePage(PFN_TYPE Page);

VOID
NTAPI
MmSetFlagsPage(
    PFN_TYPE Page,
    ULONG Flags);

ULONG
NTAPI
MmGetFlagsPage(PFN_TYPE Page);

VOID
NTAPI
MmSetSavedSwapEntryPage(
    PFN_TYPE Page,
    SWAPENTRY SavedSwapEntry);

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Page);

VOID
NTAPI
MmSetCleanPage(
    PEPROCESS Process,
    PVOID Address
);

NTSTATUS
NTAPI
MmCreatePageTable(PVOID PAddress);

VOID
NTAPI
MmDeletePageTable(
    PEPROCESS Process,
    PVOID Address
);

PFN_TYPE
NTAPI
MmGetPfnForProcess(
    PEPROCESS Process,
    PVOID Address
);

NTSTATUS
STDCALL
MmCopyMmInfo(
    PEPROCESS Src,
    PEPROCESS Dest,
    PPHYSICAL_ADDRESS DirectoryTableBase
);

NTSTATUS
NTAPI
MmReleaseMmInfo(PEPROCESS Process);

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process);

VOID
NTAPI
MmDeleteVirtualMapping(
    PEPROCESS Process,
    PVOID Address,
    BOOLEAN FreePage,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

BOOLEAN
NTAPI
MmIsDirtyPage(
    PEPROCESS Process,
    PVOID Address
);

VOID
NTAPI
MmMarkPageMapped(PFN_TYPE Page);

VOID
NTAPI
MmMarkPageUnmapped(PFN_TYPE Page);

VOID
NTAPI
MmUpdatePageDir(
    PEPROCESS Process,
    PVOID Address,
    ULONG Size
);

VOID
NTAPI
MiInitPageDirectoryMap(VOID);

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID);

/* wset.c ********************************************************************/

NTSTATUS
MmTrimUserMemory(
    ULONG Target,
    ULONG Priority,
    PULONG NrFreedPages
);

/* region.c ************************************************************/

NTSTATUS
NTAPI
MmAlterRegion(
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress,
    PLIST_ENTRY RegionListHead,
    PVOID StartAddress,
    ULONG Length,
    ULONG NewType,
    ULONG NewProtect,
    PMM_ALTER_REGION_FUNC AlterFunc
);

VOID
NTAPI
MmInitializeRegion(
    PLIST_ENTRY RegionListHead,
    SIZE_T Length,
    ULONG Type,
    ULONG Protect
);

PMM_REGION
NTAPI
MmFindRegion(
    PVOID BaseAddress,
    PLIST_ENTRY RegionListHead,
    PVOID Address,
    PVOID* RegionBaseAddress
);

/* section.c *****************************************************************/

PVOID 
STDCALL
MmAllocateSection(
    IN ULONG Length,
    PVOID BaseAddress
);

NTSTATUS
STDCALL
MmQuerySectionView(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PULONG ResultLength
);

NTSTATUS
NTAPI
MmMapViewOfSection(
    IN PVOID SectionObject,
    IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSTATUS
NTAPI
MmProtectSectionView(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    ULONG Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmWritePageSectionView(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MArea,
    PVOID Address,
    PMM_PAGEOP PageOp
);

NTSTATUS
NTAPI
MmInitSectionImplementation(VOID);

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(
    PMADDRESS_SPACE AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

NTSTATUS
NTAPI
MmPageOutSectionView(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    struct _MM_PAGEOP *PageOp
);

NTSTATUS
NTAPI
MmCreatePhysicalMemorySection(VOID);

NTSTATUS
NTAPI
MmAccessFaultSectionView(
    PMADDRESS_SPACE AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

VOID
NTAPI
MmFreeSectionSegments(PFILE_OBJECT FileObject);

/* mpw.c *********************************************************************/

NTSTATUS
NTAPI
MmInitMpwThread(VOID);

/* pager.c *******************************************************************/

BOOLEAN
NTAPI
MiIsPagerThread(VOID);

VOID
NTAPI
MiStartPagerThread(VOID);

VOID
NTAPI
MiStopPagerThread(VOID);

NTSTATUS
FASTCALL
MiQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

#endif
