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

#ifdef _M_IX86

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* FUNCTIONS ******************************************************************/

ARC_STATUS
LoadAndBootBootSector(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    PCSTR FileName;
    ULONG FileId;
    ULONG BytesRead;

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /* Read the file name */
    FileName = GetArgumentValue(Argc, Argv, "BootSectorFile");
    if (!FileName)
    {
        UiMessageBox("Boot sector file not specified for selected OS!");
        return EINVAL;
    }

    FileId = FsOpenFile(FileName);
    if (!FileId)
    {
        UiMessageBox("%s not found.", FileName);
        return ENOENT;
    }

    /* Read boot sector */
    if ((ArcRead(FileId, (PVOID)0x7c00, 512, &BytesRead) != ESUCCESS) || (BytesRead != 512))
    {
        UiMessageBox("Unable to read boot sector.");
        return EIO;
    }

    ArcClose(FileId);

    /* Check for validity */
    if (*((USHORT*)(0x7c00 + 0x1fe)) != 0xaa55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return ENOEXEC;
    }

    UiUnInitialize("Booting...");
    IniCleanup();

    /*
     * Don't stop the floppy drive motor when we
     * are just booting a bootsector, or drive, or partition.
     * If we were to stop the floppy motor then
     * the BIOS wouldn't be informed and if the
     * next read is to a floppy then the BIOS will
     * still think the motor is on and this will
     * result in a read error.
     */
    // DiskStopFloppyMotor();
    /* NOTE: Don't touch FrldrBootDrive */
    ChainLoadBiosBootSectorCode();
    Reboot(); /* Must not return! */
    return ESUCCESS;
}

static ARC_STATUS
LoadAndBootPartitionOrDrive(
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber OPTIONAL)
{
    ULONG FileId;
    ULONG BytesRead;
    CHAR ArcPath[MAX_PATH];

    /* Construct the corresponding ARC path */
    ConstructArcPath(ArcPath, "", DriveNumber, PartitionNumber);
    *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.
    if (ArcOpen(ArcPath, OpenReadOnly, &FileId) != ESUCCESS)
    {
        UiMessageBox("Unable to open %s", ArcPath);
        return ENOENT;
    }

    /*
     * Now try to read the partition boot sector or the MBR (when PartitionNumber == 0).
     * If this fails then abort.
     */
    if ((ArcRead(FileId, (PVOID)0x7c00, 512, &BytesRead) != ESUCCESS) || (BytesRead != 512))
    {
        if (PartitionNumber != 0)
            UiMessageBox("Unable to read partition's boot sector.");
        else
            UiMessageBox("Unable to read MBR boot sector.");
        return EIO;
    }

    ArcClose(FileId);

    /* Check for validity */
    if (*((USHORT*)(0x7c00 + 0x1fe)) != 0xaa55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return ENOEXEC;
    }

    UiUnInitialize("Booting...");
    IniCleanup();

    /*
     * Don't stop the floppy drive motor when we
     * are just booting a bootsector, or drive, or partition.
     * If we were to stop the floppy motor then
     * the BIOS wouldn't be informed and if the
     * next read is to a floppy then the BIOS will
     * still think the motor is on and this will
     * result in a read error.
     */
    // DiskStopFloppyMotor();
    FrldrBootDrive = DriveNumber;
    FrldrBootPartition = PartitionNumber;
    ChainLoadBiosBootSectorCode();
    Reboot(); /* Must not return! */
    return ESUCCESS;
}

ARC_STATUS
LoadAndBootPartition(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    PCSTR ArgValue;
    UCHAR DriveNumber;
    ULONG PartitionNumber;

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /* Read the boot drive */
    ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
    if (!ArgValue)
    {
        UiMessageBox("Boot drive not specified for selected OS!");
        return EINVAL;
    }
    DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

    /* Read the boot partition */
    ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
    if (!ArgValue)
    {
        UiMessageBox("Boot partition not specified for selected OS!");
        return EINVAL;
    }
    PartitionNumber = atoi(ArgValue);

    return LoadAndBootPartitionOrDrive(DriveNumber, PartitionNumber);
}

ARC_STATUS
LoadAndBootDrive(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    PCSTR ArgValue;
    UCHAR DriveNumber;

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /* Read the boot drive */
    ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
    if (!ArgValue)
    {
        UiMessageBox("Boot drive not specified for selected OS!");
        return EINVAL;
    }
    DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

    return LoadAndBootPartitionOrDrive(DriveNumber, 0);
}

#endif // _M_IX86
