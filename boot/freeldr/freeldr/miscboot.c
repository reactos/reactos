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
    PFILE FilePointer;
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

    FilePointer = FsOpenFile(FileName);
    if (!FilePointer)
    {
        UiMessageBox("%s not found.", FileName);
        return ENOENT;
    }

    /* Read boot sector */
    if (!FsReadFile(FilePointer, 512, &BytesRead, (void*)0x7c00) || (BytesRead != 512))
    {
        UiMessageBox("Unable to read boot sector.");
        return EIO;
    }

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
    // DisableA20();
    ChainLoadBiosBootSectorCode();
    return ESUCCESS;
}

ARC_STATUS
LoadAndBootPartition(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    PCSTR ArgValue;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
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

    /* Get the partition table entry */
    if (!DiskGetPartitionEntry(DriveNumber, PartitionNumber, &PartitionTableEntry))
    {
        return ENOENT;
    }

    /* Now try to read the partition boot sector. If this fails then abort. */
    if (!MachDiskReadLogicalSectors(DriveNumber, PartitionTableEntry.SectorCountBeforePartition, 1, (PVOID)0x7C00))
    {
        UiMessageBox("Unable to read partition's boot sector.");
        return EIO;
    }

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
    // DisableA20();
    FrldrBootDrive = DriveNumber;
    ChainLoadBiosBootSectorCode();
    return ESUCCESS;
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

    /* Now try to read the boot sector (or mbr). If this fails then abort. */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0, 1, (PVOID)0x7C00))
    {
        UiMessageBox("Unable to read boot sector");
        return EIO;
    }

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
    // DisableA20();
    FrldrBootDrive = DriveNumber;
    ChainLoadBiosBootSectorCode();
    return ESUCCESS;
}

#endif // _M_IX86
