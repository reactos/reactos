/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;

struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP;
typedef ULONG SWAPENTRY, *PSWAPENTRY;


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
#define MEMORY_AREA_PAGED_POOL           (14)

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
  ULONG_PTR Entry[NR_SECTION_PAGE_ENTRIES];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

typedef struct
{
   PSECTION_PAGE_TABLE PageTables[NR_SECTION_PAGE_TABLES];
} SECTION_PAGE_DIRECTORY, *PSECTION_PAGE_DIRECTORY;

#define MM_PAGEFILE_SECTION    (0x1)
#define MM_IMAGE_SECTION       (0x2)
/*
 * Flags for section objects
 */
#define SO_PHYSICAL_MEMORY                      (0x4)

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


typedef struct _MEMORY_AREA
{
#ifdef DBG
   ULONG Magic;
#endif /* DBG */
   ULONG Type;
   PVOID BaseAddress;
   ULONG Length;
   ULONG Attributes;
   LIST_ENTRY Entry;
   ULONG LockCount;
   ULONG ReferenceCount;
   struct _EPROCESS* Process;
   union
     {
       struct
	{
	  SECTION_OBJECT* Section;
	  ULONG ViewOffset;
	  LIST_ENTRY ViewListEntry;
	  PMM_SECTION_SEGMENT Segment;
	  BOOLEAN WriteCopyView;
       } SectionData;
       struct
       {
	 LIST_ENTRY SegmentListHead;
       } VirtualMemoryData;
   } Data;
} MEMORY_AREA, *PMEMORY_AREA;


#define MM_PAGEOP_PAGEIN        (1)
#define MM_PAGEOP_PAGEOUT       (2)
#define MM_PAGEOP_PAGESYNCH     (3)
#define MM_PAGEOP_ACCESSFAULT   (4)
#define MM_PAGEOP_MINIMUM       MM_PAGEOP_PAGEIN
#define MM_PAGEOP_MAXIMUM       MM_PAGEOP_ACCESSFAULT

typedef struct _MM_PAGEOP
{
#ifdef DBG
  /* Magic ID */
  ULONG Magic;
#endif /* DBG */
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


#define PAGE_STATE_VALID                (0)
#define PAGE_STATE_PROTOTYPE            (1)
#define PAGE_STATE_DEMAND_ZERO          (2)
#define PAGE_STATE_TRANSITION           (3)

#define MM_PTYPE(x)                     (x)

#define PAGE_LIST_FREE_ZEROED           (0)
#define PAGE_LIST_FREE_UNZEROED         (1)
#define PAGE_LIST_BIOS                  (2)
#define PAGE_LIST_STANDBY               (3)
#define PAGE_LIST_MODIFIED              (4)
#define PAGE_LIST_MODIFIED_NO_WRITE     (5)
#define PAGE_LIST_MPW                   (6)

/* PHYSICAL_PAGE.Flags */
#define MM_PHYSICAL_PAGE_FREE         (0x1)
#define MM_PHYSICAL_PAGE_USED         (0x2)
#define MM_PHYSICAL_PAGE_BIOS         (0x3)
#define MM_PHYSICAL_PAGE_STANDBY      (0x4)
#define MM_PHYSICAL_PAGE_MODIFIED     (0x5)
#define MM_PHYSICAL_PAGE_MPW          (0x6)

typedef VOID (*PRMAP_DELETE_CALLBACK)(IN PVOID  Context,
  IN PEPROCESS  Process,
  IN PVOID  Address);

/* FIXME: Unionize this structure */
typedef struct _PHYSICAL_PAGE
{
  ULONG Flags;
  LIST_ENTRY ListEntry;
  ULONG ReferenceCount;
  SWAPENTRY SavedSwapEntry;
  ULONG LockCount;
  ULONG MapCount;
  struct _MM_RMAP_ENTRY* RmapListHead;
  PRMAP_DELETE_CALLBACK RmapDelete;
  PVOID RmapDeleteContext;
  PMM_PAGEOP PageOp;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;


extern PPHYSICAL_PAGE MmPageArray;

#define MiPageFromDescriptor(pp)((((ULONG_PTR)(pp) - (ULONG_PTR) MmPageArray) / sizeof(PHYSICAL_PAGE)) * PAGESIZE)

typedef struct _MADDRESS_SPACE
{
#ifdef DBG
  ULONG Magic;
#endif /* DBG */
  LIST_ENTRY MAreaListHead;
  KMUTEX Lock;
  ULONG ReferenceCount;
  ULONG LowestAddress;
  struct _EPROCESS* Process;
  PUSHORT PageTableRefCountTable;
  ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;


#define MmIsCopyOnWriteMemoryArea(MemoryArea) \
( \
	((MemoryArea)->Data.SectionData.Segment->WriteCopy \
	|| (MemoryArea)->Data.SectionData.WriteCopyView) \
	&& ((MemoryArea)->Attributes == PAGE_READWRITE \
	|| (MemoryArea)->Attributes == PAGE_EXECUTE_READWRITE) \
)


extern ULONG MiMaximumModifiedPageListSize;
extern ULONG MiModifiedPageListSize;
extern ULONG MiMaximumStandbyPageListSize;
extern ULONG MiStandbyPageListSize;

/* FUNCTIONS */

#ifdef DBG

VOID
DbgMmDumpProtection(IN ULONG  Value);

VOID
MiDumpPTE(IN ULONG  Value);

VOID
MiDumpProcessPTE(IN PEPROCESS  Process,
  IN PVOID  Address);

#endif /* DBG */

VOID
MiAcquirePageListLock(IN ULONG  PageList,
  OUT PLIST_ENTRY  * ListHead);

VOID
MiReleasePageListLock();

VOID
MiReclaimPage(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty);

VOID
MmInitMpwThreads();

VOID
MiShutdownMpwThreads();

VOID
MiSignalModifiedPageWriter();

VOID
MiSignalMappedPageWriter();

VOID
MmInitializeBalanceSetManager();

ULONG
MiGetLockCountPage(IN ULONG_PTR  PhysicalAddress);

VOID
MiDisableAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN PBOOLEAN  Modified);

VOID
MiEnableAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Modified);

VOID
MiGetDirtyAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN PBOOLEAN  Dirty);

