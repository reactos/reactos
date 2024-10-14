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

#if defined(_M_IX86) || defined(_M_AMD64)

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* FUNCTIONS ******************************************************************/

static ARC_STATUS
LoadBootSector(
    IN ULONG Argc,
    IN PCHAR Argv[],
    OUT PUCHAR DriveNumber,
    OUT PULONG PartitionNumber)
{
    ARC_STATUS Status;
    PCSTR ArgValue;
    PCSTR BootPath;
    PCSTR FileName;
    ULONG FileId;
    ULONG BytesRead;
    CHAR ArcPath[MAX_PATH];
    ULONG LoadAddress;

    *DriveNumber = 0;
    *PartitionNumber = 0;

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
            *DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

            /* Retrieve the boot partition (not optional and cannot be zero) */
            *PartitionNumber = 0;
            ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
            if (ArgValue && *ArgValue)
                *PartitionNumber = atoi(ArgValue);
            if (*PartitionNumber == 0)
            {
                UiMessageBox("Boot partition cannot be 0!");
                return EINVAL;
            }

            /* Construct the corresponding ARC path */
            ConstructArcPath(ArcPath, "", *DriveNumber, *PartitionNumber);
            *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.

            BootPath = ArcPath;
        }
        else
        {
            /* Fall back to using the system partition as default path */
            BootPath = GetArgumentValue(Argc, Argv, "SystemPartition");
        }
    }

    /* Retrieve the file name */
    FileName = GetArgumentValue(Argc, Argv, "BootSectorFile");
    if (!FileName || !*FileName)
    {
        UiMessageBox("Boot sector file not specified for selected OS!");
        return EINVAL;
    }

    /* Open the boot sector file */
    Status = FsOpenFile(FileName, BootPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Unable to open %s", FileName);
        return Status;
    }

#if defined(SARCH_PC98)
    LoadAddress = Pc98GetBootSectorLoadAddress(*DriveNumber);
#else
    LoadAddress = 0x7C00;
#endif

    /* Now try to load the boot sector. If this fails then abort. */
    Status = ArcRead(FileId, UlongToPtr(LoadAddress), 512, &BytesRead);
    ArcClose(FileId);
    if ((Status != ESUCCESS) || (BytesRead != 512))
    {
        UiMessageBox("Unable to load boot sector.");
        return EIO;
    }

    /* Check for validity */
    if (*(USHORT*)UlongToPtr(LoadAddress + 0x1FE) != 0xAA55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return ENOEXEC;
    }

    /* Reset the drive and partition numbers so as to use their default values */
    *DriveNumber = 0;
    *PartitionNumber = 0;

    return ESUCCESS;
}

static ARC_STATUS
LoadPartitionOrDrive(
    IN OUT PUCHAR DriveNumber,
    IN OUT PULONG PartitionNumber,
    IN PCSTR BootPath OPTIONAL)
{
    ARC_STATUS Status;
    ULONG FileId;
    ULONG BytesRead;
    CHAR ArcPath[MAX_PATH];
    ULONG LoadAddress;

    /*
     * The ARC "BootPath" value takes precedence over
     * both the DriveNumber and PartitionNumber options.
     */
    if (BootPath && *BootPath)
    {
        PCSTR FileName = NULL;

        /*
         * Retrieve the BIOS drive and partition numbers; verify also that the
         * path is "valid" in the sense that it must not contain any file name.
         */
        if (!DissectArcPath(BootPath, &FileName, DriveNumber, PartitionNumber) ||
            (FileName && *FileName))
        {
            return EINVAL;
        }
    }
    else
    {
        /* We don't have one, so construct the corresponding ARC path */
        ConstructArcPath(ArcPath, "", *DriveNumber, *PartitionNumber);
        *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.

        BootPath = ArcPath;
    }

    /* Open the volume */
    Status = ArcOpen((PSTR)BootPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Unable to open %s", BootPath);
        return Status;
    }

#if defined(SARCH_PC98)
    LoadAddress = Pc98GetBootSectorLoadAddress(*DriveNumber);
#else
    LoadAddress = 0x7C00;
#endif

    /*
     * Now try to load the partition boot sector or the MBR (when PartitionNumber == 0).
     * If this fails then abort.
     */
    Status = ArcRead(FileId, UlongToPtr(LoadAddress), 512, &BytesRead);
    ArcClose(FileId);
    if ((Status != ESUCCESS) || (BytesRead != 512))
    {
        if (*PartitionNumber != 0)
            UiMessageBox("Unable to load partition's boot sector.");
        else
            UiMessageBox("Unable to load MBR boot sector.");
        return EIO;
    }

    /* Check for validity */
    if (*(USHORT*)UlongToPtr(LoadAddress + 0x1FE) != 0xAA55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return ENOEXEC;
    }

    return ESUCCESS;
}

