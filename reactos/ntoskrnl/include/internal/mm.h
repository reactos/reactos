#pragma once

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;

extern PMMSUPPORT MmKernelAddressSpace;
extern PFN_COUNT MiFreeSwapPages;
extern PFN_COUNT MiUsedSwapPages;
extern SIZE_T MmTotalPagedPoolQuota;
extern SIZE_T MmTotalNonPagedPoolQuota;
extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
extern PFN_COUNT MmNumberOfPhysicalPages;
extern UCHAR MmDisablePagingExecutive;
extern PFN_NUMBER MmLowestPhysicalPage;
extern PFN_NUMBER MmHighestPhysicalPage;
extern PFN_NUMBER MmAvailablePages;
extern PFN_NUMBER MmResidentAvailablePages;

extern PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
extern MEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorOrg;

extern LIST_ENTRY MmLoadedUserImageList;

extern KMUTANT MmSystemLoadLock;

extern ULONG MmNumberOfPagingFiles;

extern PVOID MmUnloadedDrivers;
extern PVOID MmLastUnloadedDrivers;
extern PVOID MmTriageActionTaken;
extern PVOID KernelVerifier;
extern MM_DRIVER_VERIFIER_DATA MmVerifierData;

extern SIZE_T MmTotalCommitLimit;
extern SIZE_T MmTotalCommittedPages;
extern SIZE_T MmSharedCommit;
extern SIZE_T MmDriverCommit;
extern SIZE_T MmProcessCommit;
extern SIZE_T MmPagedPoolCommit;
extern SIZE_T MmPeakCommitment;
extern SIZE_T MmtotalCommitLimitMaximum;

extern PVOID MiDebugMapping;
extern PMMPTE MmDebugPte;

struct _KTRAP_FRAME;
struct _EPROCESS;
struct _MM_RMAP_ENTRY;
typedef ULONG_PTR SWAPENTRY;

//
// MmDbgCopyMemory Flags
//
#define MMDBG_COPY_WRITE            0x00000001
#define MMDBG_COPY_PHYSICAL         0x00000002
#define MMDBG_COPY_UNSAFE           0x00000004
#define MMDBG_COPY_CACHED           0x00000008
#define MMDBG_COPY_UNCACHED         0x00000010
#define MMDBG_COPY_WRITE_COMBINED   0x00000020

//
// Maximum chunk size per copy
//
#define MMDBG_COPY_MAX_SIZE         0x8

#if defined(_X86_)
#define MI_STATIC_MEMORY_AREAS              (14)
#else
#define MI_STATIC_MEMORY_AREAS              (13)
#endif

#define MEMORY_AREA_SECTION_VIEW            (1)
#define MEMORY_AREA_CACHE                   (2)
#define MEMORY_AREA_OWNED_BY_ARM3           (15)
#define MEMORY_AREA_STATIC                  (0x80000000)

#define MM_PHYSICAL_PAGE_MPW_PENDING        (0x8)

#define MM_CORE_DUMP_TYPE_NONE              (0x0)
#define MM_CORE_DUMP_TYPE_MINIMAL           (0x1)
#define MM_CORE_DUMP_TYPE_FULL              (0x2)

/* Number of list heads to use */
#define MI_FREE_POOL_LISTS 4


/* Signature of free pool blocks */
#define MM_FREE_POOL_TAG    'lprF'

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
#define MC_SYSTEM                           (2)
#define MC_MAXIMUM                          (3)

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

#define PAGE_FLAGS_VALID_FOR_SECTION \
    (PAGE_READONLY | \
     PAGE_READWRITE | \
     PAGE_WRITECOPY | \
     PAGE_EXECUTE | \
     PAGE_EXECUTE_READ | \
     PAGE_EXECUTE_READWRITE | \
     PAGE_EXECUTE_WRITECOPY | \
     PAGE_NOACCESS)

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


#define InterlockedCompareExchangePte(PointerPte, Exchange, Comperand) \
    InterlockedCompareExchange((PLONG)(PointerPte), Exchange, Comperand)

