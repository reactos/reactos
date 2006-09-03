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
static CHAR KeLoaderModuleStrings[64][256];
PLOADER_MODULE CachedModules[MaximumCachedModuleType];

/* FreeLDR Memory Data */
ADDRESS_RANGE KeMemoryMap[64];
ULONG KeMemoryMapRangeCount;
ULONG_PTR FirstKrnlPhysAddr;
ULONG_PTR LastKrnlPhysAddr;
ULONG_PTR LastKernelAddress;

/* FreeLDR Loader Data */
ROS_LOADER_PARAMETER_BLOCK KeLoaderBlock;
static CHAR KeLoaderCommandLine[256];

/* FreeLDR PE Hack Data */
extern unsigned int _image_base__;
ULONG_PTR KERNEL_BASE = (ULONG_PTR)&_image_base__;
extern LDR_DATA_TABLE_ENTRY HalModuleObject;

/* FUNCTIONS *****************************************************************/

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
    CHAR* s;

    /* Load the GDT and IDT */
    Ke386SetGlobalDescriptorTable(KiGdtDescriptor);
    Ke386SetInterruptDescriptorTable(KiIdtDescriptor);

    /* Copy the Loader Block Data locally since Low-Memory will be wiped */
    memcpy(&KeLoaderBlock, LoaderBlock, sizeof(ROS_LOADER_PARAMETER_BLOCK));
    memcpy(&KeLoaderModules[1],
           (PVOID)KeLoaderBlock.ModsAddr,
           sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
    KeLoaderBlock.ModsCount++;
    KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

    /* Check for BIOS memory map */
    KeMemoryMapRangeCount = 0;
    if (KeLoaderBlock.Flags & MB_FLAGS_MMAP_INFO)
    {
        /* We have a memory map from the nice BIOS */
        size = *((PULONG)(KeLoaderBlock.MmapAddr - sizeof(ULONG)));
        i = 0;

        /* Map it until we run out of size */
        while (i < KeLoaderBlock.MmapLength)
        {
            /* Copy into the Kernel Memory Map */
            memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
                    (PVOID)(KeLoaderBlock.MmapAddr + i),
                    sizeof(ADDRESS_RANGE));

            /* Increase Memory Map Count */
            KeMemoryMapRangeCount++;

            /* Increase Size */
            i += size;
        }

        /* Save data */
        KeLoaderBlock.MmapLength = KeMemoryMapRangeCount *
                                   sizeof(ADDRESS_RANGE);
        KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }
    else
    {
        /* Nothing from BIOS */
        KeLoaderBlock.MmapLength = 0;
        KeLoaderBlock.MmapAddr = (ULONG)KeMemoryMap;
    }

    /* Save the Base Address */
    MmSystemRangeStart = (PVOID)KeLoaderBlock.KernelBase;

    /* Set the Command Line */
    strcpy(KeLoaderCommandLine, (PCHAR)LoaderBlock->CommandLine);
    KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;

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
    for (i = 1; i < KeLoaderBlock.ModsCount; i++)
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
    LastKernelAddress = PAGE_ROUND_UP(KeLoaderModules[KeLoaderBlock.
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

    /* Do general System Startup */
    KiSystemStartup(LoaderBlock);
}

/* EOF */
