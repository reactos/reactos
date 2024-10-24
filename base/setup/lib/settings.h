/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
 * FILE:            base/setup/usetup/settings.h
 * PURPOSE:         Device settings support functions
 * PROGRAMMERS:     Colin Finck
 */

#pragma once

/* Setting entries with simple 1:1 mapping */
typedef struct _GENENTRY
{
    union
    {
        PCWSTR Str;
        ULONG_PTR Ul;
    } Id;
    PCWSTR Value;
} GENENTRY, *PGENENTRY;

#if 1
PGENERIC_LIST
CreateComputerTypeList(
    _In_ HINF InfFile);

PGENERIC_LIST
CreateDisplayDriverList(
    _In_ HINF InfFile);

PGENERIC_LIST
CreateKeyboardDriverList(
    _In_ HINF InfFile);

PGENERIC_LIST
CreateLanguageList(
    _In_ HINF InfFile,
    _Inout_ LANGID* DefaultLanguage);

PGENERIC_LIST
CreateKeyboardLayoutList(
    _In_ HINF InfFile,
    _In_ LANGID LanguageId,
    _Out_ KLID* DefaultKBLayout);
#endif


/**
 * @brief
 * Callback type for enumerating "Name=Value" entries from INF section.
 *
 * @param[in]   KeyName
 * The name of the key.
 *
 * @param[in]   KeyValue
 * The optional value of the key.
 *
 * @param[in]   Parameter
 * Optional parameter context for the callback.
 *
 * @return
 * 0x00: Failure, stop the enumeration;
 * 0x01: Add the entry and continue the enumeration;
 * 0x02: Skip the entry but continue the enumeration.
 **/
typedef UCHAR
(NTAPI *PENUM_ENTRY_PROC)(
    _In_ PCWSTR KeyName,
    _In_opt_ PCWSTR KeyValue,
    /**/_In_ ULONG_PTR DefaultEntry,/**/ // PCWSTR DefaultKeyName
    _In_opt_ PVOID Parameter);

BOOLEAN
EnumComputerTypeEntries(
    _In_ HINF InfFile,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter);

BOOLEAN
EnumDisplayDriverEntries(
    _In_ HINF InfFile,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter);

BOOLEAN
EnumKeyboardDriverEntries(
    _In_ HINF InfFile,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter);

BOOLEAN
EnumLanguageEntries(
    _In_ HINF InfFile,
    _Out_ LANGID* DefaultLanguage,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter);

BOOLEAN
EnumKeyboardLayoutEntries(
    _In_ HINF InfFile,
    _In_ LANGID LanguageId,
    _Out_ KLID* DefaultKBLayout,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter);


ULONG
GetDefaultLanguageIndex(VOID);


BOOLEAN
ProcessComputerFiles(
    _In_ HINF InfFile,
    _In_ PCWSTR ComputerType,
    _Out_ PWSTR* AdditionalSectionName);

BOOLEAN
ProcessDisplayRegistry(
    _In_ HINF InfFile,
    _In_ PCWSTR DisplayType);

BOOLEAN
ProcessKeyboardLayoutRegistry(
    _In_ KLID LayoutId,
    _In_ LANGID LanguageId);

#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(
    IN PGENERIC_LIST List);
#endif

BOOLEAN
ProcessLocaleRegistry(
    _In_ LCID LocaleId);

BOOLEAN
SetGeoID(
    _In_ GEOID GeoId);

BOOLEAN
SetDefaultPagefile(
    _In_ WCHAR Drive);

/* EOF */
