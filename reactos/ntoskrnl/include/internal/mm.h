/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/ntoskrnl.h>
#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;

struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP;
typedef ULONG SWAPENTRY;

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

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)


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
  KMUTEX Lock;
  ULONG ReferenceCount;
  SECTION_PAGE_DIRECTORY PageDirectory;
  ULONG Flags;
  PVOID VirtualAddress;
  ULONG Characteristics;
  BOOLEAN WriteCopy;
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct
{
  CSHORT Type;
  CSHORT Size;
  LARGE_INTEGER MaximumSize;
  ULONG SectionPageProtection;
  ULONG AllocationAttributes;
  PFILE_OBJECT FileObject;
  LIST_ENTRY ViewListHead;
  KSPIN_LOCK ViewListLock;
  KMUTEX Lock;
  ULONG NrSegments;
  PMM_SECTION_SEGMENT Segments;
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
} SECTION_OBJECT, *PSECTION_OBJECT;

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

/* FUNCTIONS */

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmInitializeKernelAddressSpace(VOID);
PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID);
PMADDRESS_SPACE MmGetKernelAddressSpace(VOID);
NTSTATUS MmInitializeAddressSpace(struct _EPROCESS* Process,
				  PMADDRESS_SPACE AddressSpace);
NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace);
PVOID STDCALL MmAllocateSection (IN ULONG Length);
NTSTATUS MmCreateMemoryArea(struct _EPROCESS* Process,
			    PMADDRESS_SPACE AddressSpace,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result,
			    BOOL FixedAddress);
MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace, 
				       PVOID Address);
NTSTATUS MmInitMemoryAreas(VOID);
VOID ExInitNonPagedPool(ULONG BaseAddress);
NTSTATUS MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
			  PVOID BaseAddress,
			  ULONG Length,
			  VOID (*FreePage)(PVOID Context, MEMORY_AREA* MemoryArea, 
					   PVOID Address, PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry,
					   BOOLEAN Dirty),
			  PVOID FreePageContext);
VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead);
NTSTATUS MmLockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmUnlockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmInitSectionImplementation(VOID);

#define MM_LOWEST_USER_ADDRESS (4096)

PMEMORY_AREA MmSplitMemoryArea(struct _EPROCESS* Process,
			       PMADDRESS_SPACE AddressSpace,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes);
PVOID 
MmInitializePageList(PVOID FirstPhysKernelAddress,
		     PVOID LastPhysKernelAddress,
		     ULONG MemorySizeInPages,
		     ULONG LastKernelBase,
		     PADDRESS_RANGE BIOSMemoryMap,
		     ULONG AddressRangeCount);
PHYSICAL_ADDRESS
MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry);
VOID MmDereferencePage(PHYSICAL_ADDRESS PhysicalAddress);
VOID MmReferencePage(PHYSICAL_ADDRESS PhysicalAddress);
VOID MmDeletePageTable(struct _EPROCESS* Process, 
		       PVOID Address);
NTSTATUS MmCopyMmInfo(struct _EPROCESS* Src, 
		      struct _EPROCESS* Dest);
NTSTATUS MmReleaseMmInfo(struct _EPROCESS* Process);
NTSTATUS Mmi386ReleaseMmInfo(struct _EPROCESS* Process);
VOID
MmDeleteVirtualMapping(struct _EPROCESS* Process, 
		       PVOID Address, 
		       BOOL FreePage,
		       BOOL* WasDirty,
		       PHYSICAL_ADDRESS* PhysicalPage);
VOID MmUpdatePageDir(PULONG LocalPageDir, PVOID Address);

#define MM_PAGE_CLEAN     (0)
#define MM_PAGE_DIRTY     (1)

VOID MmBuildMdlFromPages(PMDL Mdl, PULONG Pages);
PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);
VOID MiShutdownMemoryManager(VOID);
PHYSICAL_ADDRESS
MmGetPhysicalAddressForProcess(struct _EPROCESS* Process,
			       PVOID Address);
NTSTATUS STDCALL
MmUnmapViewOfSection(struct _EPROCESS* Process, PVOID BaseAddress);
VOID MmInitPagingFile(VOID);

