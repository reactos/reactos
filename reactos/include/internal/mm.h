/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/linkage.h>
#include <internal/ntoskrnl.h>
#include <windows.h>

/* TYPES *********************************************************************/

enum
{
   MEMORY_AREA_INVALID,
   MEMORY_AREA_SECTION_VIEW_COMMIT,
   MEMORY_AREA_CONTINUOUS_MEMORY,
   MEMORY_AREA_NO_CACHE,
   MEMORY_AREA_IO_MAPPING,
   MEMORY_AREA_SYSTEM,
   MEMORY_AREA_MDL_MAPPING,
   MEMORY_AREA_COMMIT,
   MEMORY_AREA_RESERVE,
   MEMORY_AREA_SECTION_VIEW_RESERVE,
   MEMORY_AREA_CACHE_SEGMENT,
};

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
} SECTION_OBJECT, *PSECTION_OBJECT;

typedef struct
{
   ULONG Type;
   PVOID BaseAddress;
   ULONG Length;
   ULONG Attributes;
   LIST_ENTRY Entry;
   ULONG LockCount;
   PEPROCESS Process;
   union
     {
	struct
	  {	     
	     SECTION_OBJECT* Section;
	     ULONG ViewOffset;
	     LIST_ENTRY ViewListEntry;
	  } SectionData;
     } Data;
} MEMORY_AREA, *PMEMORY_AREA;


/* FUNCTIONS */

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmInitializeKernelAddressSpace(VOID);
PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID);
PMADDRESS_SPACE MmGetKernelAddressSpace(VOID);
NTSTATUS MmInitializeAddressSpace(PMADDRESS_SPACE AddressSpace);
NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace);
PVOID STDCALL MmAllocateSection (IN ULONG Length);
NTSTATUS MmCreateMemoryArea(PEPROCESS Process,
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

/*void VirtualInit(boot_param* bp);*/

#define MM_LOWEST_USER_ADDRESS (4096)

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
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

PVOID MmAllocPage(VOID);
VOID MmDereferencePage(PVOID PhysicalAddress);
VOID MmReferencePage(PVOID PhysicalAddress);
VOID MmDeletePageTable(PEPROCESS Process, PVOID Address);
NTSTATUS MmCopyMmInfo(PEPROCESS Src, PEPROCESS Dest);
NTSTATUS MmReleaseMmInfo(PEPROCESS Process);
NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process);
VOID MmDeletePageEntry(PEPROCESS Process, PVOID Address, BOOL FreePage);

VOID MmBuildMdlFromPages(PMDL Mdl);
PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);
VOID MiShutdownMemoryManager(VOID);
ULONG MmGetPhysicalAddressForProcess(PEPROCESS Process,
				     PVOID Address);
NTSTATUS MmUnmapViewOfSection(PEPROCESS Process,
			      PMEMORY_AREA MemoryArea);
PVOID MiTryToSharePageInSection(PSECTION_OBJECT Section, ULONG Offset);

NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
VOID MmInitPagingFile(VOID);
ULONG MmPageFault(ULONG cs, ULONG eip, ULONG error_code);

#endif
