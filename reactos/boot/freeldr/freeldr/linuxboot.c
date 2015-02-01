/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _M_ARM

#include <freeldr.h>
#include <debug.h>

#ifdef _M_IX86

#define    LINUX_READ_CHUNK_SIZE    0x20000     // Read 128k at a time

DBG_DEFAULT_CHANNEL(LINUX);

PLINUX_BOOTSECTOR    LinuxBootSector = NULL;
PLINUX_SETUPSECTOR    LinuxSetupSector = NULL;
ULONG            SetupSectorSize = 0;
BOOLEAN            NewStyleLinuxKernel = FALSE;
ULONG            LinuxKernelSize = 0;
ULONG            LinuxInitrdSize = 0;
CHAR            LinuxKernelName[260];
CHAR            LinuxInitrdName[260];
BOOLEAN            LinuxHasInitrd = FALSE;
CHAR            LinuxCommandLine[260] = "";
ULONG            LinuxCommandLineSize = 0;
PVOID            LinuxKernelLoadAddress = NULL;
PVOID            LinuxInitrdLoadAddress = NULL;
CHAR            LinuxBootDescription[80];
CHAR            LinuxBootPath[260] = "";

BOOLEAN RemoveQuotes(PCHAR QuotedString)
{
    CHAR    TempString[200];
    PCHAR p;
    PSTR Start;

    //
    // Skip spaces up to "
    //
    p = QuotedString;
    while (*p == ' ' || *p == '"')
        p++;
    Start = p;

    //
    // Go up to next "
    //
    while (*p != '"' && *p != ANSI_NULL)
        p++;
    *p = ANSI_NULL;

    //
    // Copy result
    //
    strcpy(TempString, Start);
    strcpy(QuotedString, TempString);

    return TRUE;
}

VOID
LoadAndBootLinux(IN OperatingSystemItem* OperatingSystem,
                 IN USHORT OperatingSystemVersion)
{
    PCSTR    SectionName = OperatingSystem->SystemPartition;
    PCSTR    Description = OperatingSystem->LoadIdentifier;
    PFILE    LinuxKernel = 0;
    PFILE    LinuxInitrdFile = 0;
    CHAR    TempString[260];

    UiDrawBackdrop();

    if (Description)
    {
        sprintf(LinuxBootDescription, "Loading %s...", Description);
    }
    else
    {
        strcpy(LinuxBootDescription, "Loading Linux...");
    }

    UiDrawStatusText(LinuxBootDescription);
    UiDrawProgressBarCenter(0, 100, LinuxBootDescription);

    // Parse the .ini file section
    if (!LinuxParseIniSection(SectionName))
    {
        goto LinuxBootFailed;
    }

    // Open the kernel
    LinuxKernel = FsOpenFile(LinuxKernelName);
    if (!LinuxKernel)
    {
        sprintf(TempString, "Linux kernel \'%s\' not found.", LinuxKernelName);
        UiMessageBox(TempString);
        goto LinuxBootFailed;
    }

    // Open the initrd file image (if necessary)
    if (LinuxHasInitrd)
    {
        LinuxInitrdFile = FsOpenFile(LinuxInitrdName);
        if (!LinuxInitrdFile)
        {
            sprintf(TempString, "Linux initrd image \'%s\' not found.", LinuxInitrdName);
            UiMessageBox(TempString);
            goto LinuxBootFailed;
        }
    }

    // Read the boot sector
    if (!LinuxReadBootSector(LinuxKernel))
    {
        goto LinuxBootFailed;
    }

    // Read the setup sector
    if (!LinuxReadSetupSector(LinuxKernel))
    {
        goto LinuxBootFailed;
    }

    // Calc kernel size
    LinuxKernelSize = FsGetFileSize(LinuxKernel) - (512 + SetupSectorSize);

    // Get the file size
    LinuxInitrdSize = FsGetFileSize(LinuxInitrdFile);

    // Read the kernel
    if (!LinuxReadKernel(LinuxKernel))
    {
        goto LinuxBootFailed;
    }

    // Read the initrd (if necessary)
    if (LinuxHasInitrd)
    {
        if (!LinuxReadInitrd(LinuxInitrdFile))
        {
            goto LinuxBootFailed;
        }
    }

    // If the default root device is set to FLOPPY (0000h), change to /dev/fd0 (0200h)
    if (LinuxBootSector->RootDevice == 0x0000)
    {
        LinuxBootSector->RootDevice = 0x0200;
    }

    if (LinuxSetupSector->Version >= 0x0202)
    {
        LinuxSetupSector->CommandLinePointer = 0x99000;
    }
    else
    {
        LinuxBootSector->CommandLineMagic = LINUX_COMMAND_LINE_MAGIC;
        LinuxBootSector->CommandLineOffset = 0x9000;
    }

    if (NewStyleLinuxKernel)
    {
        LinuxSetupSector->TypeOfLoader = LINUX_LOADER_TYPE_FREELOADER;
    }
    else
    {
        LinuxSetupSector->LoadFlags = 0;
    }

    RtlCopyMemory((PVOID)0x90000, LinuxBootSector, 512);
    RtlCopyMemory((PVOID)0x90200, LinuxSetupSector, SetupSectorSize);
    RtlCopyMemory((PVOID)0x99000, LinuxCommandLine, LinuxCommandLineSize);

    UiUnInitialize("Booting Linux...");

    DiskStopFloppyMotor();

    if (LinuxSetupSector->LoadFlags & LINUX_FLAG_LOAD_HIGH)
    {
        BootNewLinuxKernel();
    }
    else
    {
        BootOldLinuxKernel(LinuxKernelSize);
    }


LinuxBootFailed:

    if (LinuxKernel)
    {
        FsCloseFile(LinuxKernel);
    }
    if (LinuxInitrdFile)
    {
        FsCloseFile(LinuxInitrdFile);
    }

    if (LinuxBootSector != NULL)
    {
        MmFreeMemory(LinuxBootSector);
    }
    if (LinuxSetupSector != NULL)
    {
        MmFreeMemory(LinuxSetupSector);
    }
    if (LinuxKernelLoadAddress != NULL)
    {
        MmFreeMemory(LinuxKernelLoadAddress);
    }
    if (LinuxInitrdLoadAddress != NULL)
    {
        MmFreeMemory(LinuxInitrdLoadAddress);
    }

    LinuxBootSector = NULL;
    LinuxSetupSector = NULL;
    LinuxKernelLoadAddress = NULL;
    LinuxInitrdLoadAddress = NULL;
    SetupSectorSize = 0;
    NewStyleLinuxKernel = FALSE;
    LinuxKernelSize = 0;
    LinuxHasInitrd = FALSE;
    strcpy(LinuxCommandLine, "");
    LinuxCommandLineSize = 0;
}

