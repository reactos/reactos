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
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/bootsup.h
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

NTSTATUS
InstallMbrBootCodeToDisk(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PCWSTR DestinationDevicePathBuffer);

NTSTATUS
InstallVBRToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath,
    IN UCHAR PartitionType);

NTSTATUS
InstallFatBootcodeToFloppy(
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath);

/* EOF */
