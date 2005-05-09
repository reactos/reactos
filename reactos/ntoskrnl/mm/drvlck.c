/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/drvlck.c
 * PURPOSE:         Managing driver managing
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
VOID STDCALL
MmLockPagableSectionByHandle(IN PVOID ImageSectionHandle)
{
   //  MmLockMemoryArea((MEMORY_AREA *)ImageSectionHandle);
   DPRINT1("MmLockPagableSectionByHandle is unimplemented\n");
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
PVOID STDCALL
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
VOID STDCALL
MmUnlockPagableImageSection(IN PVOID ImageSectionHandle)
{}


/*
 * @unimplemented
 */
VOID STDCALL
MmPageEntireDriver(IN PVOID AddressWithinSection)
{}


/*
 * @unimplemented
 */
VOID STDCALL
MmResetDriverPaging(IN PVOID AddressWithinSection)
{}

/* EOF */