VOID
MiGetPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState,
  OUT PBOOLEAN  Result);

VOID
MiClearPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState);

VOID
MiSetPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState);

VOID
MiSetDirtyAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty);

BOOLEAN
MiPageState(IN PEPROCESS  Process,
  IN PVOID  Address,
  IN ULONG  PageState);

VOID
MiClearPageState(IN PEPROCESS  Process,
  IN PVOID  Address,
  IN ULONG  PageState);

VOID
MiSetPageState(IN PEPROCESS  Process,
  IN PVOID  Address,
  IN ULONG  PageState);

VOID
MmInitializeKernelAddressSpace(VOID);

PMADDRESS_SPACE
MmGetCurrentAddressSpace();

PMADDRESS_SPACE
MmGetKernelAddressSpace();

NTSTATUS
MmInitializeAddressSpace(IN PEPROCESS  Process,
  IN PMADDRESS_SPACE  AddressSpace);

NTSTATUS
MmDestroyAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

VOID
MmReferenceAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

VOID
MmDereferenceAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

VOID
MmApplyMemoryAreaProtection(IN PMEMORY_AREA  MemoryArea);

NTSTATUS
MmFlushSection(IN  PSECTION_OBJECT  SectionObject,
	IN	PLARGE_INTEGER  FileOffset  OPTIONAL,
	IN	ULONG  Length,
	OUT	PIO_STATUS_BLOCK	IoStatus  OPTIONAL);

PVOID STDCALL
MmAllocateSection(IN ULONG  Length);

NTSTATUS
MmCreateMemoryArea(IN PEPROCESS  Process,
	IN PMADDRESS_SPACE  AddressSpace,
	IN ULONG  Type,
	IN OUT PVOID*  BaseAddress,
	IN ULONG  Length,
	IN ULONG  Attributes,
	OUT PMEMORY_AREA*  Result,
	IN BOOLEAN  FixedAddress);

PMEMORY_AREA
MmOpenMemoryAreaByAddress(IN PMADDRESS_SPACE  AddressSpace, 
  IN PVOID Address);

VOID
MmCloseMemoryArea(IN PMEMORY_AREA  MemoryArea);

NTSTATUS
MmInitMemoryAreas();

VOID
ExInitNonPagedPool(IN PVOID  BaseAddress);

typedef VOID (*PFREE_MEMORY_AREA_PAGE_CALLBACK)(IN BOOLEAN Before,
  IN PVOID  Context,
  IN PMEMORY_AREA  MemoryArea,
  IN PVOID  Address,
  IN ULONG_PTR  PhysicalAddress,
  IN SWAPENTRY  SwapEntry,
  IN BOOLEAN  Dirty);

