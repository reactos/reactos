#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

extern ULONG MiFreeSwapPages;
extern ULONG MiUsedSwapPages;
extern ULONG MmPagedPoolSize;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;

struct _EPROCESS;
struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP
;
typedef ULONG SWAPENTRY;
typedef ULONG PFN_TYPE, *PPFN_TYPE;

#define MEMORY_AREA_INVALID              (0)
#define MEMORY_AREA_SECTION_VIEW         (1)
#define MEMORY_AREA_CONTINUOUS_MEMORY    (2)
#define MEMORY_AREA_NO_CACHE             (3)
#define MEMORY_AREA_IO_MAPPING           (4)
#define MEMORY_AREA_SYSTEM               (5)
#define MEMORY_AREA_MDL_MAPPING          (7)
#define MEMORY_AREA_VIRTUAL_MEMORY       (8)
#define MEMORY_AREA_CACHE_SEGMENT        (9)
#define MEMORY_AREA_SHARED_DATA          (10)
#define MEMORY_AREA_KERNEL_STACK         (11)
#define MEMORY_AREA_PAGED_POOL           (12)
#define MEMORY_AREA_NO_ACCESS            (13)
#define MEMORY_AREA_PEB_OR_TEB           (14)

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)

#define TEB_BASE        (0x7FFDE000)
#define KPCR_BASE                 0xFF000000

/* Although Microsoft says this isn't hardcoded anymore,
   they won't be able to change it. Stuff depends on it */
#define MM_VIRTMEM_GRANULARITY (64 * 1024) 

#define STATUS_MM_RESTART_OPERATION       ((NTSTATUS)0xD0000001)

/*
 * Additional flags for protection attributes
 */
#define PAGE_WRITETHROUGH                       (1024)
#define PAGE_SYSTEM                             (2048)
#define PAGE_FLAGS_VALID_FROM_USER_MODE               (PAGE_READONLY | \
						 PAGE_READWRITE | \
						 PAGE_WRITECOPY | \
						 PAGE_EXECUTE | \
						 PAGE_EXECUTE_READ | \
						 PAGE_EXECUTE_READWRITE | \
						 PAGE_EXECUTE_WRITECOPY | \
						 PAGE_GUARD | \
						 PAGE_NOACCESS | \
						 PAGE_NOCACHE)

