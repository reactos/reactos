#ifndef _INCLUDE_DDK_MMFUNCS_H
#define _INCLUDE_DDK_MMFUNCS_H
/* $Id$ */
/* MEMORY MANAGMENT ******************************************************/


#ifdef __NTOSKRNL__
extern ULONG EXPORTED MmUserProbeAddress;
extern PVOID EXPORTED MmHighestUserAddress;
#else
extern ULONG IMPORTED MmUserProbeAddress;
extern PVOID IMPORTED MmHighestUserAddress;
#endif

#ifdef __NTOSKRNL__
extern POBJECT_TYPE EXPORTED MmSectionObjectType;
#else
extern POBJECT_TYPE IMPORTED MmSectionObjectType;
#endif


/*
 * FUNCTION: Returns the number of pages spanned by an address range
 * ARGUMENTS:
 *          Va = start of range
 *          Size = Size of range
 * RETURNS: The number of pages
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Size) \
       (ULONG)((PAGE_ROUND_UP((Size) + ((ULONG_PTR)(Va))) - \
                PAGE_ROUND_DOWN((ULONG_PTR)(Va))) / PAGE_SIZE)

/*
 * FUNCTION: Returns FALSE is the pointer is NULL, TRUE otherwise
 */
#define ARGUMENT_PRESENT(arg) ((arg)!=NULL)

/*
 * FUNCTION: Returns the byte offset of the address within its page
 */
#define BYTE_OFFSET(va) (((ULONG_PTR)va)%PAGE_SIZE)
#define PAGE_OFFSET(va) BYTE_OFFSET(va)


/*
 * FUNCTION: Takes a count in bytes and returns the number of pages
 * required to hold it
 */
#define BYTES_TO_PAGES(Size) \
	(((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
  
#define PAGE_ALIGN(va) ( (PVOID) (((ULONG_PTR)(va)) & (~(PAGE_SIZE-1))) )
#define PAGE_BASE(va) PAGE_ALIGN(va)

NTSTATUS
STDCALL
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTSTATUS
STDCALL
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    );


DWORD
STDCALL
MmAdjustWorkingSetSize (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

NTSTATUS
STDCALL
MmAdvanceMdl (
    IN PMDL Mdl,
    IN ULONG NumberOfBytes
    );

PVOID
STDCALL
MmAllocateContiguousMemory (
	IN	ULONG			NumberOfBytes,
	IN	PHYSICAL_ADDRESS	HighestAcceptableAddress
	);

PVOID STDCALL 
MmAllocateContiguousMemorySpecifyCache (IN ULONG NumberOfBytes,
                IN PHYSICAL_ADDRESS LowestAcceptableAddress,
			    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
			    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
			    IN MEMORY_CACHING_TYPE CacheType
			    );

PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
					  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
			          IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
			          IN MEMORY_CACHING_TYPE CacheType OPTIONAL,
					  IN ULONG Alignment);

PVOID
STDCALL
MmAllocateMappingAddress (
     IN SIZE_T NumberOfBytes,
     IN ULONG PoolTag
     );

PVOID
STDCALL
MmAllocateNonCachedMemory (
	IN	ULONG	NumberOfBytes
	);
/*
 * FUNCTION: Fills in the corresponding physical page array for a given MDL
 * for a buffer in nonpaged system space
 * ARGUMENTS:
 *        MemoryDescriptorList = MDL to fill
 */
VOID
STDCALL
MmBuildMdlForNonPagedPool (
	PMDL	MemoryDescriptorList
	);
BOOLEAN
STDCALL
MmCanFileBeTruncated (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	PLARGE_INTEGER			NewFileSize
	);
/*
 * FUNCTION: Allocates and initializes an MDL
 * ARGUMENTS:
 *        MemoryDescriptorList = Optional caller allocated MDL to initalize
 *        Base = Base virtual address for buffer
 *        Length = Length in bytes of the buffer
 * RETURNS: A pointer to the initalized MDL
 */
PMDL
STDCALL
MmCreateMdl (
	PMDL	MemoryDescriptorList,
	PVOID	Base,
	ULONG	Length
	);
#if 0
NTSTATUS
STDCALL
MmCreateSection (
	OUT	PSECTION_OBJECT		* SectionObject,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	PLARGE_INTEGER		MaximumSize,
	IN	ULONG			SectionPageProtection,
	IN	ULONG			AllocationAttributes,
	IN	HANDLE			FileHandle		OPTIONAL,
	IN	PFILE_OBJECT		File			OPTIONAL
	);
#endif
DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
BOOLEAN
STDCALL
MmDisableModifiedWriteOfSection (
	DWORD	Unknown0
	);
BOOLEAN
STDCALL
MmFlushImageSection (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	MMFLUSH_TYPE			FlushType
	);
BOOLEAN
STDCALL
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN                  DelayClose
    );
VOID
STDCALL
MmFreeContiguousMemory (
	IN OUT	PVOID	BaseAddress
	);
VOID
STDCALL
MmFreeContiguousMemorySpecifyCache(IN PVOID BaseAddress,
				IN ULONG NumberOfBytes,
			    IN MEMORY_CACHING_TYPE CacheType
			    );

VOID
STDCALL
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     );