/* FIXME: it should be in ddk/mmfuncs.h */
NTSTATUS
STDCALL
MmCreateSection (
	OUT	PSECTION_OBJECT		* SectionObject,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	PLARGE_INTEGER		MaximumSize,
	IN	ULONG			SectionPageProtection,
	IN	ULONG			AllocationAttributes,
	IN	HANDLE			FileHandle		OPTIONAL,
	IN	PFILE_OBJECT		File			OPTIONAL
	);

NTSTATUS MmPageFault(ULONG Cs,
		     PULONG Eip,
		     PULONG Eax,
		     ULONG Cr2,
		     ULONG ErrorCode);

NTSTATUS 
MmAccessFault(KPROCESSOR_MODE Mode,
	      ULONG Address,
	      BOOLEAN FromMdl);
NTSTATUS 
MmNotPresentFault(KPROCESSOR_MODE Mode,
		  ULONG Address,
		  BOOLEAN FromMdl);
NTSTATUS 
MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
			       MEMORY_AREA* MemoryArea, 
			       PVOID Address,
			       BOOLEAN Locked);
NTSTATUS 
MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
			     MEMORY_AREA* MemoryArea, 
			     PVOID Address,
			     BOOLEAN Locked);
NTSTATUS MmWaitForPage(PVOID Page);
VOID MmClearWaitPage(PVOID Page);
VOID MmSetWaitPage(PVOID Page);
BOOLEAN MmIsDirtyPage(struct _EPROCESS* Process, PVOID Address);
BOOLEAN MmIsPageTablePresent(PVOID PAddress);
NTSTATUS 
MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       struct _MM_PAGEOP* PageOp);
NTSTATUS 
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       struct _MM_PAGEOP* PageOp);
MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length);
PVOID MmFindGap(PMADDRESS_SPACE AddressSpace, ULONG Length);
VOID ExUnmapPage(PVOID Addr);
PVOID ExAllocatePage(VOID);

BOOLEAN MmReserveSwapPages(ULONG Nr);
VOID MmDereserveSwapPages(ULONG Nr);
SWAPENTRY MmAllocSwapPage(VOID);
VOID MmFreeSwapPage(SWAPENTRY Entry);

VOID MmInit1(ULONG FirstKernelPhysAddress, 
	     ULONG LastKernelPhysAddress,
	     ULONG LastKernelAddress,
       PADDRESS_RANGE BIOSMemoryMap,
       ULONG AddressRangeCount);
VOID MmInit2(VOID);
VOID MmInit3(VOID);
NTSTATUS MmInitPagerThread(VOID);

VOID MmInitKernelMap(PVOID BaseAddress);
NTSTATUS MmCreatePageTable(PVOID PAddress);

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

PVOID 
MmGetDirtyPagesFromWorkingSet(struct _EPROCESS* Process);
NTSTATUS 
MmWriteToSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
NTSTATUS 
MmReadFromSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
VOID 
MmSetFlagsPage(PHYSICAL_ADDRESS PhysicalAddress, ULONG Flags);
ULONG 
MmGetFlagsPage(PHYSICAL_ADDRESS PhysicalAddress);
VOID MmSetSavedSwapEntryPage(PHYSICAL_ADDRESS PhysicalAddress,
			     SWAPENTRY SavedSwapEntry);
SWAPENTRY MmGetSavedSwapEntryPage(PHYSICAL_ADDRESS PhysicalAddress);
VOID MmSetCleanPage(struct _EPROCESS* Process, PVOID Address);
VOID MmLockPage(PHYSICAL_ADDRESS PhysicalPage);
VOID MmUnlockPage(PHYSICAL_ADDRESS PhysicalPage);

NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG Count);
NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG Count);
NTSTATUS 
MmCreatePhysicalMemorySection(VOID);
PHYSICAL_ADDRESS
MmGetContinuousPages(ULONG NumberOfBytes,
		     PHYSICAL_ADDRESS HighestAcceptableAddress,
		     ULONG Alignment);

#define MM_PHYSICAL_PAGE_MPW_PENDING     (0x8)

