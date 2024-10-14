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

/*
 * The x86 Linux Boot Protocol is explained at:
 * https://www.kernel.org/doc/Documentation/x86/boot.txt
 */

#if defined(_M_IX86) || defined(_M_AMD64)

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(LINUX);

/* GLOBALS ********************************************************************/

#define LINUX_READ_CHUNK_SIZE   0x20000 // Read 128k at a time

PLINUX_BOOTSECTOR  LinuxBootSector = NULL;
PLINUX_SETUPSECTOR LinuxSetupSector = NULL;
ULONG   SetupSectorSize = 0;
BOOLEAN NewStyleLinuxKernel = FALSE;
ULONG   LinuxKernelSize = 0;
ULONG   LinuxInitrdSize = 0;
PCSTR   LinuxKernelName = NULL;
PCSTR   LinuxInitrdName = NULL;
PSTR    LinuxCommandLine = NULL;
ULONG   LinuxCommandLineSize = 0;
PVOID   LinuxKernelLoadAddress = NULL;
PVOID   LinuxInitrdLoadAddress = NULL;
CHAR    LinuxBootDescription[80];

/* FUNCTIONS ******************************************************************/

static BOOLEAN LinuxReadBootSector(ULONG LinuxKernelFile);
static BOOLEAN LinuxReadSetupSector(ULONG LinuxKernelFile);
static BOOLEAN LinuxReadKernel(ULONG LinuxKernelFile);
static BOOLEAN LinuxCheckKernelVersion(VOID);
static BOOLEAN LinuxReadInitrd(ULONG LinuxInitrdFile);

static VOID
RemoveQuotes(
    IN OUT PSTR QuotedString)
{
    PCHAR  p;
    PSTR   Start;
    SIZE_T Size;

    /* Skip spaces up to " */
    p = QuotedString;
    while (*p == ' ' || *p == '\t' || *p == '"')
        ++p;
    Start = p;

    /* Go up to next " */
    while (*p != ANSI_NULL && *p != '"')
        ++p;
    /* NULL-terminate */
    *p = ANSI_NULL;

    /* Move the NULL-terminated string back into 'QuotedString' in place */
    Size = (strlen(Start) + 1) * sizeof(CHAR);
    memmove(QuotedString, Start, Size);
}