VOID
STDCALL
MmFreeNonCachedMemory (
	IN	PVOID	BaseAddress,
	IN	ULONG	NumberOfBytes
	);
/*
 * FUNCTION: Returns the length in bytes of a buffer described by an MDL
 * ARGUMENTS:
 *         Mdl = the mdl
 * RETURNS: Size of the buffer 
 */
#define MmGetMdlByteCount(Mdl)  ((Mdl)->ByteCount)

/*
 * FUNCTION: Returns the byte offset within a page of the buffer described
 * by an MDL
 * ARGUMENTS:
 *         Mdl = the mdl
 * RETURNS: The offset in bytes
 */
#define MmGetMdlByteOffset(Mdl) \
	((Mdl)->ByteOffset)

/*
 * FUNCTION: Returns the initial virtual address for a buffer described
 * by an MDL
 * ARGUMENTS:
 *        Mdl = the mdl
 * RETURNS: The initial virtual address
 */
#define MmGetMdlVirtualAddress(Mdl) \
	((PVOID) ((PCHAR) (Mdl)->StartVa + (Mdl)->ByteOffset))

/*
 * FUNCTION: Returns the physical address corresponding to a given valid
 * virtual address
 * ARGUMENTS:
 *       BaseAddress = the virtual address
 * RETURNS: The physical address
 */
PHYSICAL_ADDRESS
STDCALL
MmGetPhysicalAddress (
	IN	PVOID	BaseAddress
	);

PPHYSICAL_MEMORY_RANGE
STDCALL
MmGetPhysicalMemoryRanges (
    VOID
    );

#define MmGetProcedureAddress(Address) (Address)

/*
 * PVOID
 * MmGetSystemAddressForMdl (
 *	PMDL Mdl
 *	);
 *
 * FUNCTION:
 *	Maps the physical pages described by an MDL into system space
 *
 * ARGUMENTS:
 *	Mdl = mdl
 *
 * RETURNS:
 *	The base system address for the mapped buffer
 */
