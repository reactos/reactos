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
/* $Id: rect.c,v 1.5 2002/06/13 20:36:40 dwelch Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
CopyRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc)
{
  return FALSE;
}
WINBOOL
STDCALL
EqualRect(
  CONST RECT *lprc1,
  CONST RECT *lprc2)
{
  return FALSE;
}
WINBOOL
STDCALL
InflateRect(
  LPRECT lprc,
  int dx,
  int dy)
{
  return FALSE;
}

WINBOOL
STDCALL
IntersectRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}
WINBOOL
STDCALL
IsRectEmpty(
  CONST RECT *lprc)
{
  return FALSE;
}
WINBOOL
STDCALL
OffsetRect(
  LPRECT lprc,
  int dx,
  int dy)
{
  return FALSE;
}
WINBOOL
STDCALL
PtInRect(
  CONST RECT *lprc,
  POINT pt)
{
  return FALSE;
}
WINBOOL
STDCALL
SetRect(
  LPRECT lprc,
  int xLeft,
  int yTop,
  int xRight,
  int yBottom)
{
  return FALSE;
}

WINBOOL
STDCALL
SetRectEmpty(
  LPRECT lprc)
{
  return FALSE;
}
WINBOOL
STDCALL
SubtractRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}
WINBOOL
STDCALL
UnionRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}