ARC_STATUS
LoadAndBootLinux(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    ARC_STATUS Status;
    PCSTR Description;
    PCSTR ArgValue;
    PCSTR BootPath;
    UCHAR DriveNumber = 0;
    ULONG PartitionNumber = 0;
    ULONG LinuxKernel = 0;
    ULONG LinuxInitrdFile = 0;
    FILEINFORMATION FileInfo;
    CHAR ArcPath[MAX_PATH];

    Description = GetArgumentValue(Argc, Argv, "LoadIdentifier");
    if (Description && *Description)
        RtlStringCbPrintfA(LinuxBootDescription, sizeof(LinuxBootDescription), "Loading %s...", Description);
    else
        strcpy(LinuxBootDescription, "Loading Linux...");

    UiDrawBackdrop();
    UiDrawStatusText(LinuxBootDescription);
    UiDrawProgressBarCenter(LinuxBootDescription);

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /*
     * Check whether we have a "BootPath" value (takes precedence
     * over both "BootDrive" and "BootPartition").
     */
    BootPath = GetArgumentValue(Argc, Argv, "BootPath");
    if (!BootPath || !*BootPath)
    {
        /* We don't have one, check whether we use "BootDrive" and "BootPartition" */

        /* Retrieve the boot drive (optional, fall back to using default path otherwise) */
        ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
        if (ArgValue && *ArgValue)
        {
            DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

            /* Retrieve the boot partition (not optional and cannot be zero) */
            PartitionNumber = 0;
            ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
            if (ArgValue && *ArgValue)
                PartitionNumber = atoi(ArgValue);
            if (PartitionNumber == 0)
            {
                UiMessageBox("Boot partition cannot be 0!");
                goto LinuxBootFailed;
                // return EINVAL;
            }

            /* Construct the corresponding ARC path */
            ConstructArcPath(ArcPath, "", DriveNumber, PartitionNumber);
            *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.

            BootPath = ArcPath;
        }
        else
        {
            /* Fall back to using the system partition as default path */
            BootPath = GetArgumentValue(Argc, Argv, "SystemPartition");
        }
    }

    /* If we haven't retrieved the BIOS drive and partition numbers above, do it now */
    if (PartitionNumber == 0)
    {
        /* Retrieve the BIOS drive and partition numbers */
        if (!DissectArcPath(BootPath, NULL, &DriveNumber, &PartitionNumber))
        {
            /* This is not a fatal failure, but just an inconvenience: display a message */
            TRACE("DissectArcPath(%s) failed to retrieve BIOS drive and partition numbers.\n", BootPath);
        }
    }

    /* Get the kernel name */
    LinuxKernelName = GetArgumentValue(Argc, Argv, "Kernel");
    if (!LinuxKernelName || !*LinuxKernelName)
    {
        UiMessageBox("Linux kernel filename not specified for selected OS!");
        goto LinuxBootFailed;
    }

    /* Get the initrd name (optional) */
    LinuxInitrdName = GetArgumentValue(Argc, Argv, "Initrd");

    /* Get the command line (optional) */
    LinuxCommandLineSize = 0;
    LinuxCommandLine = GetArgumentValue(Argc, Argv, "CommandLine");
    if (LinuxCommandLine && *LinuxCommandLine)
    {
        RemoveQuotes(LinuxCommandLine);
        LinuxCommandLineSize = (ULONG)strlen(LinuxCommandLine) + 1;
        LinuxCommandLineSize = min(LinuxCommandLineSize, 260);
    }

    /* Open the kernel */
    Status = FsOpenFile(LinuxKernelName, BootPath, OpenReadOnly, &LinuxKernel);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Linux kernel '%s' not found.", LinuxKernelName);
        goto LinuxBootFailed;
    }

    /* Open the initrd file image (if necessary) */
    if (LinuxInitrdName)
    {
        Status = FsOpenFile(LinuxInitrdName, BootPath, OpenReadOnly, &LinuxInitrdFile);
        if (Status != ESUCCESS)
        {
            UiMessageBox("Linux initrd image '%s' not found.", LinuxInitrdName);
            goto LinuxBootFailed;
        }
    }

    /* Load the boot sector */
    if (!LinuxReadBootSector(LinuxKernel))
        goto LinuxBootFailed;

    /* Load the setup sector */
    if (!LinuxReadSetupSector(LinuxKernel))
        goto LinuxBootFailed;

    /* Calc kernel size */
    Status = ArcGetFileInformation(LinuxKernel, &FileInfo);
    if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
        LinuxKernelSize = 0;
    else
        LinuxKernelSize = FileInfo.EndingAddress.LowPart - (512 + SetupSectorSize);

    /* Get the initrd file image (if necessary) */
    LinuxInitrdSize = 0;
    if (LinuxInitrdName)
    {
        Status = ArcGetFileInformation(LinuxInitrdFile, &FileInfo);
        if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
            LinuxInitrdSize = 0;
        else
            LinuxInitrdSize = FileInfo.EndingAddress.LowPart;
    }

    /* Load the kernel */
    if (!LinuxReadKernel(LinuxKernel))
        goto LinuxBootFailed;

    /* Load the initrd (if necessary) */
    if (LinuxInitrdName)
    {
        if (!LinuxReadInitrd(LinuxInitrdFile))
            goto LinuxBootFailed;
    }

    // If the default root device is set to FLOPPY (0000h), change to /dev/fd0 (0200h)
    if (LinuxBootSector->RootDevice == 0x0000)
        LinuxBootSector->RootDevice = 0x0200;

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
        LinuxSetupSector->TypeOfLoader = LINUX_LOADER_TYPE_FREELOADER;
    else
        LinuxSetupSector->LoadFlags = 0;

    RtlCopyMemory((PVOID)0x90000, LinuxBootSector, 512);
    RtlCopyMemory((PVOID)0x90200, LinuxSetupSector, SetupSectorSize);
    RtlCopyMemory((PVOID)0x99000,
                  LinuxCommandLine ? LinuxCommandLine : "",
                  LinuxCommandLine ? LinuxCommandLineSize : sizeof(ANSI_NULL));

    UiUnInitialize("Booting Linux...");
    IniCleanup();

    BootLinuxKernel(LinuxKernelSize, LinuxKernelLoadAddress,
                    (LinuxSetupSector->LoadFlags & LINUX_FLAG_LOAD_HIGH)
                        ? (PVOID)LINUX_KERNEL_LOAD_ADDRESS /* == 0x100000 */
                        : (PVOID)0x10000,
                    DriveNumber, PartitionNumber);
    /* Must not return! */
    return ESUCCESS;