#define MmGetSystemAddressForMdl(Mdl) \
	(((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
		((Mdl)->MappedSystemVa):(MmMapLockedPages((Mdl),KernelMode)))

/*
 * PVOID
 * MmGetSystemAddressForMdlSafe(
 *   IN PMDL  Mdl,
 *   IN MM_PAGE_PRIORITY  Priority)
 */
#define MmGetSystemAddressForMdlSafe(_Mdl, _Priority) \
  ((_Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA \
    | MDL_SOURCE_IS_NONPAGED_POOL)) ? \
    (_Mdl)->MappedSystemVa : \
    (PVOID) MmMapLockedPagesSpecifyCache((_Mdl), \
      KernelMode, MmCached, NULL, FALSE, _Priority)

PVOID
STDCALL
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    );

PVOID
STDCALL
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

NTSTATUS
STDCALL
MmGrowKernelStack (
	DWORD	Unknown0
	);


/*
 * VOID
 * MmInitializeMdl (
 *	PMDL	MemoryDescriptorList,
 *	PVOID	BaseVa,
 *	ULONG	Length
 *	);
 *
 * FUNCTION:
 *	Initalizes an MDL
 *
 * ARGUMENTS:
 *	MemoryDescriptorList = MDL to be initalized
 *	BaseVa = Base virtual address of the buffer
 *	Length = Length in bytes of the buffer
 */
#define MmInitializeMdl(MemoryDescriptorList,BaseVa,Length) \
{ \
	(MemoryDescriptorList)->Next = (PMDL)NULL; \
	(MemoryDescriptorList)->Size = (CSHORT)(sizeof(MDL) + \
		(ADDRESS_AND_SIZE_TO_SPAN_PAGES((BaseVa),(Length)) * sizeof(PFN_NUMBER))); \
	(MemoryDescriptorList)->MdlFlags = 0; \
	(MemoryDescriptorList)->StartVa = PAGE_ALIGN(BaseVa); \
	(MemoryDescriptorList)->ByteOffset = BYTE_OFFSET(BaseVa); \
	(MemoryDescriptorList)->ByteCount = (Length); \
}

/*
 * FUNCTION: Checks whether an address is valid for read/write
 * ARGUMENTS:
 *       VirtualAddress = address to be check
 * RETURNS: TRUE if an access would be valid
 */
BOOLEAN
STDCALL
MmIsAddressValid (
	IN	PVOID	VirtualAddress
	);
ULONG
STDCALL
MmIsDriverVerifying (
    IN struct _DRIVER_OBJECT *DriverObject
    );
BOOLEAN
STDCALL
MmIsNonPagedSystemAddressValid (
	IN	PVOID	VirtualAddress
	);
BOOLEAN
STDCALL
MmIsRecursiveIoFault (
	VOID
	);
/*
 * FUNCTION: Checks if the current platform is a workstation or a server
 * RETURNS: If the system is a server returns true
 * NOTE: Drivers can use this as an estimate of the likely resources
 * available
 */
BOOLEAN
STDCALL
MmIsThisAnNtAsSystem (
	VOID
	);

NTSTATUS
STDCALL
MmIsVerifierEnabled (
    OUT PULONG VerifierFlags
    );

/*
 * FUNCTION: Locks a section of the driver's code into memory
 * ARGUMENTS:
 *        AddressWithinSection = Any address in the region
 * RETURNS: A handle to the region
 */
#define MmLockPagableCodeSection(Address) \
	MmLockPagableDataSection(Address)

/*
 * FUNCTION: Locks a section of the driver's data into memory
 * ARGUMENTS:
 *        AddressWithinSection = Any address in the region
 * RETURNS: A handle to the region
 */
PVOID
STDCALL
MmLockPagableDataSection (
	PVOID	AddressWithinSection
	);
PVOID
STDCALL
MmLockPagableImageSection (
	PVOID	AddressWithinSection
	);
/*
 * FUNCTION: Locks a section of memory
 * ARGUMENTS: 
 *         ImageSectionHandle = handle returned from MmLockPagableCodeSection
 *                              or MmLockPagableDataSection
 */
VOID
STDCALL
MmLockPagableSectionByHandle (
	PVOID	ImageSectionHandle
	);
PVOID
STDCALL
MmMapIoSpace (
	PHYSICAL_ADDRESS	PhysicalAddress,
	ULONG			NumberOfBytes,
	MEMORY_CACHING_TYPE 	CacheEnable
	);
/*
 * FUNCTION: Maps the pages described by a given MDL
 * ARGUMENTS:
 *        MemoryDescriptorList = MDL updated by MmProbeAndLockPages
 *        AccessMode = Access mode in which to map the MDL
 * RETURNS: The base virtual address which maps the buffer
 */
PVOID
STDCALL
MmMapLockedPages (
	PMDL		MemoryDescriptorList,
	KPROCESSOR_MODE	AccessMode
	);
PVOID STDCALL
MmMapLockedPagesSpecifyCache ( IN PMDL Mdl,
                               IN KPROCESSOR_MODE AccessMode,
                               IN MEMORY_CACHING_TYPE CacheType,
                               IN PVOID BaseAddress,
                               IN ULONG BugCheckOnFailure,
                               IN MM_PAGE_PRIORITY Priority);   
VOID
STDCALL
MmMapMemoryDumpMdl (
	PVOID	Unknown0
	);
PVOID
STDCALL
MmMapLockedPagesWithReservedMapping (
    IN PVOID MappingAddress,
    IN ULONG PoolTag,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    );
NTSTATUS
STDCALL
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    );
PVOID
STDCALL
MmMapVideoDisplay (
	IN	PHYSICAL_ADDRESS	PhysicalAddress,
	IN	ULONG			NumberOfBytes,
	IN	MEMORY_CACHING_TYPE	CacheType
	);
