/* $Id: notevil.c,v 1.2 1999/10/03 22:10:15 ekohl Exp $
 *
 * notevil.c
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * ReactOS Coders Console Parade
 *
 * 19990411 EA
 * 19990515 EA
 */
//#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "resource.h"

LPCTSTR app_name = _TEXT("notevil");

HANDLE	myself;
HANDLE	ScreenBuffer;
CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

void
WriteStringAt(
	LPTSTR	lpString,
	COORD	xy,
	WORD	wColor
	)
{
	DWORD	cWritten = 0;
	WORD	wLen = lstrlen(lpString);

	if (0 == wLen)
		return;
	WriteConsoleOutputCharacter(
		ScreenBuffer,
		lpString,
		wLen,
		xy,
		& cWritten
		);
	FillConsoleOutputAttribute(
		ScreenBuffer,
		wColor,
		wLen,
		xy,
		& cWritten
		);
}


#ifdef DISPLAY_COORD
void
WriteCoord(COORD c)
{
	COORD xy = {0,0};
	TCHAR buf [40];

	wsprintf(
		buf,
		_TEXT("x=%02d  y=%02d"),
		c.X,
		c.Y
		);
	WriteStringAt(
		buf,
		xy,
		(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
		);
}
#endif /* def DISPLAY_COORD */


INT
GetNextString(
	LPTSTR	Buffer,
	INT	BufferSize,
	DWORD	*Index
	)
{
	if (RES_LAST_INDEX == *Index)
	{
		*Index = RES_FIRST_INDEX;
	}
	else
	{
		++*Index;
	}
	LoadString(
		myself,
		*Index,
		Buffer,
		BufferSize
		);
	return 0;
}


VOID
DisplayTitle(VOID)
{
	LPTSTR szTitle = _TEXT("ReactOS Coders Console Parade");
	COORD  xy;

	xy.X = (ScreenBufferInfo.dwSize.X - lstrlen(szTitle)) / 2;
	xy.Y = ScreenBufferInfo.dwSize.Y / 2;

	WriteStringAt(
		szTitle,
		xy,
		(FOREGROUND_GREEN | FOREGROUND_INTENSITY)
		);
}


#define RES_DELAY_CHANGE 12
#define RES_BUFFER_SIZE  1024
void
MainLoop(void)
{
	TCHAR	NameString [RES_BUFFER_SIZE];
	DWORD	NameIndex = 1;
	INT	NameLength = 0;
	COORD	xy;
	INT	n = RES_DELAY_CHANGE;
	INT	dir_y = 1;
	INT	dir_x = 1;
	WORD	wColor = 1;

	xy.X = ScreenBufferInfo.dwSize.X / 2;
	xy.Y = ScreenBufferInfo.dwSize.Y / 2;

	for ( ; 1; ++n )
	{
		if (n == RES_DELAY_CHANGE)
		{
			n = GetNextString(
				NameString,
				RES_BUFFER_SIZE,
				& NameIndex
				);
			NameLength = lstrlen(NameString);
			wColor++;
			if ((wColor & 0x000F) == 0)
				wColor = 1;
		}
		if (xy.X == 0)
		{
			if (dir_x == -1)
				dir_x = 1;
		}
		else if (xy.X >= ScreenBufferInfo.dwSize.X - NameLength - 1)
		{
			if (dir_x == 1)
				dir_x = -1;
		}
		xy.X += dir_x;

		if (xy.Y == 0)
		{
			if (dir_y == -1)
				dir_y = 1;
		}
		else if (xy.Y >= ScreenBufferInfo.dwSize.Y - 1)
		{
			if (dir_y == 1)
				dir_y = -1;
		}
		xy.Y += dir_y;
#ifdef DISPLAY_COORD
		WriteCoord(xy);
#endif /* def DISPLAY_COORD */
		DisplayTitle();
		WriteStringAt(
			NameString,
			xy,
			wColor
			);
		Sleep(100);
		WriteStringAt(
			NameString,
			xy,
			0
			);
	}
}


int
main(
	int	argc,
	char	*argv []
	)
{
	myself = GetModuleHandle(NULL);

#if 1
	GetConsoleScreenBufferInfo (GetStdHandle(STD_OUTPUT_HANDLE),
	                            &ScreenBufferInfo);
#else
	ScreenBufferInfo.dwSize.X = 80;
	ScreenBufferInfo.dwSize.Y = 25;
#endif

	ScreenBuffer = CreateConsoleScreenBuffer(
			GENERIC_WRITE,
			0,
			NULL,
			CONSOLE_TEXTMODE_BUFFER,
			NULL
			);
	if (INVALID_HANDLE_VALUE == ScreenBuffer)
	{
		_ftprintf(
			stderr,
			_TEXT("%s: could not create a new screen buffer\n"),
			app_name
			);
		return EXIT_FAILURE;
	}

	SetConsoleActiveScreenBuffer(ScreenBuffer);
	MainLoop();
	CloseHandle(ScreenBuffer);
	return EXIT_SUCCESS;
}


/* EOF */