#define InterlockedExchangePte(PointerPte, Value) \
    InterlockedExchange((PLONG)(PointerPte), Value)

typedef struct _MM_SECTION_SEGMENT
{
    FAST_MUTEX Lock;		/* lock which protects the page directory */
	PFILE_OBJECT FileObject;
    LARGE_INTEGER RawLength;		/* length of the segment which is part of the mapped file */
    LARGE_INTEGER Length;			/* absolute length of the segment */
    ULONG ReferenceCount;
	ULONG CacheCount;
    ULONG Protection;
    ULONG Flags;
    BOOLEAN WriteCopy;
	BOOLEAN Locked;

	struct
	{
		ULONGLONG FileOffset;		/* start offset into the file for image sections */
		ULONG_PTR VirtualAddress;	/* start offset into the address range for image sections */
		ULONG Characteristics;
	} Image;

	LIST_ENTRY ListOfSegments;
	RTL_GENERIC_TABLE PageTable;
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
    PVOID Vad;
    union
    {
        struct
        {
            ROS_SECTION_OBJECT* Section;
            LARGE_INTEGER ViewOffset;
            PMM_SECTION_SEGMENT Segment;
            LIST_ENTRY RegionListHead;
        } SectionData;
        struct
        {
            LIST_ENTRY RegionListHead;
        } VirtualMemoryData;
    } Data;
} MEMORY_AREA, *PMEMORY_AREA;

typedef struct _MM_RMAP_ENTRY
{
   struct _MM_RMAP_ENTRY* Next;
   PEPROCESS Process;
   PVOID Address;
#if DBG
   PVOID Caller;
#endif
}
MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

#if MI_TRACE_PFNS
extern ULONG MI_PFN_CURRENT_USAGE;
extern CHAR MI_PFN_CURRENT_PROCESS_NAME[16];
#define MI_SET_USAGE(x)     MI_PFN_CURRENT_USAGE = x
#define MI_SET_PROCESS2(x)  memcpy(MI_PFN_CURRENT_PROCESS_NAME, x, 16)
#else
#define MI_SET_USAGE(x)
#define MI_SET_PROCESS2(x)
#endif

typedef enum _MI_PFN_USAGES
{
    MI_USAGE_NOT_SET = 0,
    MI_USAGE_PAGED_POOL,
    MI_USAGE_NONPAGED_POOL,
    MI_USAGE_NONPAGED_POOL_EXPANSION,
    MI_USAGE_KERNEL_STACK,
    MI_USAGE_KERNEL_STACK_EXPANSION,
    MI_USAGE_SYSTEM_PTE,
    MI_USAGE_VAD,
    MI_USAGE_PEB_TEB,
    MI_USAGE_SECTION,
    MI_USAGE_PAGE_TABLE,
    MI_USAGE_PAGE_DIRECTORY,
    MI_USAGE_LEGACY_PAGE_DIRECTORY,
    MI_USAGE_DRIVER_PAGE,
    MI_USAGE_CONTINOUS_ALLOCATION,
    MI_USAGE_MDL,
    MI_USAGE_DEMAND_ZERO,
    MI_USAGE_ZERO_LOOP,
    MI_USAGE_CACHE,
    MI_USAGE_PFN_DATABASE,
    MI_USAGE_BOOT_DRIVER,
    MI_USAGE_INIT_MEMORY,
    MI_USAGE_FREE_PAGE
} MI_PFN_USAGES;

//
// These two mappings are actually used by Windows itself, based on the ASSERTS
//
#define StartOfAllocation ReadInProgress
#define EndOfAllocation WriteInProgress

typedef struct _MMPFNENTRY
{
    USHORT Modified:1;
    USHORT ReadInProgress:1;                 // StartOfAllocation
    USHORT WriteInProgress:1;                // EndOfAllocation
    USHORT PrototypePte:1;
    USHORT PageColor:4;
    USHORT PageLocation:3;
    USHORT RemovalRequested:1;
    USHORT CacheAttribute:2;
    USHORT Rom:1;
    USHORT ParityError:1;                    // HasRmap
} MMPFNENTRY;

