/* $Id: aspace.c,v 1.16 2004/03/04 00:07:01 navaraf Exp $
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
#include <internal/pool.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

STATIC MADDRESS_SPACE KernelAddressSpace;

#define TAG_PTRC      TAG('P', 'T', 'R', 'C')

/* FUNCTIONS *****************************************************************/

VOID 
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
  /*
   * Don't bother with locking if we are the first thread.
   */
  if (KeGetCurrentThread() == NULL)
    {
      return;
    }
  ExAcquireFastMutex(&AddressSpace->Lock);
}

VOID 
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
  /*
   * Don't bother locking if we are the first thread.
   */
  if (KeGetCurrentThread() == NULL)
    {
      return;
    }
  ExReleaseFastMutex(&AddressSpace->Lock);
}

VOID INIT_FUNCTION
MmInitializeKernelAddressSpace(VOID)
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

NTSTATUS 
MmInitializeAddressSpace(PEPROCESS Process,
			 PMADDRESS_SPACE AddressSpace)
{
   InitializeListHead(&AddressSpace->MAreaListHead);
   ExInitializeFastMutex(&AddressSpace->Lock);
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
	AddressSpace->PageTableRefCountTable = 
	  ExAllocatePoolWithTag(NonPagedPool, 768 * sizeof(USHORT),
				TAG_PTRC);
	RtlZeroMemory(AddressSpace->PageTableRefCountTable, 768 * sizeof(USHORT));
	AddressSpace->PageTableRefCountTableSize = 768;
     }
   else
     {
	AddressSpace->PageTableRefCountTable = NULL;
	AddressSpace->PageTableRefCountTableSize = 0;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS 
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
  if (AddressSpace->PageTableRefCountTable != NULL)
    {
      ExFreePool(AddressSpace->PageTableRefCountTable);
    }
   return(STATUS_SUCCESS);
}

/* EOF */
