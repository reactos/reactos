/*
 * Drag List control
 *
 * Copyright 1999 Eric Kohl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Everything.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);


static DWORD dwLastScrollTime = 0;

/***********************************************************************
 *		MakeDragList (COMCTL32.13)
 */
BOOL WINAPI MakeDragList (HWND hwndLB)
{
    FIXME("(%p)\n", hwndLB);


    return FALSE;
}

/***********************************************************************
 *		DrawInsert (COMCTL32.15)
 */
VOID WINAPI DrawInsert (HWND hwndParent, HWND hwndLB, INT nItem)
{
    FIXME("(%p %p %d)\n", hwndParent, hwndLB, nItem);


}

/***********************************************************************
 *		LBItemFromPt (COMCTL32.14)
 */
INT WINAPI LBItemFromPt (HWND hwndLB, POINT pt, BOOL bAutoScroll)
{
    RECT rcClient;
    INT nIndex;
    DWORD dwScrollTime;

    FIXME("(%p %ld x %ld %s)\n",
	   hwndLB, pt.x, pt.y, bAutoScroll ? "TRUE" : "FALSE");

    ScreenToClient (hwndLB, &pt);
    GetClientRect (hwndLB, &rcClient);
    nIndex = (INT)SendMessageA (hwndLB, LB_GETTOPINDEX, 0, 0);

    if (PtInRect (&rcClient, pt))
    {
	/* point is inside -- get the item index */
	while (TRUE)
	{
	    if (SendMessageA (hwndLB, LB_GETITEMRECT, nIndex, (LPARAM)&rcClient) == LB_ERR)
		return -1;

	    if (PtInRect (&rcClient, pt))
		return nIndex;

	    nIndex++;
	}
    }
    else
    {
	/* point is outside */
	if (!bAutoScroll)
	    return -1;

	if ((pt.x > rcClient.right) || (pt.x < rcClient.left))
	    return -1;

	if (pt.y < 0)
	    nIndex--;
	else
	    nIndex++;

	dwScrollTime = GetTickCount ();

	if ((dwScrollTime - dwLastScrollTime) < 200)
	    return -1;

	dwLastScrollTime = dwScrollTime;

	SendMessageA (hwndLB, LB_SETTOPINDEX, (WPARAM)nIndex, 0);
    }

    return -1;
}


#if 0
static LRESULT CALLBACK
DRAGLIST_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    return FALSE;
}
#endif



