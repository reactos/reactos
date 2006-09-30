/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/freeldr.c
 * PURPOSE:         FreeLDR Bootstrap Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FreeLDR Module Data */
LOADER_MODULE KeLoaderModules[64];
ULONG KeLoaderModuleCount;
static CHAR KeLoaderModuleStrings[64][256];
PLOADER_MODULE CachedModules[MaximumCachedModuleType];

/* FreeLDR Memory Data */
ADDRESS_RANGE KeMemoryMap[64];
ULONG KeMemoryMapRangeCount;
ULONG_PTR FirstKrnlPhysAddr;
ULONG_PTR LastKrnlPhysAddr;
ULONG_PTR LastKernelAddress;
ULONG MmFreeLdrMemHigher, MmFreeLdrMemLower;
ULONG MmFreeLdrPageDirectoryStart, MmFreeLdrPageDirectoryEnd;

/* FreeLDR Loader Data */
ROS_LOADER_PARAMETER_BLOCK KeRosLoaderBlock;
static CHAR KeLoaderCommandLine[256];
BOOLEAN AcpiTableDetected;

/* FreeLDR PE Hack Data */
extern unsigned int _image_base__;
ULONG_PTR KERNEL_BASE = (ULONG_PTR)&_image_base__;
extern LDR_DATA_TABLE_ENTRY HalModuleObject;

/* NT Loader Data */
LOADER_PARAMETER_BLOCK BldrLoaderBlock;
CHAR BldrCommandLine[256];

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiRosFrldrLpbToNtLpb(IN PROS_LOADER_PARAMETER_BLOCK RosLoaderBlock,
                     IN PLOADER_PARAMETER_BLOCK *NtLoaderBlock)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;

    /* First get some kernel-loader globals */
    AcpiTableDetected = (RosLoaderBlock->Flags & MB_FLAGS_ACPI_TABLE) ? TRUE : FALSE;
    MmFreeLdrMemHigher = RosLoaderBlock->MemHigher;
    MmFreeLdrMemLower = RosLoaderBlock->MemLower;
    MmFreeLdrPageDirectoryStart = RosLoaderBlock->PageDirectoryStart;
    MmFreeLdrPageDirectoryEnd = RosLoaderBlock->PageDirectoryEnd;
    KeLoaderModuleCount = RosLoaderBlock->ModsCount;

    /* Set the NT Loader block and initialize it */
    *NtLoaderBlock = LoaderBlock = &BldrLoaderBlock;
    RtlZeroMemory(LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

    /* Setup the list heads */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);

    /* Setup command line */
    LoaderBlock->LoadOptions = BldrCommandLine;
    strcpy(BldrCommandLine, KeLoaderCommandLine);
}

