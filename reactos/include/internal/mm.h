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
} SECTION_OBJECT, *PSECTION_OBJECT;

typedef struct
{
   ULONG Type;
   PVOID BaseAddress;
   ULONG Length;
   ULONG Attributes;
   LIST_ENTRY Entry;
   ULONG LockCount;
   union
     {
	struct
	  {	     
	     SECTION_OBJECT* Section;
	     ULONG ViewOffset;
	  } SectionData;
     } Data;
} MEMORY_AREA, *PMEMORY_AREA;


NTSTATUS MmCreateMemoryArea(KPROCESSOR_MODE Mode,
			    PEPROCESS Process,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result);
MEMORY_AREA* MmOpenMemoryAreaByAddress(PEPROCESS Process, PVOID Address);
NTSTATUS MmInitMemoryAreas(VOID);
VOID ExInitNonPagedPool(ULONG BaseAddress);
NTSTATUS MmFreeMemoryArea(PEPROCESS Process,
			  PVOID BaseAddress,
			  ULONG Length,
			  BOOLEAN FreePages);
VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead);
NTSTATUS MmLockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmUnlockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmInitSectionImplementation(VOID);

void VirtualInit(boot_param* bp);

#define MM_LOWEST_USER_ADDRESS (4096)

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
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
VOID MmFreePage(PVOID PhysicalAddress, ULONG Nr);
VOID MmDeletePageTable(PEPROCESS Process, PVOID Address);
NTSTATUS MmCopyMmInfo(PEPROCESS Src, PEPROCESS Dest);
NTSTATUS MmReleaseMmInfo(PEPROCESS Process);
#endif
