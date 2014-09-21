/******************************************************************************
 *                         Memory manager Types                               *
 ******************************************************************************/
$if (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
typedef ULONG NODE_REQUIREMENT;
#define MM_ANY_NODE_OK                           0x80000000
#endif

#define MM_DONT_ZERO_ALLOCATION                  0x00000001
#define MM_ALLOCATE_FROM_LOCAL_NODE_ONLY         0x00000002
#define MM_ALLOCATE_FULLY_REQUIRED               0x00000004
#define MM_ALLOCATE_NO_WAIT                      0x00000008
#define MM_ALLOCATE_PREFER_CONTIGUOUS            0x00000010
#define MM_ALLOCATE_REQUIRE_CONTIGUOUS_CHUNKS    0x00000020

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_FREE_EXTRA_PTES         0x0200
#define MDL_DESCRIBES_AWE           0x0400
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000
#define MDL_INTERNAL                0x8000

#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE)

#define FLUSH_MULTIPLE_MAXIMUM       32

/* Section access rights */
#define SECTION_QUERY                0x0001
#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0008
#define SECTION_EXTEND_SIZE          0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY| \
                            SECTION_MAP_WRITE |                     \
                            SECTION_MAP_READ |                      \
                            SECTION_MAP_EXECUTE |                   \
                            SECTION_EXTEND_SIZE)

#define SESSION_QUERY_ACCESS         0x0001
#define SESSION_MODIFY_ACCESS        0x0002

#define SESSION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |  \
                            SESSION_QUERY_ACCESS     |  \
                            SESSION_MODIFY_ACCESS)

#define SEGMENT_ALL_ACCESS SECTION_ALL_ACCESS

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD            0x100
#define PAGE_NOCACHE          0x200
#define PAGE_WRITECOMBINE     0x400

#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_DECOMMIT         0x4000
#define MEM_RELEASE          0x8000
#define MEM_FREE            0x10000
#define MEM_PRIVATE         0x20000
#define MEM_MAPPED          0x40000
#define MEM_RESET           0x80000
#define MEM_TOP_DOWN       0x100000
#define MEM_LARGE_PAGES  0x20000000
#define MEM_4MB_PAGES    0x80000000

#define SEC_RESERVE       0x4000000
#define SEC_COMMIT        0x8000000
#define SEC_LARGE_PAGES  0x80000000

/* Section map options */
typedef enum _SECTION_INHERIT {
  ViewShare = 1,
  ViewUnmap = 2
} SECTION_INHERIT;

typedef ULONG PFN_COUNT;
typedef LONG_PTR SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG_PTR PFN_NUMBER, *PPFN_NUMBER;

_Struct_size_bytes_(_Inexpressible_(sizeof(struct _MDL) +
    (ByteOffset + ByteCount + PAGE_SIZE-1) / PAGE_SIZE * sizeof(PFN_NUMBER)))
typedef struct _MDL {
  struct _MDL *Next;
  CSHORT Size;
  CSHORT MdlFlags;
  struct _EPROCESS *Process;
  PVOID MappedSystemVa;
  PVOID StartVa;
  ULONG ByteCount;
  ULONG ByteOffset;
} MDL, *PMDL;
#if (_MSC_VER >= 1600)
typedef _Readable_bytes_(_Inexpressible_(polymorphism)) MDL *PMDLX;
#else
typedef MDL *PMDLX;
#endif

typedef enum _MEMORY_CACHING_TYPE_ORIG {
  MmFrameBufferCached = 2
} MEMORY_CACHING_TYPE_ORIG;

typedef enum _MEMORY_CACHING_TYPE {
  MmNonCached = FALSE,
  MmCached = TRUE,
  MmWriteCombined = MmFrameBufferCached,
  MmHardwareCoherentCached,
  MmNonCachedUnordered,
  MmUSWCCached,
  MmMaximumCacheType
} MEMORY_CACHING_TYPE;

typedef enum _MM_PAGE_PRIORITY {
  LowPagePriority,
  NormalPagePriority = 16,
  HighPagePriority = 32
} MM_PAGE_PRIORITY;

typedef enum _MM_SYSTEM_SIZE {
  MmSmallSystem,
  MmMediumSystem,
  MmLargeSystem
} MM_SYSTEMSIZE;

#ifndef _NTSYSTEM_
__CREATE_NTOS_DATA_IMPORT_ALIAS(Mm64BitPhysicalAddress)
extern PBOOLEAN Mm64BitPhysicalAddress;
#endif
extern NTKERNELAPI PVOID MmBadPointer;

