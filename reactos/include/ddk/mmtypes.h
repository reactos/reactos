/* $Id: mmtypes.h,v 1.9 2000/07/07 10:30:54 dwelch Exp $ */

#ifndef _INCLUDE_DDK_MMTYPES_H
#define _INCLUDE_DDK_MMTYPES_H

#include <ntos/mm.h>

#ifdef __NTOSKRNL__
PVOID EXPORTED MmUserProbeAddress;
PVOID EXPORTED MmHighestUserAddress;
#else
PVOID IMPORTED MmUserProbeAddress;
PVOID IMPORTED MmHighestUserAddress;
#endif

#ifdef __NTOSKRNL__
extern POBJECT_TYPE EXPORTED MmSectionObjectType;
#else
extern POBJECT_TYPE IMPORTED MmSectionObjectType;
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
