/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/ntoskrnl.h>
#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;
typedef ULONG SWAPENTRY;

#define MEMORY_AREA_INVALID              (0)
#define MEMORY_AREA_SECTION_VIEW_COMMIT  (1)
#define MEMORY_AREA_CONTINUOUS_MEMORY    (2)
#define MEMORY_AREA_NO_CACHE             (3)
#define MEMORY_AREA_IO_MAPPING           (4)
#define MEMORY_AREA_SYSTEM               (5)
#define MEMORY_AREA_MDL_MAPPING          (7)
#define MEMORY_AREA_VIRTUAL_MEMORY       (8)
#define MEMORY_AREA_SECTION_VIEW_RESERVE (9)
#define MEMORY_AREA_CACHE_SEGMENT        (10)
#define MEMORY_AREA_SHARED_DATA          (11)
#define MEMORY_AREA_WORKING_SET          (12)
#define MEMORY_AREA_KERNEL_STACK         (13)

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)

/*
 * Flags for section objects
 */
#define SO_PHYSICAL_MEMORY                      (0x1)

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
   ULONG Pages[NR_SECTION_PAGE_ENTRIES];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

typedef struct
{
   PSECTION_PAGE_TABLE PageTables[NR_SECTION_PAGE_TABLES];
} SECTION_PAGE_DIRECTORY, *PSECTION_PAGE_DIRECTORY;

#define MM_PAGEFILE_SECTION    (0x1)
#define MM_IMAGE_SECTION       (0x2)

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
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct
{
  CSHORT Type;
  CSHORT Size;
  LARGE_INTEGER MaximumSize;
  ULONG SectionPageProtection;
  ULONG AllocateAttributes;
  PFILE_OBJECT FileObject;
  LIST_ENTRY ViewListHead;
  KSPIN_LOCK ViewListLock;
  KMUTEX Lock;
  ULONG Flags;
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
   union
     {
	struct
	  {	     
	     SECTION_OBJECT* Section;
	     ULONG ViewOffset;
	     LIST_ENTRY ViewListEntry;
 	     PMM_SECTION_SEGMENT Segment;
	  } SectionData;
	struct
	  {
	     LIST_ENTRY SegmentListHead;
	  } VirtualMemoryData;
     } Data;
} MEMORY_AREA, *PMEMORY_AREA;

typedef struct _KCIRCULAR_QUEUE
{
  ULONG First;
  ULONG Last;
  ULONG CurrentSize;
  ULONG MaximumSize;  
  PVOID* Mem;
} KCIRCULAR_QUEUE, *PKCIRCULAR_QUEUE;

typedef struct _MADDRESS_SPACE
{
  LIST_ENTRY MAreaListHead;
  KMUTEX Lock;
  ULONG LowestAddress;
  struct _EPROCESS* Process;
  PMEMORY_AREA WorkingSetArea;
  KCIRCULAR_QUEUE WSQueue;
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
			  VOID (*FreePage)(PVOID Context, PVOID Address,
					   ULONG PhysAddr),
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
PVOID MmInitializePageList(PVOID FirstPhysKernelAddress,
			   PVOID LastPhysKernelAddress,
			   ULONG MemorySizeInPages,
			   ULONG LastKernelBase,
         PADDRESS_RANGE BIOSMemoryMap,
         ULONG AddressRangeCount);

PVOID MmAllocPage(SWAPENTRY SavedSwapEntry);
VOID MmDereferencePage(PVOID PhysicalAddress);
VOID MmReferencePage(PVOID PhysicalAddress);
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
		       ULONG* PhysicalPage);

#define MM_PAGE_CLEAN     (0)
#define MM_PAGE_DIRTY     (1)

VOID MmBuildMdlFromPages(PMDL Mdl, PULONG Pages);
PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);
VOID MiShutdownMemoryManager(VOID);
ULONG MmGetPhysicalAddressForProcess(struct _EPROCESS* Process,
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
BOOLEAN MmIsPageDirty(struct _EPROCESS* Process, PVOID Address);
BOOLEAN MmIsPageTablePresent(PVOID PAddress);
ULONG MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
			   MEMORY_AREA* MemoryArea, 
			   PVOID Address,
			   PBOOLEAN Ul);
ULONG MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
			     PMEMORY_AREA MemoryArea,
			     PVOID Address,
			     PBOOLEAN Ul);
MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length);

VOID ExUnmapPage(PVOID Addr);
PVOID ExAllocatePage(VOID);

VOID MmLockWorkingSet(struct _EPROCESS* Process);
VOID MmUnlockWorkingSet(struct _EPROCESS* Process);
VOID MmInitializeWorkingSet(struct _EPROCESS* Process,
			    PMADDRESS_SPACE AddressSpace);
ULONG MmTrimWorkingSet(struct _EPROCESS* Process,
		       ULONG ReduceHint);
VOID MmRemovePageFromWorkingSet(struct _EPROCESS* Process,
				PVOID Address);
VOID MmAddPageToWorkingSet(struct _EPROCESS* Process,
			      PVOID Address);

VOID MmInitPagingFile(VOID);
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

VOID MmWaitForFreePages(VOID);
PVOID MmMustAllocPage(SWAPENTRY SavedSwapEntry);
PVOID MmAllocPageMaybeSwap(SWAPENTRY SavedSwapEntry);
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

NTSTATUS 
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MArea,
		       PVOID Address);
NTSTATUS 
MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
			 PMEMORY_AREA MArea,
			 PVOID Address);
PVOID 
MmGetDirtyPagesFromWorkingSet(struct _EPROCESS* Process);
NTSTATUS 
MmWriteToSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
NTSTATUS 
MmReadFromSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
VOID 
MmSetFlagsPage(PVOID PhysicalAddress, ULONG Flags);
ULONG 
MmGetFlagsPage(PVOID PhysicalAddress);
VOID MmSetSavedSwapEntryPage(PVOID PhysicalAddress,
			     SWAPENTRY SavedSwapEntry);
SWAPENTRY MmGetSavedSwapEntryPage(PVOID PhysicalAddress);
VOID MmSetCleanPage(struct _EPROCESS* Process, PVOID Address);
VOID MmLockPage(PVOID PhysicalPage);
VOID MmUnlockPage(PVOID PhysicalPage);

NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG Count);
NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG Count);
NTSTATUS 
MmCreatePhysicalMemorySection(VOID);
PVOID
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
ExAllocatePageWithPhysPage(ULONG PhysPage);
ULONG
MmGetReferenceCountPage(PVOID PhysicalAddress);
BOOLEAN
MmIsUsablePage(PVOID PhysicalAddress);

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

VOID
MiDebugDumpNonPagedPool(BOOLEAN NewOnly);
VOID
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);
VOID 
MmMarkPageMapped(PVOID PhysicalAddress);
VOID 
MmMarkPageUnmapped(PVOID PhysicalAddress);
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
MiCopyFromUserPage(ULONG DestPhysPage, PVOID SourceAddress);
NTSTATUS
MiZeroPage(ULONG PhysPage);
BOOLEAN 
MmIsAccessedAndResetAccessPage(struct _EPROCESS* Process, PVOID Address);
SWAPENTRY 
MmGetSavedSwapEntryPage(PVOID PhysicalAddress);

#define STATUS_MM_RESTART_OPERATION       (0xD0000001)

NTSTATUS 
MmCreateVirtualMappingForKernel(PVOID Address, 
				ULONG flProtect,
				ULONG PhysicalAddress);

#endif
