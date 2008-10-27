#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;

extern ULONG MiFreeSwapPages;
extern ULONG MiUsedSwapPages;
extern ULONG MmPagedPoolSize;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;
extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
extern ULONG MmNumberOfPhysicalPages;

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

extern PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
extern MEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorOrg;
extern ULONG MmHighestPhysicalPage;
extern PVOID MmPfnDatabase;

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

/* Number of list heads to use */
#define MI_FREE_POOL_LISTS 4

/* Signature of free pool blocks */
#define MM_FREE_POOL_TAG    TAG('F', 'r', 'p', 'l')

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             1024

#define TEB_BASE                            0x7FFDE000

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
    LONG FileOffset;		/* start offset into the file for image sections */
    ULONG_PTR VirtualAddress;	/* dtart offset into the address range for image sections */
    ULONG RawLength;		/* length of the segment which is part of the mapped file */
    ULONG Length;			/* absolute length of the segment */
    ULONG Protection;
    FAST_MUTEX Lock;		/* lock which protects the page directory */
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
    USHORT Subsystem;
    USHORT ImageCharacteristics;
    USHORT MinorSubsystemVersion;
    USHORT MajorSubsystemVersion;
    USHORT Machine;
    BOOLEAN Executable;
    ULONG NrSegments;
    ULONG ImageSize;
    PMM_SECTION_SEGMENT Segments;
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