NTSTATUS
MmFreeMemoryArea(IN PMADDRESS_SPACE  AddressSpace,
	IN PVOID  BaseAddress,
	IN ULONG  Length,
	IN PFREE_MEMORY_AREA_PAGE_CALLBACK  FreePage,
	IN PVOID FreePageContext);

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

ULONG_PTR
MmAllocPage(IN ULONG  Consumer,
 IN SWAPENTRY  SavedSwapEntry);

VOID MmDereferencePage(IN ULONG_PTR PhysicalAddress);
VOID MmReferencePage(IN ULONG_PTR PhysicalAddress);
VOID MmDeletePageTable(struct _EPROCESS* Process, 
		       PVOID Address);
NTSTATUS MmCopyMmInfo(struct _EPROCESS* Src, 
		      struct _EPROCESS* Dest);
NTSTATUS MmReleaseMmInfo(struct _EPROCESS* Process);
NTSTATUS Mmi386ReleaseMmInfo(struct _EPROCESS* Process);
VOID
MmDeleteVirtualMapping(struct _EPROCESS* Process, 
		       PVOID Address,
		       BOOLEAN FreePage,
		       PBOOLEAN WasDirty,
		       PULONG PhysicalPage);

#define MM_PAGE_CLEAN     (0)
#define MM_PAGE_DIRTY     (1)

VOID
MmBuildMdlFromPages(IN PMDL  Mdl,
  IN PULONG_PTR  Pages);

PVOID
MmGetMdlPageAddress(IN PMDL  Mdl,
  IN PVOID  Offset);

VOID
MiShutdownMemoryManager(VOID);

ULONG
MmGetPhysicalAddressForProcess(IN struct _EPROCESS*  Process,
  IN PVOID  Address);

NTSTATUS STDCALL
MmUnmapViewOfSection(struct _EPROCESS* Process, PVOID BaseAddress);

VOID
MmInitPagingFile(VOID);

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

NTSTATUS 
MmFlushVirtualMemory(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       struct _MM_PAGEOP* PageOp);
NTSTATUS 
MmFlushSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       struct _MM_PAGEOP* PageOp);
MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length);

VOID
ExUnmapPage(IN PVOID  Addr);

PVOID
ExAllocatePage(VOID);

VOID
MmInitPagingFile(VOID);

BOOLEAN
MmReserveSwapPages(IN ULONG  Nr);

VOID
MmDereserveSwapPages(IN ULONG  Nr);

SWAPENTRY
MmAllocSwapPage(VOID);

VOID
MmFreeSwapPage(IN SWAPENTRY  Entry);

VOID
MiValidateSwapEntry(IN SWAPENTRY  Entry);

VOID
MiValidatePageOp(IN PMM_PAGEOP  PageOp);

VOID
MiValidatePhysicalAddress(IN ULONG_PTR  PhysicalAddress);

VOID
MiValidateAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

VOID
MiValidateMemoryArea(IN PMEMORY_AREA  MemoryArea);

VOID
MiValidateRmapList(struct _MM_RMAP_ENTRY*  RmapList);

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

extern BOOLEAN MiInitialized;

#ifdef DBG
#define VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress) MiValidatePhysicalAddress((ULONG_PTR)PhysicalAddress)
#define VALIDATE_SWAP_ENTRY(Entry) MiValidateSwapEntry(Entry)
#define VALIDATE_PAGEOP(PageOp) MiValidatePageOp(PageOp)
#define VALIDATE_ADDRESS_SPACE(AddressSpace) MiValidateAddressSpace(AddressSpace)
#define VALIDATE_MEMORY_AREA(MemoryArea) MiValidateMemoryArea(MemoryArea)
#define VALIDATE_RMAP_LIST(RmapList) MiValidateRmapList(RmapList)
#else /* !DBG */
#define VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress)
#define VALIDATE_SWAP_ENTRY(Entry)
#define VALIDATE_PAGEOP(PageOp)
#define VALIDATE_ADDRESS_SPACE(AddressSpace)
#define VALIDATE_MEMORY_AREA(MemoryArea)
#define VALIDATE_RMAP_LIST(RmapList)
#endif /* DBG */

#ifdef DBG

VOID
MiLockAddressSpace(IN PMADDRESS_SPACE  AddressSpace,
  IN LPSTR  FileName,
	IN ULONG  LineNumber);

VOID
MiUnlockAddressSpace(IN PMADDRESS_SPACE  AddressSpace,
  IN LPSTR  FileName,
	IN ULONG  LineNumber);

