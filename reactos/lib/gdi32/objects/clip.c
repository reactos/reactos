/*
 *  ReactOS GDI lib
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: clip.c,v 1.5 2004/04/25 14:46:54 weiden Exp $
 *
 * PROJECT:         ReactOS gdi32.dll
 * FILE:            lib/gdi32/objects/clip.c
 * PURPOSE:         Clipping functions
 * PROGRAMMER:      Ge van Geldorp (ge@gse.nl)
 * UPDATE HISTORY:
 *      2003/06/26  GvG  Created
 */

#include <windows.h>
#include <win32k/kapi.h>

/*
 * @implemented
 */
int
STDCALL
SelectClipRgn(HDC hdc, HRGN hrgn)
{
  return NtGdiSelectClipRgn(hdc, hrgn);
}

/*
 * @implemented
 */
int
STDCALL
IntersectClipRect(
	HDC		hdc,
	int		nLeftRect,
	int		nTopRect,
	int		nRightRect,
	int		nBottomRect
	)
{
  return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}