BOOLEAN LinuxParseIniSection(PCSTR SectionName)
{
    ULONG_PTR    SectionId;
    CHAR        SettingName[260];

    // Find all the message box settings and run them
    UiShowMessageBoxesInSection(SectionName);

    // Try to open the operating system section in the .ini file
    if (!IniOpenSection(SectionName, &SectionId))
    {
        sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", SectionName);
        UiMessageBox(SettingName);
        return FALSE;
    }

    if (!IniReadSettingByName(SectionId, "BootPath", LinuxBootPath, sizeof(LinuxBootPath)))
    {
        UiMessageBox("Boot path not specified for selected OS!");
        return FALSE;
    }

    // Get the kernel name
    if (!IniReadSettingByName(SectionId, "Kernel", LinuxKernelName, sizeof(LinuxKernelName)))
    {
        UiMessageBox("Linux kernel filename not specified for selected OS!");
        return FALSE;
    }

    // Get the initrd name
    if (IniReadSettingByName(SectionId, "Initrd", LinuxInitrdName, sizeof(LinuxInitrdName)))
    {
        LinuxHasInitrd = TRUE;
    }

    // Get the command line
    if (IniReadSettingByName(SectionId, "CommandLine", LinuxCommandLine, sizeof(LinuxCommandLine)))
    {
        RemoveQuotes(LinuxCommandLine);
        LinuxCommandLineSize = strlen(LinuxCommandLine) + 1;
    }

    return TRUE;
}

