#ifndef _INCLUDE_DDK_MMFUNCS_H
#define _INCLUDE_DDK_MMFUNCS_H
/* $Id: mmfuncs.h,v 1.6 2000/06/29 23:35:12 dwelch Exp $ */
/* MEMORY MANAGMENT ******************************************************/

#include <ddk/i386/pagesize.h>

#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGESIZE) ? ((((ULONG)x)&(~0xfff))+0x1000) : ((ULONG)x) )
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~0xfff))


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
extern inline unsigned int ADDRESS_AND_SIZE_TO_SPAN_PAGES(PVOID Va,
							  ULONG Size)
{
   ULONG HighestAddr;
   ULONG LowestAddr;
   
   HighestAddr = PAGE_ROUND_UP(Size + ((ULONG)Va));
   LowestAddr = PAGE_ROUND_DOWN((ULONG)Va);
   return((HighestAddr - LowestAddr) / PAGESIZE);
}

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

DWORD
STDCALL
MmAdjustWorkingSetSize (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
PVOID
STDCALL
MmAllocateContiguousMemory (
	IN	ULONG			NumberOfBytes,
	IN	PHYSICAL_ADDRESS	HighestAcceptableAddress
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
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
MmFreeContiguousMemory (
	IN OUT	PVOID	BaseAddress
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
#define MmGetMdlByteOffset(Mdl)  ((Mdl)->ByteOffset)

/*
 * FUNCTION: Returns the initial virtual address for a buffer described
 * by an MDL
 * ARGUMENTS:
 *        Mdl = the mdl
 * RETURNS: The initial virtual address
 */
#define MmGetMdlVirtualAddress(Mdl)  \
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
#define MmGetProcedureAddress(Address) (Address)

/*
 * FUNCTION: Maps the physical pages described by an MDL into system space
 * ARGUMENTS:
 *      Mdl = mdl
 * RETURNS: The base system address for the mapped buffer
 */
PVOID MmGetSystemAddressForMdl(PMDL Mdl);
NTSTATUS
STDCALL
MmGrowKernelStack (
	DWORD	Unknown0
	);
#ifdef __NTOSKRNL__
extern PVOID EXPORTED MmHighestUserAddress;
#else
extern PVOID IMPORTED MmHighestUserAddress;
#endif
/*
 * FUNCTION: Initalizes an mdl
 * ARGUMENTS: 
 *       MemoryDescriptorList = MDL to be initalized
 *       BaseVa = Base virtual address of the buffer 
 *       Length = Length in bytes of the buffer
 */
VOID MmInitializeMdl(PMDL MemoryDescriptorList, PVOID BaseVa, ULONG Length);

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
/*
 * FUNCTION: Locks a section of the driver's code into memory
 * ARGUMENTS:
 *        AddressWithinSection = Any address in the region
 * RETURNS: A handle to the region
 */
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

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
	BOOLEAN			CacheEnable
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
VOID
STDCALL
MmMapMemoryDumpMdl (
	PVOID	Unknown0
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
MmMapViewInSystemSpace (
	IN	PVOID	Section,
	OUT	PVOID	* MappedBase,
	IN	PULONG	ViewSize
	);
PVOID
STDCALL
MmMapViewOfSection (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9
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
DWORD
STDCALL
MmSecureVirtualMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
BOOLEAN
STDCALL
MmSetAddressRangeModified (
	DWORD	Unknown0,
	DWORD	Unknown1
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
VOID
STDCALL
MmProbeAndLockPages (
	PMDL		MemoryDescriptorList, 
	KPROCESSOR_MODE	AccessMode, 
	LOCK_OPERATION	Operation
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
MmUnmapVideoDisplay (
	IN	PVOID	BaseAddress,
	IN	ULONG	NumberOfBytes
	);
NTSTATUS
STDCALL
MmUnmapViewInSystemSpace (
	DWORD	Unknown0
	);
#if 0
NTSTATUS
STDCALL
MmUnmapViewOfSection (
	PEPROCESS	Process,
	PMEMORY_AREA	MemoryArea
	)
#endif
VOID
STDCALL
MmUnsecureVirtualMemory (
	DWORD	Unknown0
	);
#endif
