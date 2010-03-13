/******************************************************************************
 *                       Memory manager Functions                             *
 ******************************************************************************/

/*
 * Alignment Macros
 */
#define ALIGN_DOWN_BY(size, align) \
    ((ULONG_PTR)(size) & ~((ULONG_PTR)(align) - 1))

#define ALIGN_UP_BY(size, align) \
    (ALIGN_DOWN_BY(((ULONG_PTR)(size) + align - 1), align))

#define ALIGN_DOWN_POINTER_BY(ptr, align) \
    ((PVOID)ALIGN_DOWN_BY(ptr, align))

#define ALIGN_UP_POINTER_BY(ptr, align) \
    ((PVOID)ALIGN_UP_BY(ptr, align))

#define ALIGN_DOWN(size, type) \
    ALIGN_DOWN_BY(size, sizeof(type))

#define ALIGN_UP(size, type) \
    ALIGN_UP_BY(size, sizeof(type))

#define ALIGN_DOWN_POINTER(ptr, type) \
    ALIGN_DOWN_POINTER_BY(ptr, sizeof(type))

#define ALIGN_UP_POINTER(ptr, type) \
    ALIGN_UP_POINTER_BY(ptr, sizeof(type))

/* ULONG
 * BYTE_OFFSET(
 *   IN PVOID Va)
 */
#define BYTE_OFFSET(Va) \
  ((ULONG) ((ULONG_PTR) (Va) & (PAGE_SIZE - 1)))

/* ULONG
 * BYTES_TO_PAGES(
 *   IN ULONG Size)
 */
#define BYTES_TO_PAGES(Size) \
  (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))

/* PVOID
 * PAGE_ALIGN(
 *   IN PVOID Va)
 */
