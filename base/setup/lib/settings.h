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
 * PROGRAMMERS:     Eric Kohl
 *                  Colin Finck
 */

#pragma once

/*
 * Return values:
 * 0x00: Failure, stop the enumeration;
 * 0x01: Add the entry and continue the enumeration;
 * 0x02: Skip the entry but continue the enumeration.
 */
typedef UCHAR
(NTAPI *PPROCESS_ENTRY_ROUTINE)(
    IN PWCHAR KeyName,
    IN PWCHAR KeyValue,
    IN PCHAR DisplayText,
    IN SIZE_T DisplayTextSize,
    OUT PVOID* UserData,
    OUT PBOOLEAN Current,
    IN PVOID Parameter OPTIONAL);

LONG
AddEntriesFromInfSection(
    IN OUT PGENERIC_LIST List,
    IN HINF InfFile,
    IN PCWSTR SectionName,
    IN PINFCONTEXT pContext,
    IN PPROCESS_ENTRY_ROUTINE ProcessEntry,
    IN PVOID Parameter OPTIONAL);

UCHAR
NTAPI
DefaultProcessEntry(
    IN PWCHAR KeyName,
    IN PWCHAR KeyValue,
    IN PCHAR DisplayText,
    IN SIZE_T DisplayTextSize,
    OUT PVOID* UserData,
    OUT PBOOLEAN Current,
    IN PVOID Parameter OPTIONAL);


PGENERIC_LIST
CreateComputerTypeList(
    HINF InfFile);

PGENERIC_LIST
CreateDisplayDriverList(
    HINF InfFile);

BOOLEAN
ProcessComputerFiles(
    HINF InfFile,
    PGENERIC_LIST List,
    PWCHAR *AdditionalSectionName);

BOOLEAN
ProcessDisplayRegistry(
    HINF InfFile,
    PGENERIC_LIST List);

PGENERIC_LIST
CreateKeyboardDriverList(
    HINF InfFile);

#if 0 // FIXME: Disabled for now because it uses MUI* functions from usetup

PGENERIC_LIST
CreateKeyboardLayoutList(
    HINF InfFile,
    WCHAR *DefaultKBLayout);

PGENERIC_LIST
CreateLanguageList(
    HINF InfFile,
    WCHAR *DefaultLanguage);

ULONG
GetDefaultLanguageIndex(VOID);

BOOLEAN
ProcessKeyboardLayoutRegistry(
    PGENERIC_LIST List);

BOOLEAN
ProcessKeyboardLayoutFiles(
    PGENERIC_LIST List);

#endif

BOOLEAN
ProcessLocaleRegistry(
    PGENERIC_LIST List);

BOOLEAN
SetGeoID(
    PWCHAR Id);

BOOLEAN
SetDefaultPagefile(
    WCHAR Drive);

/* EOF */
