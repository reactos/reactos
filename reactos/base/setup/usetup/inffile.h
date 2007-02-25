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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/inffile.h
 * PURPOSE:         .inf files support functions
 * PROGRAMMER:      Hervé Poussineau
 */

#ifndef __INFFILE_H__
#define __INFFILE_H__

#ifndef __REACTOS__

#include <setupapi.h>

#else /* __REACTOS__ */

#include <infcommon.h>

#define SetupCloseInfFile InfpCloseInfFile
#define SetupFindFirstLineW InfpFindFirstLineW
#define SetupFindNextLine InfpFindNextLine
#define SetupGetBinaryField InfpGetBinaryField
#define SetupGetFieldCount InfpGetFieldCount
#define SetupGetIntField InfpGetIntField
#define SetupGetMultiSzFieldW InfpGetMultiSzFieldW
#define SetupGetStringFieldW InfpGetStringFieldW
#define SetupOpenInfFileW InfpOpenInfFileW

#define INF_STYLE_WIN4 0x00000002

/* FIXME: this structure is the one used in inflib, not in setupapi
 * Delete it once we don't use inflib anymore */
typedef struct _INFCONTEXT
{
	PVOID Inf;
	PVOID Section;
	PVOID Line;
} INFCONTEXT;

VOID WINAPI
InfpCloseInfFile(
	IN HINF InfHandle);

BOOL WINAPI
InfpFindFirstLineW(
	IN HINF InfHandle,
	IN PCWSTR Section,
	IN PCWSTR Key,
	IN OUT PINFCONTEXT Context);

BOOL WINAPI
InfpFindNextLine(
	IN PINFCONTEXT ContextIn,
	OUT PINFCONTEXT ContextOut);

BOOL WINAPI
InfpGetBinaryField(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT BYTE* ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT LPDWORD RequiredSize);

DWORD WINAPI
InfpGetFieldCount(
	IN PINFCONTEXT Context);

BOOL WINAPI
InfpGetIntField(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	OUT PINT IntegerValue);

BOOL WINAPI
InfpGetMultiSzFieldW(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT PWSTR ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT LPDWORD RequiredSize);

BOOL WINAPI
InfpGetStringFieldW(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT PWSTR ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT PDWORD RequiredSize);

HINF WINAPI
InfpOpenInfFileW(
	IN PCWSTR FileName,
	IN PCWSTR InfClass,
	IN DWORD InfStyle,
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
	OUT PUINT ErrorLine);

VOID INF_SetHeap(
	IN PVOID Heap);

#endif /* __INFFILE_H__*/

/* EOF */
