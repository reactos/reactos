/*
 *  FreeLoader - registry.h
 *
 *  Copyright (C) 2001  Eric Kohl
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

#ifndef __REGISTRY_H
#define __REGISTRY_H

typedef HANDLE HKEY, *PHKEY;

LONG
RegInitCurrentControlSet(BOOLEAN LastKnownGood);

LONG
RegEnumKey(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR Name,
    _Inout_ ULONG* NameSize,
    _Out_opt_ PHKEY SubKey);

LONG
RegOpenKey(HKEY ParentKey,
           PCWSTR KeyName,
           PHKEY Key);

LONG
RegQueryValue(HKEY Key,
          PCWSTR ValueName,
          ULONG* Type,
          PUCHAR Data,
          ULONG* DataSize);

LONG
RegEnumValue(HKEY Key,
         ULONG Index,
         PWCHAR ValueName,
         ULONG* NameSize,
         ULONG* Type,
         PUCHAR Data,
         ULONG* DataSize);

BOOLEAN
RegImportBinaryHive(PVOID ChunkBase,
             ULONG ChunkSize);

#endif /* __REGISTRY_H */

/* EOF */