typedef struct _ROS_SECTION_OBJECT
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
} ROS_SECTION_OBJECT, *PROS_SECTION_OBJECT;

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
    BOOLEAN DeleteInProgress;
    ULONG PageOpCount;
    union
    {
        struct
        {
            ROS_SECTION_OBJECT* Section;
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

typedef struct _MADDRESS_SPACE
{
    PMEMORY_AREA MemoryAreaRoot;
    PVOID LowestAddress;
    PEPROCESS Process;
    PUSHORT PageTableRefCountTable;
    PEX_PUSH_LOCK Lock;
} MADDRESS_SPACE, *PMADDRESS_SPACE;

typedef struct
{
    ULONG NrTotalPages;
    ULONG NrSystemPages;
    ULONG NrUserPages;
    ULONG NrFreePages;
    ULONG NrDirtyPages;
    ULONG NrLockedPages;
    ULONG PagingRequestsInLastMinute;
    ULONG PagingRequestsInLastFiveMinutes;
    ULONG PagingRequestsInLastFifteenMinutes;
} MM_STATS;

typedef struct _PHYSICAL_PAGE
{
    union
    {
        struct
        {
            ULONG Type: 2;
            ULONG Consumer: 3;
            ULONG Zero: 1;
            ULONG StartOfAllocation: 1;
            ULONG EndOfAllocation: 1;
        }
        Flags;
        ULONG AllFlags;
    };
    
    LIST_ENTRY ListEntry;
    ULONG ReferenceCount;
    SWAPENTRY SavedSwapEntry;
    ULONG LockCount;
    ULONG MapCount;
    struct _MM_RMAP_ENTRY* RmapListHead;
}
PHYSICAL_PAGE, *PPHYSICAL_PAGE;

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

/* Entry describing free pool memory */
typedef struct _MMFREE_POOL_ENTRY
{
    LIST_ENTRY List;
    PFN_NUMBER Size;
    ULONG Signature;
    struct _MMFREE_POOL_ENTRY *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;

/* Paged pool information */
typedef struct _MM_PAGED_POOL_INFO
{  
    PRTL_BITMAP PagedPoolAllocationMap;
    PRTL_BITMAP EndOfPagedPoolBitmap;
    PMMPTE FirstPteForPagedPool;
    PMMPTE LastPteForPagedPool;
    PMMPTE NextPdeForPagedPoolExpansion;
    ULONG PagedPoolHint;
    SIZE_T PagedPoolCommit;
    SIZE_T AllocatedPagedPool;
} MM_PAGED_POOL_INFO, *PMM_PAGED_POOL_INFO;

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

/* aspace.c ******************************************************************/

VOID
NTAPI
MmInitializeKernelAddressSpace(VOID);

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
NTAPI
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
NTAPI
MmLocateMemoryAreaByAddress(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address
);

ULONG_PTR
NTAPI
MmFindGapAtAddress(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address
);

NTSTATUS
NTAPI
MmFreeMemoryArea(
    PMADDRESS_SPACE AddressSpace,
    PMEMORY_AREA MemoryArea,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

NTSTATUS
NTAPI
MmFreeMemoryAreaByPtr(
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

VOID
NTAPI
MmDumpMemoryAreas(PMADDRESS_SPACE AddressSpace);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByRegion(
    PMADDRESS_SPACE AddressSpace,
    PVOID Address,
    ULONG_PTR Length
);

PVOID
NTAPI
MmFindGap(
    PMADDRESS_SPACE AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown
);

VOID
NTAPI
MmReleaseMemoryAreaIfDecommitted(
    struct _EPROCESS *Process,
    PMADDRESS_SPACE AddressSpace,
    PVOID BaseAddress
);

VOID
NTAPI
MmMapMemoryArea(PVOID BaseAddress,
                ULONG Length,
                ULONG Consumer,
                ULONG Protection);

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
MiAllocatePoolPages(
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes
);

POOL_TYPE
NTAPI
MmDeterminePoolType(
    IN PVOID VirtualAddress
);

ULONG
NTAPI
MiFreePoolPages(
    IN PVOID StartingAddress
);

PVOID
NTAPI
MmGetMdlPageAddress(
    PMDL Mdl,
    PVOID Offset
);

/* pool.c *******************************************************************/

PVOID
NTAPI
ExAllocateNonPagedPoolWithTag(
    POOL_TYPE type,
    ULONG size,
    ULONG Tag,
    PVOID Caller
);

PVOID
NTAPI
ExAllocatePagedPoolWithTag(
    POOL_TYPE Type,
    ULONG size,
    ULONG Tag
);

VOID
NTAPI
ExFreeNonPagedPool(PVOID block);

VOID
NTAPI
ExFreePagedPool(IN PVOID Block);

VOID
NTAPI
MmInitializePagedPool(VOID);

PVOID
NTAPI
MiAllocateSpecialPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN ULONG Underrun
);

BOOLEAN
NTAPI
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
    VOID
);

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock);

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

BOOLEAN
NTAPI
MmIsFileObjectAPagingFile(PFILE_OBJECT FileObject);

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
NTAPI
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
NTAPI
MmInitializeProcessAddressSpace(
    IN PEPROCESS Process,
    IN PEPROCESS Clone OPTIONAL,
    IN PVOID Section OPTIONAL,
    IN OUT PULONG Flags,
    IN POBJECT_NAME_INFORMATION *AuditName OPTIONAL
);

NTSTATUS
NTAPI
MmCreatePeb(struct _EPROCESS *Process);

PTEB
NTAPI
MmCreateTeb(
    struct _EPROCESS *Process,
    PCLIENT_ID ClientId,
    PINITIAL_TEB InitialTeb
);

VOID
NTAPI
MmDeleteTeb(
    struct _EPROCESS *Process,
    PTEB Teb
);

VOID
NTAPI
MmCleanProcessAddressSpace(IN PEPROCESS Process);

NTSTATUS
NTAPI
MmDeleteProcessAddressSpace(IN PEPROCESS Process);

ULONG
NTAPI
MmGetSessionLocaleId(VOID);

NTSTATUS
NTAPI
MmSetMemoryPriorityProcess(
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
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
    IN BOOLEAN StoreInstruction,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation
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
NTAPI
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
NTAPI
MmCreateKernelStack(BOOLEAN GuiStack, UCHAR Node);

VOID
NTAPI
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
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmDeleteAllRmaps(
    PFN_TYPE Page,
    PVOID Context,
    VOID (*DeleteMapping)(PVOID Context, struct _EPROCESS *Process, PVOID Address)
);

VOID
NTAPI
MmDeleteRmap(
    PFN_TYPE Page,
    struct _EPROCESS *Process,
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

#define ASSERT_PFN(x) ASSERT((x)->Flags.Type != 0)

FORCEINLINE
PPHYSICAL_PAGE
MiGetPfnEntry(IN PFN_TYPE Pfn)
{
    PPHYSICAL_PAGE Page;
    extern PPHYSICAL_PAGE MmPageArray;
    extern ULONG MmPageArraySize;

    /* Mark MmPageArraySize as unreferenced, otherwise it will appear as an unused variable on a Release build */
    UNREFERENCED_PARAMETER(MmPageArraySize);

    /* Make sure the PFN number is valid */
    ASSERT(Pfn <= MmPageArraySize);

    /* Get the entry */
    Page = &MmPageArray[Pfn];

    /* Make sure it's valid */
    ASSERT_PFN(Page);

    /* Return it */
    return Page;
};

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPage);

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID);

VOID
NTAPI
MmInsertLRULastUserPage(PFN_TYPE Page);

VOID
NTAPI
MmRemoveLRUUserPage(PFN_TYPE Page);

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

VOID
NTAPI
MmInitializePageList(
    VOID
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
MmZeroPageThreadMain(
    PVOID Context
);

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
    struct _EPROCESS *Process,
    PVOID Address,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

VOID
NTAPI
MmEnableVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address);

VOID
NTAPI
MmDeletePageFileMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    SWAPENTRY* SwapEntry
);

NTSTATUS
NTAPI
MmCreatePageFileMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    SWAPENTRY SwapEntry
);

