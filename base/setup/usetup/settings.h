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

/* EOF */
