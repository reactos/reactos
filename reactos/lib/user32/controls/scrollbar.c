/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: scrollbar.c,v 1.3 2002/09/08 10:23:09 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/controls/scrollbar.c
 * PURPOSE:         Scroll bar
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL
GetScrollBarInfo(HWND hwnd,
		 LONG idObject,
		 PSCROLLBARINFO psbi)
{
  return FALSE;
}

WINBOOL STDCALL
GetScrollInfo(HWND hwnd,
	      int fnBar,
	      LPSCROLLINFO lpsi)
{
  return FALSE;
}

int STDCALL
GetScrollPos(HWND hWnd,
	     int nBar)
{
  return 0;
}

WINBOOL STDCALL
GetScrollRange(HWND hWnd,
	       int nBar,
	       LPINT lpMinPos,
	       LPINT lpMaxPos)
{
  return FALSE;
}

int STDCALL
SetScrollInfo(HWND hwnd,
	      int fnBar,
	      LPCSCROLLINFO lpsi,
	      WINBOOL fRedraw)
{
  return 0;
}

int STDCALL
SetScrollPos(HWND hWnd,
	     int nBar,
	     int nPos,
	     WINBOOL bRedraw)
{
  return 0;
}

WINBOOL STDCALL
SetScrollRange(HWND hWnd,
	       int nBar,
	       int nMinPos,
	       int nMaxPos,
	       WINBOOL bRedraw)
{
  return FALSE;
}

WINBOOL STDCALL
ShowScrollBar(HWND hWnd,
	      int wBar,
	      WINBOOL bShow)
{
  return FALSE;
}
