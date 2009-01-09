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
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/paint.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

static HBRUSH FrameBrushes[13];
static HBITMAP hHatch;
const DWORD HatchBitmap[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};

BOOL WINAPI PolyPatBlt(HDC,DWORD,PPATRECT,INT,ULONG);

/* FUNCTIONS *****************************************************************/

VOID
CreateFrameBrushes(VOID)
{
  FrameBrushes[0] = CreateSolidBrush(RGB(0,0,0));
  FrameBrushes[1] = CreateSolidBrush(RGB(0,0,128));
  FrameBrushes[2] = CreateSolidBrush(RGB(10,36,106));
  FrameBrushes[3] = CreateSolidBrush(RGB(128,128,128));
  FrameBrushes[4] = CreateSolidBrush(RGB(181,181,181));
  FrameBrushes[5] = CreateSolidBrush(RGB(212,208,200));
  FrameBrushes[6] = CreateSolidBrush(RGB(236,233,216));
  FrameBrushes[7] = CreateSolidBrush(RGB(255,255,255));
  FrameBrushes[8] = CreateSolidBrush(RGB(49,106,197));
  FrameBrushes[9] = CreateSolidBrush(RGB(58,110,165));
  FrameBrushes[10] = CreateSolidBrush(RGB(64,64,64));
  FrameBrushes[11] = CreateSolidBrush(RGB(255,255,225));
  hHatch = CreateBitmap(8, 8, 1, 1, HatchBitmap);
  FrameBrushes[12] = CreatePatternBrush(hHatch);
}

VOID
DeleteFrameBrushes(VOID)
{
  unsigned Brush;

  for (Brush = 0; Brush < sizeof(FrameBrushes) / sizeof(HBRUSH); Brush++)
    {
      if (NULL != FrameBrushes[Brush])
	{
	  DeleteObject(FrameBrushes[Brush]);
	  FrameBrushes[Brush] = NULL;
	}
    }
  if (NULL != hHatch)
    {
      DeleteObject(hHatch);
      hHatch = NULL;
    }
}


/*
 * @implemented
 */
BOOL
WINAPI
GetUpdateRect(
  HWND Wnd,
  LPRECT Rect,
  BOOL Erase)
{
  return NtUserGetUpdateRect(Wnd, Rect, Erase);
}


/*
 * @implemented
 */
int
WINAPI
GetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase)
{
  return NtUserGetUpdateRgn(hWnd, hRgn, bErase);
}


/*
 * @implemented
 */
BOOL WINAPI
ScrollDC(HDC hDC, int dx, int dy, CONST RECT *lprcScroll, CONST RECT *lprcClip,
   HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   if (hDC == NULL) return FALSE;

   if (dx == 0 && dy == 0)
   {
      if (hrgnUpdate) SetRectRgn(hrgnUpdate, 0, 0, 0, 0);
      if (lprcUpdate) lprcUpdate->left = lprcUpdate->right =
                      lprcUpdate->top = lprcUpdate->bottom = 0;
      return TRUE;
   }

   return NtUserScrollDC(hDC, dx, dy, lprcScroll, lprcClip, hrgnUpdate,
      lprcUpdate);
}


/*
 * @implemented
 */
int
WINAPI
SetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw)
{
  return (int)NtUserSetWindowRgn(hWnd, hRgn, bRedraw);
}


/*
 * @implemented
 */
BOOL
WINAPI
UpdateWindow(
  HWND hWnd)
{
  return NtUserRedrawWindow( hWnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/*
 * @implemented
 */
BOOL
WINAPI
ValidateRect(
  HWND hWnd,
  CONST RECT *lpRect)
{
  /* FIXME: should RDW_NOCHILDREN be included too? Ros used to,
     but Wine dont so i removed it... */
  return NtUserRedrawWindow(hWnd, lpRect, 0, RDW_VALIDATE);
}


/*
 * @implemented
 */
BOOL
WINAPI
ValidateRgn(
  HWND hWnd,
  HRGN hRgn)
{
  /* FIXME: should RDW_NOCHILDREN be included too? Ros used to,
     but Wine dont so i removed it... */
  return NtUserRedrawWindow( hWnd, NULL, hRgn, RDW_VALIDATE );
}


/*
 * @implemented
 */
int
WINAPI
GetWindowRgn(
  HWND hWnd,
  HRGN hRgn)
{
  return (int)NtUserCallTwoParam((DWORD)hWnd, (DWORD)hRgn, TWOPARAM_ROUTINE_GETWINDOWRGN);
}


/*
 * @implemented
 */
int
WINAPI
GetWindowRgnBox(
    HWND hWnd,
    LPRECT lprc)
{
  return (int)NtUserCallTwoParam((DWORD)hWnd, (DWORD)lprc, TWOPARAM_ROUTINE_GETWINDOWRGNBOX);
}


const BYTE MappingTable[33] = {5,9,2,3,5,7,0,0,0,7,5,5,3,2,7,5,3,3,0,5,7,10,5,0,11,4,1,1,3,8,6,12,7};
/*
 * @implemented
 */
BOOL
WINAPI
DrawFrame(
	  HDC    hDc,
	  RECT  *r,
	  DWORD  width,
	  DWORD  type
	  )
{
	DWORD rop;
	DWORD brush;
	HBRUSH hbrFrame;
	PATRECT p[4];
	if (NULL == FrameBrushes[0])
	{
		CreateFrameBrushes();
	}
	if (type & 4)
	{
		rop = PATINVERT;
	}
	else
	{
		rop = PATCOPY;
	}
	brush = type / 8;
	if (brush >= 33)
	{
		brush = 32;
	}
	brush = MappingTable[brush];
	hbrFrame = FrameBrushes[brush];
	p[0].hBrush = hbrFrame;
	p[1].hBrush = hbrFrame;
	p[2].hBrush = hbrFrame;
	p[3].hBrush = hbrFrame;
	p[0].r.left = r->left;
	p[0].r.top = r->top;
	p[0].r.right = r->right - r->left;
	p[0].r.bottom = width;
	p[1].r.left = r->left;
	p[1].r.top = r->bottom - width;
	p[1].r.right = r->right - r->left;
	p[1].r.bottom = width;
	p[2].r.left = r->left;
	p[2].r.top = r->top + width;
	p[2].r.right = width;
	p[2].r.bottom = r->bottom - r->top - (width * 2);
	p[3].r.left = r->right - width;
	p[3].r.top = r->top + width;
	p[3].r.right = width;
	p[3].r.bottom = r->bottom - r->top - (width * 2);
	return PolyPatBlt(hDc,rop,p,4,0);
}
