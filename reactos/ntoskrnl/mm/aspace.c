/* $Id: aspace.c,v 1.9 2002/05/13 18:10:40 chorns Exp $
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

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/pool.h>

#include <internal/debug.h>

/* Define to track address space locking/unlocking */
//#define TRACK_ADDRESS_SPACE_LOCK

/* GLOBALS ******************************************************************/

STATIC MADDRESS_SPACE KernelAddressSpace;

#define TAG_ASPC      TAG('A', 'S', 'P', 'C')
#define TAG_PTRC      TAG('P', 'T', 'R', 'C')

/* FUNCTIONS *****************************************************************/

#ifdef DBG
VOID
MiValidateAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  assertmsg(AddressSpace != NULL,
   ("No address space can exist at 0x%.08x\n", AddressSpace));

  assertmsg(AddressSpace->Magic == TAG_ASPC,
   ("Bad magic (0x%.08x) for address space (0x%.08x). It should be 0x%.08x\n",
     AddressSpace->Magic, AddressSpace, TAG_ASPC));

  assertmsg(AddressSpace->ReferenceCount > 0,
    ("No outstanding references on address space (0x%.08x)\n", AddressSpace));
}
#endif /* DBG */


#ifdef DBG

VOID
MiLockAddressSpace(IN PMADDRESS_SPACE  AddressSpace,
  IN LPSTR  FileName,
	IN ULONG  LineNumber)
{
	VALIDATE_ADDRESS_SPACE(AddressSpace);
	
	/*
	* Don't bother with locking if we are the first thread.
	*/
	if (KeGetCurrentThread() == NULL)
		{
		  return;
		}

#ifdef TRACK_ADDRESS_SPACE_LOCK
	DbgPrint("(0x%.08x)(%s:%d) Locking address space 0x%.08x\n",
		KeGetCurrentThread(), FileName, LineNumber, AddressSpace);
#endif /* TRACK_ADDRESS_SPACE_LOCK */

	(VOID)KeWaitForMutexObject(&AddressSpace->Lock,
		Executive,
		KernelMode,
		FALSE,
		NULL);   

#ifdef TRACK_ADDRESS_SPACE_LOCK
	DbgPrint("(0x%.08x)(%s:%d) Locked address space 0x%.08x\n",
		KeGetCurrentThread(), FileName, LineNumber, AddressSpace);
#endif /* TRACK_ADDRESS_SPACE_LOCK */
}


VOID
MiUnlockAddressSpace(IN PMADDRESS_SPACE  AddressSpace,
  IN LPSTR  FileName,
	IN ULONG  LineNumber)
{
  VALIDATE_ADDRESS_SPACE(AddressSpace);

  /*
   * Don't bother locking if we are the first thread.
   */
  if (KeGetCurrentThread() == NULL)
    {
      return;
    }
  KeReleaseMutex(&AddressSpace->Lock, FALSE);

#ifdef TRACK_ADDRESS_SPACE_LOCK
	DbgPrint("(0x%.08x)(%s:%d) Unlocked address space 0x%.08x\n",
		KeGetCurrentThread(), FileName, LineNumber, AddressSpace);
#endif /* TRACK_ADDRESS_SPACE_LOCK */
}

#else /* !DBG */

VOID
MiLockAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  /*
	 * Don't bother with locking if we are the first thread.
	 */
	if (KeGetCurrentThread() == NULL)
		{
		  return;
		}

	(VOID)KeWaitForMutexObject(&AddressSpace->Lock,
		Executive,
		KernelMode,
		FALSE,
		NULL);   
}


VOID
MiUnlockAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  /*
   * Don't bother locking if we are the first thread.
   */
  if (KeGetCurrentThread() == NULL)
    {
      return;
    }
  KeReleaseMutex(&AddressSpace->Lock, FALSE);
}

#endif /* !DBG */


VOID 
MmInitializeKernelAddressSpace()
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
}


PMADDRESS_SPACE
MmGetCurrentAddressSpace()
{
   return(&PsGetCurrentProcess()->AddressSpace);
}


PMADDRESS_SPACE
MmGetKernelAddressSpace()
{
   return(&KernelAddressSpace);
}


NTSTATUS 
MmInitializeAddressSpace(IN PEPROCESS  Process,
  IN PMADDRESS_SPACE  AddressSpace)
{
   SET_MAGIC(AddressSpace, TAG_ASPC)

   AddressSpace->ReferenceCount = 1;
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
	AddressSpace->PageTableRefCountTable = 
	  ExAllocatePoolWithTag(NonPagedPool, 768 * sizeof(USHORT),
				TAG_PTRC);
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
MmDestroyAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  VALIDATE_ADDRESS_SPACE(AddressSpace);

  AddressSpace->ReferenceCount--;

  assertmsg(AddressSpace->ReferenceCount == 0,
    ("There are %d outstanding references on address space (0x%.08x)\n",
    AddressSpace->ReferenceCount, AddressSpace));

  if (AddressSpace->PageTableRefCountTable != NULL)
    {
      ExFreePool(AddressSpace->PageTableRefCountTable);
    }
   return(STATUS_SUCCESS);
}


VOID
MmReferenceAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  InterlockedIncrement(&AddressSpace->ReferenceCount);
}


VOID
MmDereferenceAddressSpace(IN PMADDRESS_SPACE  AddressSpace)
{
  InterlockedDecrement(&AddressSpace->ReferenceCount);

  assertmsg(AddressSpace->ReferenceCount > 0,
    ("No outstanding references on address space (0x%.08x)\n", AddressSpace));
}