#define PAGE_IS_READABLE (PAGE_READONLY | \
                          PAGE_READWRITE | \
                          PAGE_WRITECOPY | \
                          PAGE_EXECUTE_READ | \
                          PAGE_EXECUTE_READWRITE | \
                          PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITABLE (PAGE_READWRITE | \
                          PAGE_WRITECOPY | \
                          PAGE_EXECUTE_READWRITE | \
                          PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_EXECUTABLE (PAGE_EXECUTE | \
                            PAGE_EXECUTE_READ | \
                            PAGE_EXECUTE_READWRITE | \
                            PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITECOPY (PAGE_WRITECOPY | \
                           PAGE_EXECUTE_WRITECOPY)

typedef struct
{
  ULONG Entry[NR_SECTION_PAGE_ENTRIES];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

typedef struct
{
   PSECTION_PAGE_TABLE PageTables[NR_SECTION_PAGE_TABLES];
} SECTION_PAGE_DIRECTORY, *PSECTION_PAGE_DIRECTORY;

#define SEC_PHYSICALMEMORY     (0x80000000)

#define MM_PAGEFILE_SEGMENT    (0x1)
#define MM_DATAFILE_SEGMENT    (0x2)

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
} SECTION_OBJECT;

typedef struct _MEMORY_AREA
{
  PVOID StartingAddress;
  PVOID EndingAddress;
  struct _MEMORY_AREA *Parent;
  struct _MEMORY_AREA *LeftChild;
  struct _MEMORY_AREA *RightChild;
  ULONG Type;
  ULONG Attributes;
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

typedef struct _MADDRESS_SPACE
{
  PMEMORY_AREA MemoryAreaRoot;
  FAST_MUTEX Lock;
  PVOID LowestAddress;
  struct _EPROCESS* Process;
  PUSHORT PageTableRefCountTable;
  ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;

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

#define MM_PHYSICAL_PAGE_MPW_PENDING     (0x8)

#define MM_CORE_DUMP_TYPE_NONE            (0x0)
#define MM_CORE_DUMP_TYPE_MINIMAL         (0x1)
#define MM_CORE_DUMP_TYPE_FULL            (0x2)

#define MM_PAGEOP_PAGEIN        (1)
#define MM_PAGEOP_PAGEOUT       (2)
#define MM_PAGEOP_PAGESYNCH     (3)
#define MM_PAGEOP_ACCESSFAULT   (4)

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

#define MC_CACHE          (0)
#define MC_USER           (1)
#define MC_PPOOL          (2)
#define MC_NPPOOL         (3)
#define MC_MAXIMUM        (4)

typedef struct _MM_MEMORY_CONSUMER
{
   ULONG PagesUsed;
   ULONG PagesTarget;
   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
}
MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

extern MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];

extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
struct _KTRAP_FRAME;

typedef VOID (*PMM_ALTER_REGION_FUNC)(PMADDRESS_SPACE AddressSpace,
				      PVOID BaseAddress, ULONG Length,
				      ULONG OldType, ULONG OldProtect,
				      ULONG NewType, ULONG NewProtect);

typedef struct _MM_REGION
{
   ULONG Type;
   ULONG Protect;
   ULONG Length;
   LIST_ENTRY RegionListEntry;
} MM_REGION, *PMM_REGION;

typedef VOID (*PMM_FREE_PAGE_FUNC)(PVOID Context, PMEMORY_AREA MemoryArea,
                                   PVOID Address, PFN_TYPE Page,
                                   SWAPENTRY SwapEntry, BOOLEAN Dirty);

PVOID STDCALL ExAllocateNonPagedPoolWithTag (POOL_TYPE	type,
					     ULONG		size,
					     ULONG		Tag,
					     PVOID		Caller);

PVOID STDCALL ExAllocatePagedPoolWithTag (POOL_TYPE	Type,
					  ULONG		size,
					  ULONG		Tag);
VOID STDCALL ExFreeNonPagedPool (PVOID block);

VOID STDCALL
ExFreePagedPool(IN PVOID Block);
VOID MmInitializePagedPool(VOID);

PVOID
STDCALL
MiAllocateSpecialPool  (IN POOL_TYPE PoolType,
                        IN SIZE_T NumberOfBytes,
                        IN ULONG Tag,
                        IN ULONG Underrun
                        );

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

#define MM_PAGED_POOL_SIZE	(100*1024*1024)
#define MM_NONPAGED_POOL_SIZE	(100*1024*1024)

/*
 * Paged and non-paged pools are 8-byte aligned
 */
#define MM_POOL_ALIGNMENT	8

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define MM_KERNEL_MAP_SIZE	(16*1024*1024)
#define MM_KERNEL_MAP_BASE	(0xf0c00000)

/*
 * FIXME - different architectures have different cache line sizes...
 */
#define MM_CACHE_LINE_SIZE  32

#define MM_ROUND_UP(x,s)    ((PVOID)(((ULONG_PTR)(x)+(s)-1) & ~((ULONG_PTR)(s)-1)))
#define MM_ROUND_DOWN(x,s)  ((PVOID)(((ULONG_PTR)(x)) & ~((ULONG_PTR)(s)-1)))

/* FUNCTIONS */

/* aspace.c ******************************************************************/

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace);

VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace);

VOID MmInitializeKernelAddressSpace(VOID);

PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID);

PMADDRESS_SPACE MmGetKernelAddressSpace(VOID);

NTSTATUS MmInitializeAddressSpace(struct _EPROCESS* Process,
				  PMADDRESS_SPACE AddressSpace);

NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace);

/* marea.c *******************************************************************/

NTSTATUS
MmInitMemoryAreas(VOID);

NTSTATUS STDCALL
MmCreateMemoryArea(
   struct _EPROCESS* Process,
   PMADDRESS_SPACE AddressSpace,
   ULONG Type,
   PVOID *BaseAddress,
   ULONG_PTR Length,
   ULONG Attributes,
   PMEMORY_AREA *Result,
   BOOLEAN FixedAddress,
   BOOLEAN TopDown,
   PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL);

PMEMORY_AREA STDCALL
MmLocateMemoryAreaByAddress(
   PMADDRESS_SPACE AddressSpace,
   PVOID Address);