LinuxBootFailed:

    if (LinuxKernel)
        ArcClose(LinuxKernel);

    if (LinuxInitrdFile)
        ArcClose(LinuxInitrdFile);

    if (LinuxBootSector != NULL)
        MmFreeMemory(LinuxBootSector);

    if (LinuxSetupSector != NULL)
        MmFreeMemory(LinuxSetupSector);

    if (LinuxKernelLoadAddress != NULL)
        MmFreeMemory(LinuxKernelLoadAddress);

    if (LinuxInitrdLoadAddress != NULL)
        MmFreeMemory(LinuxInitrdLoadAddress);

    LinuxBootSector = NULL;
    LinuxSetupSector = NULL;
    SetupSectorSize = 0;
    NewStyleLinuxKernel = FALSE;
    LinuxKernelSize = 0;
    LinuxInitrdSize = 0;
    LinuxKernelName = NULL;
    LinuxInitrdName = NULL;
    LinuxCommandLine = NULL;
    LinuxCommandLineSize = 0;
    LinuxKernelLoadAddress = NULL;
    LinuxInitrdLoadAddress = NULL;
    *LinuxBootDescription = ANSI_NULL;

    return ENOEXEC;
}

static BOOLEAN LinuxReadBootSector(ULONG LinuxKernelFile)
{
    LARGE_INTEGER Position;

    /* Allocate memory for boot sector */
    LinuxBootSector = MmAllocateMemoryWithType(512, LoaderSystemCode);
    if (LinuxBootSector == NULL)
        return FALSE;

    /* Load the linux boot sector */
    Position.QuadPart = 0;
    if (ArcSeek(LinuxKernelFile, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    if (ArcRead(LinuxKernelFile, LinuxBootSector, 512, NULL) != ESUCCESS)
        return FALSE;

    /* Check for validity */
    if (LinuxBootSector->BootFlag != LINUX_BOOT_SECTOR_MAGIC)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return FALSE;
    }

    // DbgDumpBuffer(DPRINT_LINUX, LinuxBootSector, 512);

    TRACE("SetupSectors: %d\n"  , LinuxBootSector->SetupSectors);
    TRACE("RootFlags:    0x%x\n", LinuxBootSector->RootFlags);
    TRACE("SystemSize:   0x%x\n", LinuxBootSector->SystemSize);
    TRACE("SwapDevice:   0x%x\n", LinuxBootSector->SwapDevice);
    TRACE("RamSize:      0x%x\n", LinuxBootSector->RamSize);
    TRACE("VideoMode:    0x%x\n", LinuxBootSector->VideoMode);
    TRACE("RootDevice:   0x%x\n", LinuxBootSector->RootDevice);
    TRACE("BootFlag:     0x%x\n", LinuxBootSector->BootFlag);

    return TRUE;
}

static BOOLEAN LinuxReadSetupSector(ULONG LinuxKernelFile)
{
    LARGE_INTEGER Position;
    UCHAR TempLinuxSetupSector[512];

    /* Load the first linux setup sector */
    Position.QuadPart = 512;
    if (ArcSeek(LinuxKernelFile, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    if (ArcRead(LinuxKernelFile, TempLinuxSetupSector, 512, NULL) != ESUCCESS)
        return FALSE;

    /* Check the kernel version */
    LinuxSetupSector = (PLINUX_SETUPSECTOR)TempLinuxSetupSector;
    if (!LinuxCheckKernelVersion())
        return FALSE;

    if (NewStyleLinuxKernel)
        SetupSectorSize = 512 * LinuxBootSector->SetupSectors;
    else
        SetupSectorSize = 512 * 4; // Always 4 setup sectors

    /* Allocate memory for setup sectors */
    LinuxSetupSector = MmAllocateMemoryWithType(SetupSectorSize, LoaderSystemCode);
    if (LinuxSetupSector == NULL)
        return FALSE;

    /* Copy over first setup sector */
    RtlCopyMemory(LinuxSetupSector, TempLinuxSetupSector, 512);

    /* Load the rest of the linux setup sectors */
    Position.QuadPart = 1024;
    if (ArcSeek(LinuxKernelFile, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    if (ArcRead(LinuxKernelFile, (PVOID)((ULONG_PTR)LinuxSetupSector + 512), SetupSectorSize - 512, NULL) != ESUCCESS)
        return FALSE;

    // DbgDumpBuffer(DPRINT_LINUX, LinuxSetupSector, SetupSectorSize);

    TRACE("SetupHeaderSignature: 0x%x (HdrS)\n", LinuxSetupSector->SetupHeaderSignature);
    TRACE("Version:              0x%x\n", LinuxSetupSector->Version);
    TRACE("RealModeSwitch: 0x%x\n", LinuxSetupSector->RealModeSwitch);
    TRACE("SetupSeg:       0x%x\n", LinuxSetupSector->SetupSeg);
    TRACE("StartSystemSeg: 0x%x\n", LinuxSetupSector->StartSystemSeg);
    TRACE("KernelVersion:  0x%x\n", LinuxSetupSector->KernelVersion);
    TRACE("TypeOfLoader:   0x%x\n", LinuxSetupSector->TypeOfLoader);
    TRACE("LoadFlags:      0x%x\n", LinuxSetupSector->LoadFlags);
    TRACE("SetupMoveSize:  0x%x\n", LinuxSetupSector->SetupMoveSize);
    TRACE("Code32Start:    0x%x\n", LinuxSetupSector->Code32Start);
    TRACE("RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress);
    TRACE("RamdiskSize:    0x%x\n", LinuxSetupSector->RamdiskSize);
    TRACE("BootSectKludgeOffset:  0x%x\n", LinuxSetupSector->BootSectKludgeOffset);
    TRACE("BootSectKludgeSegment: 0x%x\n", LinuxSetupSector->BootSectKludgeSegment);
    TRACE("HeapEnd: 0x%x\n", LinuxSetupSector->HeapEnd);

    return TRUE;
}

static BOOLEAN LinuxReadKernel(ULONG LinuxKernelFile)
{
    PVOID LoadAddress;
    LARGE_INTEGER Position;
    ULONG BytesLoaded;
    CHAR  StatusText[260];

    RtlStringCbPrintfA(StatusText, sizeof(StatusText), "Loading %s", LinuxKernelName);
    UiDrawStatusText(StatusText);

    /* Try to allocate memory for the Linux kernel; if it fails, allocate somewhere else */
    LinuxKernelLoadAddress = MmAllocateMemoryAtAddress(LinuxKernelSize, (PVOID)LINUX_KERNEL_LOAD_ADDRESS, LoaderSystemCode);
    if (LinuxKernelLoadAddress != (PVOID)LINUX_KERNEL_LOAD_ADDRESS)
    {
        /* It's OK, let's allocate again somewhere else */
        LinuxKernelLoadAddress = MmAllocateMemoryWithType(LinuxKernelSize, LoaderSystemCode);
        if (LinuxKernelLoadAddress == NULL)
        {
            TRACE("Failed to allocate 0x%lx bytes for the kernel image.\n", LinuxKernelSize);
            return FALSE;
        }
    }

    LoadAddress = LinuxKernelLoadAddress;

    /* Load the linux kernel at 0x100000 (1mb) */
    Position.QuadPart = 512 + SetupSectorSize;
    if (ArcSeek(LinuxKernelFile, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    for (BytesLoaded = 0; BytesLoaded < LinuxKernelSize; )
    {
        if (ArcRead(LinuxKernelFile, LoadAddress, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return FALSE;

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LoadAddress = (PVOID)((ULONG_PTR)LoadAddress + LINUX_READ_CHUNK_SIZE);

        UiUpdateProgressBar(BytesLoaded * 100 / (LinuxKernelSize + LinuxInitrdSize), NULL);
    }

    return TRUE;
}

static BOOLEAN LinuxCheckKernelVersion(VOID)
{
    /* Just assume old kernel until we find otherwise */
    NewStyleLinuxKernel = FALSE;

    /* Check for new style setup header */
    if (LinuxSetupSector->SetupHeaderSignature != LINUX_SETUP_HEADER_ID)
    {
        NewStyleLinuxKernel = FALSE;
    }
    /* Check for version below 2.0 */
    else if (LinuxSetupSector->Version < 0x0200)
    {
        NewStyleLinuxKernel = FALSE;
    }
    /* Check for version 2.0 */
    else if (LinuxSetupSector->Version == 0x0200)
    {
        NewStyleLinuxKernel = TRUE;
    }
    /* Check for version 2.01+ */
    else if (LinuxSetupSector->Version >= 0x0201)
    {
        NewStyleLinuxKernel = TRUE;
        LinuxSetupSector->HeapEnd = 0x9000;
        LinuxSetupSector->LoadFlags |= LINUX_FLAG_CAN_USE_HEAP;
    }

    if ((NewStyleLinuxKernel == FALSE) && (LinuxInitrdName))
    {
        UiMessageBox("Error: Cannot load a ramdisk (initrd) with an old kernel image.");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN LinuxReadInitrd(ULONG LinuxInitrdFile)
{
    ULONG BytesLoaded;
    CHAR  StatusText[260];

    RtlStringCbPrintfA(StatusText, sizeof(StatusText), "Loading %s", LinuxInitrdName);
    UiDrawStatusText(StatusText);

    /* Allocate memory for the ramdisk, below 4GB */
    // LinuxInitrdLoadAddress = MmAllocateMemory(LinuxInitrdSize);
    /* Try to align it at the next MB boundary after the kernel */
    // LinuxInitrdLoadAddress = MmAllocateMemoryAtAddress(LinuxInitrdSize, (PVOID)ROUND_UP((LINUX_KERNEL_LOAD_ADDRESS + LinuxKernelSize), 0x100000));
    if (LinuxSetupSector->Version <= 0x0202)
    {
#ifdef _M_AMD64
        C_ASSERT(LINUX_MAX_INITRD_ADDRESS < 0x100000000);
#endif
        LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, (PVOID)LINUX_MAX_INITRD_ADDRESS, LoaderSystemCode);
    }
    else
    {
        LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, UlongToPtr(LinuxSetupSector->InitrdAddressMax), LoaderSystemCode);
    }
    if (LinuxInitrdLoadAddress == NULL)
    {
        return FALSE;
    }
#ifdef _M_AMD64
    ASSERT((ULONG_PTR)LinuxInitrdLoadAddress < 0x100000000);
#endif

    /* Set the information in the setup struct */
    LinuxSetupSector->RamdiskAddress = PtrToUlong(LinuxInitrdLoadAddress);
    LinuxSetupSector->RamdiskSize = LinuxInitrdSize;

    TRACE("RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress);
    TRACE("RamdiskSize:    0x%x\n", LinuxSetupSector->RamdiskSize);

    if (LinuxSetupSector->Version >= 0x0203)
    {
        TRACE("InitrdAddressMax: 0x%x\n", LinuxSetupSector->InitrdAddressMax);
    }

    /* Load the ramdisk */
    for (BytesLoaded = 0; BytesLoaded < LinuxInitrdSize; )
    {
        if (ArcRead(LinuxInitrdFile, LinuxInitrdLoadAddress, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return FALSE;

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LinuxInitrdLoadAddress = (PVOID)((ULONG_PTR)LinuxInitrdLoadAddress + LINUX_READ_CHUNK_SIZE);

        UiUpdateProgressBar((BytesLoaded + LinuxKernelSize) * 100 / (LinuxInitrdSize + LinuxKernelSize), NULL);
    }

    return TRUE;
}

#endif /* _M_IX86 || _M_AMD64 */
