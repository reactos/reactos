/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: winpos.c,v 1.26 2003/08/20 07:45:01 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  NtGdid
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <windows.h>
#include <include/winpos.h>
#include <include/rect.h>
#include <include/callback.h>
#include <include/painting.h>
#include <include/dce.h>
#include <include/vis.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MINMAX_NOSWP  (0x00010000)

#define SWP_EX_NOCOPY 0x0001
#define SWP_EX_PAINTSELF 0x0002

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

ATOM AtomInternalPos = (ATOM) NULL;

/* FUNCTIONS *****************************************************************/

#define HAS_DLGFRAME(Style, ExStyle) \
       (((ExStyle) & WS_EX_DLGMODALFRAME) || \
        (((Style) & WS_DLGFRAME) && !((Style) & WS_BORDER)))

#define HAS_THICKFRAME(Style, ExStyle) \
       (((Style) & WS_THICKFRAME) && \
        !((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)

VOID FASTCALL
WinPosSetupInternalPos(VOID)
{
  AtomInternalPos = NtAddAtom(L"SysIP", (ATOM*)(PULONG)&AtomInternalPos);
}

BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  PWINDOW_OBJECT WindowObject;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      Point->x = Point->y = 0;
      return(TRUE);
    }
  Point->x = WindowObject->ClientRect.left;
  Point->y = WindowObject->ClientRect.top;
  return(TRUE);
}

BOOL FASTCALL
WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
	return FALSE;
}

POINT STATIC FASTCALL
WinPosFindIconPos(HWND hWnd, POINT Pos)
{
  POINT point;
  //FIXME
  return point;
}

HWND STATIC FASTCALL
WinPosNtGdiIconTitle(PWINDOW_OBJECT WindowObject)
{
  return(NULL);
}

BOOL STATIC FASTCALL
WinPosShowIconTitle(PWINDOW_OBJECT WindowObject, BOOL Show)
{
  PINTERNALPOS InternalPos = (PINTERNALPOS)IntGetProp(WindowObject, AtomInternalPos);
  PWINDOW_OBJECT IconWindow;
  NTSTATUS Status;

  if (InternalPos)
    {
      HWND hWnd = InternalPos->IconTitle;

      if (hWnd == NULL)
	{
	  hWnd = WinPosNtGdiIconTitle(WindowObject);
	}
      if (Show)
	{
	  Status = 
	    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->
				       HandleTable,
				       hWnd,
				       otWindow,
				       (PVOID*)&IconWindow);
	  if (NT_SUCCESS(Status))
	    {
	      if (!(IconWindow->Style & WS_VISIBLE))
		{
		  NtUserSendMessage(hWnd, WM_SHOWWINDOW, TRUE, 0);
		  WinPosSetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE |
				     SWP_NOMOVE | SWP_NOACTIVATE | 
				     SWP_NOZORDER | SWP_SHOWWINDOW);
		}
	      ObmDereferenceObject(IconWindow);
	    }
	}
      else
	{
	  WinPosShowWindow(hWnd, SW_HIDE);
	}
    }
  return(FALSE);
}

PINTERNALPOS STATIC STDCALL
WinPosInitInternalPos(PWINDOW_OBJECT WindowObject, POINT pt, PRECT RestoreRect)
{
  PINTERNALPOS InternalPos = (PINTERNALPOS)IntGetProp(WindowObject, AtomInternalPos);
  if (InternalPos == NULL)
    {
      InternalPos = 
	ExAllocatePool(NonPagedPool, sizeof(INTERNALPOS));
      IntSetProp(WindowObject, AtomInternalPos, InternalPos);
      InternalPos->IconTitle = 0;
      InternalPos->NormalRect = WindowObject->WindowRect;
      InternalPos->IconPos.x = InternalPos->MaxPos.x = 0xFFFFFFFF;
      InternalPos->IconPos.y = InternalPos->MaxPos.y = 0xFFFFFFFF;
    }
  if (WindowObject->Style & WS_MINIMIZE)
    {
      InternalPos->IconPos = pt;
    }
  else if (WindowObject->Style & WS_MAXIMIZE)
    {
      InternalPos->MaxPos = pt;
    }
  else if (RestoreRect != NULL)
    {
      InternalPos->NormalRect = *RestoreRect;
    }
  return(InternalPos);
}

