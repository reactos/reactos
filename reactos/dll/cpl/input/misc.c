/*
 *  ReactOS
 *  Copyright (C) 2007 ReactOS Team
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
 *
 * PROJECT:         			input.dll
 * FILE:            			dll/win32/input/misc.c
 * PURPOSE:         			input.dll
 * PROGRAMMER:      		Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

#include "resource.h"
#include "input.h"

VOID
CreateKeyboardLayoutList(HWND hWnd)
{
    TCHAR Layout[256];
    int Index;
	UINT loIndex;

    for ( loIndex = BEGIN_LAYOUT; loIndex <= END_LAYOUT; loIndex++ )
	{
		LoadString(hApplet,
				   loIndex,
				   Layout,
				   sizeof(Layout) / sizeof(TCHAR));
		if (strlen((char*)Layout) > 0)
		{
			Index = (int) SendMessage(hWnd,
									  CB_INSERTSTRING,
									  0,
									  (LPARAM)Layout);
			SendMessage(hWnd,
						CB_SETITEMDATA,
						Index,
						(LPARAM)loIndex);
		}
	}
}

/* EOF */
