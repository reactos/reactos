/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 * FILE:            base/setup/lib/registry.h
 * PURPOSE:         Registry creation functions
 * PROGRAMMERS:     Eric Kohl
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

HANDLE
GetRootKeyByPredefKey(
    IN HANDLE KeyHandle,
    OUT PCWSTR* RootKeyMountPoint OPTIONAL);

HANDLE
GetRootKeyByName(
    IN PCWSTR RootKeyName,
    OUT PCWSTR* RootKeyMountPoint OPTIONAL);

BOOLEAN
ImportRegistryFile(
    IN PCWSTR SourcePath,
    IN PCWSTR FileName,
    IN PCWSTR Section,
    IN LCID LocaleId,
    IN BOOLEAN Delete);

NTSTATUS
VerifyRegistryHives(
    IN PUNICODE_STRING InstallPath,
    OUT PBOOLEAN ShouldRepairRegistry);

NTSTATUS
RegInitializeRegistry(
    IN PUNICODE_STRING InstallPath);

VOID
RegCleanupRegistry(
    IN PUNICODE_STRING InstallPath);

/* EOF */