/* Use macros for easier debugging */
#define MmLockAddressSpace(AddressSpace) MiLockAddressSpace(AddressSpace, __FILE__, __LINE__)
#define MmUnlockAddressSpace(AddressSpace) MiUnlockAddressSpace(AddressSpace, __FILE__, __LINE__)

#else /* !DBG */

VOID
MiLockAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

VOID
MiUnlockAddressSpace(IN PMADDRESS_SPACE  AddressSpace);

#define MmLockAddressSpace MiLockAddressSpace
#define MmUnlockAddressSpace MiUnlockAddressSpace

#endif /* !DBG */


#ifdef DBG

VOID
MiReferenceMemoryArea(IN PMEMORY_AREA  MemoryArea,
  IN LPSTR  FileName,
	IN ULONG  LineNumber);

VOID
MiDereferenceMemoryArea(IN PMEMORY_AREA  MemoryArea,
  IN LPSTR  FileName,
	IN ULONG  LineNumber);
	
/* Use macros for easier debugging */
#define MmReferenceMemoryArea(MemoryArea) MiReferenceMemoryArea(MemoryArea, __FILE__, __LINE__)
#define MmDereferenceMemoryArea(MemoryArea) MiDereferenceMemoryArea(MemoryArea, __FILE__, __LINE__)

#else /* !DBG */

VOID
MiReferenceMemoryArea(IN PMEMORY_AREA  MemoryArea);

VOID
MiDereferenceMemoryArea(IN PMEMORY_AREA  MemoryArea);

#define MmReferenceMemoryArea MiReferenceMemoryArea
#define MmDereferenceMemoryArea MiDereferenceMemoryArea

#endif /* !DBG */


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
MmSetFlagsPage(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  Flags);

ULONG 
MmGetFlagsPage(IN ULONG_PTR  PhysicalAddress);

VOID
MmSetSavedSwapEntryPage(IN ULONG_PTR PhysicalAddress,
  SWAPENTRY SavedSwapEntry);

SWAPENTRY
MmGetSavedSwapEntryPage(IN ULONG_PTR  PhysicalAddress);

VOID
MmSetSavedPageOp(IN ULONG_PTR  PhysicalAddress,
  IN PMM_PAGEOP  PageOp);

PMM_PAGEOP
MmGetSavedPageOp(IN ULONG_PTR  PhysicalAddress);

VOID
MmSetCleanPage(IN PEPROCESS  Process,
  IN PVOID  Address);

VOID
MmLockPage(IN ULONG_PTR  PhysicalPage);

VOID
MmUnlockPage(IN ULONG_PTR  PhysicalPage);

NTSTATUS
MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG Count);

NTSTATUS
MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG Count);

NTSTATUS
MmCreatePhysicalMemorySection(VOID);

PVOID
MmGetContinuousPages(ULONG NumberOfBytes,
	PHYSICAL_ADDRESS HighestAcceptableAddress,
	ULONG Alignment);

NTSTATUS 
MmAccessFaultSectionView(IN PMADDRESS_SPACE  AddressSpace,
	IN MEMORY_AREA*  IN MemoryArea, 
	IN PVOID  Address,
	IN BOOLEAN  Locked);

ULONG
MmGetPageProtect(IN struct _EPROCESS*  Process,
  IN PVOID  Address);

PVOID
ExAllocatePageWithPhysPage(IN ULONG_PTR Page);

ULONG
MmGetReferenceCountPage(IN ULONG_PTR PhysicalAddress);

BOOLEAN
MmIsUsablePage(IN ULONG_PTR  PhysicalAddress);

VOID
MmReleasePageOp(IN PMM_PAGEOP  PageOp);

PMM_PAGEOP
MmGetPageOp(IN PMEMORY_AREA  MArea,
  IN ULONG  Pid,
  IN PVOID  Address,
  IN PMM_SECTION_SEGMENT  Segment,
  IN ULONG  Offset,
  IN ULONG  OpType);

PMM_PAGEOP
MmGotPageOp(IN PMEMORY_AREA  MArea,
  IN ULONG  Pid,
  IN PVOID  Address,
  IN PMM_SECTION_SEGMENT  Segment,
  IN ULONG  Offset);

VOID
MiDebugDumpNonPagedPool(BOOLEAN NewOnly);

VOID
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);

VOID 
MmMarkPageMapped(IN ULONG_PTR  PhysicalAddress);