VOID
NTAPI
KiRosPrepareForSystemStartup(IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i;
    ULONG size;
    ULONG HalBase;
    ULONG DriverBase;
    ULONG DriverSize;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_OPTIONAL_HEADER OptHead;
    PLOADER_PARAMETER_BLOCK NtLoaderBlock;
    CHAR* s;

    /* Load the GDT and IDT */
    Ke386SetGlobalDescriptorTable(KiGdtDescriptor);
    Ke386SetInterruptDescriptorTable(KiIdtDescriptor);

    /* Copy the Loader Block Data locally since Low-Memory will be wiped */
    memcpy(&KeRosLoaderBlock, LoaderBlock, sizeof(ROS_LOADER_PARAMETER_BLOCK));
    memcpy(&KeLoaderModules[1],
           (PVOID)KeRosLoaderBlock.ModsAddr,
           sizeof(LOADER_MODULE) * KeRosLoaderBlock.ModsCount);
    KeRosLoaderBlock.ModsCount++;
    KeRosLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

    /* Check for BIOS memory map */
    KeMemoryMapRangeCount = 0;
    if (KeRosLoaderBlock.Flags & MB_FLAGS_MMAP_INFO)
    {
        /* We have a memory map from the nice BIOS */
        size = *((PULONG)(KeRosLoaderBlock.MmapAddr - sizeof(ULONG)));
        i = 0;

        /* Map it until we run out of size */
        while (i < KeRosLoaderBlock.MmapLength)
        {
            /* Copy into the Kernel Memory Map */
            memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
                    (PVOID)(KeRosLoaderBlock.MmapAddr + i),
                    sizeof(ADDRESS_RANGE));

            /* Increase Memory Map Count */
            KeMemoryMapRangeCount++;

            /* Increase Size */
            i += size;
        }

        /* Save data */
        KeRosLoaderBlock.MmapLength = KeMemoryMapRangeCount *
                                   sizeof(ADDRESS_RANGE);
        KeRosLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }
    else
    {
        /* Nothing from BIOS */
        KeRosLoaderBlock.MmapLength = 0;
        KeRosLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }

    /* Save the Base Address */
    MmSystemRangeStart = (PVOID)KeRosLoaderBlock.KernelBase;

    /* Set the Command Line */
    strcpy(KeLoaderCommandLine, (PCHAR)LoaderBlock->CommandLine);
    KeRosLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;

    /* Write the first Module (the Kernel) */
    strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
    KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
    KeLoaderModules[0].ModStart = KERNEL_BASE;

    /* Read PE Data */
    NtHeader = RtlImageNtHeader((PVOID)KeLoaderModules[0].ModStart);
    OptHead = &NtHeader->OptionalHeader;

    /* Set Kernel Ending */
    KeLoaderModules[0].ModEnd = KeLoaderModules[0].ModStart +
                                PAGE_ROUND_UP((ULONG)OptHead->SizeOfImage);

    /* Create a block for each module */
    for (i = 1; i < KeRosLoaderBlock.ModsCount; i++)
    {
        /* Check if we have to copy the path or not */
        if ((s = strrchr((PCHAR)KeLoaderModules[i].String, '/')) != 0)
        {
            strcpy(KeLoaderModuleStrings[i], s + 1);
        }
        else
        {
            strcpy(KeLoaderModuleStrings[i], (PCHAR)KeLoaderModules[i].String);
        }

        /* Substract the base Address in Physical Memory */
        KeLoaderModules[i].ModStart -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeLoaderModules[i].ModStart += KERNEL_BASE;

        /* Substract the base Address in Physical Memory */
        KeLoaderModules[i].ModEnd -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeLoaderModules[i].ModEnd += KERNEL_BASE;

        /* Select the proper String */
        KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

    /* Choose last module address as the final kernel address */
    LastKernelAddress = PAGE_ROUND_UP(KeLoaderModules[KeRosLoaderBlock.
                                                      ModsCount - 1].ModEnd);

    /* Select the HAL Base */
    HalBase = KeLoaderModules[1].ModStart;

    /* Choose Driver Base */
    DriverBase = LastKernelAddress;
    LdrHalBase = (ULONG_PTR)DriverBase;

    /* Initialize Module Management */
    LdrInitModuleManagement();

    /* Load HAL.DLL with the PE Loader */
    LdrSafePEProcessModule((PVOID)HalBase,
                            (PVOID)DriverBase,
                            (PVOID)KERNEL_BASE,
                            &DriverSize);

    //
    //
    // HACK HACK HACK WHEN WILL YOU PEOPLE FIX FREELDR?!?!?!
    // FREELDR SENDS US AN ***INVALID*** HAL PE HEADER!!!
    // WE READ IT IN LdrInitModuleManagement ABOVE!!!
    // WE SET .SizeOfImage TO A *GARBAGE* VALUE!!!
    //
    // This dirty hack fixes it, and should make symbol lookup work too.
    //
    HalModuleObject.SizeOfImage =  RtlImageNtHeader((PVOID)HalModuleObject.
                                                    DllBase)->
                                                    OptionalHeader.SizeOfImage;

    /* Increase the last kernel address with the size of HAL */
    LastKernelAddress += PAGE_ROUND_UP(DriverSize);

    /* Now select the final beginning and ending Kernel Addresses */
    FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - KERNEL_BASE + 0x200000;
    LastKrnlPhysAddr = LastKernelAddress - KERNEL_BASE + 0x200000;

    /* Setup the IDT */
    KeInitExceptions(); // ONCE HACK BELOW IS GONE, MOVE TO KISYSTEMSTARTUP!
    KeInitInterrupts(); // ROS HACK DEPRECATED SOON BY NEW HAL

    /* Load the Kernel with the PE Loader */
    LdrSafePEProcessModule((PVOID)KERNEL_BASE,
                           (PVOID)KERNEL_BASE,
                           (PVOID)DriverBase,
                           &DriverSize);

    /* Convert the loader block */
    KiRosFrldrLpbToNtLpb(&KeRosLoaderBlock, &NtLoaderBlock);

    /* Do general System Startup */
    KiSystemStartup(NtLoaderBlock);
}

/* EOF */