ULONG_PTR STDCALL
MmFindGapAtAddress(
   PMADDRESS_SPACE AddressSpace,
   PVOID Address);

NTSTATUS STDCALL
MmFreeMemoryArea(
   PMADDRESS_SPACE AddressSpace,
   PMEMORY_AREA MemoryArea,
   PMM_FREE_PAGE_FUNC FreePage,
   PVOID FreePageContext);

NTSTATUS STDCALL
MmFreeMemoryAreaByPtr(
   PMADDRESS_SPACE AddressSpace,
   PVOID BaseAddress,
   PMM_FREE_PAGE_FUNC FreePage,
   PVOID FreePageContext);

VOID STDCALL
MmDumpMemoryAreas(PMADDRESS_SPACE AddressSpace);

PMEMORY_AREA STDCALL
MmLocateMemoryAreaByRegion(
   PMADDRESS_SPACE AddressSpace,
   PVOID Address,
   ULONG_PTR Length);

PVOID STDCALL
MmFindGap(
   PMADDRESS_SPACE AddressSpace,
   ULONG_PTR Length,
   ULONG_PTR Granularity,
   BOOLEAN TopDown);

VOID STDCALL
MmReleaseMemoryAreaIfDecommitted(
   PEPROCESS Process,
   PMADDRESS_SPACE AddressSpace,
   PVOID BaseAddress);

/* npool.c *******************************************************************/

VOID MiDebugDumpNonPagedPool(BOOLEAN NewOnly);

VOID MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);

VOID MiInitializeNonPagedPool(VOID);

PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);

/* pool.c *******************************************************************/

BOOLEAN
STDCALL
MiRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN ULONG CurrentMaxQuota,
    OUT PULONG NewMaxQuota
    );

/* mdl.c *********************************************************************/

VOID MmBuildMdlFromPages(PMDL Mdl, PULONG Pages);

/* mminit.c ******************************************************************/

VOID MiShutdownMemoryManager(VOID);

VOID MmInit1(ULONG_PTR FirstKernelPhysAddress,
	     ULONG_PTR LastKernelPhysAddress,
	     ULONG_PTR LastKernelAddress,
	     PADDRESS_RANGE BIOSMemoryMap,
	     ULONG AddressRangeCount,
	     ULONG MaxMemInMeg);

VOID MmInit2(VOID);

VOID MmInit3(VOID);

VOID MiFreeInitMemory(VOID);

VOID MmInitializeMdlImplementation(VOID);

/* pagefile.c ****************************************************************/

SWAPENTRY MmAllocSwapPage(VOID);

VOID MmDereserveSwapPages(ULONG Nr);

VOID MmFreeSwapPage(SWAPENTRY Entry);

VOID MmInitPagingFile(VOID);

NTSTATUS MmReadFromSwapPage(SWAPENTRY SwapEntry, PFN_TYPE Page);

BOOLEAN MmReserveSwapPages(ULONG Nr);

NTSTATUS MmWriteToSwapPage(SWAPENTRY SwapEntry, PFN_TYPE Page);

NTSTATUS STDCALL
MmDumpToPagingFile(ULONG BugCode,
		   ULONG BugCodeParameter1,
		   ULONG BugCodeParameter2,
		   ULONG BugCodeParameter3,
		   ULONG BugCodeParameter4,
		   struct _KTRAP_FRAME* TrapFrame);

BOOLEAN MmIsAvailableSwapPage(VOID);

VOID MmShowOutOfSpaceMessagePagingFile(VOID);

/* process.c ****************************************************************/

NTSTATUS
STDCALL
MmCreateProcessAddressSpace(IN struct _EPROCESS* Process,
                            IN PSECTION_OBJECT Section OPTIONAL);

NTSTATUS
STDCALL
MmCreatePeb(PEPROCESS Process);

struct _TEB*
STDCALL
MmCreateTeb(PEPROCESS Process,
            PCLIENT_ID ClientId,
            PINITIAL_TEB InitialTeb);

VOID
STDCALL
MmDeleteTeb(PEPROCESS Process,
            struct _TEB* Teb);

/* i386/pfault.c *************************************************************/

NTSTATUS MmPageFault(ULONG Cs,
		     PULONG Eip,
		     PULONG Eax,
		     ULONG Cr2,
		     ULONG ErrorCode);

