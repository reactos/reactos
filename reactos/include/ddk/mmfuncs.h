/* MEMORY MANAGMENT ******************************************************/

/*
 * FUNCTION: Determines if the given virtual address is page aligned
 */
#define IS_PAGE_ALIGNED(Va) (((ULONG)Va)&0xfff)

/*
 * PURPOSE: Returns the byte offset of a field within a structure
 */
#define FIELD_OFFSET(Type,Field) (LONG)(&(((Type *)(0))->Field))

/*
 * PURPOSE: Returns the base address structure if the caller knows the 
 * address of a field within the structure
 * ARGUMENTS:
 *          Address = address of the field
 *          Type = Type of the whole structure
 *          Field = Name of the field whose address is none
 */
#define CONTAINING_RECORD(Address,Type,Field) (Type *)(((LONG)Address) - FIELD_OFFSET(Type,Field))

/*
 * FUNCTION: Returns the number of pages spanned by an address range
 * ARGUMENTS:
 *          Va = start of range
 *          Size = Size of range
 * RETURNS: The number of pages
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Size)

/*
 * FUNCTION: Returns FALSE is the pointer is NULL, TRUE otherwise
 */
#define ARGUMENT_PRESENT(arg) ((arg)!=NULL)

/*
 * FUNCTION: Returns the byte offset of the address within its page
 */
#define BYTE_OFFSET(va) (((ULONG)va)%PAGESIZE)

/*
 * FUNCTION: Takes a count in bytes and returns the number of pages
 * required to hold it
 */
#define BYTES_TO_PAGES(size) (?)

/*
 * FUNCTION: Allocates a range of physically contiguous cache aligned 
 * memory from the non-paged pool
 * ARGUMENTS:
 *          NumberOfBytes = Size of the memory block to allocate
 *          HighestAcceptableAddress = Highest address valid for the caller
 * RETURNS: The virtual address of the memory block on success
 *          NULL on error
 */
PVOID MmAllocateContiguousMemory(ULONG NumberOfBytes,
				 PHYSICAL_ADDRESS HighestAcceptableAddress);

/*
 * FUNCTION: Allocates a virtual address range of noncached and cache
 * aligned memory
 * ARGUMENTS:
 *         NumberOfBytes = Size of region to allocate
 * RETURNS: The base address of the range on success
 *          NULL on failure
 */
PVOID MmAllocateNonCachedMemory(ULONG NumberOfBytes);

/*
 * FUNCTION: Fills in the corresponding physical page array for a given MDL
 * for a buffer in nonpaged system space
 * ARGUMENTS:
 *        MemoryDescriptorList = MDL to fill
 */
VOID MmBuildMdlForNonPagedPool(PMDL MemoryDescriptorList);

/*
 * FUNCTION: Allocates and initializes an MDL
 * ARGUMENTS:
 *        MemoryDescriptorList = Optional caller allocated MDL to initalize
 *        Base = Base virtual address for buffer
 *        Length = Length in bytes of the buffer
 * RETURNS: A pointer to the initalized MDL
 */
PMDL MmCreateMdl(PMDL MemoryDescriptorList, PVOID Base, ULONG Length);

/*
 * FUNCTION: Releases a range of physically contiguous memory allocated
 * with MmAllocateContiguousMemory
 * ARGUMENTS:
 *         BaseAddress = Vritual address of the memory to be freed
 */
VOID MmFreeContiguousMemory(PVOID BaseAddress);

/*
 * FUNCTION: Releases a range of noncached memory allocated with 
 * MmAllocateNonCachedMemory
 * ARGUMENTS:
 *         BaseAddress = Virtual address to be freed
 *         NumberOfBytes = size of the region to be freed
 */
VOID MmFreeNonCachedMemory(PVOID BaseAddress, ULONG NumberOfBytes);

/*
 * FUNCTION: Returns the length in bytes of a buffer described by an MDL
 * ARGUMENTS:
 *         Mdl = the mdl
 * RETURNS: Size of the buffer 
 */
ULONG MmGetMdlByteCount(PMDL Mdl);

/*
 * FUNCTION: Returns the byte offset within a page of the buffer described
 * by an MDL
 * ARGUMENTS:
 *         Mdl = the mdl
 * RETURNS: The offset in bytes
 */
ULONG MmGetMdlByteOffset(PMDL Mdl);

/*
 * FUNCTION: Returns the initial virtual address for a buffer described
 * by an MDL
 * ARGUMENTS:
 *        Mdl = the mdl
 * RETURNS: The initial virtual address
 */
PVOID MmGetMdlVirtualAddress(PMDL Mdl);

/*
 * FUNCTION: Returns the physical address corresponding to a given valid
 * virtual address
 * ARGUMENTS:
 *       BaseAddress = the virtual address
 * RETURNS: The physical address
 */
PHYSICAL_ADDRESS MmGetPhysicalAddress(PVOID BaseAddress);