static ARC_STATUS
LoadPartition(
    IN ULONG Argc,
    IN PCHAR Argv[],
    OUT PUCHAR DriveNumber,
    OUT PULONG PartitionNumber)
{
    PCSTR ArgValue;
    PCSTR BootPath;

    *DriveNumber = 0;
    *PartitionNumber = 0;

    /*
     * Check whether we have a "BootPath" value (takes precedence
     * over both "BootDrive" and "BootPartition").
     */
    BootPath = GetArgumentValue(Argc, Argv, "BootPath");
    if (!BootPath || !*BootPath)
    {
        /* We don't have one */

        /* Retrieve the boot drive */
        ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
        if (!ArgValue || !*ArgValue)
        {
            UiMessageBox("Boot drive not specified for selected OS!");
            return EINVAL;
        }
        *DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

        /* Retrieve the boot partition (optional, fall back to zero otherwise) */
        *PartitionNumber = 0;
        ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
        if (ArgValue && *ArgValue)
            *PartitionNumber = atoi(ArgValue);
    }

    return LoadPartitionOrDrive(DriveNumber, PartitionNumber, BootPath);
}

static ARC_STATUS
LoadDrive(
    IN ULONG Argc,
    IN PCHAR Argv[],
    OUT PUCHAR DriveNumber,
    OUT PULONG PartitionNumber)
{
    PCSTR ArgValue;
    PCSTR BootPath;

    *DriveNumber = 0;
    *PartitionNumber = 0;

    /* Check whether we have a "BootPath" value (takes precedence over "BootDrive") */
    BootPath = GetArgumentValue(Argc, Argv, "BootPath");
    if (BootPath && *BootPath)
    {
        /*
         * We have one, check that it does not contain any
         * "partition()" specification, and fail if so.
         */
        if (strstr(BootPath, ")partition("))
        {
            UiMessageBox("Invalid 'BootPath' value!");
            return EINVAL;
        }
    }
    else
    {
        /* We don't, retrieve the boot drive value instead */
        ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
        if (!ArgValue || !*ArgValue)
        {
            UiMessageBox("Boot drive not specified for selected OS!");
            return EINVAL;
        }
        *DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);
    }

    return LoadPartitionOrDrive(DriveNumber, PartitionNumber, BootPath);
}


ARC_STATUS
LoadAndBootDevice(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    ARC_STATUS Status;
    PCSTR ArgValue;
    UCHAR Type;
    UCHAR DriveNumber = 0;
    ULONG PartitionNumber = 0;

    /* Retrieve the (mandatory) boot type */
    ArgValue = GetArgumentValue(Argc, Argv, "BootType");
    if (!ArgValue || !*ArgValue)
        return EINVAL;
    if (_stricmp(ArgValue, "Drive") == 0)
        Type = 1;
    else if (_stricmp(ArgValue, "Partition") == 0)
        Type = 2;
    else if (_stricmp(ArgValue, "BootSector") == 0)
        Type = 3;
    else
        return EINVAL;

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /* Load the corresponding device */
    switch (Type)
    {
        case 1:
            Status = LoadDrive(Argc, Argv, &DriveNumber, &PartitionNumber);
            break;
        case 2:
            Status = LoadPartition(Argc, Argv, &DriveNumber, &PartitionNumber);
            break;
        case 3:
            Status = LoadBootSector(Argc, Argv, &DriveNumber, &PartitionNumber);
            break;
        default:
            return EINVAL;
    }
    if (Status != ESUCCESS)
        return Status;

    UiUnInitialize("Booting...");
    IniCleanup();

#ifndef UEFIBOOT
    /* Boot the loaded sector code */
    ChainLoadBiosBootSectorCode(DriveNumber, PartitionNumber);
#endif
    /* Must not return! */
    return ESUCCESS;
}

#endif /* _M_IX86 || _M_AMD64 */
