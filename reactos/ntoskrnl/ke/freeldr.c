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
ADDRESS_RANGE KeMemoryMap[64];
ULONG KeMemoryMapRangeCount;

/* NT Loader Module/Descriptor Count */
ULONG BldrCurrentMd;
ULONG BldrCurrentMod;

/* NT Loader Data. Eats up about 80KB! */
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
WCHAR BldrModuleStringsFull[64][260];                   // 0x9BDC
NLS_DATA_BLOCK BldrNlsDataBlock;                        // 0x11DDC
SETUP_LOADER_BLOCK BldrSetupBlock;                      // 0x11DE8
ARC_DISK_INFORMATION BldrArcDiskInfo;                   // 0x12134
CHAR BldrArcNames[32][256];                             // 0x1213C
ARC_DISK_SIGNATURE BldrDiskInfo[32];                    // 0x1413C
                                                        // 0x1443C

/* FUNCTIONS *****************************************************************/

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
KiRosGetMdFromArray(VOID)
{
    /* Return the next MD from the list, but make sure we don't overflow */
    if (BldrCurrentMd > 64) KEBUGCHECK(0);
    return &BldrMemoryDescriptors[BldrCurrentMd++];
}

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
    WCHAR PathToDrivers[] = L"\\SystemRoot\\System32\\drivers\\";
    WCHAR PathToSystem32[] = L"\\SystemRoot\\System32\\";
    CHAR DriverNameLow[256];

    /* First get some kernel-loader globals */
    AcpiTableDetected = (RosLoaderBlock->Flags & MB_FLAGS_ACPI_TABLE) ? TRUE : FALSE;
    MmFreeLdrMemHigher = RosLoaderBlock->MemHigher;
    MmFreeLdrPageDirectoryEnd = RosLoaderBlock->PageDirectoryEnd;
    if (!MmFreeLdrPageDirectoryEnd) MmFreeLdrPageDirectoryEnd = 0x40000;

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

    /* Create one large blob of free memory */
    MdEntry = KiRosGetMdFromArray();
    MdEntry->MemoryType = LoaderFree;
    MdEntry->BasePage = 0;
    MdEntry->PageCount = MmFreeLdrMemHigher / 4;
    InsertTailList(&LoaderBlock->MemoryDescriptorListHead, &MdEntry->ListEntry);

    /*
     * FIXME: Instead of just "inserting" MDs into the list,
     * we need to make then be "consumed" from the Free Descriptor.
     * This will happen soon (and also ensure a sorted list).
     */

    /* Loop the BIOS Memory Map */
    for (j = 0; j < KeMemoryMapRangeCount; j++)
    {
        /* Check if this is a reserved entry */
        if (KeMemoryMap[j].Type == 2)
        {
            /* It is, build an entry for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderSpecialMemory;
            MdEntry->BasePage = KeMemoryMap[j].BaseAddrLow >> PAGE_SHIFT;
            MdEntry->PageCount = (KeMemoryMap[j].LengthLow + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }
    }

    /* Page 0 is reserved: build an entry for it */
    MdEntry = KiRosGetMdFromArray();
    MdEntry->MemoryType = LoaderFirmwarePermanent;
    MdEntry->BasePage = 0;
    MdEntry->PageCount = 1;
    InsertTailList(&LoaderBlock->MemoryDescriptorListHead, &MdEntry->ListEntry);

    /* Build an entry for the KPCR (which we put in page 1) */
    MdEntry = KiRosGetMdFromArray();
    MdEntry->MemoryType = LoaderMemoryData;
    MdEntry->BasePage = 1;
    MdEntry->PageCount = 1;
    InsertTailList(&LoaderBlock->MemoryDescriptorListHead, &MdEntry->ListEntry);

    /* Build an entry for the PDE */
    MdEntry = KiRosGetMdFromArray();
    MdEntry->MemoryType = LoaderMemoryData;
    MdEntry->BasePage = (ULONG_PTR)MmGetPageDirectory() >> PAGE_SHIFT;
    MdEntry->PageCount = (MmFreeLdrPageDirectoryEnd -
                          (ULONG_PTR)MmGetPageDirectory()) / PAGE_SIZE;
    InsertTailList(&LoaderBlock->MemoryDescriptorListHead, &MdEntry->ListEntry);

    /* Mark Video ROM as reserved */
    MdEntry = KiRosGetMdFromArray();
    MdEntry->MemoryType = LoaderFirmwarePermanent;
    MdEntry->BasePage = 0xA0000 >> PAGE_SHIFT;
    MdEntry->PageCount = (0xE8000 - 0xA0000) / PAGE_SIZE;
    InsertTailList(&LoaderBlock->MemoryDescriptorListHead, &MdEntry->ListEntry);

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
            ModStart = (PVOID)((ULONG_PTR)ModStart + (KSEG0_BASE - 0x200000));
            LoaderBlock->NlsData->AnsiCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }
        else if (!_stricmp(DriverName, "oem.nls"))
        {
            /* OEM Code page */
            ModStart = (PVOID)((ULONG_PTR)ModStart + (KSEG0_BASE - 0x200000));
            LoaderBlock->NlsData->OemCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }
        else if (!_stricmp(DriverName, "casemap.nls"))
        {
            /* Unicode Code page */
            ModStart = (PVOID)((ULONG_PTR)ModStart + (KSEG0_BASE - 0x200000));
            LoaderBlock->NlsData->UnicodeCodePageData = ModStart;

            /* Create an MD for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderNlsData;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the SYSTEM hive */
        if (!(_stricmp(DriverName, "system")) ||
            !(_stricmp(DriverName, "system.hiv")))
        {
            /* Save registry data */
            ModStart = (PVOID)((ULONG_PTR)ModStart + (KSEG0_BASE - 0x200000));
            LoaderBlock->RegistryBase = ModStart;
            LoaderBlock->RegistryLength = ModSize;

            /* Disable setup mode */
            LoaderBlock->SetupLdrBlock = NULL;

            /* Create an MD for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderRegistryData;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the HARDWARE hive */
        if (!(_stricmp(DriverName, "hardware")) ||
            !(_stricmp(DriverName, "hardware.hiv")))
        {
            /* Create an MD for it */
            ModStart = (PVOID)((ULONG_PTR)ModStart + (KSEG0_BASE - 0x200000));
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderRegistryData;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
            continue;
        }

        /* Check if this is the kernel */
        if (!(_stricmp(DriverName, "ntoskrnl.exe")))
        {
            /* Create an MD for it */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderSystemCode;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }
        else if (!(_stricmp(DriverName, "hal.dll")))
        {
            /* Create an MD for the HAL */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderHalCode;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }
        else
        {
            /* Create an MD for any driver */
            MdEntry = KiRosGetMdFromArray();
            MdEntry->MemoryType = LoaderBootDriver;
            MdEntry->BasePage = ((ULONG_PTR)ModStart &~ KSEG0_BASE) >> PAGE_SHIFT;
            MdEntry->PageCount = (ModSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
            InsertTailList(&LoaderBlock->MemoryDescriptorListHead,
                           &MdEntry->ListEntry);
        }

        /* Lowercase the drivername so we can check its extension later */
        strcpy(DriverNameLow, DriverName);
        _strlwr(DriverNameLow);

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

        /* Construct a correct full name */
        BldrModuleStringsFull[i][0] = 0;
        LdrEntry->FullDllName.MaximumLength = 260 * sizeof(WCHAR);
        LdrEntry->FullDllName.Length = 0;
        LdrEntry->FullDllName.Buffer = BldrModuleStringsFull[i];

        /* Guess the path */
        if (strstr(DriverNameLow, ".dll") || strstr(DriverNameLow, ".exe"))
        {
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&TempString, PathToSystem32);
            RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &TempString);
        }
        else /* .sys */
        {
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&TempString, PathToDrivers);
            RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &TempString);
        }

        /* Append base name of the driver */
        RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &LdrEntry->BaseDllName);

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
    PLOADER_PARAMETER_BLOCK NtLoaderBlock;
    ULONG size, i;
