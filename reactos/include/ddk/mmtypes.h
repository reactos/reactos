/* $Id$ */

#ifndef _INCLUDE_DDK_MMTYPES_H
#define _INCLUDE_DDK_MMTYPES_H

#include <ddk/i386/pagesize.h>
#include <ntos/mm.h>

/*
 * FUNCTION: Determines if the given virtual address is page aligned
 */
#define IS_PAGE_ALIGNED(Va) (((ULONG)Va)&0xfff)

/*
 * PURPOSE: Returns the byte offset of a field within a structure
 */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(Type,Field) ((ULONG_PTR)(&(((Type *)(0))->Field)))
#endif

/*
 * PURPOSE: Returns the base address structure if the caller knows the 
 * address of a field within the structure
 * ARGUMENTS:
 *          Address = address of the field
 *          Type = Type of the whole structure
 *          Field = Name of the field whose address is none
 */
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(Address,Type,Field) ((Type *)(((ULONG_PTR)Address) - FIELD_OFFSET(Type,Field)))
#endif

#ifdef _M_IX86
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
#elif _M_IA64
typedef ULONG64 PFN_NUMBER, *PPFN_NUMBER;
#else
#error Unknown architecture
#endif

#define   MDL_MAPPED_TO_SYSTEM_VA      (0x1)
#define   MDL_PAGES_LOCKED             (0x2)
#define   MDL_SOURCE_IS_NONPAGED_POOL  (0x4)
#define   MDL_ALLOCATED_FIXED_SIZE     (0x8)
#define   MDL_PARTIAL                  (0x10)
#define   MDL_PARTIAL_HAS_BEEN_MAPPED  (0x20)
#define   MDL_IO_PAGE_READ             (0x40)
#define   MDL_WRITE_OPERATION          (0x80)
#define   MDL_PARENT_MAPPED_SYSTEM_VA  (0x100)
#define   MDL_LOCK_HELD                (0x200)
#define   MDL_SCATTER_GATHER_VA        (0x400)
#define   MDL_IO_SPACE                 (0x800)
#define   MDL_NETWORK_HEADER           (0x1000)
#define   MDL_MAPPING_CAN_FAIL         (0x2000)
#define   MDL_ALLOCATED_MUST_SUCCEED   (0x4000)
#define   MDL_64_BIT_VA                (0x8000)

typedef enum _MM_PAGE_PRIORITY {
  LowPagePriority,
  NormalPagePriority = 16,
  HighPagePriority = 32
} MM_PAGE_PRIORITY;

typedef struct _MDL
/*
 * PURPOSE: Describes a user buffer passed to a system API
 */
{
   struct _MDL* Next;
   CSHORT Size;
   CSHORT MdlFlags;
   struct _EPROCESS* Process;
   PVOID MappedSystemVa;
   PVOID StartVa;
   ULONG ByteCount;
   ULONG ByteOffset;
} MDL, *PMDL;

typedef struct _PHYSICAL_MEMORY_RANGE {
	PHYSICAL_ADDRESS BaseAddress;
	LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE; 

// read file scatter / write file scatter
typedef union _FILE_SEGMENT_ELEMENT {
	PVOID Buffer;
	ULONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

typedef struct _READ_LIST {
	struct _FILE_OBJECT* FileObject;
    ULONG NumberOfEntries;
    ULONG IsImage;
    FILE_SEGMENT_ELEMENT List[1];
} READ_LIST, *PREAD_LIST;

#define MmSmallSystem (0)
#define MmMediumSystem (1)
#define MmLargeSystem (2)

/* Used in MmFlushImageSection */
typedef enum _MMFLUSH_TYPE
{
	MmFlushForDelete,
	MmFlushForWrite
} MMFLUSH_TYPE;

typedef enum _MEMORY_CACHING_TYPE_ORIG {
    MmFrameBufferCached = 2
} MEMORY_CACHING_TYPE_ORIG;

typedef enum _MEMORY_CACHING_TYPE
{
	MmNonCached = FALSE,
	MmCached = TRUE,
	MmWriteCombined = MmFrameBufferCached,
	MmHardwareCoherentCached,
	MmMaximumCacheType
} MEMORY_CACHING_TYPE;

typedef struct _MMWSL *PMMWSL;

typedef struct _MMSUPPORT_FLAGS {
    ULONG SessionSpace              : 1;
    ULONG BeingTrimmed              : 1;
    ULONG SessionLeader             : 1;
    ULONG TrimHard                  : 1;
    ULONG WorkingSetHard            : 1;
    ULONG AddressSpaceBeingDeleted  : 1;
    ULONG Available                 : 10;
    ULONG AllowWorkingSetAdjustment : 8;
    ULONG MemoryPriority            : 8;
} MMSUPPORT_FLAGS, *PMMSUPPORT_FLAGS;

typedef struct _MMSUPPORT
{
    LARGE_INTEGER   LastTrimTime;
    MMSUPPORT_FLAGS Flags;
    ULONG           PageFaultCount;
    ULONG           PeakWorkingSetSize;
    ULONG           WorkingSetSize;
    ULONG           MinimumWorkingSetSize;
    ULONG           MaximumWorkingSetSize;
    PMMWSL          VmWorkingSetList;
    LIST_ENTRY      WorkingSetExpansionLinks;
    ULONG           Claim;
    ULONG           NextEstimationSlot;
    ULONG           NextAgingSlot;
    ULONG           EstimatedAvailable;
    ULONG           GrowthSinceLastEstimate;
} MMSUPPORT, *PMMSUPPORT;

#endif
