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
 *     _In_ PVOID Va)
 */
#define BYTE_OFFSET(Va) \
  ((ULONG) ((ULONG_PTR) (Va) & (PAGE_SIZE - 1)))

/* ULONG
 * BYTES_TO_PAGES(
 *     _In_ ULONG Size)
 *
 * Note: This needs to be like this to avoid overflows!
 */
#define BYTES_TO_PAGES(Size) \
  (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))

/* PVOID
 * PAGE_ALIGN(
 *     _In_ PVOID Va)
 */
#define PAGE_ALIGN(Va) \
  ((PVOID) ((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

/* ULONG_PTR
 * ROUND_TO_PAGES(
 *     _In_ ULONG_PTR Size)
 */
#define ROUND_TO_PAGES(Size) \
  (((ULONG_PTR) (Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* ULONG
 * ADDRESS_AND_SIZE_TO_SPAN_PAGES(
 *     _In_ PVOID Va,
 *     _In_ ULONG Size)
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(_Va, _Size) \
  ((ULONG) ((((ULONG_PTR) (_Va) & (PAGE_SIZE - 1)) \
    + (_Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

#define COMPUTE_PAGES_SPANNED(Va, Size) \
    ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size)

/*
 * ULONG
 * MmGetMdlByteCount(
 *     _In_ PMDL  Mdl)
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
 *     _In_ PMDL  Mdl)
 */
#define MmGetMdlPfnArray(_Mdl) \
  ((PPFN_NUMBER) ((_Mdl) + 1))

/*
 * PVOID
 * MmGetMdlVirtualAddress(
 *     _In_ PMDL  Mdl)
 */
#define MmGetMdlVirtualAddress(_Mdl) \
  ((PVOID) ((PCHAR) ((_Mdl)->StartVa) + (_Mdl)->ByteOffset))

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

/* PVOID MmGetSystemAddressForMdl(
 *     _In_ PMDL Mdl);
 */
#define MmGetSystemAddressForMdl(Mdl) \
  (((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | \
    MDL_SOURCE_IS_NONPAGED_POOL)) ? \
      ((Mdl)->MappedSystemVa) : \
      (MmMapLockedPages((Mdl), KernelMode)))

/* PVOID
 * MmGetSystemAddressForMdlSafe(
 *     _In_ PMDL Mdl,
 *     _In_ MM_PAGE_PRIORITY Priority)
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
 *     _In_ PMDL MemoryDescriptorList,
 *     _In_ PVOID BaseVa,
 *     _In_ SIZE_T Length)
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
 *     _In_ PMDL Mdl)
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
$endif (_WDMDDK_)
$if (_NTIFS_)

FORCEINLINE
ULONG
HEAP_MAKE_TAG_FLAGS(
  _In_ ULONG TagBase,
  _In_ ULONG Tag)
{
  //__assume_bound(TagBase); // FIXME
  return ((ULONG)((TagBase) + ((Tag) << HEAP_TAG_SHIFT)));
}
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes))
NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemory(
  _In_ SIZE_T NumberOfBytes,
  _In_ PHYSICAL_ADDRESS HighestAcceptableAddress);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes))
NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(
  _In_ SIZE_T NumberOfBytes,
  _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
  _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
  _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
  _In_ MEMORY_CACHING_TYPE CacheType);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdl(
  _In_ PHYSICAL_ADDRESS LowAddress,
  _In_ PHYSICAL_ADDRESS HighAddress,
  _In_ PHYSICAL_ADDRESS SkipBytes,
  _In_ SIZE_T TotalBytes);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmBuildMdlForNonPagedPool(
  _Inout_ PMDL MemoryDescriptorList);

//DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
PMDL
NTAPI
MmCreateMdl(
  _Out_writes_bytes_opt_ (sizeof (MDL) + (sizeof (PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES (Base, Length)))
    PMDL MemoryDescriptorList,
  _In_reads_bytes_opt_ (Length) PVOID Base,
  _In_ SIZE_T Length);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemory(
  _In_ PVOID BaseAddress);

_IRQL_requires_max_ (DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(
  _In_reads_bytes_ (NumberOfBytes) PVOID BaseAddress,
  _In_ SIZE_T NumberOfBytes,
  _In_ MEMORY_CACHING_TYPE CacheType);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmFreePagesFromMdl(
  _Inout_ PMDL MemoryDescriptorList);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PVOID
NTAPI
MmGetSystemRoutineAddress(
  _In_ PUNICODE_STRING SystemRoutineName);

NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifying(
  _In_ struct _DRIVER_OBJECT *DriverObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
MmLockPagableDataSection(
  _In_ PVOID AddressWithinSection);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Out_writes_bytes_opt_ (NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
MmMapIoSpace(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ SIZE_T NumberOfBytes,
  _In_ MEMORY_CACHING_TYPE CacheType);

_Must_inspect_result_
_When_(AccessMode==KernelMode, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(AccessMode==UserMode, _Maybe_raises_SEH_exception_ _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
PVOID
NTAPI
MmMapLockedPages(
  _Inout_ PMDL MemoryDescriptorList,
  _In_ __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst)
    KPROCESSOR_MODE AccessMode);

_Post_writable_byte_size_(MemoryDescriptorList->ByteCount)
_When_(AccessMode==KernelMode, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(AccessMode==UserMode, _Maybe_raises_SEH_exception_ _IRQL_requires_max_(APC_LEVEL) _Post_notnull_)
_At_(MemoryDescriptorList->MappedSystemVa, _Post_writable_byte_size_(MemoryDescriptorList->ByteCount))
_Must_inspect_result_
_Success_(return != NULL)
NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesSpecifyCache(
  _Inout_ PMDL MemoryDescriptorList,
  _In_ __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst) KPROCESSOR_MODE AccessMode,
  _In_ __drv_strictTypeMatch(__drv_typeCond) MEMORY_CACHING_TYPE CacheType,
  _In_opt_ PVOID RequestedAddress,
  _In_ ULONG BugCheckOnFailure,
  _In_ ULONG Priority);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
MmPageEntireDriver(
  _In_ PVOID AddressWithinSection);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(MemoryDescriptorList->StartVa + MemoryDescriptorList->ByteOffset,
  _Field_size_bytes_opt_(MemoryDescriptorList->ByteCount))
NTKERNELAPI
VOID
NTAPI
MmProbeAndLockPages(
  _Inout_ PMDL MemoryDescriptorList,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ LOCK_OPERATION Operation);

NTKERNELAPI
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmResetDriverPaging(
  _In_ PVOID AddressWithinSection);

NTKERNELAPI
SIZE_T
NTAPI
MmSizeOfMdl(
  _In_reads_bytes_opt_ (Length) PVOID Base,
  _In_ SIZE_T Length);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnlockPagableImageSection(
  _In_ PVOID ImageSectionHandle);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnlockPages(
  _Inout_ PMDL MemoryDescriptorList);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnmapIoSpace(
  _In_reads_bytes_ (NumberOfBytes) PVOID BaseAddress,
  _In_ SIZE_T NumberOfBytes);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmProbeAndLockProcessPages(
  _Inout_ PMDL MemoryDescriptorList,
  _In_ PEPROCESS Process,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ LOCK_OPERATION Operation);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnmapLockedPages(
  _In_ PVOID BaseAddress,
  _Inout_ PMDL MemoryDescriptorList);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes))
NTKERNELAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCacheNode(
  _In_ SIZE_T NumberOfBytes,
  _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
  _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
  _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
  _In_ MEMORY_CACHING_TYPE CacheType,
  _In_ NODE_REQUIREMENT PreferredNode);
$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(VOID);

NTKERNELAPI
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(
  _In_ PVOID BaseAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  _In_ PVOID VirtualAddress);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_Out_writes_bytes_opt_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  _In_ SIZE_T NumberOfBytes);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  _In_reads_bytes_(NumberOfBytes) PVOID BaseAddress,
  _In_ SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmGetVirtualForPhysical(
  _In_ PHYSICAL_ADDRESS PhysicalAddress);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmMapUserAddressesToPage(
  _In_reads_bytes_(NumberOfBytes) PVOID BaseAddress,
  _In_ SIZE_T NumberOfBytes,
  _In_ PVOID PageAddress);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_Out_writes_bytes_opt_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
MmMapVideoDisplay(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ SIZE_T NumberOfBytes,
  _In_ MEMORY_CACHING_TYPE CacheType);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSessionSpace(
  _In_ PVOID Section,
  _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
  _Inout_ PSIZE_T ViewSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSystemSpace(
  _In_ PVOID Section,
  _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
  _Inout_ PSIZE_T ViewSize);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  _In_ PVOID VirtualAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmLockPagableSectionByHandle(
  _In_ PVOID ImageSectionHandle);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(
  _In_ PVOID MappedBase);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(
  _In_ PVOID MappedBase);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnsecureVirtualMemory(
  _In_ HANDLE SecureHandle);

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  _In_ PPHYSICAL_ADDRESS StartAddress,
  _Inout_ PLARGE_INTEGER NumberOfBytes);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  __in_data_source(USER_MODE) _In_reads_bytes_ (Size) PVOID Address,
  _In_ __in_data_source(USER_MODE) SIZE_T Size,
  _In_ ULONG ProbeMode);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnmapVideoDisplay(
  _In_reads_bytes_(NumberOfBytes) PVOID BaseAddress,
  _In_ SIZE_T NumberOfBytes);

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmAddPhysicalMemory(
  _In_ PPHYSICAL_ADDRESS StartAddress,
  _Inout_ PLARGE_INTEGER NumberOfBytes);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
BOOLEAN
NTAPI
MmIsRecursiveIoFault(VOID);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmForceSectionClosed(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ BOOLEAN DelayClose);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmFlushImageSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ MMFLUSH_TYPE FlushType);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmCanFileBeTruncated(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER NewFileSize);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmSetAddressRangeModified(
  _In_reads_bytes_ (Length) PVOID Address,
  _In_ SIZE_T Length);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
$endif (_WDMDDK_ || _NTIFS_)

$if (_WDMDDK_)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmAdvanceMdl(
  _Inout_ PMDL Mdl,
  _In_ ULONG NumberOfBytes);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_When_ (return != NULL, _Out_writes_bytes_opt_ (NumberOfBytes))
NTKERNELAPI
PVOID
NTAPI
MmAllocateMappingAddress(
  _In_ SIZE_T NumberOfBytes,
  _In_ ULONG PoolTag);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmFreeMappingAddress(
  _In_ PVOID BaseAddress,
  _In_ ULONG PoolTag);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmIsVerifierEnabled(
  _Out_ PULONG VerifierFlags);

_Post_writable_byte_size_(MemoryDescriptorList->ByteCount)
_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(MemoryDescriptorList->MappedSystemVa + MemoryDescriptorList->ByteOffset,
  _Post_writable_byte_size_(MemoryDescriptorList->ByteCount))
_Must_inspect_result_
_Success_(return != NULL)
NTKERNELAPI
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(
  _In_ PVOID MappingAddress,
  _In_ ULONG PoolTag,
  _Inout_ PMDL MemoryDescriptorList,
  _In_ __drv_strictTypeMatch(__drv_typeCond)
    MEMORY_CACHING_TYPE CacheType);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(
  _In_ PMDL MemoryDescriptorList,
  _In_ ULONG NewProtect);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
MmUnmapReservedMapping(
  _In_ PVOID BaseAddress,
  _In_ ULONG PoolTag,
  _Inout_ PMDL MemoryDescriptorList);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmAddVerifierThunks(
  _In_reads_bytes_ (ThunkBufferSize) PVOID ThunkBuffer,
  _In_ ULONG ThunkBufferSize);
$endif (_WDMDDK_)
$if (_NTIFS_)

_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmPrefetchPages(
  _In_ ULONG NumberOfLists,
  _In_reads_ (NumberOfLists) PREAD_LIST *ReadLists);
$endif (_NTIFS_)

$if (_WDMDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
$endif (_WDMDDK_ || _NTIFS_)
$if (_WDMDDK_ || _NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03)
$endif (_WDMDDK_ || _NTDDK_)
$if (_WDMDDK_)
_IRQL_requires_max_ (DISPATCH_LEVEL)
NTKERNELAPI
LOGICAL
NTAPI
MmIsIoSpaceActive(
  _In_ PHYSICAL_ADDRESS StartAddress,
  _In_ SIZE_T NumberOfBytes);
$endif (_WDMDDK_)

$if (_NTDDK_)
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmCreateMirror(VOID);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WS03) */
$endif (_WDMDDK_ || _NTDDK_)
$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)
_Must_inspect_result_
_IRQL_requires_max_ (DISPATCH_LEVEL)
NTKERNELAPI
PMDL
NTAPI
MmAllocatePagesForMdlEx(
  _In_ PHYSICAL_ADDRESS LowAddress,
  _In_ PHYSICAL_ADDRESS HighAddress,
  _In_ PHYSICAL_ADDRESS SkipBytes,
  _In_ SIZE_T TotalBytes,
  _In_ MEMORY_CACHING_TYPE CacheType,
  _In_ ULONG Flags);
#endif
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_WDMDDK_)
_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
LOGICAL
NTAPI
MmIsDriverVerifyingByAddress(
  _In_ PVOID AddressWithinSection);
$endif (_WDMDDK_)
$if (_NTDDK_)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
MmRotatePhysicalView(
  _In_ PVOID VirtualAddress,
  _Inout_ PSIZE_T NumberOfBytes,
  _In_opt_ PMDLX NewMdl,
  _In_ MM_ROTATE_DIRECTION Direction,
  _In_ PMM_ROTATE_COPY_CALLBACK_FUNCTION CopyFunction,
  _In_opt_ PVOID Context);
$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
MmDoesFileHaveUserWritableReferences(
  _In_ PSECTION_OBJECT_POINTERS SectionPointer);

_Must_inspect_result_
_At_(*BaseAddress, __drv_allocatesMem(Mem))
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _Outptr_result_buffer_(*RegionSize) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

__kernel_entry
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType);

$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

