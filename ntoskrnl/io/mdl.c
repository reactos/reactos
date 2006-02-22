/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/mdl.c
 * PURPOSE:         Io manager mdl functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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
         ASSERT(Irp->MdlAddress);

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
   PPFN_TYPE TargetPages = (PPFN_TYPE)(TargetMdl + 1);
   PPFN_TYPE SourcePages = (PPFN_TYPE)(SourceMdl + 1);
   ULONG Count;
   ULONG Delta;

   DPRINT("VirtualAddress 0x%p, SourceMdl->StartVa 0x%p, SourceMdl->MappedSystemVa 0x%p\n",
          VirtualAddress, SourceMdl->StartVa, SourceMdl->MappedSystemVa);

   TargetMdl->StartVa = (PVOID)PAGE_ROUND_DOWN(VirtualAddress);
   TargetMdl->ByteOffset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)TargetMdl->StartVa;
   TargetMdl->ByteCount = Length;
   TargetMdl->Process = SourceMdl->Process;
   Delta = (ULONG_PTR)VirtualAddress - ((ULONG_PTR)SourceMdl->StartVa + SourceMdl->ByteOffset);
   TargetMdl->MappedSystemVa = (char*)SourceMdl->MappedSystemVa + Delta;

   TargetMdl->MdlFlags = SourceMdl->MdlFlags & (MDL_IO_PAGE_READ|MDL_SOURCE_IS_NONPAGED_POOL|MDL_MAPPED_TO_SYSTEM_VA);
   TargetMdl->MdlFlags |= MDL_PARTIAL;

   Delta = ((ULONG_PTR)TargetMdl->StartVa - (ULONG_PTR)SourceMdl->StartVa) / PAGE_SIZE;
   Count = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress,Length);

   SourcePages += Delta;

   DPRINT("Delta %d, Count %d\n", Delta, Count);

   memcpy(TargetPages, SourcePages, Count * sizeof(PFN_TYPE));

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
   ASSERT(Mdl);
   ASSERT_IRQL(DISPATCH_LEVEL);

   MmPrepareMdlForReuse(Mdl);

   ExFreePoolWithTag(Mdl, TAG_MDL);
}


/* EOF */