/* mm.c **********************************************************************/

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG_PTR Address,
		       BOOLEAN FromMdl);

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG_PTR Address,
			   BOOLEAN FromMdl);

/* anonmem.c *****************************************************************/

NTSTATUS MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
					MEMORY_AREA* MemoryArea,
					PVOID Address,
					BOOLEAN Locked);

NTSTATUS MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
				PMEMORY_AREA MemoryArea,
				PVOID Address,
				struct _MM_PAGEOP* PageOp);
NTSTATUS STDCALL
MmQueryAnonMem(PMEMORY_AREA MemoryArea,
	       PVOID Address,
	       PMEMORY_BASIC_INFORMATION Info,
	       PULONG ResultLength);

VOID MmFreeVirtualMemory(struct _EPROCESS* Process, PMEMORY_AREA MemoryArea);

NTSTATUS MmProtectAnonMem(PMADDRESS_SPACE AddressSpace,
			  PMEMORY_AREA MemoryArea,
			  PVOID BaseAddress,
			  ULONG Length,
			  ULONG Protect,
			  PULONG OldProtect);

NTSTATUS MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
				  PMEMORY_AREA MArea,
				  PVOID Address,
				  PMM_PAGEOP PageOp);

/* kmap.c ********************************************************************/

PVOID ExAllocatePage(VOID);

VOID ExUnmapPage(PVOID Addr);

PVOID ExAllocatePageWithPhysPage(PFN_TYPE Page);

NTSTATUS MiCopyFromUserPage(PFN_TYPE Page, PVOID SourceAddress);

NTSTATUS MiZeroPage(PFN_TYPE Page);

/* memsafe.s *****************************************************************/

PVOID FASTCALL MmSafeReadPtr(PVOID Source);

/* pageop.c ******************************************************************/

VOID
MmReleasePageOp(PMM_PAGEOP PageOp);

PMM_PAGEOP
MmGetPageOp(PMEMORY_AREA MArea, HANDLE Pid, PVOID Address,
	    PMM_SECTION_SEGMENT Segment, ULONG Offset, ULONG OpType, BOOL First);
PMM_PAGEOP
MmCheckForPageOp(PMEMORY_AREA MArea, HANDLE Pid, PVOID Address,
		 PMM_SECTION_SEGMENT Segment, ULONG Offset);
VOID
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

VOID MmInitializeMemoryConsumer(ULONG Consumer,
			        NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed));

VOID MmInitializeBalancer(ULONG NrAvailablePages, ULONG NrSystemPages);

NTSTATUS MmReleasePageMemoryConsumer(ULONG Consumer, PFN_TYPE Page);

NTSTATUS MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN MyWait, PPFN_TYPE AllocatedPage);

VOID MiInitBalancerThread(VOID);

VOID MmRebalanceMemoryConsumers(VOID);

/* rmap.c **************************************************************/

VOID MmSetRmapListHeadPage(PFN_TYPE Page, struct _MM_RMAP_ENTRY* ListHead);

struct _MM_RMAP_ENTRY* MmGetRmapListHeadPage(PFN_TYPE Page);

VOID MmInsertRmap(PFN_TYPE Page, PEPROCESS Process, PVOID Address);

VOID MmDeleteAllRmaps(PFN_TYPE Page, PVOID Context,
		      VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process, PVOID Address));

VOID MmDeleteRmap(PFN_TYPE Page, PEPROCESS Process, PVOID Address);

VOID MmInitializeRmapList(VOID);

VOID MmSetCleanAllRmaps(PFN_TYPE Page);

VOID MmSetDirtyAllRmaps(PFN_TYPE Page);

BOOL MmIsDirtyPageRmap(PFN_TYPE Page);

NTSTATUS MmWritePagePhysicalAddress(PFN_TYPE Page);

NTSTATUS MmPageOutPhysicalAddress(PFN_TYPE Page);

/* freelist.c **********************************************************/

PFN_TYPE MmGetLRUNextUserPage(PFN_TYPE PreviousPage);

PFN_TYPE MmGetLRUFirstUserPage(VOID);

VOID MmSetLRULastPage(PFN_TYPE Page);