BOOLEAN
NTAPI
MmIsPageSwapEntry(
    struct _EPROCESS *Process,
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
    struct _EPROCESS *Process,
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
    struct _EPROCESS *Process,
    PVOID Address
);

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsPageInUse(PFN_TYPE Page);

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
    struct _EPROCESS *Process,
    PVOID Address
);

NTSTATUS
NTAPI
MmCreatePageTable(PVOID PAddress);

VOID
NTAPI
MmDeletePageTable(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_TYPE
NTAPI
MmGetPfnForProcess(
    struct _EPROCESS *Process,
    PVOID Address
);

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(
    IN ULONG MinWs,
    IN PEPROCESS Dest,
    IN PLARGE_INTEGER DirectoryTableBase
);

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(
    IN PEPROCESS Process,
    IN PLARGE_INTEGER DirectoryTableBase
);


NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
MmReleaseMmInfo(struct _EPROCESS *Process);

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(struct _EPROCESS *Process);

VOID
NTAPI
MmDeleteVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    BOOLEAN FreePage,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

BOOLEAN
NTAPI
MmIsDirtyPage(
    struct _EPROCESS *Process,
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
    struct _EPROCESS *Process,
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

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(
    IN PROS_SECTION_OBJECT Section
);
NTSTATUS
NTAPI
MmGetFileNameForAddress(
    IN PVOID Address,
    OUT PUNICODE_STRING ModuleName
);

NTSTATUS
NTAPI
MmGetFileNameForSection(
    IN PROS_SECTION_OBJECT Section,
    OUT POBJECT_NAME_INFORMATION *ModuleName
);

PVOID
NTAPI
MmAllocateSection(
    IN ULONG Length,
    PVOID BaseAddress
);

NTSTATUS
NTAPI
MmQuerySectionView(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PULONG ResultLength
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

NTSTATUS
NTAPI
MmInitBsmThread(VOID);

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

/* sysldr.c ******************************************************************/

VOID
NTAPI
MiReloadBootLoadedDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
MiInitializeLoadedModuleList(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTSTATUS
NTAPI
MmLoadSystemImage(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedName OPTIONAL,
    IN ULONG Flags,
    OUT PVOID *ModuleObject,
    OUT PVOID *ImageBaseAddress
);

NTSTATUS
NTAPI
MmUnloadSystemImage(
    IN PVOID ImageHandle
);

NTSTATUS
NTAPI
MmCheckSystemImage(
    IN HANDLE ImageHandle,
    IN BOOLEAN PurgeSection
);

FORCEINLINE
VOID
NTAPI
MiSyncThreadProcessViews(IN PVOID Process,
                         IN PVOID Address,
                         IN ULONG Size)
{
    MmUpdatePageDir((PEPROCESS)Process, Address, Size);
}


extern MADDRESS_SPACE MmKernelAddressSpace;

FORCEINLINE
VOID
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(AddressSpace->Lock);
}

FORCEINLINE
VOID
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
    ExReleasePushLock(AddressSpace->Lock);
    KeLeaveCriticalRegion();
}

FORCEINLINE
PMADDRESS_SPACE
MmGetCurrentAddressSpace(VOID)
{
    return (PMADDRESS_SPACE)&((PEPROCESS)KeGetCurrentThread()->ApcState.Process)->VadRoot;
}

FORCEINLINE
PMADDRESS_SPACE
MmGetKernelAddressSpace(VOID)
{
    return &MmKernelAddressSpace;
}

#endif
