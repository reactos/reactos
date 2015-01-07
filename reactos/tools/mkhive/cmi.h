/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/cmi.h
 * PURPOSE:         Registry file manipulation routines
 * PROGRAMMER:      Hervé Poussineau
 */

#define VERIFY_KEY_CELL(key)
#define VERIFY_VALUE_LIST_CELL(cell)

NTSTATUS
CmiInitializeTempHive(
    IN OUT PCMHIVE Hive);

NTSTATUS
CmiAddSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN ULONG CreateOptions,
    OUT PCM_KEY_NODE *pSubKeyCell,
    OUT HCELL_INDEX *pBlockOffset);

NTSTATUS
CmiScanForSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN ULONG Attributes,
    OUT PCM_KEY_NODE *pSubKeyCell,
    OUT HCELL_INDEX *pBlockOffset);

NTSTATUS
CmiAddValueKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX KeyCellOffset,
    IN PCUNICODE_STRING ValueName,
    OUT PCM_KEY_VALUE *pValueCell,
    OUT HCELL_INDEX *pValueCellOffset);

NTSTATUS
CmiScanForValueKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX KeyCellOffset,
    IN PCUNICODE_STRING ValueName,
    OUT PCM_KEY_VALUE *pValueCell,
    OUT HCELL_INDEX *pValueCellOffset);