VOID MmLockPage(PFN_TYPE Page);
VOID MmLockPageUnsafe(PFN_TYPE Page);

VOID MmUnlockPage(PFN_TYPE Page);

ULONG MmGetLockCountPage(PFN_TYPE Page);

PVOID MmInitializePageList(ULONG_PTR FirstPhysKernelAddress,
		           ULONG_PTR LastPhysKernelAddress,
			   ULONG MemorySizeInPages,
			   ULONG_PTR LastKernelBase,
			   PADDRESS_RANGE BIOSMemoryMap,
			   ULONG AddressRangeCount);

PFN_TYPE MmGetContinuousPages(ULONG NumberOfBytes,
			      PHYSICAL_ADDRESS LowestAcceptableAddress,
			      PHYSICAL_ADDRESS HighestAcceptableAddress,
			      ULONG Alignment);

NTSTATUS MmInitZeroPageThread(VOID);

/* i386/page.c *********************************************************/

PVOID MmCreateHyperspaceMapping(PFN_TYPE Page);

PFN_TYPE MmChangeHyperspaceMapping(PVOID Address, PFN_TYPE Page);

PFN_TYPE MmDeleteHyperspaceMapping(PVOID Address);

NTSTATUS MmCreateVirtualMappingForKernel(PVOID Address,
					 ULONG flProtect,
					 PPFN_TYPE Pages,
					 ULONG PageCount);

NTSTATUS MmCommitPagedPoolAddress(PVOID Address, BOOLEAN Locked);

NTSTATUS MmCreateVirtualMapping(struct _EPROCESS* Process,
				PVOID Address,
				ULONG flProtect,
				PPFN_TYPE Pages,
				ULONG PageCount);

NTSTATUS MmCreateVirtualMappingUnsafe(struct _EPROCESS* Process,
				      PVOID Address,
				      ULONG flProtect,
				      PPFN_TYPE Pages,
				      ULONG PageCount);

ULONG MmGetPageProtect(struct _EPROCESS* Process, PVOID Address);

VOID MmSetPageProtect(struct _EPROCESS* Process,
		      PVOID Address,
		      ULONG flProtect);

BOOLEAN MmIsPagePresent(struct _EPROCESS* Process,
			PVOID Address);

VOID MmInitGlobalKernelPageDirectory(VOID);

VOID MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOL* WasDirty, PPFN_TYPE Page);

VOID MmEnableVirtualMapping(PEPROCESS Process, PVOID Address);

VOID MmRawDeleteVirtualMapping(PVOID Address);

VOID MmDeletePageFileMapping(PEPROCESS Process, PVOID Address, SWAPENTRY* SwapEntry);

NTSTATUS MmCreatePageFileMapping(PEPROCESS Process, PVOID Address, SWAPENTRY SwapEntry);

BOOLEAN MmIsPageSwapEntry(PEPROCESS Process, PVOID Address);

VOID MmTransferOwnershipPage(PFN_TYPE Page, ULONG NewConsumer);

VOID MmSetDirtyPage(PEPROCESS Process, PVOID Address);

PFN_TYPE MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry);

LONG MmAllocPagesSpecifyRange(ULONG Consumer,
                              PHYSICAL_ADDRESS LowestAddress,
                              PHYSICAL_ADDRESS HighestAddress,
                              ULONG NumberOfPages,
                              PPFN_TYPE Pages);

VOID MmDereferencePage(PFN_TYPE Page);

VOID MmReferencePage(PFN_TYPE Page);
VOID MmReferencePageUnsafe(PFN_TYPE Page);

BOOLEAN MmIsAccessedAndResetAccessPage(struct _EPROCESS* Process, PVOID Address);

ULONG MmGetReferenceCountPage(PFN_TYPE Page);

BOOLEAN MmIsUsablePage(PFN_TYPE Page);

VOID MmSetFlagsPage(PFN_TYPE Page, ULONG Flags);

ULONG MmGetFlagsPage(PFN_TYPE Page);

VOID MmSetSavedSwapEntryPage(PFN_TYPE Page, SWAPENTRY SavedSwapEntry);

SWAPENTRY MmGetSavedSwapEntryPage(PFN_TYPE Page);

VOID MmSetCleanPage(struct _EPROCESS* Process, PVOID Address);

NTSTATUS MmCreatePageTable(PVOID PAddress);

