/* $Id: mmtypes.h,v 1.15 2003/05/28 18:09:09 chorns Exp $ */

#ifndef _INCLUDE_DDK_MMTYPES_H
#define _INCLUDE_DDK_MMTYPES_H

#include <ddk/i386/pagesize.h>
#include <ntos/mm.h>


#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGE_SIZE) ? ((((ULONG)x)&(~0xfff))+0x1000) : ((ULONG)x) )
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~0xfff))


/*
 * FUNCTION: Determines if the given virtual address is page aligned
 */
#define IS_PAGE_ALIGNED(Va) (((ULONG)Va)&0xfff)

/*
 * PURPOSE: Returns the byte offset of a field within a structure
 */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(Type,Field) (LONG)(&(((Type *)(0))->Field))
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
#define CONTAINING_RECORD(Address,Type,Field) (Type *)(((LONG)Address) - FIELD_OFFSET(Type,Field))
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

#define MmSmallSystem (0)
#define MmMediumSystem (1)
#define MmLargeSystem (2)

/* Used in MmFlushImageSection */
typedef enum _MMFLUSH_TYPE
{
	MmFlushForDelete,
	MmFlushForWrite
} MMFLUSH_TYPE;

typedef enum _MEMORY_CACHING_TYPE
{
	MmNonCached = FALSE,
	MmCached = TRUE,
	MmFrameBufferCached,
	MmHardwareCoherentCached,
	MmMaximumCacheType
} MEMORY_CACHING_TYPE;

#endif
