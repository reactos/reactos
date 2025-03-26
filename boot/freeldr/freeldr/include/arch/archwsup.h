/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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

#pragma once

/* PROTOTYPES ***************************************************************/

VOID
AddReactOSArcDiskInfo(
    IN PSTR ArcName,
    IN ULONG Signature,
    IN ULONG Checksum,
    IN BOOLEAN ValidPartitionTable);

//
// ARC Component Configuration Routines
//
VOID
FldrSetConfigurationData(
    _Inout_ PCONFIGURATION_COMPONENT_DATA ComponentData,
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _In_ ULONG Size);

VOID
FldrCreateSystemKey(
    _Out_ PCONFIGURATION_COMPONENT_DATA* SystemNode,
    _In_ PCSTR IdentifierString);

VOID
FldrCreateComponentKey(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemNode,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_ IDENTIFIER_FLAG Flags,
    _In_ ULONG Key,
    _In_ ULONG Affinity,
    _In_ PCSTR IdentifierString,
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _In_ ULONG Size,
    _Out_ PCONFIGURATION_COMPONENT_DATA* ComponentKey);

/* EOF */