UINT STDCALL
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  UINT SwpFlags = 0;

  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, Size, 
				      &WindowObject->WindowRect); 

  if (InternalPos)
    {
      if (WindowObject->Style & WS_MINIMIZE)
	{
	  if (!NtUserSendMessage(WindowObject->Self, WM_QUERYOPEN, 0, 0))
	    {
	      return(SWP_NOSIZE | SWP_NOMOVE);
	    }
	  SwpFlags |= SWP_NOCOPYBITS;
	}
      switch (ShowFlag)
	{
	case SW_MINIMIZE:
	  {
	    if (WindowObject->Style & WS_MAXIMIZE)
	      {
		WindowObject->Flags |= WINDOWOBJECT_RESTOREMAX;
		WindowObject->Style &= ~WS_MAXIMIZE;
	      }
	    else
	      {
		WindowObject->Style &= ~WINDOWOBJECT_RESTOREMAX;
	      }
	    WindowObject->Style |= WS_MINIMIZE;
	    InternalPos->IconPos = WinPosFindIconPos(WindowObject,
						     InternalPos->IconPos);
	    NtGdiSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
			NtUserGetSystemMetrics(SM_CXICON),
			NtUserGetSystemMetrics(SM_CYICON));
	    SwpFlags |= SWP_NOCOPYBITS;
	    break;
	  }

	case SW_MAXIMIZE:
	  {
	    WinPosGetMinMaxInfo(WindowObject, &Size, &InternalPos->MaxPos, 
				NULL, NULL);
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WinPosShowIconTitle(WindowObject, FALSE);
		WindowObject->Style &= ~WS_MINIMIZE;
	      }
	    WindowObject->Style |= WS_MINIMIZE;
	    NtGdiSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
			Size.x, Size.y);
	    break;
	  }

	case SW_RESTORE:
	  {
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
		WinPosShowIconTitle(WindowObject, FALSE);
		if (WindowObject->Flags & WINDOWOBJECT_RESTOREMAX)
		  {
		    WinPosGetMinMaxInfo(WindowObject, &Size,
					&InternalPos->MaxPos, NULL, NULL);
		    WindowObject->Style |= WS_MAXIMIZE;
		    NtGdiSetRect(NewPos, InternalPos->MaxPos.x,
				InternalPos->MaxPos.y, Size.x, Size.y);
		    break;
		  }
	      }
	    else
	      {
		if (!(WindowObject->Style & WS_MAXIMIZE))
		  {
		    return(-1);
		  }
		else
		  {
		    WindowObject->Style &= ~WS_MAXIMIZE;
		  }	      
		*NewPos = InternalPos->NormalRect;
		NewPos->right -= NewPos->left;
		NewPos->bottom -= NewPos->top;
		break;
	      }
	  }
	}
    }
  else
    {
      SwpFlags |= SWP_NOSIZE | SWP_NOMOVE;
    }
  return(SwpFlags);
}

UINT STDCALL
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;
  INT XInc, YInc;
  INTERNALPOS* Pos;

  /* Get default values. */
  MinMax.ptMaxSize.x = NtUserGetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxSize.y = NtUserGetSystemMetrics(SM_CYSCREEN);
  MinMax.ptMinTrackSize.x = NtUserGetSystemMetrics(SM_CXMINTRACK);
  MinMax.ptMinTrackSize.y = NtUserGetSystemMetrics(SM_CYMINTRACK);
  MinMax.ptMaxTrackSize.x = NtUserGetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxTrackSize.y = NtUserGetSystemMetrics(SM_CYSCREEN);

  if (HAS_DLGFRAME(Window->Style, Window->ExStyle))
    {
      XInc = NtUserGetSystemMetrics(SM_CXDLGFRAME);
      YInc = NtUserGetSystemMetrics(SM_CYDLGFRAME);
    }
  else
    {
      XInc = YInc = 0;
      if (HAS_THICKFRAME(Window->Style, Window->ExStyle))
	{
	  XInc += NtUserGetSystemMetrics(SM_CXFRAME);
	  YInc += NtUserGetSystemMetrics(SM_CYFRAME);
	}
      if (Window->Style & WS_BORDER)
	{
	  XInc += NtUserGetSystemMetrics(SM_CXBORDER);
	  YInc += NtUserGetSystemMetrics(SM_CYBORDER);
	}
    }
  MinMax.ptMaxSize.x += 2 * XInc;
  MinMax.ptMaxSize.y += 2 * YInc;

  Pos = (PINTERNALPOS)IntGetProp(Window, AtomInternalPos);
  if (Pos != NULL)
    {
      MinMax.ptMaxPosition = Pos->MaxPos;
    }
  else
    {
      MinMax.ptMaxPosition.x -= XInc;
      MinMax.ptMaxPosition.y -= YInc;
    }

  IntSendMessage(Window->Self, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax, TRUE);

  MinMax.ptMaxTrackSize.x = max(MinMax.ptMaxTrackSize.x,
				MinMax.ptMinTrackSize.x);
  MinMax.ptMaxTrackSize.y = max(MinMax.ptMaxTrackSize.y,
				MinMax.ptMinTrackSize.y);

  if (MaxSize) *MaxSize = MinMax.ptMaxSize;
  if (MaxPos) *MaxPos = MinMax.ptMaxPosition;
  if (MinTrack) *MinTrack = MinMax.ptMinTrackSize;
  if (MaxTrack) *MaxTrack = MinMax.ptMaxTrackSize;

  return 0; //FIXME: what does it return?
}