#define PAGE_ALIGN(Va) \
  ((PVOID) ((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

/* ULONG_PTR
 * ROUND_TO_PAGES(
 *   IN ULONG_PTR Size)
 */
#define ROUND_TO_PAGES(Size) \
  (((ULONG_PTR) (Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* ULONG
 * ADDRESS_AND_SIZE_TO_SPAN_PAGES(
 *   IN PVOID Va,
 *   IN ULONG Size)
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(_Va, _Size) \
  ((ULONG) ((((ULONG_PTR) (_Va) & (PAGE_SIZE - 1)) \
    + (_Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

/*
 * ULONG
 * MmGetMdlByteCount(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlByteCount(_Mdl) \
  ((_Mdl)->ByteCount)

/*
 * ULONG
 * MmGetMdlByteOffset(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlByteOffset(_Mdl) \
  ((_Mdl)->ByteOffset)

/*
 * PPFN_NUMBER
 * MmGetMdlPfnArray(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlPfnArray(_Mdl) \
  ((PPFN_NUMBER) ((_Mdl) + 1))

/*
 * PVOID
 * MmGetMdlVirtualAddress(
 *   IN PMDL  Mdl)
 */
#define MmGetMdlVirtualAddress(_Mdl) \
  ((PVOID) ((PCHAR) ((_Mdl)->StartVa) + (_Mdl)->ByteOffset))

#define MmGetProcedureAddress(Address) (Address)

/* PVOID MmGetSystemAddressForMdl(
 *     IN PMDL Mdl);
 */
#define MmGetSystemAddressForMdl(Mdl) \
  (((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | \
    MDL_SOURCE_IS_NONPAGED_POOL)) ? \
      ((Mdl)->MappedSystemVa) : \
      (MmMapLockedPages((Mdl), KernelMode)))

/* PVOID
 * MmGetSystemAddressForMdlSafe(
 *     IN PMDL Mdl,
 *     IN MM_PAGE_PRIORITY Priority)
 */
#define MmGetSystemAddressForMdlSafe(_Mdl, _Priority) \
  (((_Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA \
    | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
    (_Mdl)->MappedSystemVa : \
    (PVOID) MmMapLockedPagesSpecifyCache((_Mdl), \
      KernelMode, MmCached, NULL, FALSE, (_Priority)))

/*
 * VOID
 * MmInitializeMdl(
 *   IN PMDL  MemoryDescriptorList,
 *   IN PVOID  BaseVa,
 *   IN SIZE_T  Length)
 */
#define MmInitializeMdl(_MemoryDescriptorList, \
                        _BaseVa, \
                        _Length) \
{ \
  (_MemoryDescriptorList)->Next = (PMDL) NULL; \
  (_MemoryDescriptorList)->Size = (CSHORT) (sizeof(MDL) + \
    (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(_BaseVa, _Length))); \
  (_MemoryDescriptorList)->MdlFlags = 0; \
  (_MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN(_BaseVa); \
  (_MemoryDescriptorList)->ByteOffset = BYTE_OFFSET(_BaseVa); \
  (_MemoryDescriptorList)->ByteCount = (ULONG) _Length; \
}

/*
 * VOID
 * MmPrepareMdlForReuse(
 *   IN PMDL  Mdl)
 */
#define MmPrepareMdlForReuse(_Mdl) \
{ \
  if (((_Mdl)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) { \
    ASSERT(((_Mdl)->MdlFlags & MDL_PARTIAL) != 0); \
    MmUnmapLockedPages((_Mdl)->MappedSystemVa, (_Mdl)); \
  } else if (((_Mdl)->MdlFlags & MDL_PARTIAL) == 0) { \
    ASSERT(((_Mdl)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0); \
  } \
}

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemory(
  IN SIZE_T  NumberOfBytes,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(
  IN SIZE_T  NumberOfBytes,
  IN PHYSICAL_ADDRESS  LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS  HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS  BoundaryAddressMultiple  OPTIONAL,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdl(
  IN PHYSICAL_ADDRESS  LowAddress,
  IN PHYSICAL_ADDRESS  HighAddress,
  IN PHYSICAL_ADDRESS  SkipBytes,
  IN SIZE_T  TotalBytes);

NTKERNELAPI
VOID
NTAPI
MmBuildMdlForNonPagedPool(
  IN OUT PMDLX  MemoryDescriptorList);

//DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
PMDL
NTAPI
MmCreateMdl(
  IN PMDL  MemoryDescriptorList  OPTIONAL,
  IN PVOID  Base,
  IN SIZE_T  Length);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemory(
  IN PVOID  BaseAddress);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
VOID
NTAPI
MmFreePagesFromMdl(
  IN PMDLX MemoryDescriptorList);

NTKERNELAPI
PVOID
NTAPI
MmGetSystemRoutineAddress(
  IN PUNICODE_STRING  SystemRoutineName);

NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifying(
  IN struct _DRIVER_OBJECT *DriverObject);

NTKERNELAPI
PVOID
NTAPI
MmLockPagableDataSection(
  IN PVOID  AddressWithinSection);

NTKERNELAPI
PVOID
NTAPI
MmMapIoSpace(
  IN PHYSICAL_ADDRESS  PhysicalAddress,
  IN SIZE_T  NumberOfBytes,
  IN MEMORY_CACHING_TYPE  CacheEnable);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPages(
  IN PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesSpecifyCache(
  IN PMDLX MemoryDescriptorList,
  IN KPROCESSOR_MODE AccessMode,
  IN MEMORY_CACHING_TYPE CacheType,
  IN PVOID BaseAddress OPTIONAL,
  IN ULONG BugCheckOnFailure,
  IN MM_PAGE_PRIORITY Priority);

NTKERNELAPI
PVOID
NTAPI
MmPageEntireDriver(
  IN PVOID  AddressWithinSection);

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTKERNELAPI
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(
  VOID);

NTKERNELAPI
VOID
NTAPI
MmResetDriverPaging(
  IN PVOID  AddressWithinSection);

NTKERNELAPI
SIZE_T
NTAPI
MmSizeOfMdl(
  IN PVOID  Base,
  IN SIZE_T  Length);

NTKERNELAPI
VOID
NTAPI
MmUnlockPagableImageSection(
  IN PVOID  ImageSectionHandle);

NTKERNELAPI
VOID
NTAPI
MmUnlockPages(
  IN OUT PMDL  MemoryDescriptorList);

NTKERNELAPI
VOID
NTAPI
MmUnmapIoSpace(
  IN PVOID  BaseAddress,
  IN SIZE_T  NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockProcessPages(
  IN OUT PMDL  MemoryDescriptorList,
  IN PEPROCESS  Process,
  IN KPROCESSOR_MODE  AccessMode,
  IN LOCK_OPERATION  Operation);

NTKERNELAPI
VOID
NTAPI
MmUnmapLockedPages(
  IN PVOID  BaseAddress,
  IN PMDL  MemoryDescriptorList);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
MmAdvanceMdl(
  IN OUT PMDL  Mdl,
  IN ULONG  NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmAllocateMappingAddress(
  IN SIZE_T  NumberOfBytes,
  IN ULONG  PoolTag);

NTKERNELAPI
VOID
NTAPI
MmFreeMappingAddress(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag);

NTKERNELAPI
NTSTATUS
NTAPI
MmIsVerifierEnabled(
  OUT PULONG  VerifierFlags);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(
  IN PVOID  MappingAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList,
  IN MEMORY_CACHING_TYPE  CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(
  IN PMDL  MemoryDescriptorList,
  IN ULONG  NewProtect);

NTKERNELAPI
VOID
NTAPI
MmUnmapReservedMapping(
  IN PVOID  BaseAddress,
  IN ULONG  PoolTag,
  IN PMDL  MemoryDescriptorList);

#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)
NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdlEx(
  IN PHYSICAL_ADDRESS LowAddress,
  IN PHYSICAL_ADDRESS HighAddress,
  IN PHYSICAL_ADDRESS SkipBytes,
  IN SIZE_T TotalBytes,
  IN MEMORY_CACHING_TYPE CacheType,
  IN ULONG Flags);
#endif

