/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/xipdisp.c
 * PURPOSE:         eXecute In Place (XIP) Support.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
XIPDispatch(IN ULONG DispatchCode,
            OUT PVOID Buffer,
            IN ULONG BufferSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
INIT_FUNCTION
XIPpFindMemoryDescriptor(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor = NULL;

    /* Loop the memory descriptors */
    for (NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &LoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the current descriptor and check if it's the XIP ROM */
        Descriptor = CONTAINING_RECORD(NextEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);
        if (Descriptor->MemoryType == LoaderXIPRom) return Descriptor;
    }

    /* Nothing found if we got here */
    return NULL;
}

VOID
NTAPI
INIT_FUNCTION
XIPInit(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine, XipBoot, XipRom, XipMegs, XipVerbose, XipRam;
    PMEMORY_ALLOCATION_DESCRIPTOR XipDescriptor;

    /* Get the command line */
    CommandLine = LoaderBlock->LoadOptions;
    if (!CommandLine) return;

    /* Get XIP settings */
    XipBoot = strstr(CommandLine, "XIPBOOT");
    XipRam = strstr(CommandLine, "XIPRAM=");
    XipRom = strstr(CommandLine, "XIPROM=");
    XipMegs = strstr(CommandLine, "XIPMEGS=");
    XipVerbose = strstr(CommandLine, "XIPVERBOSE");

    /* Check if this is a verbose boot */
    if (XipVerbose)
    {
        /* Print out our header */
        DbgPrint("\n\nXIP: debug timestamp at line %d in %s:   <<<%s %s>>>\n\n",
                 __LINE__,
                 __FILE__,
                 __DATE__,
                 __TIME__);
    }

    /* Find the XIP memory descriptor */
    XipDescriptor = XIPpFindMemoryDescriptor(LoaderBlock);
    if (!XipDescriptor) return;
    
    //
    // Make sure this is really XIP, and not RAM Disk -- also validate XIP
    // Basically, either this is a ROM boot or a RAM boot, but not both nor none
    //
    if (!((ULONG_PTR)XipRom ^ (ULONG_PTR)XipRam)) return;

    /* FIXME: TODO */
    DPRINT1("ReactOS does not yet support eXecute In Place boot technology\n");
    DPRINT("%s MB requested (XIP = %s)\n", XipMegs, XipBoot);
}

/* EOF */
