/* $Id: aspace.c,v 1.4 2000/08/20 17:02:08 dwelch Exp $
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
#include <internal/ps.h>

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
}

PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID)
{
   return(&PsGetCurrentProcess()->AddressSpace);
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
   if (Process != NULL)
     {
	AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
     }
   else
     {
	AddressSpace->LowestAddress = KERNEL_BASE;
     }
   AddressSpace->Process = Process;
   if (Process != NULL)
     {
	MmInitializeWorkingSet(Process, AddressSpace);
     }
   if (Process != NULL)
     {
	AddressSpace->PageTableRefCountTable = 
	  ExAllocatePool(NonPagedPool, 768 * sizeof(USHORT));
	AddressSpace->PageTableRefCountTableSize = 768;
     }
   else
     {
	AddressSpace->PageTableRefCountTable = NULL;
	AddressSpace->PageTableRefCountTableSize = 0;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   return(STATUS_SUCCESS);
}
