/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/ntoskrnl.h>
#include <internal/mmhal.h>

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


#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)

#define SPE_PAGEIN_PENDING                      (0x1)
#define SPE_MPW_PENDING                         (0x2)
#define SPE_PAGEOUT_PENDING                     (0x4)
#define SPE_DIRTY                               (0x8)
#define SPE_IN_PAGEFILE                         (0x10)

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
  SECTION_PAGE_DIRECTORY PageDirectory;
  ULONG Flags;
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
	  } SectionData;
	struct
	  {
	     LIST_ENTRY SegmentListHead;
	  } VirtualMemoryData;
     } Data;
} MEMORY_AREA, *PMEMORY_AREA;

#define WSET_ADDRESSES_IN_PAGE    (1020)

typedef struct _MWORKING_SET
{
   PVOID Address[WSET_ADDRESSES_IN_PAGE];
   struct _MWORKING_SET* Next;
} MWORKING_SET, *PMWORKING_SET;

typedef struct _MADDRESS_SPACE
{
   LIST_ENTRY MAreaListHead;
   KMUTEX Lock;
   ULONG LowestAddress;
   struct _EPROCESS* Process;
   ULONG WorkingSetSize;
   ULONG WorkingSetLruFirst;
   ULONG WorkingSetLruLast;
   ULONG WorkingSetPagesAllocated;
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
			    MEMORY_AREA** Result);
MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace, 
				       PVOID Address);
NTSTATUS MmInitMemoryAreas(VOID);
VOID ExInitNonPagedPool(ULONG BaseAddress);
NTSTATUS MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
			  PVOID BaseAddress,
			  ULONG Length,
			  BOOLEAN FreePages);
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
			   ULONG LastKernelBase);

PVOID MmAllocPage(SWAPENTRY SavedSwapEntry);
VOID MmDereferencePage(PVOID PhysicalAddress);
VOID MmReferencePage(PVOID PhysicalAddress);
VOID MmDeletePageTable(struct _EPROCESS* Process, 
		       PVOID Address);
NTSTATUS MmCopyMmInfo(struct _EPROCESS* Src, 
		      struct _EPROCESS* Dest);
NTSTATUS MmReleaseMmInfo(struct _EPROCESS* Process);
NTSTATUS Mmi386ReleaseMmInfo(struct _EPROCESS* Process);
VOID MmDeleteVirtualMapping(struct _EPROCESS* Process, 
			    PVOID Address, 
			    BOOL FreePage);

VOID MmBuildMdlFromPages(PMDL Mdl, PULONG Pages);
PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);
VOID MiShutdownMemoryManager(VOID);
ULONG MmGetPhysicalAddressForProcess(struct _EPROCESS* Process,
				     PVOID Address);
NTSTATUS STDCALL MmUnmapViewOfSection(struct _EPROCESS* Process,
				      PMEMORY_AREA MemoryArea);
NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
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

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address);
NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address);
NTSTATUS MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
					MEMORY_AREA* MemoryArea, 
					PVOID Address);
NTSTATUS MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
				      MEMORY_AREA* MemoryArea, 
				      PVOID Address);
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
BOOLEAN MmAddPageToWorkingSet(struct _EPROCESS* Process,
			      PVOID Address);

VOID MmInitPagingFile(VOID);
VOID MmReserveSwapPages(ULONG Nr);
VOID MmDereserveSwapPages(ULONG Nr);
SWAPENTRY MmAllocSwapPage(VOID);
VOID MmFreeSwapPage(SWAPENTRY Entry);

VOID MmInit1(ULONG FirstKernelPhysAddress, 
	     ULONG LastKernelPhysAddress,
	     ULONG LastKernelAddress);
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

NTSTATUS MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
				PMEMORY_AREA MArea,
				PVOID Address);
NTSTATUS MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
				  PMEMORY_AREA MArea,
				  PVOID Address);
PVOID MmGetDirtyPagesFromWorkingSet(struct _EPROCESS* Process);
NTSTATUS MmWriteToSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
NTSTATUS MmReadFromSwapPage(SWAPENTRY SwapEntry, PMDL Mdl);
VOID MmSetFlagsPage(PVOID PhysicalAddress, ULONG Flags);
ULONG MmGetFlagsPage(PVOID PhysicalAddress);
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
		     PHYSICAL_ADDRESS HighestAcceptableAddress);

#define MM_PHYSICAL_PAGE_MPW_PENDING     (0x8)

#endif