/*
 * FUNCTION: Maps the physical pages described by an MDL into system space
 * ARGUMENTS:
 *      Mdl = mdl
 * RETURNS: The base system address for the mapped buffer
 */
PVOID MmGetSystemAddressForMdl(PMDL Mdl);

/*
 * FUNCTION: Initalizes an mdl
 * ARGUMENTS: 
 *       MemoryDescriptorList = MDL to be initalized
 *       BaseVa = Base virtual address of the buffer 
 *       Length = Length in bytes of the buffer
 */
VOID MmInitalizeMdl(PMDL MemoryDescriptorList, PVOID BaseVa, ULONG Length);

/*
 * FUNCTION: Checks whether an address is valid for read/write
 * ARGUMENTS:
 *       VirtualAddress = address to be check
 * RETURNS: TRUE if an access would be valid
 */
BOOLEAN MmIsAddressValid(PVOID VirtualAddress);

/*
 * FUNCTION: Checks if the current platform is a workstation or a server
 * RETURNS: If the system is a server returns true
 * NOTE: Drivers can use this as an estimate of the likely resources
 * available
 */
BOOLEAN MmIsThisAnAsSystem(VOID);
   
/*
 * FUNCTION: Locks a section of the driver's code into memory
 * ARGUMENTS:
 *        AddressWithinSection = Any address in the region
 * RETURNS: A handle to the region
 */
PVOID MmLockPagableCodeSection(PVOID AddressWithinSection);

/*
 * FUNCTION: Locks a section of the driver's data into memory
 * ARGUMENTS:
 *        AddressWithinSection = Any address in the region
 * RETURNS: A handle to the region
 */
PVOID MmLockPagableDataSection(PVOID AddressWithinSection);

/*
 * FUNCTION: Locks a section of memory
 * ARGUMENTS: 
 *         ImageSectionHandle = handle returned from MmLockPagableCodeSection
 *                              or MmLockPagableDataSection
 */
VOID MmLockPagableSectionByHandle(PVOID ImageSectionHandle);
   
/*
 * FUNCTION: Maps a physical memory range into system space
 * ARGUMENTS:
 *         PhysicalAddress = First physical address to map
 *         NumberOfBytes = Number of bytes to map
 *         CacheEnable = TRUE if the range can be cached
 * RETURNS: The base virtual address which maps the region
 */
PVOID MmMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress, ULONG NumberOfBytes,
		   BOOLEAN CacheEnable);

/*
 * FUNCTION: Maps the pages described by a given MDL
 * ARGUMENTS:
 *        MemoryDescriptorList = MDL updated by MmProbeAndLockPages
 *        AccessMode = Access mode in which to map the MDL
 * RETURNS: The base virtual address which maps the buffer
 */
PVOID MmMapLockedPages(PMDL MemoryDescriptorList, KPROCESSOR_MODE AccessMode);

/*
 * FUNCTION: Makes the whole driver pageable
 * ARGUMENTS:
 *         AddressWithinSection = Any address within the driver
 */
VOID MmPageEntireDriver(PVOID AddressWithinSection);
   
/*
 * FUNCTION: Resets the pageable status of a driver's sections to their
 * compile time settings
 * ARGUMENTS:
 *         AddressWithinSection = Any address within the driver
 */
VOID MmResetDriverPaging(PVOID AddressWithinSection);
   
/*
 * FUNCTION: Reinitializes a caller-allocated MDL
 * ARGUMENTS:
 *         Mdl = Points to the MDL that will be reused
 */
VOID MmPrepareMdlForReuse(PMDL Mdl);
   
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
VOID MmProbeAndLockPages(PMDL MemoryDescriptorList, 
			 KPROCESSOR_MODE AccessMode, 
			 LOCK_OPERATION Operation);
   
/*
 * FUNCTION: Returns an estimate of the amount of memory in the system
 * RETURNS: Either MmSmallSystem, MmMediumSystem or MmLargeSystem
 */
MM_SYSTEM_SIZE MmQuerySystemSize(VOID);

/*
 * FUNCTION: Returns the number of bytes to allocate for an MDL 
 * describing a given address range
 * ARGUMENTS:
 *          Base = Base virtual address for the region
 *          Length = size in bytes of the region
 * RETURNS: The number of bytes required for the MDL
 */
ULONG MmSizeOfMdl(PVOID Base, ULONG Length);
   
/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *          Mdl = Mdl to unlock
 */
VOID MmUnlockPages(PMDL Mdl);
   
/*
 * FUNCTION: Releases a section of driver code or data previously locked into 
 * memory
 * ARGUMENTS: 
 *         ImageSectionHandle = Handle for the locked section
 */
VOID MmUnlockPagableImageSection(PVOID ImageSectionHandle);

VOID MmUnmapIoSpace(PVOID BaseAddress, ULONG NumberOfBytes);
VOID MmUnmapLockedPages(PVOID BaseAddress, PMDL MemoryDescriptorList);
