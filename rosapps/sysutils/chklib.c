/* $Id: chklib.c,v 1.2 2000/02/29 23:57:46 ea Exp $
 * 
 * chklib.c
 * 
 * Copyright (C) 1998, 1999 Emanuele Aliberti.
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this software; see the file COPYING. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * Check a PE DLL for loading and get an exported symbol's address
 * (relocated).
 *
 */
//#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "win32err.h"

#ifdef DISPLAY_VERSION
static
void
DisplayVersion(
	HANDLE  dll,
	PCHAR   ModuleName
	)
{
	DWORD   Zero;
	DWORD   Size;
	PVOID   vi = NULL;

	assert(ModuleName);
	Size = GetFileVersionInfoSize(
		ModuleName,
		& Zero
		);
	if (Size == 0) 
	{
		PrintWin32Error(
			L"GetFileVersionInfoSize",
			GetLastError()
			);
		return;
	}
	vi = (PVOID) LocalAlloc(LMEM_ZEROINIT,Size);
	if (!vi) return;
	assert(dll != INVALID_HANDLE_VALUE);
	if (0 == GetFileVersionInfo(
			ModuleName,
			(DWORD) dll,
			Size,
			vi
			)
	) {
		PrintWin32Error(
			L"GetFileVersionInfo",
			GetLastError()
			);
		return;
	}
/*
	VerQueryValue(
		vi, 
		L"\\StringFileInfo\\040904E4\\FileDescription", 
		& lpBuffer,
		& dwBytes
		);
*/
	LocalFree(vi);
}
#endif /* def DISPLAY_VERSION */


static
void
DisplayEntryPoint(
	const HANDLE	dll,
	LPCSTR		SymbolName
	)
{
	FARPROC	EntryPoint;
	
	printf(
		"[%s]\n",
		SymbolName
		);
	EntryPoint = GetProcAddress(
			dll,
			SymbolName
			);
	if (!EntryPoint)
	{
		PrintWin32Error(
			L"GetProcAddress",
			GetLastError()
			);
		return;
	}
	printf(
		"%08X  %s\n",
		EntryPoint,
		SymbolName
		);
}


/* --- MAIN --- */


int
main(
	int	argc,
	char *	argv []
	)
{
	HINSTANCE	dll;
	TCHAR		ModuleName [_MAX_PATH];

	if (argc < 2) 
	{
		fprintf(
			stderr, 
			"\
ReactOS System Tools\n\
Check a Dynamic Link Library (DLL) for loading\n\
Copyright (c) 1998, 1999 Emanuele Aliberti\n\n\
usage: %s module [symbol [, ...]]\n",
			argv[0]
			);
		exit(EXIT_FAILURE);
	}
	dll = LoadLibraryA(argv[1]);
	if (!dll)
	{
		UINT    LastError;

		LastError = GetLastError();
		PrintWin32Error(L"LoadLibrary",LastError);
		fprintf(
			stderr,
			"%s: loading %s failed (%d).\n",
			argv[0],
			argv[1],
			LastError       
			);
		exit(EXIT_FAILURE);
	}
	GetModuleFileName(
		(HANDLE) dll,
		ModuleName,
		sizeof ModuleName
		);
	printf(
		"%s loaded.\n",
		ModuleName
		);
#ifdef DISPLAY_VERSION
	DisplayVersion(dll,ModuleName);
#endif
	if (argc > 2)
	{
		int	CurrentSymbol;

		for (	CurrentSymbol = 2;
			(CurrentSymbol < argc);
			++CurrentSymbol
			)
		{
			DisplayEntryPoint( dll, argv[CurrentSymbol] );
		}
	}
	FreeLibrary(dll);
	printf(
		"%s unloaded.\n",
		ModuleName
		);
	return EXIT_SUCCESS;
}

/* EOF */
