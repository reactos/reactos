
enum
{
   MDL_MAPPED_TO_SYSTEM_VA = 0x1,
   MDL_PAGES_LOCKED = 0x2,
   MDL_SOURCE_IS_NONPAGED_POOL = 0x4,
   MDL_ALLOCATED_FIXED_SIZE = 0x8,
   MDL_PARTIAL = 0x10,
   MDL_PARTIAL_HAS_BEEN_MAPPED = 0x20,
   MDL_IO_PAGE_READ = 0x40,
   MDL_WRITE_OPERATION = 0x80,
   MDL_PARENT_MAPPED_SYSTEM_VA = 0x100,
   MDL_LOCK_HELD = 0x200,
   MDL_SCATTER_GATHER_VA = 0x400,
   MDL_IO_SPACE = 0x800,
   MDL_NETWORK_HEADER = 0x1000,
   MDL_MAPPING_CAN_FAIL = 0x2000,
   MDL_ALLOCATED_MUST_SUCCEED = 0x4000,
   MDL_64_BIT_VA = 0x8000,
};

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
