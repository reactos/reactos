/* $Id: notevil.c,v 1.1 1999/05/15 07:23:34 ea Exp $
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

void
WriteStringAt(
	LPTSTR	lpString,
	COORD	xy,
	WORD	wColor
	)
{
	DWORD	cWritten = 0;
	WORD	wLen = lstrlen(lpString);

	if (0 == wLen) return;
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
		_TEXT("x=%d  y=%d"),
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
	COORD	xy = {24,12};

	WriteStringAt(
		_TEXT("ReactOS Coders Console Parade"),
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
	COORD	xy = {40,12}; 
	INT	n = RES_DELAY_CHANGE;
	INT	dir_y = 1;
	INT	dir_x = 1;
	WORD	wColor = 0;

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
			++wColor;
		}
		if (!xy.X)
		{
			if (dir_x == -1) dir_x = 1;
		}
		else if (xy.X >	80 - NameLength)
		{
			if (dir_x == 1) dir_x = -1;
		}
		xy.X += dir_x;
		switch (xy.Y)
		{
			case 0:
				if (dir_y == -1) dir_y = 1;
				break;
			case 24:
				if (dir_y == 1) dir_y = -1;
				break;
		}
		xy.Y += dir_y;
#ifdef DISPLAY_COORD
		WriteCoord(xy);
#endif /* def DISPLAY_COORD */
		DisplayTitle();
		WriteStringAt(
			NameString,
			xy,
			(wColor & 0x000F)
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
