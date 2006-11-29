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

/* FreeLDR Memory Data */
ULONG_PTR MmFreeLdrFirstKrnlPhysAddr, MmFreeLdrLastKrnlPhysAddr;
ULONG_PTR MmFreeLdrLastKernelAddress;
ULONG MmFreeLdrMemHigher;
ULONG MmFreeLdrPageDirectoryEnd;

/* FreeLDR Loader Data */
PROS_LOADER_PARAMETER_BLOCK KeRosLoaderBlock;
BOOLEAN AcpiTableDetected;

/* NT Loader Data. Eats up about 50KB! */
LOADER_PARAMETER_BLOCK BldrLoaderBlock;                 // 0x0000
LOADER_PARAMETER_EXTENSION BldrExtensionBlock;          // 0x0060
CHAR BldrCommandLine[256];                              // 0x00DC
CHAR BldrArcBootPath[64];                               // 0x01DC
CHAR BldrArcHalPath[64];                                // 0x021C
CHAR BldrNtHalPath[64];                                 // 0x025C
CHAR BldrNtBootPath[64];                                // 0x029C
LDR_DATA_TABLE_ENTRY BldrModules[64];                   // 0x02DC
MEMORY_ALLOCATION_DESCRIPTOR BldrMemoryDescriptors[64]; // 0x14DC
WCHAR BldrModuleStrings[64][260];                       // 0x19DC
NLS_DATA_BLOCK BldrNlsDataBlock;                        // 0x9BDC
SETUP_LOADER_BLOCK BldrSetupBlock;                      // 0x9BE8
ARC_DISK_INFORMATION BldrArcDiskInfo;                   // 0x9F34
CHAR BldrArcNames[32][256];                             // 0x9F3C
ARC_DISK_SIGNATURE BldrDiskInfo[32];                    // 0xBF3C
                                                        // 0xC23C