BOOL STATIC FASTCALL
WinPosChangeActiveWindow(HWND hWnd, BOOL MouseMsg)
{
  PWINDOW_OBJECT WindowObject;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return FALSE;
    }

  NtUserSendMessage(hWnd,
    WM_ACTIVATE,
	  MAKELONG(MouseMsg ? WA_CLICKACTIVE : WA_CLICKACTIVE,
      (WindowObject->Style & WS_MINIMIZE) ? 1 : 0),
      (LPARAM)IntGetDesktopWindow());  /* FIXME: Previous active window */

  IntReleaseWindowObject(WindowObject);

  return TRUE;
}

LONG STATIC STDCALL
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  return 0; /* FIXME:  Calculate non client size */
}

BOOL STDCALL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      IntSendWINDOWPOSCHANGINGMessage(WindowObject->Self, WinPos);
    }
  
  *WindowRect = WindowObject->WindowRect;
  *ClientRect = 
    (WindowObject->Style & WS_MINIMIZE) ? WindowObject->WindowRect :
    WindowObject->ClientRect;

  if (!(WinPos->flags & SWP_NOSIZE))
    {
      WindowRect->right = WindowRect->left + WinPos->cx;
      WindowRect->bottom = WindowRect->top + WinPos->cy;
    }

  if (!(WinPos->flags & SWP_NOMOVE))
    {
      WindowRect->left = WinPos->x;
      WindowRect->top = WinPos->y;
      WindowRect->right += WinPos->x - WindowObject->WindowRect.left;
      WindowRect->bottom += WinPos->y - WindowObject->WindowRect.top;
    }

  if (!(WinPos->flags & SWP_NOSIZE) || !(WinPos->flags & SWP_NOMOVE))
    {
      WinPosGetNonClientSize(WindowObject->Self, WindowRect, ClientRect);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;
  return(TRUE);
}

/***********************************************************************
 *           SWP_DoSimpleFrameChanged
 *
 * NOTE: old and new client rect origins are identical, only
 *	 extents may have changed. Window extents are the same.
 */
VOID STATIC 
WinPosDoSimpleFrameChanged( PWINDOW_OBJECT wndPtr, RECT* pOldClientRect, WORD swpFlags, UINT uFlags )
{
    INT	 i = 0;
    RECT rect;
    HRGN hrgn = 0;

    if( !(swpFlags & SWP_NOCLIENTSIZE) )
    {
	/* Client rect changed its position/size, most likely a scrollar
	 * was added/removed.
	 *
	 * FIXME: WVR alignment flags 
	 */

	if( wndPtr->ClientRect.right >  pOldClientRect->right ) /* right edge */
	{
	    i++;
	    rect.top = 0; 
	    rect.bottom = wndPtr->ClientRect.bottom - wndPtr->ClientRect.top;
	    rect.right = wndPtr->ClientRect.right - wndPtr->ClientRect.left;
	    if(!(uFlags & SWP_EX_NOCOPY))
		rect.left = pOldClientRect->right - wndPtr->ClientRect.left;
	    else
	    {
		rect.left = 0;
		goto redraw;
	    }
	}

	if( wndPtr->ClientRect.bottom > pOldClientRect->bottom ) /* bottom edge */
	{
	    if( i )
		hrgn = NtGdiCreateRectRgnIndirect( &rect );
	    rect.left = 0;
	    rect.right = wndPtr->ClientRect.right - wndPtr->ClientRect.left;
	    rect.bottom = wndPtr->ClientRect.bottom - wndPtr->ClientRect.top;
	    if(!(uFlags & SWP_EX_NOCOPY))
		rect.top = pOldClientRect->bottom - wndPtr->ClientRect.top;
	    else
		rect.top = 0;
	    if( i++ ) 
	      {
		HRGN hRectRgn = NtGdiCreateRectRgnIndirect(&rect);
	        NtGdiCombineRgn(hrgn, hrgn, hRectRgn, RGN_OR);
		NtGdiDeleteObject(hRectRgn);
	      }
	}

	if( i == 0 && (uFlags & SWP_EX_NOCOPY) ) /* force redraw anyway */
	{
	    rect = wndPtr->WindowRect;
	    NtGdiOffsetRect( &rect, wndPtr->WindowRect.left - wndPtr->ClientRect.left,
			    wndPtr->WindowRect.top - wndPtr->ClientRect.top );
	    i++;
	}
    }

    if( i )
    {
redraw:
	PaintRedrawWindow( wndPtr, &rect, hrgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
			    RDW_ERASENOW | RDW_ALLCHILDREN, RDW_EX_TOPFRAME | RDW_EX_USEHRGN );
    }
    else
    {
        PaintUpdateNCRegion(wndPtr, 0, UNC_UPDATE | UNC_ENTIRE);
    }

    if( hrgn > (HRGN)1 )
	NtGdiDeleteObject( hrgn );
}

/***********************************************************************
 *	     SWP_CopyValidBits
 *
 * Make window look nice without excessive repainting
 *
 * visible and update regions are in window coordinates
 * client and window rectangles are in parent client coordinates
 *
 * Returns: uFlags and a dirty region in *pVisRgn.
 */
