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

PGENERIC_LIST
CreateComputerTypeList(
    IN HINF InfFile);

PGENERIC_LIST
CreateDisplayDriverList(
    IN HINF InfFile);

BOOLEAN
ProcessComputerFiles(
    IN HINF InfFile,
    IN PGENERIC_LIST List,
    OUT PWSTR* AdditionalSectionName);

BOOLEAN
ProcessDisplayRegistry(
    IN HINF InfFile,
    IN PGENERIC_LIST List);

PGENERIC_LIST
CreateKeyboardDriverList(
    IN HINF InfFile);

PGENERIC_LIST
CreateKeyboardLayoutList(
    IN HINF InfFile,
    IN PCWSTR LanguageId,
    OUT PWSTR DefaultKBLayout);

PGENERIC_LIST
CreateLanguageList(
    IN HINF InfFile,
    OUT PWSTR DefaultLanguage);

ULONG
GetDefaultLanguageIndex(VOID);

BOOLEAN
ProcessKeyboardLayoutRegistry(
    IN PGENERIC_LIST List,
    IN PCWSTR LanguageId);

BOOLEAN
ProcessKeyboardLayoutFiles(
    IN PGENERIC_LIST List);

BOOLEAN
ProcessLocaleRegistry(
    IN PGENERIC_LIST List);

BOOLEAN
SetGeoID(
    IN PCWSTR Id);

BOOLEAN
SetDefaultPagefile(
    IN WCHAR Drive);

/* EOF */
