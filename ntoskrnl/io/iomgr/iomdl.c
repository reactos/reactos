/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/mdl.c
 * PURPOSE:         I/O Wrappers for MDL Allocation and Deallocation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PMDL
NTAPI
IoAllocateMdl(IN PVOID VirtualAddress,
              IN ULONG Length,
              IN BOOLEAN SecondaryBuffer,
              IN BOOLEAN ChargeQuota,
              IN PIRP Irp)
{
    PMDL Mdl = NULL, p;
    ULONG Flags = 0;
    ULONG Size;

    /* Fail if allocation is over 2GB */
    if (Length & 0x80000000) return NULL;

    /* Calculate the number of pages for the allocation */
    Size = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, Length);
    if (Size > 23)
    {
        /* This is bigger then our fixed-size MDLs. Calculate real size */
        Size *= sizeof(PFN_NUMBER);
        Size += sizeof(MDL);
        if (Size > MAXUSHORT) return NULL;
    }
    else
    {
        /* Use an internal fixed MDL size */
        Size = (23 * sizeof(PFN_NUMBER)) + sizeof(MDL);
        Flags |= MDL_ALLOCATED_FIXED_SIZE;

        /* Allocate one from the lookaside list */
        Mdl = IopAllocateMdlFromLookaside(LookasideMdlList);
    }

    /* Check if we don't have an mdl yet */
    if (!Mdl)
    {
        /* Allocate one from pool */
        Mdl = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MDL);
        if (!Mdl) return NULL;
    }

    /* Initialize it */
    MmInitializeMdl(Mdl, VirtualAddress, Length);
    Mdl->MdlFlags |= Flags;

    /* Check if an IRP was given too */
    if (Irp)
    {
        /* Check if it came with a secondary buffer */
        if (SecondaryBuffer)
        {
            /* Insert the MDL at the end */
            p = Irp->MdlAddress;
            while (p->Next) p = p->Next;
            p->Next = Mdl;
      }
      else
      {
            /* Otherwise, insert it directly */
            Irp->MdlAddress = Mdl;
      }
   }

    /* Return the allocated mdl */
    return Mdl;
}

/*
 * @implemented
 */
VOID
NTAPI
IoBuildPartialMdl(IN PMDL SourceMdl,
                  IN PMDL TargetMdl,
                  IN PVOID VirtualAddress,
                  IN ULONG Length)
{
    PPFN_NUMBER TargetPages = (PPFN_NUMBER)(TargetMdl + 1);
    PPFN_NUMBER SourcePages = (PPFN_NUMBER)(SourceMdl + 1);
    ULONG Offset;
    ULONG FlagsMask = (MDL_IO_PAGE_READ |
                       MDL_SOURCE_IS_NONPAGED_POOL |
                       MDL_MAPPED_TO_SYSTEM_VA |
                       MDL_IO_SPACE);

    /* Calculate the offset */
    Offset = (ULONG)((ULONG_PTR)VirtualAddress -
                     (ULONG_PTR)SourceMdl->StartVa) -
                     SourceMdl->ByteOffset;

    /* Check if we don't have a length and calculate it */
    if (!Length) Length = SourceMdl->ByteCount - Offset;

    /* Write the process, start VA and byte data */
    TargetMdl->StartVa = (PVOID)PAGE_ROUND_DOWN(VirtualAddress);
    TargetMdl->Process = SourceMdl->Process;
    TargetMdl->ByteCount = Length;
    TargetMdl->ByteOffset = BYTE_OFFSET(VirtualAddress);

    /* Recalculate the length in pages */
    Length = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, Length);

    /* Set the MDL Flags */
    TargetMdl->MdlFlags &= (MDL_ALLOCATED_FIXED_SIZE | MDL_ALLOCATED_MUST_SUCCEED);
    TargetMdl->MdlFlags |= SourceMdl->MdlFlags & FlagsMask;
    TargetMdl->MdlFlags |= MDL_PARTIAL;

    /* Set the mapped VA */
    TargetMdl->MappedSystemVa = (PCHAR)SourceMdl->MappedSystemVa + Offset;

    /* Now do the copy */
    Offset = ((ULONG_PTR)TargetMdl->StartVa - (ULONG_PTR)SourceMdl->StartVa) >>
             PAGE_SHIFT;
    SourcePages += Offset;
    RtlCopyMemory(TargetPages, SourcePages, Length * sizeof(PFN_NUMBER));
}

/*
 * @implemented
 */
VOID
NTAPI
IoFreeMdl(PMDL Mdl)
{
    /* Tell Mm to reuse the MDL */
    MmPrepareMdlForReuse(Mdl);

    /* Check if this was a pool allocation */
    if (!(Mdl->MdlFlags & MDL_ALLOCATED_FIXED_SIZE))
    {
        /* Free it from the pool */
        ExFreePoolWithTag(Mdl, TAG_MDL);
    }
    else
    {
        /* Free it from the lookaside */
        IopFreeMdlFromLookaside(Mdl, LookasideMdlList);
    }
}

/* EOF */
