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
/* $Id: rect.c,v 1.11 2002/09/17 23:46:23 dwelch Exp $
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

WINBOOL STDCALL
CopyRect(LPRECT lprcDst, CONST RECT *lprcSrc)
{
  *lprcDst = *lprcSrc;
  return(TRUE);
}

WINBOOL
STDCALL
EqualRect(
  CONST RECT *lprc1,
  CONST RECT *lprc2)
{
  if ((lprc1->left   == lprc2->left)
   && (lprc1->top    == lprc2->top)
   && (lprc1->right  == lprc2->right)
   && (lprc1->bottom == lprc2->bottom))
  {
    return TRUE;
  }
  /* TODO: return the correct error code. */
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
InflateRect(LPRECT rect, int dx, int dy)
{
  rect->left -= dx;
  rect->top -= dy;
  rect->right -= dx;
  rect->bottom -= dy;
  return(TRUE);
}

WINBOOL STDCALL
IntersectRect(LPRECT lprcDst,
	      CONST RECT *lprcSrc1,
	      CONST RECT *lprcSrc2)
{
  if (IsRectEmpty(lprcSrc1) || IsRectEmpty(lprcSrc2) ||
      lprcSrc1->left >= lprcSrc2->right || 
      lprcSrc2->left >= lprcSrc1->right ||
      lprcSrc1->top >= lprcSrc2->bottom || 
      lprcSrc2->top >= lprcSrc1->bottom)
    {
      SetRectEmpty(lprcDst);
      return(FALSE);
    }
  lprcDst->left = max(lprcSrc1->left, lprcSrc2->left);
  lprcDst->right = min(lprcSrc1->right, lprcSrc2->right);
  lprcDst->top = max(lprcSrc1->top, lprcSrc2->top);
  lprcDst->bottom = min(lprcSrc1->bottom, lprcSrc2->bottom);
  return(TRUE);
}

WINBOOL STDCALL
IsRectEmpty(CONST RECT *lprc)
{
  return((lprc->left >= lprc->right) || (lprc->top >= lprc->bottom));
}

WINBOOL STDCALL
OffsetRect(LPRECT rect, int dx, int dy)
{
  rect->left += dx;
  rect->top += dy;
  rect->right += dx;
  rect->bottom += dy;
  return(TRUE);  
}

WINBOOL STDCALL
PtInRect(CONST RECT *lprc, POINT pt)
{
  return((pt.x >= lprc->left) && (pt.x < lprc->right) &&
	 (pt.y >= lprc->top) && (pt.y < lprc->bottom));
}

WINBOOL STDCALL
SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
  lprc->left = xLeft;
  lprc->top = yTop;
  lprc->right = xRight;
  lprc->bottom = yBottom;
  return(TRUE);
}

BOOL STDCALL
SetRectEmpty(LPRECT lprc)
{
  lprc->left = lprc->right = lprc->top = lprc->bottom = 0;
  return(TRUE);
}

WINBOOL STDCALL
SubtractRect(LPRECT lprcDst, CONST RECT *lprcSrc1, CONST RECT *lprcSrc2)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
UnionRect(LPRECT lprcDst, CONST RECT *lprcSrc1, CONST RECT *lprcSrc2)
{
  if (IsRectEmpty(lprcSrc1))
    {
      if (IsRectEmpty(lprcSrc2))
	{
	  SetRectEmpty(lprcDst);
	  return(FALSE);
	}
      else
	{
	  *lprcDst = *lprcSrc2;
	}
    }
  else
    {
      if (IsRectEmpty(lprcSrc2))
	{
	  *lprcDst = *lprcSrc1;
	}
      else
	{
	  lprcDst->left = min(lprcSrc1->left, lprcSrc2->left);
	  lprcDst->top = min(lprcSrc1->top, lprcSrc2->top);
	  lprcDst->right = max(lprcSrc1->right, lprcSrc2->right);
	  lprcDst->bottom = max(lprcSrc1->bottom, lprcSrc2->bottom);
	}
    }
  return(TRUE);
}
