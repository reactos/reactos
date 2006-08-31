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
 * FILE:            subsys/system/usetup/inffile.c
 * PURPOSE:         .inf files support functions
 * PROGRAMMER:      Hervé Poussineau
 */

/* INCLUDES ******************************************************************/

#include "usetup.h"
#include <infros.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID WINAPI
InfpCloseInfFile(
	IN HINF InfHandle)
{
	InfCloseFile(InfHandle);
}

BOOL WINAPI
InfpFindFirstLineW(
	IN HINF InfHandle,
	IN PCWSTR Section,
	IN PCWSTR Key,
	IN OUT PINFCONTEXT Context)
{
	PINFCONTEXT pContext;
	BOOL ret;

	ret = InfFindFirstLine(InfHandle, Section, Key, &pContext);
	if (!ret)
		return FALSE;

	memcpy(Context, pContext, sizeof(INFCONTEXT));
	InfFreeContext(pContext);
	return TRUE;
}

BOOL WINAPI
InfpFindNextLine(
	IN PINFCONTEXT ContextIn,
	OUT PINFCONTEXT ContextOut)
{
	return InfFindNextLine(ContextIn, ContextOut);
}

BOOL WINAPI
InfpGetBinaryField(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT BYTE* ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT LPDWORD RequiredSize)
{
	return InfGetBinaryField(Context, FieldIndex, ReturnBuffer, ReturnBufferSize, RequiredSize);
}

DWORD WINAPI
InfpGetFieldCount(
	IN PINFCONTEXT Context)
{
	return (DWORD)InfGetFieldCount(Context);
}

BOOL WINAPI
InfpGetIntField(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	OUT PINT IntegerValue)
{
	LONG IntegerValueL;
	BOOL ret;

	ret = InfGetIntField(Context, FieldIndex, &IntegerValueL);
	*IntegerValue = (INT)IntegerValueL;
	return ret;
}

BOOL WINAPI
InfpGetMultiSzFieldW(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT PWSTR ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT LPDWORD RequiredSize)
{
	return InfGetMultiSzField(Context, FieldIndex, ReturnBuffer, ReturnBufferSize, RequiredSize);
}

BOOL WINAPI
InfpGetStringFieldW(
	IN PINFCONTEXT Context,
	IN DWORD FieldIndex,
	IN OUT PWSTR ReturnBuffer,
	IN DWORD ReturnBufferSize,
	OUT PDWORD RequiredSize)
{
	return InfGetStringField(Context, FieldIndex, ReturnBuffer, ReturnBufferSize, RequiredSize);
}

HINF WINAPI
InfpOpenInfFileW(
	IN PCWSTR FileName,
	IN PCWSTR InfClass,
	IN DWORD InfStyle,
	OUT PUINT ErrorLine)
{
	HINF hInf = NULL;
	UNICODE_STRING FileNameU;
	ULONG ErrorLineUL;
	NTSTATUS Status;

	RtlInitUnicodeString(&FileNameU, FileName);
	Status = InfOpenFile(
		&hInf,
		&FileNameU,
		&ErrorLineUL);
	*ErrorLine = (UINT)ErrorLineUL;
	if (!NT_SUCCESS(Status))
		return NULL;

	return hInf;
}

BOOLEAN
INF_GetData(
	IN PINFCONTEXT Context,
	OUT PWCHAR *Key,
	OUT PWCHAR *Data)
{
	return InfGetData(Context, Key, Data);
}

BOOLEAN
INF_GetDataField(
	IN PINFCONTEXT Context,
	IN ULONG FieldIndex,
	OUT PWCHAR *Data)
{
	return InfGetDataField(Context, FieldIndex, Data);
}

HINF WINAPI
INF_OpenBufferedFileA(
	IN PSTR FileBuffer,
	IN ULONG FileSize,
	IN PCSTR InfClass,
	IN DWORD InfStyle,
	OUT PUINT ErrorLine)
{
	HINF hInf = NULL;
	ULONG ErrorLineUL;
	NTSTATUS Status;

	Status = InfOpenBufferedFile(
		&hInf,
		FileBuffer,
		FileSize,
		&ErrorLineUL);
	*ErrorLine = (UINT)ErrorLineUL;
	if (!NT_SUCCESS(Status))
		return NULL;

	return hInf;
}

VOID INF_SetHeap(
	IN PVOID Heap)
{
	InfSetHeap(Heap);
}

/* EOF */
