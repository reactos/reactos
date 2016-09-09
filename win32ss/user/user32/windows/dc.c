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
 * FILE:            win32ss/user/user32/windows/dc.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

/* FUNCTIONS *****************************************************************/


BOOL
WINAPI
GdiReleaseDC(HDC hdc);

/*
 * @implemented
 */
int
WINAPI
ReleaseDC(HWND hWnd,
          HDC hDC)
{
    // From msdn: if the DC was not released return zero.
    //            if the DC was released return one.

    if (!hDC) return FALSE; // Null hDC return zero.

    GdiReleaseDC ( hDC ); // Release locals.

    return NtUserxReleaseDC(hDC);
}


/*
 * @implemented
 */
HWND
WINAPI
WindowFromDC(HDC hDC)
{
    return NtUserxWindowFromDC(hDC);
}
