/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/iomgr/drvlck.c
 * PURPOSE:         Managing driver managing
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID MmUnlockPagableImageSection(PVOID ImageSectionHandle)
{
   UNIMPLEMENTED;
}

VOID MmLockPagableSectionByHandle(PVOID ImageSectionHandle)
{
   UNIMPLEMENTED;
}

PVOID MmLockPagableCodeSection(PVOID AddressWithinSection)
{
   UNIMPLEMENTED;
}

PVOID MmLockPagableDataSection(PVOID AddressWithinSection)
{
   UNIMPLEMENTED;
}

VOID MmPageEntireDriver(PVOID AddressWithinSection)
{
   UNIMPLEMENTED;
}

VOID MmResetDriverPaging(PVOID AddressWithinSection)
{
   UNIMPLEMENTED;
}