typedef struct _MMPFN
{
    union
    {
        PFN_NUMBER Flink;
        ULONG WsIndex;                       // SavedSwapEntry
        PKEVENT Event;
        NTSTATUS ReadStatus;
        SINGLE_LIST_ENTRY NextStackPfn;
    } u1;
    PMMPTE PteAddress;
    union
    {
        PFN_NUMBER Blink;
        ULONG_PTR ShareCount;
    } u2;
    union
    {
        struct
        {
            USHORT ReferenceCount;           // ReferenceCount
            MMPFNENTRY e1;
        };
        struct
        {
            USHORT ReferenceCount;
            USHORT ShortFlags;
        } e2;
    } u3;
    union
    {
        MMPTE OriginalPte;
        LONG AweReferenceCount;              // RmapListHead
    };
    union
    {
        ULONG_PTR EntireFrame;
        struct
        {
            ULONG_PTR PteFrame:25;
            ULONG_PTR InPageError:1;
            ULONG_PTR VerifierAllocation:1;
            ULONG_PTR AweAllocation:1;
            ULONG_PTR Priority:3;
            ULONG_PTR MustBeCached:1;
        };
    } u4;
#if MI_TRACE_PFNS
    MI_PFN_USAGES PfnUsage;
    CHAR ProcessName[16];
#endif
} MMPFN, *PMMPFN;

extern PMMPFN MmPfnDatabase;

typedef struct _MMPFNLIST
{
    PFN_NUMBER Total;
    MMLISTS ListName;
    PFN_NUMBER Flink;
    PFN_NUMBER Blink;
} MMPFNLIST, *PMMPFNLIST;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;

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
    SIZE_T Length;
    LIST_ENTRY RegionListEntry;
} MM_REGION, *PMM_REGION;

/* Entry describing free pool memory */
typedef struct _MMFREE_POOL_ENTRY
{
    LIST_ENTRY List;
    PFN_COUNT Size;
    ULONG Signature;
    struct _MMFREE_POOL_ENTRY *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;

/* Signature of a freed block */
#define MM_FREE_POOL_SIGNATURE 'ARM3'

/* Paged pool information */
typedef struct _MM_PAGED_POOL_INFO
{
    PRTL_BITMAP PagedPoolAllocationMap;
    PRTL_BITMAP EndOfPagedPoolBitmap;
    PMMPTE FirstPteForPagedPool;
    PMMPTE LastPteForPagedPool;
    PMMPDE NextPdeForPagedPoolExpansion;
    ULONG PagedPoolHint;
    SIZE_T PagedPoolCommit;
    SIZE_T AllocatedPagedPool;
} MM_PAGED_POOL_INFO, *PMM_PAGED_POOL_INFO;

extern MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];

typedef VOID
(*PMM_ALTER_REGION_FUNC)(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    SIZE_T Length,
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
    PFN_NUMBER Page,
    SWAPENTRY SwapEntry,
    BOOLEAN Dirty
);

//
// Mm copy support for Kd
//
NTSTATUS
NTAPI
MmDbgCopyMemory(
    IN ULONG64 Address,
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Flags
);

//
// Determines if a given address is a session address
//
BOOLEAN
NTAPI
MmIsSessionAddress(
    IN PVOID Address
);

/* marea.c *******************************************************************/

NTSTATUS
NTAPI
MmCreateMemoryArea(
    PMMSUPPORT AddressSpace,
    ULONG Type,
    PVOID *BaseAddress,
    SIZE_T Length,
    ULONG Protection,
    PMEMORY_AREA *Result,
    BOOLEAN FixedAddress,
    ULONG AllocationFlags,
    PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL
);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByAddress(
    PMMSUPPORT AddressSpace,
    PVOID Address
);

ULONG_PTR
NTAPI
MmFindGapAtAddress(
    PMMSUPPORT AddressSpace,
    PVOID Address
);