ULONG Guard = 0xCACA1234;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiRosFrldrLpbToNtLpb(IN PROS_LOADER_PARAMETER_BLOCK RosLoaderBlock,
                     IN PLOADER_PARAMETER_BLOCK *NtLoaderBlock)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdEntry;
    PLOADER_MODULE RosEntry;
    ULONG i, j, ModSize;
    PVOID ModStart;
    PCHAR DriverName;
    PCHAR BootPath, HalPath;
    CHAR CommandLine[256];
    PARC_DISK_SIGNATURE RosDiskInfo, ArcDiskInfo;
    PIMAGE_NT_HEADERS NtHeader;

    /* First get some kernel-loader globals */
    AcpiTableDetected = (RosLoaderBlock->Flags & MB_FLAGS_ACPI_TABLE) ? TRUE : FALSE;
    MmFreeLdrMemHigher = RosLoaderBlock->MemHigher;
    MmFreeLdrPageDirectoryEnd = RosLoaderBlock->PageDirectoryEnd;

    /* Set the NT Loader block and initialize it */
    *NtLoaderBlock = LoaderBlock = &BldrLoaderBlock;
    RtlZeroMemory(LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

    /* Set the NLS Data block */
    LoaderBlock->NlsData = &BldrNlsDataBlock;

    /* Set the ARC Data block */
    LoaderBlock->ArcDiskInformation = &BldrArcDiskInfo;

    /* Assume this is from FreeLDR's SetupLdr */
    LoaderBlock->SetupLdrBlock = &BldrSetupBlock;

    /* Setup the list heads */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    /* Loop boot driver list */
    for (i = 0; i < RosLoaderBlock->ModsCount; i++)
    {
        /* Get the ROS loader entry */
        RosEntry = &RosLoaderBlock->ModsAddr[i];
        DriverName = (PCHAR)RosEntry->String;
        ModStart = (PVOID)RosEntry->ModStart;
        ModSize = RosEntry->ModEnd - (ULONG_PTR)ModStart;

        /* Check if this is any of the NLS files */
        if (!_stricmp(DriverName, "ansi.nls"))
        {
            /* ANSI Code page */
            LoaderBlock->NlsData->AnsiCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }
        else if (!_stricmp(DriverName, "oem.nls"))
        {
            /* OEM Code page */
            LoaderBlock->NlsData->OemCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }
        else if (!_stricmp(DriverName, "casemap.nls"))
        {
            /* Unicode Code page */
            LoaderBlock->NlsData->UnicodeCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the SYSTEM hive */
        if (!(_stricmp(DriverName, "system")) ||
            !(_stricmp(DriverName, "system.hiv")))
        {
            /* Save registry data */
            LoaderBlock->RegistryBase = ModStart;
            LoaderBlock->RegistryLength = ModSize;

            /* Disable setup mode */
            LoaderBlock->SetupLdrBlock = NULL;

            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderRegistryData;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the HARDWARE hive */
        if (!(_stricmp(DriverName, "hardware")) ||
            !(_stricmp(DriverName, "hardware.hiv")))
        {
            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderRegistryData;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the kernel */
        if (!(_stricmp(DriverName, "ntoskrnl.exe")))
        {
            /* Create an MD for it */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderSystemCode;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }
        else if (!(_stricmp(DriverName, "hal.dll")))
        {
            /* Create an MD for the HAL */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderHalCode;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }
        else
        {
            /* Create an MD for any driver */
            MdEntry = &BldrMemoryDescriptors[i];
            MdEntry->MemoryType = LoaderBootDriver;
            MdEntry->BasePage = (ULONG_PTR)ModStart >> PAGE_SHIFT;
            MdEntry->PageCount = ModSize >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }

        /* Setup the loader entry */
        LdrEntry = &BldrModules[i];
        RtlZeroMemory(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY));

        /* Convert driver name from ANSI to Unicode */
        for (j = 0; j < strlen(DriverName); j++)
        {
            BldrModuleStrings[i][j] = DriverName[j];
        }

        /* Setup driver name */
        RtlInitUnicodeString(&LdrEntry->BaseDllName, BldrModuleStrings[i]);
        RtlInitUnicodeString(&LdrEntry->FullDllName, BldrModuleStrings[i]);

        /* Copy data from Freeldr Module Entry */
        LdrEntry->DllBase = ModStart;
        LdrEntry->SizeOfImage = ModSize;

        /* Copy additional data */
        NtHeader = RtlImageNtHeader(ModStart);
        LdrEntry->EntryPoint = RVA(ModStart,
                                   NtHeader->
                                   OptionalHeader.AddressOfEntryPoint);

        /* Initialize other data */
        LdrEntry->LoadCount = 1;
        LdrEntry->Flags = LDRP_IMAGE_DLL |
                          LDRP_ENTRY_PROCESSED;
        if (RosEntry->Reserved) LdrEntry->Flags |= LDRP_ENTRY_INSERTED;

        /* Insert it into the loader block */
        InsertTailList(&LoaderBlock->LoadOrderListHead,
                       &LdrEntry->InLoadOrderLinks);
    }

    /* Setup command line */
    LoaderBlock->LoadOptions = BldrCommandLine;
    strcpy(BldrCommandLine, RosLoaderBlock->CommandLine);

    /* Setup the extension block */
    LoaderBlock->Extension = &BldrExtensionBlock;
    LoaderBlock->Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    LoaderBlock->Extension->MajorVersion = 5;
    LoaderBlock->Extension->MinorVersion = 2;

    /* Now setup the setup block if we have one */
    if (LoaderBlock->SetupLdrBlock)
    {
        /* All we'll setup right now is the flag for text-mode setup */
        LoaderBlock->SetupLdrBlock->Flags = 1;
    }

    /* Make a copy of the command line */
    strcpy(CommandLine, LoaderBlock->LoadOptions);

    /* Find the first \, separating the ARC path from NT path */
    BootPath = strchr(CommandLine, '\\');
    *BootPath = ANSI_NULL;
    strncpy(BldrArcBootPath, CommandLine, 63);
    LoaderBlock->ArcBootDeviceName = BldrArcBootPath;

    /* The rest of the string is the NT path */
    HalPath = strchr(BootPath + 1, ' ');
    *HalPath = ANSI_NULL;
    BldrNtBootPath[0] = '\\';
    strncat(BldrNtBootPath, BootPath + 1, 63);
    strcat(BldrNtBootPath,"\\");
    LoaderBlock->NtBootPathName = BldrNtBootPath;

    /* Set the HAL paths */
    strncpy(BldrArcHalPath, BldrArcBootPath, 63);
    LoaderBlock->ArcHalDeviceName = BldrArcHalPath;
    strcpy(BldrNtHalPath, "\\");
    LoaderBlock->NtHalPathName = BldrNtHalPath;

    /* Use this new command line */
    strncpy(LoaderBlock->LoadOptions, HalPath + 2, 255);

    /* Parse it and change every slash to a space */
    BootPath = LoaderBlock->LoadOptions;
    do {if (*BootPath == '/') *BootPath = ' ';} while (*BootPath++);

    /* Now let's loop ARC disk information */
    for (i = 0; i < RosLoaderBlock->DrivesCount; i++)
    {
        /* Get the ROS loader entry */
        RosDiskInfo = &RosLoaderBlock->DrivesAddr[i];

        /* Get the ARC structure */
        ArcDiskInfo = &BldrDiskInfo[i];

        /* Copy the data over */
        ArcDiskInfo->Signature = RosDiskInfo->Signature;
        ArcDiskInfo->CheckSum = RosDiskInfo->CheckSum;

        /* Copy the ARC Name */
        strcpy(BldrArcNames[i], RosDiskInfo->ArcName);
        ArcDiskInfo->ArcName = BldrArcNames[i];

        /* Insert into the list */
        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                       &ArcDiskInfo->ListEntry);
    }
}

VOID
FASTCALL
KiRosPrepareForSystemStartup(IN ULONG Dummy,
                             IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i;
    PLOADER_PARAMETER_BLOCK NtLoaderBlock;
    PKTSS Tss;
    PKGDTENTRY TssEntry;

    /* Load the GDT and IDT */
    Ke386SetGlobalDescriptorTable(KiGdtDescriptor);
    Ke386SetInterruptDescriptorTable(KiIdtDescriptor);

    /* Initialize the boot TSS */
    Tss = &KiBootTss;
    TssEntry = &KiBootGdt[KGDT_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);

    /* Save pointer to ROS Block */
    KeRosLoaderBlock = LoaderBlock;

    /* Save the Base Address */
    MmSystemRangeStart = (PVOID)KeRosLoaderBlock->KernelBase;

    /* Convert every driver address to virtual memory */
    for (i = 2; i < KeRosLoaderBlock->ModsCount; i++)
    {
        /* Subtract the base Address in Physical Memory */
        KeRosLoaderBlock->ModsAddr[i].ModStart -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeRosLoaderBlock->ModsAddr[i].ModStart += KSEG0_BASE;

        /* Subtract the base Address in Physical Memory */
        KeRosLoaderBlock->ModsAddr[i].ModEnd -= 0x200000;

        /* Add the Kernel Base Address in Virtual Memory */
        KeRosLoaderBlock->ModsAddr[i].ModEnd += KSEG0_BASE;
    }

    /* Save memory manager data */
    MmFreeLdrLastKernelAddress = PAGE_ROUND_UP(KeRosLoaderBlock->
                                               ModsAddr[KeRosLoaderBlock->
                                                        ModsCount - 1].
                                                ModEnd);
    MmFreeLdrFirstKrnlPhysAddr = KeRosLoaderBlock->ModsAddr[0].ModStart -
                                 KSEG0_BASE + 0x200000;
    MmFreeLdrLastKrnlPhysAddr = MmFreeLdrLastKernelAddress -
                                KSEG0_BASE + 0x200000;

    /* Set up the VDM Data */
    NtEarlyInitVdm();

    /* Convert the loader block */
    KiRosFrldrLpbToNtLpb(KeRosLoaderBlock, &NtLoaderBlock);

    /* Do general System Startup */
    KiSystemStartup(NtLoaderBlock);
}

/* EOF */
