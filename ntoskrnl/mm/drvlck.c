/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/drvlck.c
 * PURPOSE:         Managing driver managing
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#undef MmLockPagableDataSection

#if 0
VOID
MmUnlockPagableImageSection(IN PVOID ImageSectionHandle)
/*
 * FUNCTION: Releases a section of driver code or driver data, previously
 * locked into system space with MmLockPagableCodeSection,
 * MmLockPagableDataSection or MmLockPagableSectionByHandle
 * ARGUMENTS:
 *        ImageSectionHandle = Handle returned by MmLockPagableCodeSection or
 *                             MmLockPagableDataSection
 */
{
   //  MmUnlockMemoryArea((MEMORY_AREA *)ImageSectionHandle);
   UNIMPLEMENTED;
}
#endif


/*
 * @unimplemented
 */
VOID NTAPI
MmLockPagableSectionByHandle(IN PVOID ImageSectionHandle)
{
    UNIMPLEMENTED;
}


#if 0
PVOID
MmLockPagableCodeSection(IN PVOID AddressWithinSection)
{
   PVOID Handle;
   Handle = MmLocateMemoryAreaByAddress(NULL,AddressWithinSection);
   MmLockPagableSectionByHandle(Handle);
   return(Handle);
}
#endif


/*
 * @implemented
 */
PVOID NTAPI
MmLockPagableDataSection(IN PVOID AddressWithinSection)
{
   PVOID Handle;
   Handle = MmLocateMemoryAreaByAddress(MmGetKernelAddressSpace(),
                                        AddressWithinSection);
   MmLockPagableSectionByHandle(Handle);
   return(Handle);
}


/*
 * @unimplemented
 */
VOID NTAPI
MmUnlockPagableImageSection(IN PVOID ImageSectionHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID NTAPI
MmPageEntireDriver(IN PVOID AddressWithinSection)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
VOID NTAPI
MmResetDriverPaging(IN PVOID AddressWithinSection)
{
    UNIMPLEMENTED;
}

/* EOF */
