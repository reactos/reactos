/* $Id: mdl.c,v 1.14 2004/04/20 19:01:47 gdalsnes Exp $
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
#include <internal/pool.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MDL    TAG('M', 'D', 'L', ' ')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
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
   MmInitializeMdl(Mdl, (char*)VirtualAddress, Length);
   
   if (Irp)
   {
      if (SecondaryBuffer)
      {
         assert(Irp->MdlAddress);
         
         /* FIXME: add to end of list maybe?? */
         Mdl->Next = Irp->MdlAddress->Next;
         Irp->MdlAddress->Next = Mdl;
      }
      else
      {
         /* 
          * What if there's allready an mdl at Irp->MdlAddress?
          * Is that bad and should we do something about it?
          */
         Irp->MdlAddress = Mdl;
      }
   }
   
   return(Mdl);
}

/*
 * @implemented
 *
 * You must IoFreeMdl the slave before freeing the master.
 *
 * IoBuildPartialMdl is more similar to MmBuildMdlForNonPagedPool, the difference
 * is that the former takes the physical addresses from the master MDL, while the
 * latter - from the known location of the NPP.
 */
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
                 PAGE_SIZE;

   for (Va = 0; Va < (PAGE_ROUND_UP(Length)/PAGE_SIZE); Va++)
   {
      TargetPages[Va] = SourcePages[Va+Delta];
   }
   
   TargetMdl->MdlFlags |= MDL_PARTIAL;
}

/*
 * @implemented
 */
VOID STDCALL
IoFreeMdl(PMDL Mdl)
{   
   /* 
    * This unmaps partial mdl's from kernel space but also asserts that non-partial
    * mdl's isn't still mapped into kernel space.
    */
   MmPrepareMdlForReuse(Mdl);
   
   ExFreePool(Mdl);
}


/* EOF */
