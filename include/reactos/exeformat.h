#ifndef REACTOS_EXEFORMAT_H_INCLUDED_
#define REACTOS_EXEFORMAT_H_INCLUDED_ 1

/*
 * LOADER API
 */
/* OUT flags returned by a loader */
#define EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED       (1 << 0)
#define EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP   (1 << 1)
#define EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED (1 << 2)

#define EXEFMT_LOAD_ASSUME_SEGMENTS_OK \
 ( \
  EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED | \
  EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP | \
  EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED \
 )

/*
 Minumum size of the buffer passed to each loader for identification of the
 executable
*/
#define EXEFMT_LOAD_HEADER_SIZE (0x2000)

/* Special values for the base address of images */
/*
 Base address can't be represented in an ULONG_PTR: any effective load address
 will require relocation
*/
#define EXEFMT_LOAD_BASE_NONE ((ULONG_PTR)-1)

/* Base address never matters, relocation never required */
#define EXEFMT_LOAD_BASE_ANY  ((ULONG_PTR)-2)

typedef NTSTATUS (NTAPI * PEXEFMT_CB_READ_FILE)
(
 IN PVOID File,
 IN PLARGE_INTEGER Offset,
 IN ULONG Length,
 OUT PVOID * Data,
 OUT PVOID * AllocBase,
 OUT PULONG ReadSize
);

typedef PMM_SECTION_SEGMENT (NTAPI * PEXEFMT_CB_ALLOCATE_SEGMENTS)
(
 IN ULONG NrSegments
);

typedef NTSTATUS (NTAPI * PEXEFMT_LOADER)
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

/*
 * STATUS CONSTANTS
 */

#define FACILITY_ROS_EXEFMT        (0x10)

/*
 * Returned by ExeFormat loaders to tell the caller the format isn't supported,
 * as opposed to STATUS_INVALID_IMAGE_FORMAT meaning the format is supported,
 * but the particular file is malformed
 */
#define STATUS_ROS_EXEFMT_UNKNOWN_FORMAT ((NTSTATUS)0xA0100001)

/*
 * Returned by MmCreateSection to signal successful loading of an executable
 * image, saving the caller the effort of determining the executable's format
 * again. The full status to return is obtained by performing a bitwise OR of
 * STATUS_ROS_EXEFMT_LOADED_FORMAT and the appropriate EXEFMT_LOADED_XXX
 */
#define FACILITY_ROS_EXEFMT_FORMAT      (0x11)
#define STATUS_ROS_EXEFMT_LOADED_FORMAT ((NTSTATUS)0x60110000)

/* non-standard format, ZwQuerySection required to retrieve the format tag */
#define EXEFMT_LOADED_EXTENDED (0x0000FFFF)

/* Windows PE32/PE32+ */
#define EXEFMT_LOADED_PE32     (0x00000000)
#define EXEFMT_LOADED_PE64     (0x00000001)

/* Wine ELF */
#define EXEFMT_LOADED_WINE32   (0x00000002)
#define EXEFMT_LOADED_WINE64   (0x00000003)

/* regular ELF */
#define EXEFMT_LOADED_ELF32    (0x00000004)
#define EXEFMT_LOADED_ELF64    (0x00000005)

/* TODO: Intsafe should be made into a library, as it's generally useful */
static __inline BOOLEAN Intsafe_CanAddULongPtr(IN ULONG_PTR Addend1, IN ULONG_PTR Addend2)
{
    return Addend1 <= (MAXULONG_PTR - Addend2);
}

static __inline BOOLEAN Intsafe_CanAddLong64(IN LONG64 Addend1, IN LONG64 Addend2)
{
    return Addend1 <= (MAXLONGLONG - Addend2);
}

static __inline BOOLEAN Intsafe_CanAddULong32(IN ULONG Addend1, IN ULONG Addend2)
{
    return Addend1 <= (MAXULONG - Addend2);
}

static __inline BOOLEAN Intsafe_AddULong32(OUT PULONG Result, IN ULONG Addend1, IN ULONG Addend2)
{
    if(!Intsafe_CanAddULong32(Addend1, Addend2))
	return FALSE;

    *Result = Addend1 + Addend2;
    return TRUE;
}

static __inline BOOLEAN Intsafe_CanMulULong32(IN ULONG Factor1, IN ULONG Factor2)
{
    return Factor1 <= (MAXULONG / Factor2);
}

static __inline BOOLEAN Intsafe_CanOffsetPointer(IN CONST VOID * Pointer, IN SIZE_T Offset)
{
    /* FIXME: (PVOID)MAXULONG_PTR isn't necessarily a valid address */
    return Intsafe_CanAddULongPtr((ULONG_PTR)Pointer, Offset);
}

static __inline BOOLEAN IsPowerOf2(IN ULONG Number)
{
    if(Number == 0)
		return FALSE;
    return (Number & (Number - 1)) == 0;
}

static __inline ULONG ModPow2(IN ULONG Address, IN ULONG Alignment)
{
    ASSERT(IsPowerOf2(Alignment));
    return Address & (Alignment - 1);
}

static __inline BOOLEAN IsAligned(IN ULONG Address, IN ULONG Alignment)
{
    return ModPow2(Address, Alignment) == 0;
}

static __inline BOOLEAN AlignUp(OUT PULONG AlignedAddress, IN ULONG Address, IN ULONG Alignment)
{
    ULONG nExcess = ModPow2(Address, Alignment);

    if(nExcess == 0)
    {
	*AlignedAddress = Address;
	return nExcess == 0;
    }
    else
	return Intsafe_AddULong32(AlignedAddress, Address, Alignment - nExcess);
}

#define PEFMT_FIELDS_EQUAL(TYPE1_, TYPE2_, FIELD_) \
 ( \
  (FIELD_OFFSET(TYPE1_, FIELD_) == FIELD_OFFSET(TYPE2_, FIELD_)) && \
  (RTL_FIELD_SIZE(TYPE1_, FIELD_) == RTL_FIELD_SIZE(TYPE2_, FIELD_)) \
 )
#endif

/* EOF */
