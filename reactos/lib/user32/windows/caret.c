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
/* $Id: caret.c,v 1.2 2003/10/16 22:07:37 weiden Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/caret.c
 * PURPOSE:         Caret
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
WINBOOL STDCALL
CreateCaret(HWND hWnd,
	    HBITMAP hBitmap,
	    int nWidth,
	    int nHeight)
{
  return (WINBOOL)NtUserCreateCaret(hWnd, hBitmap, nWidth, nHeight);
}


/*
 * @implemented
 */
WINBOOL STDCALL
DestroyCaret(VOID)
{
  return (WINBOOL)NtUserCallNoParam(NOPARAM_ROUTINE_DESTROY_CARET);
}


/*
 * @implemented
 */
UINT STDCALL
GetCaretBlinkTime(VOID)
{
  return NtUserGetCaretBlinkTime();
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetCaretPos(LPPOINT lpPoint)
{
  return (WINBOOL)NtUserGetCaretPos(lpPoint);
}


/*
 * @implemented
 */
WINBOOL STDCALL
HideCaret(HWND hWnd)
{
  return (WINBOOL)NtUserHideCaret(hWnd);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetCaretBlinkTime(UINT uMSeconds)
{
  return (WINBOOL)NtUserCallOneParam(ONEPARAM_ROUTINE_SETCARETBLINKTIME, (DWORD)uMSeconds);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetCaretPos(int X,
	    int Y)
{
  return (WINBOOL)NtUserCallTwoParam(TWOPARAM_ROUTINE_SETCARETPOS, (DWORD)X, (DWORD)Y);
}


/*
 * @implemented
 */
WINBOOL STDCALL
ShowCaret(HWND hWnd)
{
  return (WINBOOL)NtUserShowCaret(hWnd);
}

/* EOF */
