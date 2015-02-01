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

#include <freeldr.h>

#ifdef _M_IX86

VOID
LoadAndBootBootSector(IN OperatingSystemItem* OperatingSystem,
                      IN USHORT OperatingSystemVersion)
{
    ULONG_PTR    SectionId;
    PCSTR    SectionName = OperatingSystem->SystemPartition;
    CHAR    FileName[260];
    PFILE    FilePointer;
    ULONG    BytesRead;
    CHAR    SettingName[80];

    // Find all the message box settings and run them
    UiShowMessageBoxesInSection(SectionName);

    // Try to open the operating system section in the .ini file
    if (!IniOpenSection(SectionName, &SectionId))
    {
        sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", SectionName);
        UiMessageBox(SettingName);
        return;
    }

    if (!IniReadSettingByName(SectionId, "BootSectorFile", FileName, sizeof(FileName)))
    {
        UiMessageBox("Boot sector file not specified for selected OS!");
        return;
    }

    FilePointer = FsOpenFile(FileName);
    if (!FilePointer)
    {
        strcat(FileName, " not found.");
        UiMessageBox(FileName);
        return;
    }

    // Read boot sector
    if (!FsReadFile(FilePointer, 512, &BytesRead, (void*)0x7c00) || (BytesRead != 512))
    {
        UiMessageBox("Unable to read boot sector.");
        return;
    }

    // Check for validity
    if (*((USHORT*)(0x7c00 + 0x1fe)) != 0xaa55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return;
    }

    UiUnInitialize("Booting...");
    // Don't stop the floppy drive motor when we
    // are just booting a bootsector, or drive, or partition.
    // If we were to stop the floppy motor then
    // the BIOS wouldn't be informed and if the
    // next read is to a floppy then the BIOS will
    // still think the motor is on and this will
    // result in a read error.
    //DiskStopFloppyMotor();
    //DisableA20();
    ChainLoadBiosBootSectorCode();
}

VOID
LoadAndBootPartition(IN OperatingSystemItem* OperatingSystem,
                     IN USHORT OperatingSystemVersion)
{
    ULONG_PTR        SectionId;
    PCSTR            SectionName = OperatingSystem->SystemPartition;
    CHAR            SettingName[80];
    CHAR            SettingValue[80];
    PARTITION_TABLE_ENTRY    PartitionTableEntry;
    UCHAR            DriveNumber;
    ULONG            PartitionNumber;

    // Find all the message box settings and run them
    UiShowMessageBoxesInSection(SectionName);

    // Try to open the operating system section in the .ini file
    if (!IniOpenSection(SectionName, &SectionId))
    {
        sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", SectionName);
        UiMessageBox(SettingName);
        return;
    }

    // Read the boot drive
    if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, sizeof(SettingValue)))
    {
        UiMessageBox("Boot drive not specified for selected OS!");
        return;
    }

    DriveNumber = DriveMapGetBiosDriveNumber(SettingValue);

    // Read the boot partition
    if (!IniReadSettingByName(SectionId, "BootPartition", SettingValue, sizeof(SettingValue)))
    {
        UiMessageBox("Boot partition not specified for selected OS!");
        return;
    }

    PartitionNumber = atoi(SettingValue);

    // Get the partition table entry
    if (!DiskGetPartitionEntry(DriveNumber, PartitionNumber, &PartitionTableEntry))
    {
        return;
    }

    // Now try to read the partition boot sector
    // If this fails then abort
    if (!MachDiskReadLogicalSectors(DriveNumber, PartitionTableEntry.SectorCountBeforePartition, 1, (PVOID)0x7C00))
    {
        UiMessageBox("Unable to read partition's boot sector.");
        return;
    }

    // Check for validity
    if (*((USHORT*)(0x7c00 + 0x1fe)) != 0xaa55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return;
    }

    UiUnInitialize("Booting...");
    // Don't stop the floppy drive motor when we
    // are just booting a bootsector, or drive, or partition.
    // If we were to stop the floppy motor then
    // the BIOS wouldn't be informed and if the
    // next read is to a floppy then the BIOS will
    // still think the motor is on and this will
    // result in a read error.
    //DiskStopFloppyMotor();
    //DisableA20();
    FrldrBootDrive = DriveNumber;
    ChainLoadBiosBootSectorCode();
}

VOID
LoadAndBootDrive(IN OperatingSystemItem* OperatingSystem,
                 IN USHORT OperatingSystemVersion)
{
    ULONG_PTR    SectionId;
    PCSTR    SectionName = OperatingSystem->SystemPartition;
    CHAR    SettingName[80];
    CHAR    SettingValue[80];
    UCHAR    DriveNumber;

    // Find all the message box settings and run them
    UiShowMessageBoxesInSection(SectionName);

    // Try to open the operating system section in the .ini file
    if (!IniOpenSection(SectionName, &SectionId))
    {
        sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", SectionName);
        UiMessageBox(SettingName);
        return;
    }

    if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, sizeof(SettingValue)))
    {
        UiMessageBox("Boot drive not specified for selected OS!");
        return;
    }

    DriveNumber = DriveMapGetBiosDriveNumber(SettingValue);

    // Now try to read the boot sector (or mbr)
    // If this fails then abort
    if (!MachDiskReadLogicalSectors(DriveNumber, 0, 1, (PVOID)0x7C00))
    {
        UiMessageBox("Unable to read boot sector");
        return;
    }

    // Check for validity
    if (*((USHORT*)(0x7c00 + 0x1fe)) != 0xaa55)
    {
        UiMessageBox("Invalid boot sector magic (0xaa55)");
        return;
    }

    UiUnInitialize("Booting...");
    // Don't stop the floppy drive motor when we
    // are just booting a bootsector, or drive, or partition.
    // If we were to stop the floppy motor then
    // the BIOS wouldn't be informed and if the
    // next read is to a floppy then the BIOS will
    // still think the motor is on and this will
    // result in a read error.
    //DiskStopFloppyMotor();
    //DisableA20();
    FrldrBootDrive = DriveNumber;
    ChainLoadBiosBootSectorCode();
}

#endif // _M_IX86