BOOLEAN LinuxReadBootSector(PFILE LinuxKernelFile)
{
    // Allocate memory for boot sector
    LinuxBootSector = MmAllocateMemoryWithType(512, LoaderSystemCode);
    if (LinuxBootSector == NULL)
    {
        return FALSE;
    }

    // Read linux boot sector
    FsSetFilePointer(LinuxKernelFile, 0);
    if (!FsReadFile(LinuxKernelFile, 512, NULL, LinuxBootSector))
    {
        return FALSE;
    }

    // Check for validity
    if (LinuxBootSector->BootFlag != LINUX_BOOT_SECTOR_MAGIC)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return FALSE;
    }

    // DbgDumpBuffer(DPRINT_LINUX, LinuxBootSector, 512);

    TRACE("SetupSectors: %d\n", LinuxBootSector->SetupSectors);
    TRACE("RootFlags: 0x%x\n", LinuxBootSector->RootFlags);
    TRACE("SystemSize: 0x%x\n", LinuxBootSector->SystemSize);
    TRACE("SwapDevice: 0x%x\n", LinuxBootSector->SwapDevice);
    TRACE("RamSize: 0x%x\n", LinuxBootSector->RamSize);
    TRACE("VideoMode: 0x%x\n", LinuxBootSector->VideoMode);
    TRACE("RootDevice: 0x%x\n", LinuxBootSector->RootDevice);
    TRACE("BootFlag: 0x%x\n", LinuxBootSector->BootFlag);

    return TRUE;
}