#if defined(_M_IX86)
    PKTSS Tss;
    PKGDTENTRY TssEntry;

    /* Load the GDT and IDT */
    Ke386SetGlobalDescriptorTable(*(PKDESCRIPTOR)&KiGdtDescriptor.Limit);
    Ke386SetInterruptDescriptorTable(*(PKDESCRIPTOR)&KiIdtDescriptor.Limit);

    /* Initialize the boot TSS */
    Tss = &KiBootTss;
    TssEntry = &KiBootGdt[KGDT_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);
#endif

    /* Save pointer to ROS Block */
    KeRosLoaderBlock = LoaderBlock;

    /* Save memory manager data */
    MmFreeLdrLastKernelAddress = PAGE_ROUND_UP(KeRosLoaderBlock->
                                               ModsAddr[KeRosLoaderBlock->
                                                        ModsCount - 1].
                                                ModEnd);
    MmFreeLdrFirstKrnlPhysAddr = KeRosLoaderBlock->ModsAddr[0].ModStart -
                                 KSEG0_BASE + 0x200000;
    MmFreeLdrLastKrnlPhysAddr = MmFreeLdrLastKernelAddress -
                                KSEG0_BASE + 0x200000;

    KeMemoryMapRangeCount = 0;
    if (LoaderBlock->Flags & MB_FLAGS_MMAP_INFO)
    {
        /* We have a memory map from the nice BIOS */
        size = *((PULONG)(LoaderBlock->MmapAddr - sizeof(ULONG)));
        i = 0;

        /* Map it until we run out of size */
        while (i < LoaderBlock->MmapLength)
        {
            /* Copy into the Kernel Memory Map */
            memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
                    (PVOID)(LoaderBlock->MmapAddr + i),
                    sizeof(ADDRESS_RANGE));

            /* Increase Memory Map Count */
            KeMemoryMapRangeCount++;

            /* Increase Size */
            i += size;
        }

        /* Save data */
        LoaderBlock->MmapLength = KeMemoryMapRangeCount * sizeof(ADDRESS_RANGE);
        LoaderBlock->MmapAddr = (ULONG)KeMemoryMap;
    }
    else
    {
        /* Nothing from BIOS */
        LoaderBlock->MmapLength = 0;
        LoaderBlock->MmapAddr = (ULONG)KeMemoryMap;
    }

#if defined(_M_IX86)
    /* Set up the VDM Data */
    NtEarlyInitVdm();
#endif

    /* Convert the loader block */
    KiRosFrldrLpbToNtLpb(KeRosLoaderBlock, &NtLoaderBlock);

    /* Do general System Startup */
    KiSystemStartup(NtLoaderBlock);
}

/* EOF */
