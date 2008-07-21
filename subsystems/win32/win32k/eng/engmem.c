/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmem.c
 * PURPOSE:         Memory Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

PVOID
APIENTRY
EngAllocMem(IN ULONG Flags,
            IN ULONG MemSize,
            IN ULONG Tag)
{
    PVOID AllocatedMemory;
	POOL_TYPE AllocationType;

	/* Get the requested pool type */
    AllocationType = (Flags & FL_NONPAGED_MEMORY) ? NonPagedPool : PagedPool;

	/* Allocate the memory */
	AllocatedMemory = ExAllocatePoolWithTag(AllocationType,
	                                        MemSize,
	                                        Tag);

    /* Zero initialize the memory if requested */
    if (Flags & FL_ZERO_MEMORY && AllocatedMemory)
    {
        /* Zero it out */
        RtlZeroMemory(AllocatedMemory,
                      MemSize);
    }

    /* Return the allocated memory */
    return AllocatedMemory;
}

VOID
APIENTRY
EngFreeMem(IN PVOID Memory)
{
    /* Free the memory */
    ExFreePool(Memory);
}

PVOID
APIENTRY
EngAllocUserMem(IN SIZE_T cjSize,
                IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
EngFreeUserMem(IN PVOID UserMemory)
{
    UNIMPLEMENTED;
}

PVOID
APIENTRY
EngAllocSectionMem(IN PVOID SectionObject,
                   IN ULONG Flags,
                   IN SIZE_T MemSize,
                   IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}
 
BOOLEAN
APIENTRY
EngFreeSectionMem(IN PVOID SectionObject OPTIONAL,
                  IN PVOID MappedBase)
{
    UNIMPLEMENTED;
    return FALSE;
}
    
HANDLE
APIENTRY
EngSecureMem(IN PVOID Address,
             IN ULONG Length)
{
    /* Secure the memory area for read & write access */
    return MmSecureVirtualMemory(Address,
                                 Length,
                                 PAGE_READWRITE);
}

VOID
APIENTRY
EngUnsecureMem(IN HANDLE hSecure)
{
    /* Unsecure the memory area */
    MmUnsecureVirtualMemory((PVOID)hSecure);
}


BOOLEAN
APIENTRY
EngMapSection(IN PVOID SectionObject,
              IN BOOLEAN Map,
              IN HANDLE Process,
              IN PVOID* BaseAddress)
{
    UNIMPLEMENTED;
    return FALSE;
}