NTSTATUS 
MmAccessFaultSectionView(PMADDRESS_SPACE AddressSpace,
			 MEMORY_AREA* MemoryArea, 
			 PVOID Address,
			 BOOLEAN Locked);
ULONG
MmGetPageProtect(struct _EPROCESS* Process, PVOID Address);
PVOID 
ExAllocatePageWithPhysPage(PHYSICAL_ADDRESS PhysPage);
ULONG
MmGetReferenceCountPage(PHYSICAL_ADDRESS PhysicalAddress);
BOOLEAN
MmIsUsablePage(PHYSICAL_ADDRESS PhysicalAddress);

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

VOID
MmReleasePageOp(PMM_PAGEOP PageOp);

PMM_PAGEOP
MmGetPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
	    PMM_SECTION_SEGMENT Segment, ULONG Offset, ULONG OpType);
PMM_PAGEOP
MmCheckForPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
		 PMM_SECTION_SEGMENT Segment, ULONG Offset);
VOID
MmInitializePageOp(VOID);
VOID
MiDebugDumpNonPagedPool(BOOLEAN NewOnly);
VOID
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);
VOID 
MmMarkPageMapped(PHYSICAL_ADDRESS PhysicalAddress);
VOID 
MmMarkPageUnmapped(PHYSICAL_ADDRESS PhysicalAddress);
VOID
MmFreeSectionSegments(PFILE_OBJECT FileObject);

typedef struct _MM_IMAGE_SECTION_OBJECT
{
  ULONG NrSegments;
  MM_SECTION_SEGMENT Segments[0];
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

VOID 
MmFreeVirtualMemory(struct _EPROCESS* Process, PMEMORY_AREA MemoryArea);
NTSTATUS
MiCopyFromUserPage(PHYSICAL_ADDRESS DestPhysPage, PVOID SourceAddress);
NTSTATUS
MiZeroPage(PHYSICAL_ADDRESS PhysPage);
BOOLEAN 
MmIsAccessedAndResetAccessPage(struct _EPROCESS* Process, PVOID Address);

#define STATUS_MM_RESTART_OPERATION       ((NTSTATUS)0xD0000001)

NTSTATUS 
MmCreateVirtualMappingForKernel(PVOID Address, 
				ULONG flProtect,
				PHYSICAL_ADDRESS PhysicalAddress);
NTSTATUS MmCommitPagedPoolAddress(PVOID Address);
NTSTATUS MmCreateVirtualMapping(struct _EPROCESS* Process,
				PVOID Address, 
				ULONG flProtect,
				PHYSICAL_ADDRESS PhysicalAddress,
				BOOLEAN MayWait);
NTSTATUS 
MmCreateVirtualMappingUnsafe(struct _EPROCESS* Process,
			     PVOID Address, 
			     ULONG flProtect,
			     PHYSICAL_ADDRESS PhysicalAddress,
			     BOOLEAN MayWait);

VOID MmSetPageProtect(struct _EPROCESS* Process,
		      PVOID Address,
		      ULONG flProtect);
BOOLEAN MmIsPagePresent(struct _EPROCESS* Process, 
			PVOID Address);

VOID MmInitGlobalKernelPageDirectory(VOID);

/* Memory balancing. */
VOID
MmInitializeMemoryConsumer(ULONG Consumer, 
			   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, 
					    PULONG NrFreed));
VOID
MmInitializeBalancer(ULONG NrAvailablePages);
NTSTATUS
MmReleasePageMemoryConsumer(ULONG Consumer, PHYSICAL_ADDRESS Page);
NTSTATUS
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait, 
			    PHYSICAL_ADDRESS* AllocatedPage);

#define MC_CACHE          (0)
#define MC_USER           (1)
#define MC_PPOOL          (2)
#define MC_NPPOOL         (3)
#define MC_MAXIMUM        (4)

VOID 
MmSetRmapListHeadPage(PHYSICAL_ADDRESS PhysicalAddress, 
		      struct _MM_RMAP_ENTRY* ListHead);
struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(PHYSICAL_ADDRESS PhysicalAddress);
VOID
MmInsertRmap(PHYSICAL_ADDRESS PhysicalAddress, PEPROCESS Process, 
	     PVOID Address);
