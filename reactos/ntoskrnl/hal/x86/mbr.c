/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/hal/x86/mbr.c
 * PURPOSE:         Functions for reading the master boot record (MBR)
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

VOID HalExamineMBR(PDEVICE_OBJECT DeviceObject,
		   ULONG SectorSize,
		   ULONG MBRTypeIdentifier,
		   PVOID Buffer)
{
   UNIMPLEMENTED;
}
