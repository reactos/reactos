/* $Id: mdl.c,v 1.6 2001/03/07 16:48:42 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/mdl.c
 * PURPOSE:         Io manager mdl functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mmhal.h>
#include <internal/pool.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MDL    TAG('M', 'D', 'L', ' ')

/* FUNCTIONS *****************************************************************/

PMDL
STDCALL
IoAllocateMdl(PVOID VirtualAddress,
		   ULONG Length,
		   BOOLEAN SecondaryBuffer,
		   BOOLEAN ChargeQuota,
		   PIRP Irp)
{
   PMDL Mdl;
   
   if (ChargeQuota)
     {
//	Mdl = ExAllocatePoolWithQuota(NonPagedPool,
//				      MmSizeOfMdl(VirtualAddress,Length));
	Mdl = ExAllocatePoolWithTag(NonPagedPool,
				    MmSizeOfMdl(VirtualAddress,Length),
				    TAG_MDL);
     }
   else
     {
	Mdl = ExAllocatePoolWithTag(NonPagedPool,
				    MmSizeOfMdl(VirtualAddress,Length),
				    TAG_MDL);
     }
   MmInitializeMdl(Mdl,VirtualAddress,Length);
   if (Irp!=NULL && !SecondaryBuffer)
     {
	Irp->MdlAddress = Mdl;
     }
   return(Mdl);
}

VOID
STDCALL
IoBuildPartialMdl(PMDL SourceMdl,
		       PMDL TargetMdl,
		       PVOID VirtualAddress,
		       ULONG Length)
{
   PULONG TargetPages = (PULONG)(TargetMdl + 1);
   PULONG SourcePages = (PULONG)(SourceMdl + 1);
   ULONG Va;
   ULONG Delta = (PAGE_ROUND_DOWN(VirtualAddress) - (ULONG)SourceMdl->StartVa)/
                 PAGESIZE;

   for (Va = 0; Va < (PAGE_ROUND_UP(Length)/PAGESIZE); Va++)
     {
	TargetPages[Va] = SourcePages[Va+Delta];
     }
}

VOID
STDCALL
IoFreeMdl(PMDL Mdl)
{   
   MmUnmapLockedPages(MmGetSystemAddressForMdl(Mdl),Mdl);
   MmUnlockPages(Mdl);
   ExFreePool(Mdl);
}


/* EOF */
