/* $Id: aspace.c,v 1.2 2000/04/07 02:24:00 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/aspace.c
 * PURPOSE:         Manages address spaces
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static MADDRESS_SPACE KernelAddressSpace;

/* FUNCTIONS *****************************************************************/

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   (VOID)KeWaitForMutexObject(&AddressSpace->Lock,
			      0,
			      KernelMode,
			      FALSE,
			      NULL);   
}

VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   KeReleaseMutex(&AddressSpace->Lock, FALSE);
}

VOID MmInitializeKernelAddressSpace(VOID)
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
   KernelAddressSpace.LowestAddress = KERNEL_BASE;
}

PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID)
{
   return(&PsGetCurrentProcess()->Pcb.AddressSpace);
}

PMADDRESS_SPACE MmGetKernelAddressSpace(VOID)
{
   return(&KernelAddressSpace);
}

NTSTATUS MmInitializeAddressSpace(PEPROCESS Process,
				  PMADDRESS_SPACE AddressSpace)
{
   InitializeListHead(&AddressSpace->MAreaListHead);
   KeInitializeMutex(&AddressSpace->Lock, 1);
   AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
   AddressSpace->Process = Process;
   return(STATUS_SUCCESS);
}

NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   return(STATUS_SUCCESS);
}