static UINT WinPosCopyValidBits( PWINDOW_OBJECT Wnd, HRGN* pVisRgn,
                               LPRECT lpOldWndRect,
                               LPRECT lpOldClientRect, UINT uFlags )
{
 RECT r;
 HRGN newVisRgn, dirtyRgn;
 INT  my = COMPLEXREGION;

 if( Wnd->UpdateRegion == (HRGN)1 )
     uFlags |= SWP_EX_NOCOPY; /* whole window is invalid, nothing to copy */

 newVisRgn = DceGetVisRgn( Wnd->Self, DCX_WINDOW | DCX_CLIPSIBLINGS, 0, 0);
 NtGdiOffsetRgn(newVisRgn, -Wnd->WindowRect.left, -Wnd->WindowRect.top);
 dirtyRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );

 if( !(uFlags & SWP_EX_NOCOPY) ) /* make sure dst region covers only valid bits */
     my = NtGdiCombineRgn( dirtyRgn, newVisRgn, *pVisRgn, RGN_AND );

 if( (my == NULLREGION) || (uFlags & SWP_EX_NOCOPY) )
 {
nocopy:

     /* set dirtyRgn to the sum of old and new visible regions 
      * in parent client coordinates */

     NtGdiOffsetRgn( newVisRgn, Wnd->WindowRect.left, Wnd->WindowRect.top );
     NtGdiOffsetRgn( *pVisRgn, lpOldWndRect->left, lpOldWndRect->top );

     NtGdiCombineRgn(*pVisRgn, *pVisRgn, newVisRgn, RGN_OR );
 }
 else			/* copy valid bits to a new location */
 {
     INT  dx, dy, ow, oh, nw, nh, ocw, ncw, och, nch;
     HRGN hrgnValid = dirtyRgn; /* non-empty intersection of old and new visible rgns */

     /* subtract already invalid region inside Wnd from the dst region */

     if( Wnd->UpdateRegion )
         if( NtGdiCombineRgn( hrgnValid, hrgnValid, Wnd->UpdateRegion, RGN_DIFF) == NULLREGION )
	     goto nocopy;

     /* check if entire window can be copied */

     ow = lpOldWndRect->right - lpOldWndRect->left;
     oh = lpOldWndRect->bottom - lpOldWndRect->top;
     nw = Wnd->WindowRect.right - Wnd->WindowRect.left;
     nh = Wnd->WindowRect.bottom - Wnd->WindowRect.top;

     ocw = lpOldClientRect->right - lpOldClientRect->left;
     och = lpOldClientRect->bottom - lpOldClientRect->top;
     ncw = Wnd->ClientRect.right  - Wnd->ClientRect.left;
     nch = Wnd->ClientRect.bottom  - Wnd->ClientRect.top;

     if(  (ocw != ncw) || (och != nch) ||
	  ( ow !=  nw) || ( oh !=  nh) ||
	  ((lpOldClientRect->top - lpOldWndRect->top)   != 
	   (Wnd->ClientRect.top - Wnd->WindowRect.top)) ||
          ((lpOldClientRect->left - lpOldWndRect->left) !=
           (Wnd->ClientRect.left - Wnd->WindowRect.left)) )
     {
         if(uFlags & SWP_EX_PAINTSELF)
         {
             /* movement relative to the window itself */
             dx = (Wnd->ClientRect.left - Wnd->WindowRect.left) -
                 (lpOldClientRect->left - lpOldWndRect->left) ;
             dy = (Wnd->ClientRect.top - Wnd->WindowRect.top) -
                 (lpOldClientRect->top - lpOldWndRect->top) ;
         }
         else
         {
             /* movement relative to the parent's client area */
             dx = Wnd->ClientRect.left - lpOldClientRect->left;
             dy = Wnd->ClientRect.top - lpOldClientRect->top;
         }

	/* restrict valid bits to the common client rect */

	r.left = Wnd->ClientRect.left - Wnd->WindowRect.left;
        r.top = Wnd->ClientRect.top  - Wnd->WindowRect.top;
	r.right = r.left + min( ocw, ncw );
	r.bottom = r.top + min( och, nch );

	REGION_CropRgn( hrgnValid, hrgnValid, &r, 
			(uFlags & SWP_EX_PAINTSELF) ? NULL : (POINT*)&(Wnd->WindowRect));
	NtGdiGetRgnBox( hrgnValid, &r );
	if( NtGdiIsEmptyRect( &r ) )
	    goto nocopy;
	r = *lpOldClientRect;
     }
     else
     {
         if(uFlags & SWP_EX_PAINTSELF) {
             /* 
              * with SWP_EX_PAINTSELF, the window repaints itself. Since a window can't move 
              * relative to itself, only the client area can change.
              * if the client rect didn't change, there's nothing to do.
              */
             dx = 0;
             dy = 0;
         }
         else
         {
             dx = Wnd->WindowRect.left - lpOldWndRect->left;
             dy = Wnd->WindowRect.top -  lpOldWndRect->top;
             NtGdiOffsetRgn( hrgnValid, Wnd->WindowRect.left, Wnd->WindowRect.top );
         }
	r = *lpOldWndRect;
     }

     if( !(uFlags & SWP_EX_PAINTSELF) )
     {
        /* Move remaining regions to parent coordinates */
	NtGdiOffsetRgn( newVisRgn, Wnd->WindowRect.left, Wnd->WindowRect.top );
	NtGdiOffsetRgn( *pVisRgn,  lpOldWndRect->left, lpOldWndRect->top );
     }
     else
	NtGdiOffsetRect( &r, -lpOldWndRect->left, -lpOldWndRect->top );

     /* Compute combined dirty region (old + new - valid) */
     NtGdiCombineRgn( *pVisRgn, *pVisRgn, newVisRgn, RGN_OR);
     NtGdiCombineRgn( *pVisRgn, *pVisRgn, hrgnValid, RGN_DIFF);

     /* Blt valid bits, r is the rect to copy  */

     if( dx || dy )
     {
	 RECT rClip;
	 HDC hDC;

	 /* get DC and clip rect with drawable rect to avoid superfluous expose events
	    from copying clipped areas */

	 if( uFlags & SWP_EX_PAINTSELF )
	 {
	     hDC = NtUserGetDCEx( Wnd->Self, hrgnValid, DCX_WINDOW | DCX_CACHE |
			    DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CLIPSIBLINGS );
	     rClip.right = nw; rClip.bottom = nh;
	 }
	 else
	 {
	     hDC = NtUserGetDCEx( Wnd->Parent->Self, hrgnValid, DCX_CACHE |
			    DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CLIPSIBLINGS );
	     rClip.right = Wnd->Parent->ClientRect.right - Wnd->Parent->ClientRect.left;
	     rClip.bottom = Wnd->Parent->ClientRect.bottom - Wnd->Parent->ClientRect.top;
	 }
	 rClip.left = rClip.top = 0;    

         if( oh > nh ) r.bottom = r.top  + nh;
         if( ow < nw ) r.right = r.left  + nw;

         if( NtGdiIntersectRect( &r, &r, &rClip ) )
         {
	        NtGdiBitBlt(hDC, dx + r.left, dy + r.top, r.right - r.left, r.bottom - r.top, hDC, r.left, r.top, SRCCOPY);

                 /* When you copy the bits without repainting, parent doesn't
                    get validated appropriately. Therefore, we have to validate
                    the parent with the windows' updated region when the
                    parent's update region is not empty. */

                if (Wnd->Parent->UpdateRegion != 0 && !(Wnd->Parent->Style & WS_CLIPCHILDREN))
                {
                  NtGdiOffsetRect(&r, dx, dy);
                  NtUserValidateRect(Wnd->Parent->Self, &r);
                }
	 }
         NtUserReleaseDC( (uFlags & SWP_EX_PAINTSELF) ? 
		        Wnd->Self :  Wnd->Parent->Self, hDC); 
     }
 }

 /* *pVisRgn now points to the invalidated region */

 NtGdiDeleteObject(newVisRgn);
 NtGdiDeleteObject(dirtyRgn);
 return uFlags;
}


