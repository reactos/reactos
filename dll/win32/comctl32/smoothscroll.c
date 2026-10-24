/*
 * Undocumented SmoothScrollWindow function from COMCTL32.DLL
 *
 * Copyright 2000 Marcus Meissner <marcus@jet.franken.de>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO
 *     - actually add smooth scrolling
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

static DWORD	smoothscroll = 2;

typedef BOOL (CALLBACK *SCROLLWINDOWEXPROC)(HWND,INT,INT,LPRECT,LPRECT,HRGN,LPRECT,DWORD);
typedef struct tagSMOOTHSCROLLSTRUCT {
	DWORD			dwSize;
	DWORD			x2;
	HWND			hwnd;
	DWORD			dx;

	DWORD			dy;
	LPRECT			lpscrollrect;
	LPRECT			lpcliprect;
	HRGN			hrgnupdate;

	LPRECT			lpupdaterect;
	DWORD			flags;
	DWORD			stepinterval;
	DWORD			dx_step;

	DWORD			dy_step;
	SCROLLWINDOWEXPROC	scrollfun;	/* same parameters as ScrollWindowEx */
} SMOOTHSCROLLSTRUCT;

/**************************************************************************
 * SmoothScrollWindow [COMCTL32.382]
 *
 * Lots of magic for smooth scrolling windows.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * BUGS
 *     Currently only scrolls ONCE. The comctl32 implementation uses GetTickCount
 *     and what else to do smooth scrolling.
 */
BOOL WINAPI SmoothScrollWindow( const SMOOTHSCROLLSTRUCT *smooth ) {
   LPRECT	lpupdaterect = smooth->lpupdaterect;
   HRGN		hrgnupdate = smooth->hrgnupdate;
   RECT		tmprect;
   DWORD	flags = smooth->flags;

   if (smooth->dwSize!=sizeof(SMOOTHSCROLLSTRUCT))
       return FALSE;

   if (!lpupdaterect)
       lpupdaterect = &tmprect;
   SetRectEmpty(lpupdaterect);

   if (!(flags & 0x40000)) { /* no override, use system wide defaults */
       if (smoothscroll == 2) {
	   HKEY	hkey;

	   smoothscroll = 0;
	   if (!RegOpenKeyA(HKEY_CURRENT_USER,"Control Panel\\Desktop",&hkey)) {
	       DWORD	len = 4;

	       RegQueryValueExA(hkey,"SmoothScroll",0,0,(LPBYTE)&smoothscroll,&len);
	       RegCloseKey(hkey);
	   }
       }
       if (!smoothscroll)
	   flags |= 0x20000;
   }

   if (flags & 0x20000) { /* are we doing jump scrolling? */
       if ((smooth->x2 & 1) && smooth->scrollfun)
	   return smooth->scrollfun(
	       smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	       smooth->lpcliprect,hrgnupdate,lpupdaterect,
	       flags & 0xffff
	   );
       else
	   return ScrollWindowEx(
	       smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	       smooth->lpcliprect,hrgnupdate,lpupdaterect,
	       flags & 0xffff
	   );
   }

   FIXME("(hwnd=%p,flags=%lx,x2=%lx): should smooth scroll here.\n", smooth->hwnd, flags, smooth->x2);
   /* FIXME: do timer based smooth scrolling */
   if ((smooth->x2 & 1) && smooth->scrollfun)
       return smooth->scrollfun(
	   smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	   smooth->lpcliprect,hrgnupdate,lpupdaterect,
	   flags & 0xffff
       );
   else
       return ScrollWindowEx(
	   smooth->hwnd,smooth->dx,smooth->dy,smooth->lpscrollrect,
	   smooth->lpcliprect,hrgnupdate,lpupdaterect,
	   flags & 0xffff
       );
}
