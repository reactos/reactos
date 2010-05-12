/******************************************************************************
 *                       Memory manager Functions                             *
 ******************************************************************************/
$if (_WDMDDK_)
/* Alignment Macros */
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

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field) ((ULONG)&(((type *)0)->field))
#endif

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

#define POOL_TAGGING                             1

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif /* DBG */

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
  (((Size) + PAGE_SIZE - 1) >> PAGE_SHIFT)

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

#define COMPUTE_PAGES_SPANNED(Va, Size) \
    ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size)

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

#define MmGetMdlBaseVa(Mdl) ((Mdl)->StartVa)

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
$endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_NTDDK_)
NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(VOID);

NTKERNELAPI
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(
  IN PVOID BaseAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmGetVirtualForPhysical(
  IN PHYSICAL_ADDRESS PhysicalAddress);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapUserAddressesToPage(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes,
  IN PVOID PageAddress);

NTKERNELAPI
PVOID
NTAPI
MmMapVideoDisplay(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSessionSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSystemSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID);

NTKERNELAPI
VOID
NTAPI
MmLockPagableSectionByHandle(
  IN PVOID ImageSectionHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(
  IN PVOID MappedBase);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(
  IN PVOID MappedBase);

NTKERNELAPI
VOID
NTAPI
MmUnsecureVirtualMemory(
  IN HANDLE SecureHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS StartAddress,
  IN OUT PLARGE_INTEGER NumberOfBytes);

NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  IN PVOID Address,
  IN SIZE_T Size,
  IN ULONG ProbeMode);

NTKERNELAPI
VOID
NTAPI
MmUnmapVideoDisplay(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
NTSTATUS
NTAPI
MmAddPhysicalMemory(
  IN PPHYSICAL_ADDRESS StartAddress,
  IN OUT PLARGE_INTEGER NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemory(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCacheNode(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
  IN MEMORY_CACHING_TYPE CacheType,
  IN NODE_REQUIREMENT PreferredNode);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemory(
  IN PVOID BaseAddress);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheType);
$endif

$if (_WDMDDK_)
NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemory(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdl(
  IN PHYSICAL_ADDRESS LowAddress,
  IN PHYSICAL_ADDRESS HighAddress,
  IN PHYSICAL_ADDRESS SkipBytes,
  IN SIZE_T TotalBytes);

NTKERNELAPI
VOID
NTAPI
MmBuildMdlForNonPagedPool(
  IN OUT PMDLX MemoryDescriptorList);

//DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
PMDL
NTAPI
MmCreateMdl(
  IN PMDL MemoryDescriptorList OPTIONAL,
  IN PVOID Base,
  IN SIZE_T Length);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemory(
  IN PVOID BaseAddress);

NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
VOID
NTAPI
MmFreePagesFromMdl(
  IN PMDLX MemoryDescriptorList);

NTKERNELAPI
PVOID
NTAPI
MmGetSystemRoutineAddress(
  IN PUNICODE_STRING SystemRoutineName);

NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifying(
  IN struct _DRIVER_OBJECT *DriverObject);

NTKERNELAPI
PVOID
NTAPI
MmLockPagableDataSection(
  IN PVOID AddressWithinSection);

NTKERNELAPI
PVOID
NTAPI
MmMapIoSpace(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheEnable);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPages(
  IN PMDL MemoryDescriptorList,
  IN KPROCESSOR_MODE AccessMode);

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
  IN PVOID AddressWithinSection);

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockPages(
  IN OUT PMDL MemoryDescriptorList,
  IN KPROCESSOR_MODE AccessMode,
  IN LOCK_OPERATION Operation);

NTKERNELAPI
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID);

NTKERNELAPI
VOID
NTAPI
MmResetDriverPaging(
  IN PVOID AddressWithinSection);

NTKERNELAPI
SIZE_T
NTAPI
MmSizeOfMdl(
  IN PVOID Base,
  IN SIZE_T Length);

NTKERNELAPI
VOID
NTAPI
MmUnlockPagableImageSection(
  IN PVOID ImageSectionHandle);

NTKERNELAPI
VOID
NTAPI
MmUnlockPages(
  IN OUT PMDL MemoryDescriptorList);

NTKERNELAPI
VOID
NTAPI
MmUnmapIoSpace(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmProbeAndLockProcessPages(
  IN OUT PMDL MemoryDescriptorList,
  IN PEPROCESS Process,
  IN KPROCESSOR_MODE AccessMode,
  IN LOCK_OPERATION Operation);

NTKERNELAPI
VOID
NTAPI
MmUnmapLockedPages(
  IN PVOID BaseAddress,
  IN PMDL MemoryDescriptorList);

NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCacheNode(
  IN SIZE_T NumberOfBytes,
  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
  IN PHYSICAL_ADDRESS HighestAcceptableAddress,
  IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
  IN MEMORY_CACHING_TYPE CacheType,
  IN NODE_REQUIREMENT PreferredNode);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
MmAdvanceMdl(
  IN OUT PMDL Mdl,
  IN ULONG NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmAllocateMappingAddress(
  IN SIZE_T NumberOfBytes,
  IN ULONG PoolTag);

NTKERNELAPI
VOID
NTAPI
MmFreeMappingAddress(
  IN PVOID BaseAddress,
  IN ULONG PoolTag);

NTKERNELAPI
NTSTATUS
NTAPI
MmIsVerifierEnabled(
  OUT PULONG VerifierFlags);

NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(
  IN PVOID MappingAddress,
  IN ULONG PoolTag,
  IN PMDL MemoryDescriptorList,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(
  IN PMDL MemoryDescriptorList,
  IN ULONG NewProtect);

NTKERNELAPI
VOID
NTAPI
MmUnmapReservedMapping(
  IN PVOID BaseAddress,
  IN ULONG PoolTag,
  IN PMDL MemoryDescriptorList);

NTKERNELAPI
NTSTATUS
NTAPI
MmAddVerifierThunks(
  IN PVOID ThunkBuffer,
  IN ULONG ThunkBufferSize);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
$if (_NTDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
MmCreateMirror(VOID);
$endif
$if (_WDMDDK_)
NTKERNELAPI
LOGICAL
NTAPI
MmIsIoSpaceActive(
  IN PHYSICAL_ADDRESS StartAddress,
  IN SIZE_T NumberOfBytes);
$endif
#endif

$if (_WDMDDK_)
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
$endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_NTDDK_)
NTSTATUS
NTAPI
MmRotatePhysicalView(
  IN PVOID VirtualAddress,
  IN OUT PSIZE_T NumberOfBytes,
  IN PMDLX NewMdl OPTIONAL,
  IN MM_ROTATE_DIRECTION Direction,
  IN PMM_ROTATE_COPY_CALLBACK_FUNCTION CopyFunction,
  IN PVOID Context OPTIONAL);
$endif

$if (_WDMDDK_)
NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifyingByAddress(
  IN PVOID AddressWithinSection);
$endif
#endif