VOID MmDeletePageTable(struct _EPROCESS* Process, PVOID Address);

PFN_TYPE MmGetPfnForProcess(struct _EPROCESS* Process, PVOID Address);

NTSTATUS
STDCALL
MmCopyMmInfo(struct _EPROCESS* Src,
             struct _EPROCESS* Dest,
             PPHYSICAL_ADDRESS DirectoryTableBase);

NTSTATUS MmReleaseMmInfo(struct _EPROCESS* Process);

NTSTATUS Mmi386ReleaseMmInfo(struct _EPROCESS* Process);

VOID MmDeleteVirtualMapping(struct _EPROCESS* Process,
			    PVOID Address,
			    BOOL FreePage,
			    BOOL* WasDirty,
			    PPFN_TYPE Page);

BOOLEAN MmIsDirtyPage(struct _EPROCESS* Process, PVOID Address);

VOID MmMarkPageMapped(PFN_TYPE Page);

VOID MmMarkPageUnmapped(PFN_TYPE Page);

VOID MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size);

VOID MiInitPageDirectoryMap(VOID);

ULONG MiGetUserPageDirectoryCount(VOID);

/* wset.c ********************************************************************/

NTSTATUS MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages);

/* cont.c ********************************************************************/

PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
					  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
			          IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
			          IN MEMORY_CACHING_TYPE CacheType OPTIONAL,
					  IN ULONG Alignment);
                      
/* region.c ************************************************************/

NTSTATUS MmAlterRegion(PMADDRESS_SPACE AddressSpace, PVOID BaseAddress,
	               PLIST_ENTRY RegionListHead, PVOID StartAddress, ULONG Length,
	               ULONG NewType, ULONG NewProtect,
	               PMM_ALTER_REGION_FUNC AlterFunc);

VOID MmInitialiseRegion(PLIST_ENTRY RegionListHead, ULONG Length, ULONG Type,
		        ULONG Protect);

PMM_REGION MmFindRegion(PVOID BaseAddress, PLIST_ENTRY RegionListHead, PVOID Address,
	                PVOID* RegionBaseAddress);

/* section.c *****************************************************************/

PVOID STDCALL
MmAllocateSection (IN ULONG Length, PVOID BaseAddress);

NTSTATUS STDCALL
MmQuerySectionView(PMEMORY_AREA MemoryArea,
		   PVOID Address,
		   PMEMORY_BASIC_INFORMATION Info,
		   PULONG ResultLength);

NTSTATUS
MmProtectSectionView(PMADDRESS_SPACE AddressSpace,
		     PMEMORY_AREA MemoryArea,
		     PVOID BaseAddress,
		     ULONG Length,
		     ULONG Protect,
		     PULONG OldProtect);

NTSTATUS
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MArea,
		       PVOID Address,
		       PMM_PAGEOP PageOp);

NTSTATUS MmInitSectionImplementation(VOID);

NTSTATUS 
STDCALL
MmCreateSection (OUT	PSECTION_OBJECT		* SectionObject,
		 IN	ACCESS_MASK		DesiredAccess,
		 IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
		 IN	PLARGE_INTEGER		MaximumSize,
		 IN	ULONG			SectionPageProtection,
		 IN	ULONG			AllocationAttributes,
		 IN	HANDLE			FileHandle		OPTIONAL,
		 IN	PFILE_OBJECT		File			OPTIONAL);

NTSTATUS
MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
			     MEMORY_AREA* MemoryArea,
			     PVOID Address,
			     BOOLEAN Locked);

NTSTATUS
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       struct _MM_PAGEOP* PageOp);

NTSTATUS
MmCreatePhysicalMemorySection(VOID);

NTSTATUS
MmAccessFaultSectionView(PMADDRESS_SPACE AddressSpace,
			 MEMORY_AREA* MemoryArea,
			 PVOID Address,
			 BOOLEAN Locked);

VOID
MmFreeSectionSegments(PFILE_OBJECT FileObject);

/* mpw.c *********************************************************************/

NTSTATUS MmInitMpwThread(VOID);

/* pager.c *******************************************************************/

BOOLEAN MiIsPagerThread(VOID);

VOID MiStartPagerThread(VOID);

VOID MiStopPagerThread(VOID);

#endif
