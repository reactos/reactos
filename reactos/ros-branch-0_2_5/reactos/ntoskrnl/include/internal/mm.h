/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/ntoskrnl.h>
#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

extern ULONG MiFreeSwapPages;
extern ULONG MiUsedSwapPages;
extern ULONG MmPagedPoolSize;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;

struct _EPROCESS;

struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP;
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

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)

#ifndef __USE_W32API
#define MM_LOWEST_USER_ADDRESS (4096)
#endif

#define MM_VIRTMEM_GRANULARITY (64 * 1024) /* Although Microsoft says this isn't hardcoded anymore,
                                              they won't be able to change it. Stuff depends on it */

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

#define MM_SECTION_SEGMENT_BSS (0x1)

typedef struct _MM_SECTION_SEGMENT
{
  ULONG FileOffset;
  ULONG Protection;
  ULONG Attributes;
  ULONG Length;
  ULONG RawLength;
  FAST_MUTEX Lock;
  ULONG ReferenceCount;
  SECTION_PAGE_DIRECTORY PageDirectory;
  ULONG Flags;
  PVOID VirtualAddress;
  ULONG Characteristics;
  BOOLEAN WriteCopy;
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct _MM_IMAGE_SECTION_OBJECT
{
  PVOID ImageBase;
  PVOID EntryPoint;
  ULONG StackReserve;
  ULONG StackCommit;
  ULONG Subsystem;
  ULONG MinorSubsystemVersion;
  ULONG MajorSubsystemVersion;
  ULONG ImageCharacteristics;
  USHORT Machine;
  BOOLEAN Executable;
  ULONG NrSegments;
  MM_SECTION_SEGMENT Segments[0];
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

typedef struct _SECTION_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  LARGE_INTEGER MaximumSize;
  ULONG SectionPageProtection;
  ULONG AllocationAttributes;
  PFILE_OBJECT FileObject;
  LIST_ENTRY ViewListHead;
  KSPIN_LOCK ViewListLock;
  union
  {
    PMM_IMAGE_SECTION_OBJECT ImageSection;
    PMM_SECTION_SEGMENT Segment;
  };
} SECTION_OBJECT;

#ifndef __USE_W32API

typedef struct _SECTION_OBJECT *PSECTION_OBJECT;

typedef struct _EPROCESS_QUOTA_BLOCK {
KSPIN_LOCK      QuotaLock;
ULONG           ReferenceCount;
ULONG           QuotaPeakPoolUsage[2];
ULONG           QuotaPoolUsage[2];
ULONG           QuotaPoolLimit[2];
ULONG           PeakPagefileUsage;
ULONG           PagefileUsage;
ULONG           PagefileLimit;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

/*
 * header mess....
 */

typedef struct _PAGEFAULT_HISTORY
{
    ULONG                                 CurrentIndex;
    ULONG                                 MaxIndex;
    KSPIN_LOCK                            SpinLock;
    PVOID                                 Reserved;
    struct _PROCESS_WS_WATCH_INFORMATION  WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;

#endif /* __USE_W32API */

typedef struct
{
  ULONG Type;
  PVOID BaseAddress;
  ULONG Length;
  ULONG Attributes;
  LIST_ENTRY Entry;
  ULONG LockCount;
  struct _EPROCESS* Process;
  BOOLEAN DeleteInProgress;
  ULONG PageOpCount;
  union
  {
    struct
    {
      SECTION_OBJECT* Section;
      ULONG ViewOffset;
      LIST_ENTRY ViewListEntry;
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
  LIST_ENTRY MAreaListHead;
  FAST_MUTEX Lock;
  ULONG LowestAddress;
  struct _EPROCESS* Process;
  PUSHORT PageTableRefCountTable;
  ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;

typedef struct _KNODE {
   ULONG ProcessorMask;
   ULONG Color;
   ULONG MmShiftedColor;
   ULONG FreeCount[2];
   SLIST_HEADER DeadStackList;
   SLIST_HEADER PfnDereferenceSListHead;
   struct _SINGLE_LIST_ENTRY *PfnDeferredList;
   UCHAR Seed;
   UCHAR NodeNumber;
   ULONG Flags;
} KNODE, *PKNODE;

#ifndef __USE_W32API
/* VARIABLES */

#ifdef __NTOSKRNL__
extern PVOID EXPORTED MmSystemRangeStart;
#else
extern PVOID IMPORTED MmSystemRangeStart;
#endif

#endif /* __USE_W32API */

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
  ULONG Pid;
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

NTSTATUS MmCreateMemoryArea(struct _EPROCESS* Process,
			    PMADDRESS_SPACE AddressSpace,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result,
			    BOOL FixedAddress,
			    BOOL TopDown,
			    PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL);

MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace, 
				       PVOID Address);

ULONG MmFindGapAtAddress(PMADDRESS_SPACE AddressSpace, 
			 PVOID Address);

NTSTATUS MmInitMemoryAreas(VOID);

NTSTATUS MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
			  PVOID BaseAddress,
			  ULONG Length,
			  VOID (*FreePage)(PVOID Context, MEMORY_AREA* MemoryArea, 
					   PVOID Address, PFN_TYPE Page, SWAPENTRY SwapEntry,
					   BOOLEAN Dirty),
			  PVOID FreePageContext);

VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead);

NTSTATUS MmLockMemoryArea(MEMORY_AREA* MemoryArea);

NTSTATUS MmUnlockMemoryArea(MEMORY_AREA* MemoryArea);

MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length);

PVOID MmFindGap(PMADDRESS_SPACE AddressSpace, ULONG Length, ULONG Granularity, BOOL TopDown);

void MmReleaseMemoryAreaIfDecommitted(PEPROCESS Process,
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

VOID MmInit1(ULONG FirstKernelPhysAddress, 
	     ULONG LastKernelPhysAddress,
	     ULONG LastKernelAddress,
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

/* i386/pfault.c *************************************************************/

NTSTATUS MmPageFault(ULONG Cs,
		     PULONG Eip,
		     PULONG Eax,
		     ULONG Cr2,
		     ULONG ErrorCode);

/* mm.c **********************************************************************/

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address,
		       BOOLEAN FromMdl);

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address,
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

VOID MiInitKernelMap(VOID);

PVOID ExAllocatePageWithPhysPage(PFN_TYPE Page);

NTSTATUS MiCopyFromUserPage(PFN_TYPE Page, PVOID SourceAddress);

NTSTATUS MiZeroPage(PFN_TYPE Page);

/* memsafe.s *****************************************************************/

NTSTATUS MmSafeCopyFromUser(PVOID Dest, const VOID *Src, ULONG Count);

NTSTATUS MmSafeCopyToUser(PVOID Dest, const VOID *Src, ULONG Count);

/* pageop.c ******************************************************************/

VOID
MmReleasePageOp(PMM_PAGEOP PageOp);

PMM_PAGEOP
MmGetPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
	    PMM_SECTION_SEGMENT Segment, ULONG Offset, ULONG OpType, BOOL First);
PMM_PAGEOP
MmCheckForPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
		 PMM_SECTION_SEGMENT Segment, ULONG Offset);
VOID
MmInitializePageOp(VOID);

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

VOID MmUnlockPage(PFN_TYPE Page);

ULONG MmGetLockCountPage(PFN_TYPE Page);

PVOID MmInitializePageList(PVOID FirstPhysKernelAddress,
		           PVOID LastPhysKernelAddress,
			   ULONG MemorySizeInPages,
			   ULONG LastKernelBase,
			   PADDRESS_RANGE BIOSMemoryMap,
			   ULONG AddressRangeCount);

PFN_TYPE MmGetContinuousPages(ULONG NumberOfBytes,
			      PHYSICAL_ADDRESS LowestAcceptableAddress,
			      PHYSICAL_ADDRESS HighestAcceptableAddress,
			      ULONG Alignment);

NTSTATUS MmInitZeroPageThread(VOID);

/* i386/page.c *********************************************************/

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

VOID MmDereferencePage(PFN_TYPE Page);

VOID MmReferencePage(PFN_TYPE Page);

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

NTSTATUS MmCopyMmInfo(struct _EPROCESS* Src, struct _EPROCESS* Dest);

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

NTSTATUS STDCALL
MmUnmapViewOfSection(struct _EPROCESS* Process, PVOID BaseAddress);

/* FIXME: it should be in ddk/mmfuncs.h */
NTSTATUS STDCALL
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
