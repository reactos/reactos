/* $Id: drvlck.c,v 1.4 2003/07/10 21:05:03 royce Exp $
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


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
PVOID STDCALL
MmLockPagableDataSection(IN PVOID AddressWithinSection)
{
  PVOID Handle;
  Handle = MmOpenMemoryAreaByAddress(NULL,AddressWithinSection);
  MmLockPagableSectionByHandle(Handle);
  return(Handle);
}


/*
 * @unimplemented
 */
VOID STDCALL
MmUnlockPagableImageSection(IN PVOID ImageSectionHandle)
{
}


/*
 * @unimplemented
 */
VOID STDCALL
MmPageEntireDriver(IN PVOID AddressWithinSection)
{
}


/*
 * @unimplemented
 */
VOID STDCALL
MmResetDriverPaging(IN PVOID AddressWithinSection)
{
}

/* EOF */
