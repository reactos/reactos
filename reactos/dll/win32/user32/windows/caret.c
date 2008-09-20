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
 * FILE:            lib/user32/windows/caret.c
 * PURPOSE:         Caret
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
/* FUNCTIONS *****************************************************************/

void
DrawCaret(HWND hWnd,
          PTHRDCARETINFO CaretInfo)
{
    HDC hDC, hComp;

    hDC = GetDC(hWnd);
    if(hDC)
    {
        if(CaretInfo->Bitmap && GetBitmapDimensionEx(CaretInfo->Bitmap, &CaretInfo->Size))
        {
            hComp = CreateCompatibleDC(hDC);
            if(hComp)
            {
                SelectObject(hComp, CaretInfo->Bitmap);
                BitBlt(hDC,
                       CaretInfo->Pos.x,
                       CaretInfo->Pos.y,
                       CaretInfo->Size.cx,
                       CaretInfo->Size.cy,
                       hComp,
                       0,
                       0,
                       SRCINVERT);
                DeleteDC(hComp);
            }
            else
                PatBlt(hDC,
                       CaretInfo->Pos.x,
                       CaretInfo->Pos.y,
                       CaretInfo->Size.cx,
                       CaretInfo->Size.cy,
                       DSTINVERT);
        }
        else
        {
            PatBlt(hDC,
                   CaretInfo->Pos.x,
                   CaretInfo->Pos.y,
                   CaretInfo->Size.cx,
                   CaretInfo->Size.cy,
                   DSTINVERT);
        }
        ReleaseDC(hWnd, hDC);
    }
}

/*
 * @implemented
 */
BOOL
STDCALL
CreateCaret(HWND hWnd,
            HBITMAP hBitmap,
            int nWidth,
            int nHeight)
{
    return (BOOL)NtUserCreateCaret(hWnd, hBitmap, nWidth, nHeight);
}


/*
 * @implemented
 */
BOOL
STDCALL
DestroyCaret(VOID)
{
    return (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_DESTROY_CARET);
}


/*
 * @implemented
 */
UINT
STDCALL
GetCaretBlinkTime(VOID)
{
    return NtUserGetCaretBlinkTime();
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCaretPos(LPPOINT lpPoint)
{
    return (BOOL)NtUserGetCaretPos(lpPoint);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCaretBlinkTime(UINT uMSeconds)
{
    return NtUserSetCaretBlinkTime(uMSeconds);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCaretPos(int X,
            int Y)
{
    return NtUserSetCaretPos(X, Y);
}

/* EOF */
