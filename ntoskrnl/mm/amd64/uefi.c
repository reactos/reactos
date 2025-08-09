/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        ntoskrnl/mm/amd64/uefi.c
 * PURPOSE:     UEFI-specific Memory Manager Support for AMD64
 * PROGRAMMERS: ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS *******************************************************************/

BOOLEAN MmUefiBootMode = FALSE;
PHYSICAL_ADDRESS MmUefiFramebufferBase = {0};
ULONG MmUefiFramebufferSize = 0;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeUefiSupport(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION Extension;
    
    /* Check if this is a UEFI boot */
    if (!LoaderBlock || !LoaderBlock->Extension)
    {
        DPRINT1("MM: No loader extension, assuming BIOS boot\n");
        return STATUS_NOT_SUPPORTED;
    }
    
    Extension = LoaderBlock->Extension;
    if (Extension->Size < sizeof(LOADER_PARAMETER_EXTENSION))
    {
        DPRINT1("MM: Loader extension too small\n");
        return STATUS_NOT_SUPPORTED;
    }
    
    /* Check for UEFI boot flag */
    if (!Extension->BootViaEFI)
    {
        DPRINT1("MM: Not a UEFI boot\n");
        return STATUS_NOT_SUPPORTED;
    }
    
    /* We're in UEFI mode */
    MmUefiBootMode = TRUE;
    DPRINT("MM: UEFI boot detected\n");
    
    /* Save framebuffer information if available */
    if (Extension->UefiFramebuffer.FrameBufferBase.QuadPart != 0)
    {
        MmUefiFramebufferBase = Extension->UefiFramebuffer.FrameBufferBase;
        MmUefiFramebufferSize = Extension->UefiFramebuffer.FrameBufferSize;
        
        DPRINT("MM: UEFI Framebuffer at 0x%llx, size 0x%lx\n",
               MmUefiFramebufferBase.QuadPart,
               MmUefiFramebufferSize);
    }
    
    /* Process memory descriptors from loader block */
    /* UEFI memory map is already converted to NT format by the loader */
    DPRINT("MM: UEFI boot - memory descriptors will be processed by MiInitMachineDependent\n");
    
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
PVOID
NTAPI
MmMapUefiFramebuffer(VOID)
{
    PVOID MappedAddress;
    
    /* Check if we have framebuffer info */
    if (!MmUefiBootMode || MmUefiFramebufferBase.QuadPart == 0)
    {
        DPRINT1("MM: No UEFI framebuffer to map\n");
        return NULL;
    }
    
    /* Map the framebuffer as non-cached device memory */
    MappedAddress = MmMapIoSpace(MmUefiFramebufferBase,
                                  MmUefiFramebufferSize,
                                  MmNonCached);
    
    if (MappedAddress)
    {
        DPRINT("MM: UEFI framebuffer mapped at %p\n", MappedAddress);
    }
    else
    {
        DPRINT1("MM: Failed to map UEFI framebuffer\n");
    }
    
    return MappedAddress;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeUefiMemoryMap(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG DescriptorCount = 0;
    
    /* Count memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        DescriptorCount++;
    }
    
    DPRINT("MM: Processing %lu memory descriptors for UEFI\n", DescriptorCount);
    
    /* Process each descriptor */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        MdBlock = CONTAINING_RECORD(ListEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        
        /* Handle UEFI-specific memory types */
        switch (MdBlock->MemoryType)
        {
            case LoaderFirmwarePermanent:
                /* UEFI runtime services memory */
                DPRINT("MM: UEFI Runtime Services at 0x%lx, %lu pages\n",
                       MdBlock->BasePage, MdBlock->PageCount);
                break;
                
            case LoaderSpecialMemory:
                /* UEFI ACPI or other special memory */
                DPRINT("MM: UEFI Special Memory at 0x%lx, %lu pages\n",
                       MdBlock->BasePage, MdBlock->PageCount);
                break;
                
            default:
                /* Standard memory type */
                break;
        }
    }
    
    return STATUS_SUCCESS;
}