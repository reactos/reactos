/*
 *  FreeLoader - fdebug.c
 *
 *  Copyright (C) 2003  Brian Palmer  <brianp@sginet.com>
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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "rs232.h"

void main(void)
{
	BYTE	Byte;

	Rs232OpenPortWin32(TEXT("COM1"));
	Rs232ConfigurePortWin32(TEXT("19200,n,8,1"));

	for (;;)
	{
		if (Rs232ReadByteWin32(&Byte))
		{
			_tprintf(TEXT("%c"), Byte);
		}
	}

	_tprintf(TEXT("\n"));
}