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

#ifdef __REACTOS__
#include <infros.h>
#endif

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#ifdef __REACTOS__

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
		return INVALID_HANDLE_VALUE;

	return hInf;
}

#endif /* __REACTOS__ */

BOOLEAN
INF_GetData(
	IN PINFCONTEXT Context,
	OUT PWCHAR *Key,
	OUT PWCHAR *Data)
{
#ifdef __REACTOS__
	return InfGetData(Context, Key, Data);
#else
	static PWCHAR pLastCallData[4] = { NULL, NULL, NULL, NULL };
	static DWORD currentIndex = 0;
	DWORD dwSize, i;
	BOOL ret;

	currentIndex ^= 2;

	if (Key) *Key = NULL;
	if (Data) *Data = NULL;

	if (SetupGetFieldCount(Context) != 1)
		return FALSE;

	for (i = 0; i <= 1; i++)
	{
		ret = SetupGetStringFieldW(
			Context,
			i,
			NULL,
			0,
			&dwSize);
		if (!ret)
			return FALSE;
		HeapFree(GetProcessHeap(), 0, pLastCallData[i + currentIndex]);
		pLastCallData[i + currentIndex] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
		ret = SetupGetStringFieldW(
			Context,
			i,
			pLastCallData[i + currentIndex],
			dwSize,
			NULL);
		if (!ret)
			return FALSE;
	}

	if (Key)
		*Key = pLastCallData[0 + currentIndex];
	if (Data)
		*Data = pLastCallData[1 + currentIndex];
	return TRUE;
#endif /* !__REACTOS__ */
}

BOOLEAN
INF_GetDataField(
	IN PINFCONTEXT Context,
	IN ULONG FieldIndex,
	OUT PWCHAR *Data)
{
#ifdef __REACTOS__
	return InfGetDataField(Context, FieldIndex, Data);
#else
	static PWCHAR pLastCallsData[] = { NULL, NULL, NULL };
	static DWORD NextIndex = 0;
	DWORD dwSize;
	BOOL ret;

	*Data = NULL;

	ret = SetupGetStringFieldW(
		Context,
		FieldIndex,
		NULL,
		0,
		&dwSize);
	if (!ret)
		return FALSE;
	HeapFree(GetProcessHeap(), 0, pLastCallsData[NextIndex]);
	pLastCallsData[NextIndex] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
	ret = SetupGetStringFieldW(
		Context,
		FieldIndex,
		pLastCallsData[NextIndex],
		dwSize,
		NULL);
	if (!ret)
		return FALSE;

	*Data = pLastCallsData[NextIndex];
	NextIndex = (NextIndex + 1) % (sizeof(pLastCallsData) / sizeof(pLastCallsData[0]));
	return TRUE;
#endif /* !__REACTOS__ */
}

HINF WINAPI
INF_OpenBufferedFileA(
	IN PSTR FileBuffer,
	IN ULONG FileSize,
	IN PCSTR InfClass,
	IN DWORD InfStyle,
	OUT PUINT ErrorLine)
{
#ifdef __REACTOS__
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
		return INVALID_HANDLE_VALUE;

	return hInf;
#else
	return INVALID_HANDLE_VALUE;
#endif /* !__REACTOS__ */
}

VOID INF_SetHeap(
	IN PVOID Heap)
{
#ifdef __REACTOS__
	InfSetHeap(Heap);
#endif
}

/* EOF */
