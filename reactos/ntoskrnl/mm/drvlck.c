/* $Id: drvlck.c,v 1.3 2002/09/08 10:23:32 chorns Exp $
 *
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


VOID STDCALL
MmLockPagableSectionByHandle(IN PVOID ImageSectionHandle)
{
//  MmLockMemoryArea((MEMORY_AREA *)ImageSectionHandle);
  UNIMPLEMENTED;
}


#if 0
PVOID
MmLockPagableCodeSection(IN PVOID AddressWithinSection)
{
  PVOID Handle;
  Handle = MmOpenMemoryAreaByAddress(NULL,AddressWithinSection);
  MmLockPagableSectionByHandle(Handle);
  return(Handle);
}
#endif


PVOID STDCALL
MmLockPagableDataSection(IN PVOID AddressWithinSection)
{
  PVOID Handle;
  Handle = MmOpenMemoryAreaByAddress(NULL,AddressWithinSection);
  MmLockPagableSectionByHandle(Handle);
  return(Handle);
}


VOID STDCALL
MmUnlockPagableImageSection(IN PVOID ImageSectionHandle)
{
}


VOID STDCALL
MmPageEntireDriver(IN PVOID AddressWithinSection)
{
}


VOID STDCALL
MmResetDriverPaging(IN PVOID AddressWithinSection)
{
}

/* EOF */
