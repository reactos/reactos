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


/*
 * FUNCTION: Gets a page with a restricted max physical address (i.e.
 * suitable for dma)
 * RETURNS:
 *      The physical address of the page if it succeeds
 *      NULL if it fails.
 * NOTES: This is very inefficent because the list isn't sorted. On the
 * other hand sorting the list would be quite expensive especially if dma
 * is only used infrequently. Perhaps a special cache of dma pages should
 * be maintained?
 */
unsigned int get_dma_page(unsigned int max_address);

/*
 * FUNCTION: Allocate a page and return its physical address
 * RETURNS: The physical address of the page allocated
 */
asmlinkage unsigned int get_free_page(void);

/*
 * FUNCTION: Adds pages to the free list
 * ARGUMENTS:
 *          physical_base = Physical address of the base of the region to
 *                          be freed
 *          nr = number of continuous pages to free
 */
asmlinkage void free_page(unsigned int physical_base, unsigned int nr);

void mark_page_not_writable(unsigned int vaddr);

void VirtualInit(boot_param* bp);

#define MM_LOWEST_USER_ADDRESS (4096)

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes);

#endif