BOOLEAN STDCALL
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  WINDOWPOS WinPos;
  RECT NewWindowRect;
  RECT NewClientRect;
  HRGN VisRgn = NULL;
  HRGN VisibleRgn = NULL;
  ULONG WvrFlags = 0;
  RECT OldWindowRect, OldClientRect;
  UINT FlagsEx = 0;

  /* FIXME: Get current active window from active queue. */

  /* Check if the window is for a desktop. */
  if (Wnd == PsGetWin32Thread()->Desktop->DesktopWindow)
    {
      return(FALSE);
    }

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Wnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  
  /* Fix up the flags. */
  if (Window->Style & WS_VISIBLE)
    {
      flags &= ~SWP_SHOWWINDOW;
    }
  else 
    {
      if (!(flags & SWP_SHOWWINDOW))
	{
	  flags |= SWP_NOREDRAW;
	}
      flags &= ~SWP_HIDEWINDOW;
    }

  cx = max(cx, 0);
  cy = max(cy, 0);

  if ((Window->WindowRect.right - Window->WindowRect.left) == cx &&
      (Window->WindowRect.bottom - Window->WindowRect.top) == cy)
    {
      flags |= SWP_NOSIZE;
    }
  if (Window->WindowRect.left == x && Window->WindowRect.top == y)
    {
      flags |= SWP_NOMOVE;
    }
  if (Window->Style & WIN_NCACTIVATED)
    {
      flags |= SWP_NOACTIVATE;
    }
  else if ((Window->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
      if (!(flags & SWP_NOACTIVATE))
	{
	  flags &= ~SWP_NOZORDER;
	  WndInsertAfter = HWND_TOP;
	}
    }

  if (WndInsertAfter == HWND_TOPMOST || WndInsertAfter == HWND_NOTOPMOST)
    {
      WndInsertAfter = HWND_TOP;
    }

  if (WndInsertAfter != HWND_TOP && WndInsertAfter != HWND_BOTTOM)
    {
      /* FIXME: Find the window to insert after. */
    }

  WinPos.hwnd = Wnd;
  WinPos.hwndInsertAfter = WndInsertAfter;
  WinPos.x = x;
  WinPos.y = y;
  WinPos.cx = cx;
  WinPos.cy = cy;
  WinPos.flags = flags;

  WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

  if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
      SWP_NOZORDER)
    {
      /* FIXME: SWP_DoOwnedPopups. */
    }
  
  /* FIXME: Adjust flags based on WndInsertAfter */

  if ((!(WinPos.flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
       WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
		       SWP_HIDEWINDOW | SWP_FRAMECHANGED)) != 
      (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
    {
      if (Window->Style & WS_CLIPCHILDREN)
	{
	  VisRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                    Window, FALSE, FALSE, TRUE);
	}
      else
	{
	  VisRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                    Window, FALSE, FALSE, FALSE);
	}
      NtGdiOffsetRgn(VisRgn, -Window->WindowRect.left, -Window->WindowRect.top);
    }

  WvrFlags = WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect,
				&NewClientRect);

  /* FIXME: Relink windows. */

  /* FIXME: Reset active DCEs */

  OldWindowRect = Window->WindowRect;
  OldClientRect = Window->ClientRect;

  /* FIXME: Check for redrawing the whole client rect. */

  Window->WindowRect = NewWindowRect;
  Window->ClientRect = NewClientRect;

  if (WinPos.flags & SWP_SHOWWINDOW)
    {
      Window->Style |= WS_VISIBLE;
      FlagsEx |= SWP_EX_PAINTSELF;
      /* Redraw entire window. */
      VisRgn = (HRGN) 1;
    }
  else if (!(WinPos.flags & SWP_NOREDRAW))
    {
      if (WinPos.flags & SWP_HIDEWINDOW)
	{
	  if (VisRgn > (HRGN)1)
	    {
	      NtGdiOffsetRgn(VisRgn, OldWindowRect.left, OldWindowRect.top);	     
	    }
	  else
	    {
	      VisRgn = 0;
	    }
	}
      else
	{
	  if ((WinPos.flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE)
	    {
	      if ((WinPos.flags & SWP_AGG_NOGEOMETRYCHANGE) == SWP_AGG_NOGEOMETRYCHANGE)
		{
		  FlagsEx |= SWP_EX_PAINTSELF;
		}
	      FlagsEx = WinPosCopyValidBits(Window, &VisRgn, &OldWindowRect, &OldClientRect, FlagsEx);
	    }
	  else
	    {
	      if (WinPos.flags & SWP_FRAMECHANGED)
		{
		  WinPosDoSimpleFrameChanged(Window, &OldClientRect, WinPos.flags, FlagsEx);
		}
	      if (VisRgn != 0)
		{
		  NtGdiDeleteObject(VisRgn);
		  VisRgn = 0;
		}
	    }
	}
    }  

  if (WinPos.flags & SWP_HIDEWINDOW)
    {
      VisibleRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop, Window,
                                            FALSE, FALSE, FALSE);
      Window->Style &= ~WS_VISIBLE;
      VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, Window, VisibleRgn);
      NtGdiDeleteObject(VisibleRgn);
    }

  /* FIXME: Hide or show the claret */

  if (VisRgn)
    {
      if (!(WinPos.flags & SWP_NOREDRAW))
	{
	  if (FlagsEx & SWP_EX_PAINTSELF) 
	    {
	      PaintRedrawWindow(Window, NULL,
				(VisRgn == (HRGN) 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
				RDW_ALLCHILDREN | RDW_ERASENOW, 
				RDW_EX_XYWINDOW | RDW_EX_USEHRGN);
	    }
	  else
	    {
	      PaintRedrawWindow(Window->Parent, NULL,
				(VisRgn == (HRGN) 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN |
	                        RDW_ERASENOW,
				RDW_EX_USEHRGN);
	    }
	  /* FIXME: Redraw the window parent. */
	}
      NtGdiDeleteObject(VisRgn);
    }

  if (!(flags & SWP_NOACTIVATE))
    {
      WinPosChangeActiveWindow(WinPos.hwnd, FALSE);
    }

  /* FIXME: Check some conditions before doing this. */
  IntSendWINDOWPOSCHANGEDMessage(Window->Self, &WinPos);

  ObmDereferenceObject(Window);
  return(TRUE);
}

LRESULT STDCALL
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
  *ClientRect = *WindowRect;
  return(IntSendNCCALCSIZEMessage(Wnd, FALSE, ClientRect, NULL));
}

