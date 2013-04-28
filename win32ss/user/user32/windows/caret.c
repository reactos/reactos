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
/*
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
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

void
DrawCaret(HWND hWnd,
          PTHRDCARETINFO CaretInfo)
{
    HDC hdc, hdcMem;
    HBITMAP hbmOld;
    BOOL bDone = FALSE;

    hdc = GetDC(hWnd);
    if (!hdc)
    {
        ERR("GetDC failed\n");
        return;
    }

    if(CaretInfo->Bitmap && GetBitmapDimensionEx(CaretInfo->Bitmap, &CaretInfo->Size))
    {
        hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
        {
            hbmOld = SelectObject(hdcMem, CaretInfo->Bitmap);
            bDone = BitBlt(hdc,
                           CaretInfo->Pos.x,
                           CaretInfo->Pos.y,
                           CaretInfo->Size.cx,
                           CaretInfo->Size.cy,
                           hdcMem,
                           0,
                           0,
                           SRCINVERT);
            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
        }
    }

    if (!bDone)
    {
        PatBlt(hdc,
               CaretInfo->Pos.x,
               CaretInfo->Pos.y,
               CaretInfo->Size.cx,
               CaretInfo->Size.cy,
               DSTINVERT);
    }

    ReleaseDC(hWnd, hdc);
}


/*
 * @implemented
 */
BOOL
WINAPI
DestroyCaret(VOID)
{
    return NtUserxDestroyCaret();
}


/*
 * @implemented
 */
BOOL
WINAPI
SetCaretBlinkTime(UINT uMSeconds)
{
    return NtUserxSetCaretBlinkTime(uMSeconds);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetCaretPos(int X, int Y)
{
    return NtUserxSetCaretPos(X, Y);
}

/* EOF */