$endif (_WDMDDK_)
$if (_NTDDK_)

typedef struct _PHYSICAL_MEMORY_RANGE {
  PHYSICAL_ADDRESS BaseAddress;
  LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

typedef NTSTATUS
(NTAPI *PMM_ROTATE_COPY_CALLBACK_FUNCTION)(
  _In_ PMDL DestinationMdl,
  _In_ PMDL SourceMdl,
  _In_ PVOID Context);

typedef enum _MM_ROTATE_DIRECTION {
  MmToFrameBuffer,
  MmToFrameBufferNoCopy,
  MmToRegularMemory,
  MmToRegularMemoryNoCopy,
  MmMaximumRotateDirection
} MM_ROTATE_DIRECTION, *PMM_ROTATE_DIRECTION;

$endif (_NTDDK_)
$if (_NTIFS_)
typedef enum _MMFLUSH_TYPE {
  MmFlushForDelete,
  MmFlushForWrite
} MMFLUSH_TYPE;

typedef struct _READ_LIST {
  PFILE_OBJECT FileObject;
  ULONG NumberOfEntries;
  LOGICAL IsImage;
  FILE_SEGMENT_ELEMENT List[ANYSIZE_ARRAY];
} READ_LIST, *PREAD_LIST;

#if (NTDDI_VERSION >= NTDDI_WINXP)

typedef union _MM_PREFETCH_FLAGS {
  struct {
    ULONG Priority : SYSTEM_PAGE_PRIORITY_BITS;
    ULONG RepurposePriority : SYSTEM_PAGE_PRIORITY_BITS;
  } Flags;
  ULONG AllFlags;
} MM_PREFETCH_FLAGS, *PMM_PREFETCH_FLAGS;

#define MM_PREFETCH_FLAGS_MASK ((1 << (2*SYSTEM_PAGE_PRIORITY_BITS)) - 1)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_GENERATE_EXCEPTIONS        0x00000004
#define HEAP_ZERO_MEMORY                0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020
#define HEAP_FREE_CHECKING_ENABLED      0x00000040
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080

#define HEAP_CREATE_ALIGN_16            0x00010000
#define HEAP_CREATE_ENABLE_TRACING      0x00020000
#define HEAP_CREATE_ENABLE_EXECUTE      0x00040000

#define HEAP_SETTABLE_USER_VALUE        0x00000100
#define HEAP_SETTABLE_USER_FLAG1        0x00000200
#define HEAP_SETTABLE_USER_FLAG2        0x00000400
#define HEAP_SETTABLE_USER_FLAG3        0x00000800
#define HEAP_SETTABLE_USER_FLAGS        0x00000E00

#define HEAP_CLASS_0                    0x00000000
#define HEAP_CLASS_1                    0x00001000
#define HEAP_CLASS_2                    0x00002000
#define HEAP_CLASS_3                    0x00003000
#define HEAP_CLASS_4                    0x00004000
#define HEAP_CLASS_5                    0x00005000
#define HEAP_CLASS_6                    0x00006000
#define HEAP_CLASS_7                    0x00007000
#define HEAP_CLASS_8                    0x00008000
#define HEAP_CLASS_MASK                 0x0000F000

#define HEAP_MAXIMUM_TAG                0x0FFF
#define HEAP_GLOBAL_TAG                 0x0800
#define HEAP_PSEUDO_TAG_FLAG            0x8000
#define HEAP_TAG_SHIFT                  18
#define HEAP_TAG_MASK                  (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_CREATE_VALID_MASK         (HEAP_NO_SERIALIZE             |   \
                                        HEAP_GROWABLE                 |   \
                                        HEAP_GENERATE_EXCEPTIONS      |   \
                                        HEAP_ZERO_MEMORY              |   \
                                        HEAP_REALLOC_IN_PLACE_ONLY    |   \
                                        HEAP_TAIL_CHECKING_ENABLED    |   \
                                        HEAP_FREE_CHECKING_ENABLED    |   \
                                        HEAP_DISABLE_COALESCE_ON_FREE |   \
                                        HEAP_CLASS_MASK               |   \
                                        HEAP_CREATE_ALIGN_16          |   \
                                        HEAP_CREATE_ENABLE_TRACING    |   \
                                        HEAP_CREATE_ENABLE_EXECUTE)
$endif (_NTIFS_)