VOID 
MmMarkPageUnmapped(IN ULONG_PTR  PhysicalAddress);

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
MiZeroPage(IN ULONG_PTR  Page);

BOOLEAN 
MmIsAccessedAndResetAccessPage(struct _EPROCESS* Process, PVOID Address);

#define STATUS_MM_RESTART_OPERATION       (0xD0000001)

NTSTATUS 
MmCreateVirtualMappingForKernel(PVOID Address, 
				ULONG flProtect,
				ULONG PhysicalAddress);
NTSTATUS MmCommitPagedPoolAddress(PVOID Address);

/* Memory balancing. */
VOID
MmInitializeMemoryConsumer(ULONG Consumer, 
			   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, 
					    PULONG NrFreed));
VOID
MmInitializeBalancer(ULONG NrAvailablePages);

NTSTATUS
MmReleasePageMemoryConsumer(IN ULONG Consumer,
  IN ULONG_PTR  Page);

NTSTATUS
MiFreePageMemoryConsumer(IN ULONG  Consumer,
  IN ULONG_PTR  Page);

NTSTATUS
MmRequestPageMemoryConsumer(IN ULONG  Consumer,
  IN BOOLEAN  CanWait,
  OUT PULONG_PTR  pPage);

VOID
MiSatisfyAllocationRequest();

#define MC_CACHE          (0)
#define MC_USER           (1)
#define MC_PPOOL          (2)
#define MC_NPPOOL         (3)
#define MC_MAXIMUM        (4)

VOID
MiTransitionAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Reference,
  OUT PBOOLEAN  Modified);

NTSTATUS
MiAbortTransition(IN ULONG_PTR  Address);

NTSTATUS
MiFinishTransition(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty);

VOID 
MmSetRmapListHeadPage(IN ULONG_PTR PhysicalAddress,
  IN struct _MM_RMAP_ENTRY*  ListHead);

struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(IN ULONG_PTR  PhysicalAddress);

VOID
MmSetRmapCallback(IN ULONG_PTR  PhysicalAddress,
  IN PRMAP_DELETE_CALLBACK  RmapDelete,
  IN PVOID  RmapDeleteContext);

VOID
MmGetRmapCallback(IN ULONG_PTR  PhysicalAddress,
  IN PRMAP_DELETE_CALLBACK  *RmapDelete,
  IN PVOID  *RmapDeleteContext);

NTSTATUS
MmPageOutPhysicalAddress(ULONG_PTR PhysicalAddress);

VOID
MmInsertRmap(ULONG_PTR PhysicalAddress, PEPROCESS Process, PVOID Address);

VOID
MmDeleteAllRmaps(ULONG_PTR PhysicalAddress, PVOID Context,
		 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process,
				       PVOID Address));

VOID
MmDeleteRmap(IN ULONG_PTR  PhysicalAddress,
  IN PEPROCESS  Process,
  IN PVOID  Address);

VOID
MmInitializeRmapList(VOID);

ULONG_PTR
MmGetLRUNextUserPage(IN ULONG_PTR PreviousPhysicalAddress);

ULONG_PTR
MmGetLRUFirstUserPage(VOID);

NTSTATUS
MmPrepareFlushPhysicalAddress(IN ULONG_PTR  PhysicalAddress);

NTSTATUS
MmFlushPhysicalAddress(IN ULONG_PTR  PhysicalAddress);

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages);

VOID
MmDisableVirtualMapping(IN PEPROCESS  Process,
  IN PVOID  Address,
  OUT PBOOLEAN  WasDirty,
  OUT PULONG_PTR  PhysicalAddr);

VOID MmEnableVirtualMapping(IN PEPROCESS  Process,
  IN PVOID  Address);

VOID
MmDeletePageFileMapping(IN PEPROCESS  Process,
  IN PVOID  Address,
  OUT PSWAPENTRY  SwapEntry);

NTSTATUS
MmCreatePageFileMapping(IN PEPROCESS  Process,
  IN PVOID  Address,
  IN SWAPENTRY  SwapEntry);

BOOLEAN
MmIsPageSwapEntry(IN PEPROCESS  Process,
  IN PVOID  Address);

VOID
MmTransferOwnershipPage(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  NewConsumer);

VOID
MmSetDirtyPage(IN PEPROCESS  Process,
  IN PVOID  Address);

NTSTATUS
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
		     MEMORY_AREA* MemoryArea,
		     PVOID Address,
		     PMM_PAGEOP PageOp);

NTSTATUS
MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       PMM_PAGEOP PageOp);

#endif
