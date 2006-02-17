/*
 *  ReactOS INF Helper
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
 *
 * PROJECT:         INF Helper
 * FILE:            infinst.c
 * PURPOSE:         Pass INF files to setupapi.dll for execution
 * PROGRAMMER:      Michael Biggins
 * UPDATE HISTORY:
 *                  Created 19/09/2004
 */
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#ifdef UNICODE
VOID WINAPI InstallHinfSectionW(HWND hwnd, HINSTANCE handle, LPCWSTR cmdline, INT show);
#define InstallHinfSection InstallHinfSectionW
#else
VOID WINAPI InstallHinfSectionA(HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show);
#define InstallHinfSection InstallHinfSectionA
#endif

#define FILEOPEN_FILTER	TEXT("Inf Files (*.inf)\0*.inf\0All Files (*.*)\0*.*\0\0")
#define FILEOPEN_TITLE	TEXT("INF file to process")
#define FILEOPEN_DEFEXT	TEXT(".inf")
#define INF_COMMAND	TEXT("DefaultInstall 128 %s")

int
_tmain(int argc, TCHAR *argv[])
{
	TCHAR infCommand[MAX_PATH + 32];

	if (argc <= 1)
	{
		TCHAR FileName[MAX_PATH + 1];
		OPENFILENAME ofc;
		int rv;

		ZeroMemory(&ofc, sizeof(ofc));
		ZeroMemory(FileName, MAX_PATH + 1);
		ofc.lStructSize = sizeof(ofc);
		ofc.lpstrFilter = FILEOPEN_FILTER;
		ofc.nFilterIndex = 1;
		ofc.lpstrTitle = FILEOPEN_TITLE;
		ofc.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_PATHMUSTEXIST;
		ofc.lpstrDefExt = FILEOPEN_DEFEXT;
		ofc.lpstrFile = FileName;
		ofc.nMaxFile = sizeof(FileName) / sizeof(TCHAR);

		rv = GetOpenFileName(&ofc);

		if (rv == 0)
			return 1;

		_stprintf(infCommand, INF_COMMAND, FileName);
	}
	else
	{
		if (_tcslen(argv[1]) > MAX_PATH)
		{
			MessageBox(NULL, TEXT("Command line too long to be a valid file name"), NULL, MB_OK | MB_ICONERROR);
			return 2; /* User error */
		}
		_stprintf(infCommand, INF_COMMAND, argv[1]);
	}

	InstallHinfSection(NULL, NULL, infCommand, 0);

	return 0;
}
