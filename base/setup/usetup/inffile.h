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
 * FILE:            base/setup/usetup/inffile.h
 * PURPOSE:         .inf files support functions
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#ifndef __REACTOS__

#include <setupapi.h>

#else /* __REACTOS__ */

/* Functions from the INFLIB library */
#include <infcommon.h>

#define INF_STYLE_WIN4 0x00000002

/* FIXME: this structure is the one used in inflib, not in setupapi
 * Delete it once we don't use inflib anymore */
typedef struct _INFCONTEXT
{
  PVOID Inf;
  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT;
C_ASSERT(sizeof(INFCONTEXT) == 2 * sizeof(PVOID) + 2 * sizeof(UINT));

extern VOID InfSetHeap(PVOID Heap);

// #define SetupCloseInfFile InfCloseFile
VOID
WINAPI
SetupCloseInfFile(HINF InfHandle);

// #define SetupFindFirstLineW InfpFindFirstLineW
BOOL
WINAPI
SetupFindFirstLineW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN OUT PINFCONTEXT Context);

// #define SetupFindNextLine InfFindNextLine
BOOL
WINAPI
SetupFindNextLine(PINFCONTEXT ContextIn,
                  PINFCONTEXT ContextOut);

// #define SetupGetFieldCount InfGetFieldCount
LONG
WINAPI
SetupGetFieldCount(PINFCONTEXT Context);

// #define SetupGetIntField InfGetIntField
BOOLEAN
WINAPI
SetupGetIntField(PINFCONTEXT Context,
                 ULONG FieldIndex,
                 INT *IntegerValue);

// #define SetupGetBinaryField InfGetBinaryField
BOOL
WINAPI
SetupGetBinaryField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    PUCHAR ReturnBuffer,
                    ULONG ReturnBufferSize,
                    PULONG RequiredSize);

// #define SetupGetMultiSzFieldW InfGetMultiSzField
BOOL
WINAPI
SetupGetMultiSzFieldW(PINFCONTEXT Context,
                      ULONG FieldIndex,
                      PWSTR ReturnBuffer,
                      ULONG ReturnBufferSize,
                      PULONG RequiredSize);

// #define SetupGetStringFieldW InfGetStringField
BOOL
WINAPI
SetupGetStringFieldW(PINFCONTEXT Context,
                     ULONG FieldIndex,
                     PWSTR ReturnBuffer,
                     ULONG ReturnBufferSize,
                     PULONG RequiredSize);

/* SetupOpenInfFileW with support for a user-provided LCID */
// #define SetupOpenInfFileExW InfpOpenInfFileW
HINF
WINAPI
SetupOpenInfFileExW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);

#endif /* __REACTOS__ */


/* HELPER FUNCTIONS **********************************************************/

BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PWCHAR *Key,
    OUT PWCHAR *Data);

BOOLEAN
INF_GetDataField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT PWCHAR *Data);

HINF WINAPI
INF_OpenBufferedFileA(
    IN PSTR FileBuffer,
    IN ULONG FileSize,
    IN PCSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);

/* EOF */
