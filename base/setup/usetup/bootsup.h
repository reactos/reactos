/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/bootsup.h
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

NTSTATUS
CreateFreeLoaderIniForDos(
    PWCHAR IniPath,
    PWCHAR ArcPath);

NTSTATUS
CreateFreeLoaderIniForReactOS(
    PWCHAR IniPath,
    PWCHAR ArcPath);

NTSTATUS
UpdateFreeLoaderIni(
    PWCHAR IniPath,
    PWCHAR ArcPath);

NTSTATUS
SaveCurrentBootSector(
    PWSTR RootPath,
    PWSTR DstPath);

NTSTATUS
InstallFat16BootCodeToFile(
    PWSTR SrcPath,
    PWSTR DstPath,
    PWSTR RootPath);

NTSTATUS
InstallFat32BootCodeToFile(
    PWSTR SrcPath,
    PWSTR DstPath,
    PWSTR RootPath);

NTSTATUS
InstallMbrBootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath);

NTSTATUS
InstallFat16BootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath);

NTSTATUS
InstallFat32BootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath);

NTSTATUS
UpdateBootIni(
    PWSTR BootIniPath,
    PWSTR EntryName,
    PWSTR EntryValue);

BOOLEAN
CheckInstallFatBootcodeToPartition(
    PUNICODE_STRING SystemRootPath);

NTSTATUS
InstallFatBootcodeToPartition(
    PUNICODE_STRING SystemRootPath,
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath,
    UCHAR PartitionType);

NTSTATUS
InstallVBRToPartition(
    PUNICODE_STRING SystemRootPath,
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath,
    UCHAR PartitionType);

NTSTATUS
InstallFatBootcodeToFloppy(
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath);

/* EOF */
