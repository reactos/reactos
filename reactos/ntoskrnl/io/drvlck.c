/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/drvlck.c
 * PURPOSE:         Managing driver managing
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID MmUnlockPagableImageSection(PVOID ImageSectionHandle)
/*
 * FUNCTION: Releases a section of driver code or driver data, previously
 * locked into system space with MmLockPagableCodeSection, 
 * MmLockPagableDataSection or MmLockPagableSectionByHandle
 * ARGUMENTS:
 *        ImageSectionHandle = Handle returned by MmLockPagableCodeSection or
 *                             MmLockPagableDataSection
 */
{
   MmUnlockMemoryArea((MEMORY_AREA *)ImageSectionHandle);
}

VOID MmLockPagableSectionByHandle(PVOID ImageSectionHandle)
{
   MmLockMemoryArea((MEMORY_AREA *)ImageSectionHandle);
}

PVOID MmLockPagableCodeSection(PVOID AddressWithinSection)
{
   PVOID Handle;
   Handle = MmOpenMemoryAreaByAddress(NULL,AddressWithinSection);
   MmLockPagableSectionByHandle(Handle);
   return(Handle);
}

PVOID MmLockPagableDataSection(PVOID AddressWithinSection)
{
   return(MmLockPagableCodeSection(AddressWithinSection));
}

VOID MmPageEntireDriver(PVOID AddressWithinSection)
{
}

VOID MmResetDriverPaging(PVOID AddressWithinSection)
{
}

