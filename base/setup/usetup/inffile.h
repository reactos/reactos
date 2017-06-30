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
 * PROGRAMMER:      Hervé Poussineau
 */

#pragma once

#ifndef __REACTOS__

#include <setupapi.h>

#else /* __REACTOS__ */

#include <infcommon.h>

extern VOID InfSetHeap(PVOID Heap);
extern VOID InfCloseFile(HINF InfHandle);
extern BOOLEAN InfFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
extern BOOLEAN InfGetBinaryField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PUCHAR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);
extern BOOLEAN InfGetMultiSzField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  PWSTR ReturnBuffer,
                                  ULONG ReturnBufferSize,
                                  PULONG RequiredSize);
extern BOOLEAN InfGetStringField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PWSTR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);

#define SetupCloseInfFile InfCloseFile
#define SetupFindNextLine InfFindNextLine
#define SetupGetBinaryField InfGetBinaryField
#define SetupGetMultiSzFieldW InfGetMultiSzField
#define SetupGetStringFieldW InfGetStringField


#define SetupFindFirstLineW InfpFindFirstLineW
#define SetupGetFieldCount InfGetFieldCount
#define SetupGetIntField InfGetIntField

// SetupOpenInfFileW with support for a user-provided LCID
#define SetupOpenInfFileExW InfpOpenInfFileW

#define INF_STYLE_WIN4 0x00000002

/* FIXME: this structure is the one used in inflib, not in setupapi
 * Delete it once we don't use inflib anymore */
typedef struct _INFCONTEXT
{
  HINF Inf;
  HINF CurrentInf;
  UINT Section;
  UINT Line;
} INFCONTEXT;
C_ASSERT(sizeof(INFCONTEXT) == 2 * sizeof(PVOID) + 2 * sizeof(UINT));

BOOL
WINAPI
InfpFindFirstLineW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN OUT PINFCONTEXT Context);

HINF
WINAPI
InfpOpenInfFileW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);

#endif /* __REACTOS__ */

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