VOID
MmDeleteAllRmaps(PHYSICAL_ADDRESS PhysicalAddress, PVOID Context, 
		 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process, PVOID Address));
VOID
MmDeleteRmap(PHYSICAL_ADDRESS PhysicalAddress, PEPROCESS Process, 
	     PVOID Address);
VOID
MmInitializeRmapList(VOID);
PHYSICAL_ADDRESS
MmGetLRUNextUserPage(PHYSICAL_ADDRESS PreviousPhysicalAddress);
PHYSICAL_ADDRESS
MmGetLRUFirstUserPage(VOID);
NTSTATUS
MmPageOutPhysicalAddress(PHYSICAL_ADDRESS PhysicalAddress);
NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages);

VOID
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOL* WasDirty, PHYSICAL_ADDRESS* PhysicalAddr);
VOID MmEnableVirtualMapping(PEPROCESS Process, PVOID Address);
VOID
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address, 
			SWAPENTRY* SwapEntry);
NTSTATUS 
MmCreatePageFileMapping(PEPROCESS Process,
			PVOID Address,
			SWAPENTRY SwapEntry);
BOOLEAN MmIsPageSwapEntry(PEPROCESS Process, PVOID Address);
VOID
MmTransferOwnershipPage(PHYSICAL_ADDRESS PhysicalAddress, ULONG NewConsumer);
VOID MmSetDirtyPage(PEPROCESS Process, PVOID Address);
VOID
MmInitializeMdlImplementation(VOID);
extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
struct _KTRAP_FRAME;
NTSTATUS STDCALL 
MmDumpToPagingFile(ULONG BugCode,
		   ULONG BugCodeParameter1,
		   ULONG BugCodeParameter2,
		   ULONG BugCodeParameter3,
		   ULONG BugCodeParameter4,
		   struct _KTRAP_FRAME* TrapFrame);

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

NTSTATUS
MmAlterRegion(PMADDRESS_SPACE AddressSpace, PVOID BaseAddress, 
	      PLIST_ENTRY RegionListHead, PVOID StartAddress, ULONG Length, 
	      ULONG NewType, ULONG NewProtect, 
	      PMM_ALTER_REGION_FUNC AlterFunc);
VOID
MmInitialiseRegion(PLIST_ENTRY RegionListHead, ULONG Length, ULONG Type,
		   ULONG Protect);
PMM_REGION
MmFindRegion(PVOID BaseAddress, PLIST_ENTRY RegionListHead, PVOID Address,
	     PVOID* RegionBaseAddress);
NTSTATUS STDCALL
MmQueryAnonMem(PMEMORY_AREA MemoryArea,
	       PVOID Address,
	       PMEMORY_BASIC_INFORMATION Info,
	       PULONG ResultLength);
NTSTATUS STDCALL
MmQuerySectionView(PMEMORY_AREA MemoryArea,
		   PVOID Address,
		   PMEMORY_BASIC_INFORMATION Info,
		   PULONG ResultLength);
NTSTATUS
MmProtectAnonMem(PMADDRESS_SPACE AddressSpace,
		 PMEMORY_AREA MemoryArea,
		 PVOID BaseAddress,
		 ULONG Length,
		 ULONG Protect,
		 PULONG OldProtect);
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
NTSTATUS 
MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
			 PMEMORY_AREA MArea,
			 PVOID Address,
			 PMM_PAGEOP PageOp);
VOID
MmSetCleanAllRmaps(PHYSICAL_ADDRESS PhysicalAddress);
VOID
MmSetDirtyAllRmaps(PHYSICAL_ADDRESS PhysicalAddress);
NTSTATUS
MmWritePagePhysicalAddress(PHYSICAL_ADDRESS PhysicalAddress);
BOOL
MmIsDirtyPageRmap(PHYSICAL_ADDRESS PhysicalAddress);
NTSTATUS MmInitMpwThread(VOID);
BOOLEAN
MmIsAvailableSwapPage(VOID);
VOID
MmShowOutOfSpaceMessagePagingFile(VOID);

#endif