NTSTATUS
STDCALL
MmMapViewInSessionSpace (
    IN	PVOID Section,
    OUT PVOID *MappedBase,
    IN	OUT PSIZE_T ViewSize
    );
NTSTATUS
STDCALL
MmMapViewInSystemSpace (
	IN	PVOID	SectionObject,
	OUT	PVOID	* MappedBase,
	IN	PULONG	ViewSize
	);

NTSTATUS
STDCALL
MmMapViewOfSection (
	IN	PVOID		SectionObject,
	IN	PEPROCESS	Process,
	IN OUT	PVOID		* BaseAddress,
	IN	ULONG		ZeroBits,
	IN	ULONG		CommitSize,
	IN OUT	PLARGE_INTEGER	SectionOffset OPTIONAL,
	IN OUT	PULONG		ViewSize,
	IN	SECTION_INHERIT	InheritDisposition,
	IN	ULONG		AllocationType,
	IN	ULONG		Protect
	);
NTSTATUS
STDCALL
MmMarkPhysicalMemoryAsBad(
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTSTATUS
STDCALL
MmMarkPhysicalMemoryAsGood(
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

/*
 * FUNCTION: Makes the whole driver pageable
 * ARGUMENTS:
 *         AddressWithinSection = Any address within the driver
 */
VOID
STDCALL
MmPageEntireDriver (
	PVOID	AddressWithinSection
	);

NTSTATUS
STDCALL
MmPrefetchPages (
    IN ULONG NumberOfLists,
    IN PREAD_LIST *ReadLists
    );

/*
 * VOID
 * MmPrepareMdlForReuse (
 *	PMDL	Mdl
 *	);
 *
 * FUNCTION:
 *	Reinitializes a caller-allocated MDL
 *
 * ARGUMENTS:
 *	Mdl = Points to the MDL that will be reused
 */
#define MmPrepareMdlForReuse(Mdl) \
	if (((Mdl)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) \
	{ \
		ASSERT(((Mdl)->MdlFlags & MDL_PARTIAL) != 0); \
		MmUnmapLockedPages ((Mdl)->MappedSystemVa, (Mdl)); \
	} \
	else if (((Mdl)->MdlFlags & MDL_PARTIAL) == 0) \
	{ \
		ASSERT(((Mdl)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0); \
	}

/*
 * FUNCTION: Probes the specified pages, makes them resident and locks
 * the physical pages mapped by the virtual address range 
 * ARGUMENTS:
 *         MemoryDescriptorList = MDL which supplies the virtual address,
 *                                byte offset and length
 *         AccessMode = Access mode with which to probe the arguments
 *         Operation = Types of operation for which the pages should be
 *                     probed
 */
VOID
STDCALL
MmProbeAndLockPages (
	PMDL		MemoryDescriptorList,
	KPROCESSOR_MODE	AccessMode,
	LOCK_OPERATION	Operation
	);

VOID
STDCALL
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

VOID 
STDCALL
MmProbeAndLockSelectedPages(
	IN OUT PMDL MemoryDescriptorList,
	IN LARGE_INTEGER PageList[],
	IN KPROCESSOR_MODE AccessMode,
	IN LOCK_OPERATION Operation
	);

NTSTATUS
STDCALL
MmProtectMdlSystemAddress (
    IN PMDL MemoryDescriptorList,
    IN ULONG NewProtect
    );


/*
 * FUNCTION: Returns an estimate of the amount of memory in the system
 * RETURNS: Either MmSmallSystem, MmMediumSystem or MmLargeSystem
 */
MM_SYSTEM_SIZE
STDCALL
MmQuerySystemSize (
	VOID
	);

NTSTATUS
STDCALL
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

/*
 * FUNCTION: Resets the pageable status of a driver's sections to their
 * compile time settings
 * ARGUMENTS:
 *         AddressWithinSection = Any address within the driver
 */
VOID
STDCALL
MmResetDriverPaging (
	PVOID	AddressWithinSection
	);

PVOID
STDCALL
MmSecureVirtualMemory (
	PVOID   Address,
	SIZE_T	Length,
	ULONG   Mode
	);
BOOLEAN
STDCALL
MmSetAddressRangeModified (
    IN PVOID    Address,
    IN ULONG    Length
	);
NTSTATUS
STDCALL
MmSetBankedSection (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	);

/*
 * FUNCTION: Returns the number of bytes to allocate for an MDL 
 * describing a given address range
 * ARGUMENTS:
 *          Base = Base virtual address for the region
 *          Length = size in bytes of the region
 * RETURNS: The number of bytes required for the MDL
 */
ULONG
STDCALL
MmSizeOfMdl (
	PVOID	Base,
	ULONG	Length
	);

ULONG
STDCALL
MmTrimAllSystemPagableMemory (
	IN ULONG PurgeTransitionList
	);

/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *          Mdl = Mdl to unlock
 */
VOID
STDCALL
MmUnlockPages (
	PMDL	Mdl
	);
/*
 * FUNCTION: Releases a section of driver code or data previously locked into 
 * memory
 * ARGUMENTS: 
 *         ImageSectionHandle = Handle for the locked section
 */
VOID
STDCALL
MmUnlockPagableImageSection (
	PVOID	ImageSectionHandle
	);
VOID
STDCALL
MmUnmapIoSpace (
	PVOID	BaseAddress,
	ULONG	NumberOfBytes
	);
VOID
STDCALL
MmUnmapLockedPages (
	PVOID	BaseAddress,
	PMDL	MemoryDescriptorList
	);
VOID
STDCALL
MmUnmapReservedMapping (
     IN PVOID BaseAddress,
     IN ULONG PoolTag,
     IN PMDL MemoryDescriptorList
     );
VOID
STDCALL
MmUnmapVideoDisplay (
	IN	PVOID	BaseAddress,
	IN	ULONG	NumberOfBytes
	);
NTSTATUS
STDCALL
MmUnmapViewInSystemSpace (
	IN	PVOID	MappedBase
	);
NTSTATUS
STDCALL
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    );
#if 0
NTSTATUS
STDCALL
MmUnmapViewOfSection (
	PEPROCESS	Process,
	PMEMORY_AREA	MemoryArea
	);
#endif
VOID
STDCALL
MmUnsecureVirtualMemory (
	PVOID	SecureMem
	);

VOID STDCALL
ProbeForRead (IN CONST VOID *Address,
	      IN ULONG Length,
	      IN ULONG Alignment);

VOID STDCALL
ProbeForWrite (IN CONST VOID *Address,
	       IN ULONG Length,
	       IN ULONG Alignment);

#endif /* _INCLUDE_DDK_MMFUNCS_H */

/* EOF */