NTSTATUS
NTAPI
MmFreeMemoryArea(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

NTSTATUS
NTAPI
MmFreeMemoryAreaByPtr(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

VOID
NTAPI
MmDumpMemoryAreas(PMMSUPPORT AddressSpace);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByRegion(
    PMMSUPPORT AddressSpace,
    PVOID Address,
    SIZE_T Length
);

PVOID
NTAPI
MmFindGap(
    PMMSUPPORT AddressSpace,
    SIZE_T Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown
);

VOID
NTAPI
MmReleaseMemoryAreaIfDecommitted(
    struct _EPROCESS *Process,
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress
);

VOID
NTAPI
MmMapMemoryArea(PVOID BaseAddress,
                SIZE_T Length,
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

BOOLEAN
NTAPI
ExpIsPoolTagDebuggable(ULONG Tag);

PVOID
NTAPI
ExpAllocateDebugPool(
    POOL_TYPE Type,
    ULONG Size,
    ULONG Tag,
    PVOID Caller,
    BOOLEAN EndOfPage
);

VOID
NTAPI
ExpFreeDebugPool(PVOID Block, BOOLEAN PagedPool);

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
    PPFN_NUMBER Pages
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
    PFN_NUMBER Page
);

BOOLEAN
NTAPI
MmReserveSwapPages(ULONG Nr);

NTSTATUS
NTAPI
MmWriteToSwapPage(
    SWAPENTRY SwapEntry,
    PFN_NUMBER Page
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
MmCreatePeb(
    IN PEPROCESS Process,
    IN PINITIAL_PEB InitialPeb,
    OUT PPEB *BasePeb
);

NTSTATUS
NTAPI
MmCreateTeb(
    IN PEPROCESS Process,
    IN PCLIENT_ID ClientId,
    IN PINITIAL_TEB InitialTeb,
    OUT PTEB* BaseTeb
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

/* special.c *****************************************************************/

VOID
NTAPI
MiInitializeSpecialPool();

BOOLEAN
NTAPI
MmUseSpecialPool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

BOOLEAN
NTAPI
MmIsSpecialPoolAddress(
    IN PVOID P);

PVOID
NTAPI
MmAllocateSpecialPool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialType);

VOID
NTAPI
MmFreeSpecialPool(
    IN PVOID P);

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
    PMMSUPPORT AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address
);

NTSTATUS
NTAPI
MmPageOutVirtualMemory(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmQueryAnonMem(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PSIZE_T ResultLength
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
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    SIZE_T Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmWritePageVirtualMemory(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MArea,
    PVOID Address,
    PFN_NUMBER Page
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
ExAllocatePageWithPhysPage(PFN_NUMBER Page);

NTSTATUS
NTAPI
MiCopyFromUserPage(
    PFN_NUMBER Page,
    PVOID SourceAddress
);

NTSTATUS
NTAPI
MiZeroPage(PFN_NUMBER Page);

/* memsafe.s *****************************************************************/

PVOID
FASTCALL
MmSafeReadPtr(PVOID Source);

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
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(
    ULONG Consumer,
    BOOLEAN MyWait,
    PPFN_NUMBER AllocatedPage
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
    PFN_NUMBER Page,
    struct _MM_RMAP_ENTRY* ListHead
);

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_NUMBER Page);

VOID
NTAPI
MmInsertRmap(
    PFN_NUMBER Page,
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmDeleteAllRmaps(
    PFN_NUMBER Page,
    PVOID Context,
    VOID (*DeleteMapping)(PVOID Context, struct _EPROCESS *Process, PVOID Address)
);

VOID
NTAPI
MmDeleteRmap(
    PFN_NUMBER Page,
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmInitializeRmapList(VOID);

VOID
NTAPI
MmSetCleanAllRmaps(PFN_NUMBER Page);

VOID
NTAPI
MmSetDirtyAllRmaps(PFN_NUMBER Page);

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_NUMBER Page);

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_NUMBER Page);

/* freelist.c **********************************************************/

FORCEINLINE
PMMPFN
MiGetPfnEntry(IN PFN_NUMBER Pfn)
{
    PMMPFN Page;
    extern RTL_BITMAP MiPfnBitMap;

    /* Make sure the PFN number is valid */
    if (Pfn > MmHighestPhysicalPage) return NULL;

    /* Make sure this page actually has a PFN entry */
    if ((MiPfnBitMap.Buffer) && !(RtlTestBit(&MiPfnBitMap, (ULONG)Pfn))) return NULL;

    /* Get the entry */
    Page = &MmPfnDatabase[Pfn];

    /* Return it */
    return Page;
};

FORCEINLINE
PFN_NUMBER
MiGetPfnEntryIndex(IN PMMPFN Pfn1)
{
    //
    // This will return the Page Frame Number (PFN) from the MMPFN
    //
    return Pfn1 - MmPfnDatabase;
}

PFN_NUMBER
NTAPI
MmGetLRUNextUserPage(PFN_NUMBER PreviousPage);

PFN_NUMBER
NTAPI
MmGetLRUFirstUserPage(VOID);

VOID
NTAPI
MmInsertLRULastUserPage(PFN_NUMBER Page);

VOID
NTAPI
MmRemoveLRUUserPage(PFN_NUMBER Page);

VOID
NTAPI
MmLockPage(PFN_NUMBER Page);

VOID
NTAPI
MmUnlockPage(PFN_NUMBER Page);

ULONG
NTAPI
MmGetLockCountPage(PFN_NUMBER Page);

VOID
NTAPI
MmInitializePageList(
    VOID
);

VOID
NTAPI
MmDumpArmPfnDatabase(
   IN BOOLEAN StatusOnly
);

PFN_NUMBER
NTAPI
MmGetContinuousPages(
    ULONG NumberOfBytes,
    PHYSICAL_ADDRESS LowestAcceptableAddress,
    PHYSICAL_ADDRESS HighestAcceptableAddress,
    PHYSICAL_ADDRESS BoundaryAddressMultiple,
    BOOLEAN ZeroPages
);

VOID
NTAPI
MmZeroPageThread(
    VOID
);

/* hypermap.c *****************************************************************/

extern PEPROCESS HyperProcess;
extern KIRQL HyperIrql;

PVOID
NTAPI
MiMapPageInHyperSpace(IN PEPROCESS Process,
                      IN PFN_NUMBER Page,
                      IN PKIRQL OldIrql);

VOID
NTAPI
MiUnmapPageInHyperSpace(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN KIRQL OldIrql);

PVOID
NTAPI
MiMapPagesInZeroSpace(IN PMMPFN Pfn1,
                      IN PFN_NUMBER NumberOfPages);

VOID
NTAPI
MiUnmapPagesInZeroSpace(IN PVOID VirtualAddress,
                        IN PFN_NUMBER NumberOfPages);

//
// ReactOS Compatibility Layer
//
FORCEINLINE
PVOID
MmCreateHyperspaceMapping(IN PFN_NUMBER Page)
{
    HyperProcess = (PEPROCESS)KeGetCurrentThread()->ApcState.Process;
    return MiMapPageInHyperSpace(HyperProcess, Page, &HyperIrql);
}

#define MmDeleteHyperspaceMapping(x) MiUnmapPageInHyperSpace(HyperProcess, x, HyperIrql);

/* i386/page.c *********************************************************/

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(
    PVOID Address,
    ULONG flProtect,
    PPFN_NUMBER Pages,
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
    PPFN_NUMBER Pages,
    ULONG PageCount
);

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PPFN_NUMBER Pages,
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
    PPFN_NUMBER Page
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
MmGetPageFileMapping(
	struct _EPROCESS *Process,
	PVOID Address,
	SWAPENTRY* SwapEntry);

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
    PFN_NUMBER Page,
    ULONG NewConsumer
);

VOID
NTAPI
MmSetDirtyPage(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_NUMBER
NTAPI
MmAllocPage(
    ULONG Consumer
);

LONG
NTAPI
MmAllocPagesSpecifyRange(
    ULONG Consumer,
    PHYSICAL_ADDRESS LowestAddress,
    PHYSICAL_ADDRESS HighestAddress,
    ULONG NumberOfPages,
    PPFN_NUMBER Pages
);

VOID
NTAPI
MmDereferencePage(PFN_NUMBER Page);

VOID
NTAPI
MmReferencePage(PFN_NUMBER Page);

ULONG
NTAPI
MmGetReferenceCountPage(PFN_NUMBER Page);

BOOLEAN
NTAPI
MmIsPageInUse(PFN_NUMBER Page);

VOID
NTAPI
MmSetSavedSwapEntryPage(
    PFN_NUMBER Page,
    SWAPENTRY SavedSwapEntry);

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_NUMBER Page);

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

PFN_NUMBER
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
    IN PULONG_PTR DirectoryTableBase
);

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(
    IN PEPROCESS Process,
    IN PULONG_PTR DirectoryTableBase
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
MmSetExecuteOptions(IN ULONG ExecuteOptions);

NTSTATUS
NTAPI
MmGetExecuteOptions(IN PULONG ExecuteOptions);

VOID
NTAPI
MmDeleteProcessPageDirectory(struct _EPROCESS *Process);

VOID
NTAPI
MmDeleteVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    BOOLEAN FreePage,
    BOOLEAN* WasDirty,
    PPFN_NUMBER Page
);

BOOLEAN
NTAPI
MmIsDirtyPage(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmMarkPageMapped(PFN_NUMBER Page);

VOID
NTAPI
MmMarkPageUnmapped(PFN_NUMBER Page);

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
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    PLIST_ENTRY RegionListHead,
    PVOID StartAddress,
    SIZE_T Length,
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
    IN PVOID Section
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
    IN PVOID Section,
    OUT POBJECT_NAME_INFORMATION *ModuleName
);

PVOID
NTAPI
MmAllocateSection(
    IN SIZE_T Length,
    PVOID BaseAddress
);

NTSTATUS
NTAPI
MmQuerySectionView(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PSIZE_T ResultLength
);

NTSTATUS
NTAPI
MmProtectSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    SIZE_T Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmInitSectionImplementation(VOID);

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(
    PMMSUPPORT AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

NTSTATUS
NTAPI
MmPageOutSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    ULONG_PTR Entry
);

NTSTATUS
NTAPI
MmCreatePhysicalMemorySection(VOID);

NTSTATUS
NTAPI
MmAccessFaultSectionView(
    PMMSUPPORT AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address
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
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
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

NTSTATUS
NTAPI
MmCallDllInitialize(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    IN PLIST_ENTRY ListHead
);


/* procsup.c *****************************************************************/

NTSTATUS
NTAPI
MmGrowKernelStack(
    IN PVOID StackPointer
);


FORCEINLINE
VOID
MmLockAddressSpace(PMMSUPPORT AddressSpace)
{
    KeAcquireGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
    //ASSERT(Thread->OwnsProcessAddressSpaceExclusive == 0);
    //Thread->OwnsProcessAddressSpaceExclusive = TRUE;
}

FORCEINLINE
VOID
MmUnlockAddressSpace(PMMSUPPORT AddressSpace)
{
    //ASSERT(Thread->OwnsProcessAddressSpaceExclusive == 1);
    KeReleaseGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
    //Thread->OwnsProcessAddressSpaceExclusive = 0;
}

FORCEINLINE
PEPROCESS
MmGetAddressSpaceOwner(IN PMMSUPPORT AddressSpace)
{
    if (AddressSpace == MmKernelAddressSpace) return NULL;
    return CONTAINING_RECORD(AddressSpace, EPROCESS, Vm);
}

FORCEINLINE
PMMSUPPORT
MmGetCurrentAddressSpace(VOID)
{
    return &((PEPROCESS)KeGetCurrentThread()->ApcState.Process)->Vm;
}

FORCEINLINE
PMMSUPPORT
MmGetKernelAddressSpace(VOID)
{
    return MmKernelAddressSpace;
}