BOOLEAN LinuxReadSetupSector(PFILE LinuxKernelFile)
{
    UCHAR    TempLinuxSetupSector[512];

    LinuxSetupSector = (PLINUX_SETUPSECTOR)TempLinuxSetupSector;

    // Read first linux setup sector
    FsSetFilePointer(LinuxKernelFile, 512);
    if (!FsReadFile(LinuxKernelFile, 512, NULL, TempLinuxSetupSector))
    {
        return FALSE;
    }

    // Check the kernel version
    if (!LinuxCheckKernelVersion())
    {
        return FALSE;
    }

    if (NewStyleLinuxKernel)
    {
        SetupSectorSize = 512 * LinuxBootSector->SetupSectors;
    }
    else
    {
        SetupSectorSize = 4 * 512; // Always 4 setup sectors
    }

    // Allocate memory for setup sectors
    LinuxSetupSector = MmAllocateMemoryWithType(SetupSectorSize, LoaderSystemCode);
    if (LinuxSetupSector == NULL)
    {
        return FALSE;
    }

    // Copy over first setup sector
    RtlCopyMemory(LinuxSetupSector, TempLinuxSetupSector, 512);

    // Read in the rest of the linux setup sectors
    FsSetFilePointer(LinuxKernelFile, 1024);
    if (!FsReadFile(LinuxKernelFile, SetupSectorSize - 512, NULL, (PVOID)((ULONG_PTR)LinuxSetupSector + 512)))
    {
        return FALSE;
    }

    // DbgDumpBuffer(DPRINT_LINUX, LinuxSetupSector, SetupSectorSize);

    TRACE("SetupHeaderSignature: 0x%x (HdrS)\n", LinuxSetupSector->SetupHeaderSignature);
    TRACE("Version: 0x%x\n", LinuxSetupSector->Version);
    TRACE("RealModeSwitch: 0x%x\n", LinuxSetupSector->RealModeSwitch);
    TRACE("SetupSeg: 0x%x\n", LinuxSetupSector->SetupSeg);
    TRACE("StartSystemSeg: 0x%x\n", LinuxSetupSector->StartSystemSeg);
    TRACE("KernelVersion: 0x%x\n", LinuxSetupSector->KernelVersion);
    TRACE("TypeOfLoader: 0x%x\n", LinuxSetupSector->TypeOfLoader);
    TRACE("LoadFlags: 0x%x\n", LinuxSetupSector->LoadFlags);
    TRACE("SetupMoveSize: 0x%x\n", LinuxSetupSector->SetupMoveSize);
    TRACE("Code32Start: 0x%x\n", LinuxSetupSector->Code32Start);
    TRACE("RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress);
    TRACE("RamdiskSize: 0x%x\n", LinuxSetupSector->RamdiskSize);
    TRACE("BootSectKludgeOffset: 0x%x\n", LinuxSetupSector->BootSectKludgeOffset);
    TRACE("BootSectKludgeSegment: 0x%x\n", LinuxSetupSector->BootSectKludgeSegment);
    TRACE("HeapEnd: 0x%x\n", LinuxSetupSector->HeapEnd);

    return TRUE;
}

BOOLEAN LinuxReadKernel(PFILE LinuxKernelFile)
{
    ULONG        BytesLoaded;
    CHAR    StatusText[260];
    PVOID    LoadAddress;

    sprintf(StatusText, "Loading %s", LinuxKernelName);
    UiDrawStatusText(StatusText);

    // Allocate memory for Linux kernel
    LinuxKernelLoadAddress = MmAllocateMemoryAtAddress(LinuxKernelSize, (PVOID)LINUX_KERNEL_LOAD_ADDRESS, LoaderSystemCode);
    if (LinuxKernelLoadAddress != (PVOID)LINUX_KERNEL_LOAD_ADDRESS)
    {
        return FALSE;
    }

    LoadAddress = LinuxKernelLoadAddress;

    // Read linux kernel to 0x100000 (1mb)
    FsSetFilePointer(LinuxKernelFile, 512 + SetupSectorSize);
    for (BytesLoaded=0; BytesLoaded<LinuxKernelSize; )
    {
        if (!FsReadFile(LinuxKernelFile, LINUX_READ_CHUNK_SIZE, NULL, LoadAddress))
        {
            return FALSE;
        }

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LoadAddress = (PVOID)((ULONG_PTR)LoadAddress + LINUX_READ_CHUNK_SIZE);

        UiDrawProgressBarCenter(BytesLoaded, LinuxKernelSize + LinuxInitrdSize, LinuxBootDescription);
    }

    return TRUE;
}

BOOLEAN LinuxCheckKernelVersion(VOID)
{
    // Just assume old kernel until we find otherwise
    NewStyleLinuxKernel = FALSE;

    // Check for new style setup header
    if (LinuxSetupSector->SetupHeaderSignature != LINUX_SETUP_HEADER_ID)
    {
        NewStyleLinuxKernel = FALSE;
    }
    // Check for version below 2.0
    else if (LinuxSetupSector->Version < 0x0200)
    {
        NewStyleLinuxKernel = FALSE;
    }
    // Check for version 2.0
    else if (LinuxSetupSector->Version == 0x0200)
    {
        NewStyleLinuxKernel = TRUE;
    }
    // Check for version 2.01+
    else if (LinuxSetupSector->Version >= 0x0201)
    {
        NewStyleLinuxKernel = TRUE;
        LinuxSetupSector->HeapEnd = 0x9000;
        LinuxSetupSector->LoadFlags |= LINUX_FLAG_CAN_USE_HEAP;
    }

    if ((NewStyleLinuxKernel == FALSE) && (LinuxHasInitrd))
    {
        UiMessageBox("Error: Cannot load a ramdisk (initrd) with an old kernel image.");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN LinuxReadInitrd(PFILE LinuxInitrdFile)
{
    ULONG        BytesLoaded;
    CHAR    StatusText[260];

    sprintf(StatusText, "Loading %s", LinuxInitrdName);
    UiDrawStatusText(StatusText);

    // Allocate memory for the ramdisk
    //LinuxInitrdLoadAddress = MmAllocateMemory(LinuxInitrdSize);
    // Try to align it at the next MB boundary after the kernel
    //LinuxInitrdLoadAddress = MmAllocateMemoryAtAddress(LinuxInitrdSize, (PVOID)ROUND_UP((LINUX_KERNEL_LOAD_ADDRESS + LinuxKernelSize), 0x100000));
    if (LinuxSetupSector->Version <= 0x0202)
    {
        LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, (PVOID)LINUX_MAX_INITRD_ADDRESS, LoaderSystemCode);
    }
    else
    {
        LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, (PVOID)LinuxSetupSector->InitrdAddressMax, LoaderSystemCode);
    }
    if (LinuxInitrdLoadAddress == NULL)
    {
        return FALSE;
    }

    // Set the information in the setup struct
    LinuxSetupSector->RamdiskAddress = (ULONG)LinuxInitrdLoadAddress;
    LinuxSetupSector->RamdiskSize = LinuxInitrdSize;

    TRACE("RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress);
    TRACE("RamdiskSize: 0x%x\n", LinuxSetupSector->RamdiskSize);

    if (LinuxSetupSector->Version >= 0x0203)
    {
        TRACE("InitrdAddressMax: 0x%x\n", LinuxSetupSector->InitrdAddressMax);
    }

    // Read in the ramdisk
    for (BytesLoaded=0; BytesLoaded<LinuxInitrdSize; )
    {
        if (!FsReadFile(LinuxInitrdFile, LINUX_READ_CHUNK_SIZE, NULL, (PVOID)LinuxInitrdLoadAddress))
        {
            return FALSE;
        }

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LinuxInitrdLoadAddress = (PVOID)((ULONG_PTR)LinuxInitrdLoadAddress + LINUX_READ_CHUNK_SIZE);

        UiDrawProgressBarCenter(BytesLoaded + LinuxKernelSize, LinuxInitrdSize + LinuxKernelSize, LinuxBootDescription);
    }

    return TRUE;
}

#endif // _M_IX86

#endif