BOOLEAN FASTCALL
WinPosShowWindow(HWND Wnd, INT Cmd)
{
  BOOLEAN WasVisible;
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  UINT Swp = 0;
  RECT NewPos;
  BOOLEAN ShowFlag;
  HRGN VisibleRgn;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Wnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  
  WasVisible = (Window->Style & WS_VISIBLE) != 0;

  switch (Cmd)
    {
    case SW_HIDE:
      {
	if (!WasVisible)
	  {
	    ObmDereferenceObject(Window);
	    return(FALSE);
	  }
	Swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE |
	  SWP_NOZORDER;
	break;
      }

    case SW_SHOWMINNOACTIVE:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWMINIMIZED:
      Swp |= SWP_SHOWWINDOW;
      /* Fall through. */
    case SW_MINIMIZE:
      {
	Swp |= SWP_FRAMECHANGED;
	if (!(Window->Style & WS_MINIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MINIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWMAXIMIZED:
      {
	Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
	if (!(Window->Style & WS_MAXIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MAXIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWNA:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOW:
      Swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
      /* Don't activate the topmost window. */
      break;

    case SW_SHOWNOACTIVATE:
      Swp |= SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWNORMAL:
    case SW_SHOWDEFAULT:
    case SW_RESTORE:
      Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
      if (Window->Style & (WS_MINIMIZE | WS_MAXIMIZE))
	{
	  Swp |= WinPosMinMaximize(Window, SW_RESTORE, &NewPos);	 
	}
      else
	{
	  Swp |= SWP_NOSIZE | SWP_NOMOVE;
	}
      break;
    }

  ShowFlag = (Cmd != SW_HIDE);
  if (ShowFlag != WasVisible)
    {
      NtUserSendMessage(Wnd, WM_SHOWWINDOW, ShowFlag, 0);
      /* 
       * FIXME: Need to check the window wasn't destroyed during the 
       * window procedure. 
       */
    }

  if (Window->Style & WS_CHILD &&
      !IntIsWindowVisible(Window->Parent->Self) &&
      (Swp & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE))
    {
      if (Cmd == SW_HIDE)
	{
	  VisibleRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop, Window,
	                                        FALSE, FALSE, FALSE);
	  Window->Style &= ~WS_VISIBLE;
	  VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, Window, VisibleRgn);
	  NtGdiDeleteObject(VisibleRgn);
	}
      else
	{
	  Window->Style |= WS_VISIBLE;
	}
    }
  else
    {
      if (Window->Style & WS_CHILD &&
	  !(Window->ExStyle & WS_EX_MDICHILD))
	{
	  Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	}
      if (!(Swp & MINMAX_NOSWP))
	{
	  WinPosSetWindowPos(Wnd, HWND_TOP, NewPos.left, NewPos.top,
			     NewPos.right, NewPos.bottom, LOWORD(Swp));
	  if (Cmd == SW_HIDE)
	    {
	      /* Hide the window. */
	      if (Wnd == IntGetActiveWindow())
		{
		  WinPosActivateOtherWindow(Window);
		}
	      /* Revert focus to parent. */
	      if (Wnd == IntGetFocusWindow() ||
		  IntIsChildWindow(Wnd, IntGetFocusWindow()))
		{
		  IntSetFocusWindow(Window->Parent->Self);
		}
	    }
	}
      /* FIXME: Check for window destruction. */
      /* Show title for minimized windows. */
      if (Window->Style & WS_MINIMIZE)
	{
	  WinPosShowIconTitle(Window, TRUE);
	}
    }

  if (Window->Flags & WINDOWOBJECT_NEED_SIZE)
    {
      WPARAM wParam = SIZE_RESTORED;

      Window->Flags &= ~WINDOWOBJECT_NEED_SIZE;
      if (Window->Style & WS_MAXIMIZE)
	{
	  wParam = SIZE_MAXIMIZED;
	}
      else if (Window->Style & WS_MINIMIZE)
	{
	  wParam = SIZE_MINIMIZED;
	}

      NtUserSendMessage(Wnd, WM_SIZE, wParam,
			MAKELONG(Window->ClientRect.right - 
				 Window->ClientRect.left,
				 Window->ClientRect.bottom -
				 Window->ClientRect.top));
      NtUserSendMessage(Wnd, WM_MOVE, 0,
			MAKELONG(Window->ClientRect.left,
				 Window->ClientRect.top));
    }

  /* Activate the window if activation is not requested and the window is not minimized */
  if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->Style & WS_MINIMIZE))
    {
      WinPosChangeActiveWindow(Wnd, FALSE);
    }

  ObmDereferenceObject(Window);
  return(WasVisible);
}

BOOL STATIC FASTCALL
WinPosPtInWindow(PWINDOW_OBJECT Window, POINT Point)
{
  return(Point.x >= Window->WindowRect.left &&
	 Point.x < Window->WindowRect.right &&
	 Point.y >= Window->WindowRect.top &&
	 Point.y < Window->WindowRect.bottom);
}

USHORT STATIC STDCALL
WinPosSearchChildren(PWINDOW_OBJECT ScopeWin, POINT Point,
		     PWINDOW_OBJECT* Window)
{
  PWINDOW_OBJECT Current;

  ExAcquireFastMutexUnsafe(&ScopeWin->ChildrenListLock);
  Current = ScopeWin->FirstChild;
  while (Current)
    {
      if (Current->Style & WS_VISIBLE &&
	  ((!(Current->Style & WS_DISABLED)) ||
	   (Current->Style & (WS_CHILD | WS_POPUP)) != WS_CHILD) &&
	  WinPosPtInWindow(Current, Point))
	{
	  *Window = Current;
	  if (Current->Style & WS_DISABLED)
	    {
		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(HTERROR);
	    }
	  if (Current->Style & WS_MINIMIZE)
	    {
		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(HTCAPTION);
	    }
	  if (Point.x >= Current->ClientRect.left &&
	      Point.x < Current->ClientRect.right &&
	      Point.y >= Current->ClientRect.top &&
	      Point.y < Current->ClientRect.bottom)
	    {

		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(WinPosSearchChildren(Current, Point, Window));
	    }

	  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	  return(0);
	}
      Current = Current->NextSibling;
    }
		  
  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
  return(0);
}

USHORT STDCALL
WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, POINT WinPoint, 
		      PWINDOW_OBJECT* Window)
{
  HWND DesktopWindowHandle;
  PWINDOW_OBJECT DesktopWindow;
  POINT Point = WinPoint;
  USHORT HitTest;

  *Window = NULL;

  if (ScopeWin->Style & WS_DISABLED)
    {
      return(HTERROR);
    }

  /* Translate the point to the space of the scope window. */
  DesktopWindowHandle = IntGetDesktopWindow();
  DesktopWindow = IntGetWindowObject(DesktopWindowHandle);
  Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
  Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
  IntReleaseWindowObject(DesktopWindow);

  HitTest = WinPosSearchChildren(ScopeWin, Point, Window);
  if (HitTest != 0)
    {
      return(HitTest);
    }

  if ((*Window) == NULL)
    {
      return(HTNOWHERE);
    }
  if ((*Window)->MessageQueue == PsGetWin32Thread()->MessageQueue)
    {
      HitTest = IntSendMessage((*Window)->Self, WM_NCHITTEST, 0,
				MAKELONG(Point.x, Point.y), FALSE);
      /* FIXME: Check for HTTRANSPARENT here. */
    }
  else
    {
      HitTest = HTCLIENT;
    }

  return(HitTest);
}

BOOL
WinPosSetActiveWindow(PWINDOW_OBJECT Window, BOOL Mouse, BOOL ChangeFocus)
{
  PUSER_MESSAGE_QUEUE ActiveQueue;
  HWND PrevActive;

  ActiveQueue = IntGetFocusMessageQueue();
  if (ActiveQueue != NULL)
    {
      PrevActive = ActiveQueue->ActiveWindow;
    }
  else
    {
      PrevActive = NULL;
    }

  if (Window->Self == IntGetActiveDesktop() || Window->Self == PrevActive)
    {
      return(FALSE);
    }
  if (PrevActive != NULL)
    {
      PWINDOW_OBJECT PrevActiveWindow = IntGetWindowObject(PrevActive);
      WORD Iconised = HIWORD(PrevActiveWindow->Style & WS_MINIMIZE);
      if (!IntSendMessage(PrevActive, WM_NCACTIVATE, FALSE, 0, TRUE))
	{
	  /* FIXME: Check if the new window is system modal. */
	  return(FALSE);
	}
      IntSendMessage(PrevActive, 
		      WM_ACTIVATE, 
		      MAKEWPARAM(WA_INACTIVE, Iconised), 
		      (LPARAM)Window->Self,
		      TRUE);
      /* FIXME: Check if anything changed while processing the message. */
      IntReleaseWindowObject(PrevActiveWindow);
    }

  if (Window != NULL)
    {
      Window->MessageQueue->ActiveWindow = Window->Self;
    }
  else if (ActiveQueue != NULL)
    {
      ActiveQueue->ActiveWindow = NULL;
    }
  /* FIXME:  Unset this flag for inactive windows */
  //if ((Window->Style) & WS_CHILD) Window->Flags |= WIN_NCACTIVATED;

  /* FIXME: Send palette messages. */

  /* FIXME: Redraw icon title of previously active window. */

  /* FIXME: Bring the window to the top. */  

  /* FIXME: Send WM_ACTIVATEAPP */
  
  IntSetFocusMessageQueue(Window->MessageQueue);

  /* FIXME: Send activate messages. */

  /* FIXME: Change focus. */

  /* FIXME: Redraw new window icon title. */

  return(TRUE);
}

HWND STDCALL
NtUserGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ActiveQueue;

  ActiveQueue = IntGetFocusMessageQueue();
  if (ActiveQueue == NULL)
    {
      return(NULL);
    }
  return(ActiveQueue->ActiveWindow);
}

HWND STDCALL
NtUserSetActiveWindow(HWND hWnd)
{
  PWINDOW_OBJECT Window;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  HWND Prev;

  Window = IntGetWindowObject(hWnd);
  if (Window == NULL || (Window->Style & (WS_DISABLED | WS_CHILD)))
    {
      IntReleaseWindowObject(Window);
      return(0);
    }
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  if (Window->MessageQueue != ThreadQueue)
    {
      IntReleaseWindowObject(Window);
      return(0);
    }
  Prev = Window->MessageQueue->ActiveWindow;
  WinPosSetActiveWindow(Window, FALSE, FALSE);
  IntReleaseWindowObject(Window);
  return(Prev);
}


/* EOF */
