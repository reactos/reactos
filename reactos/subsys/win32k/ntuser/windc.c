/* $Id: windc.c,v 1.4 2002/08/27 21:20:45 jfilby Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/rect.h>
#include <include/dce.h>

#define NDEBUG
#include <debug.h>

static PDCE firstDCE;

/* FUNCTIONS *****************************************************************/

VOID DCE_FreeWindowDCE(HWND);
INT  DCE_ExcludeRgn(HDC, HWND, HRGN);
BOOL DCE_InvalidateDCE(HWND, const PRECTL);

BOOL STATIC
DceGetVisRect(PWINDOW_OBJECT Window, BOOL ClientArea, RECT* Rect)
{
  if (ClientArea)
    {
      *Rect = Window->ClientRect;
    }
  else
    {
      *Rect = Window->WindowRect;
    }

  if (Window->Style & WS_VISIBLE)
    {
      INT XOffset = Rect->left;
      INT YOffset = Rect->top;

      while ((Window = Window->Parent) != NULL)
	{
	  if ((Window->Style & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE)
	    {
	      W32kSetEmptyRect(Rect);
	      return(FALSE);
	    }
	  XOffset += Window->ClientRect.left;
	  YOffset += Window->ClientRect.top;
	  W32kOffsetRect(Rect, Window->ClientRect.left, 
			 Window->ClientRect.top);
	  if (Window->ClientRect.left >= Window->ClientRect.right ||
	      Window->ClientRect.top >= Window->ClientRect.bottom ||
	      Rect->left >= Window->ClientRect.right ||
	      Rect->right <= Window->ClientRect.left ||
	      Rect->top >= Window->ClientRect.bottom ||
	      Rect->bottom <= Window->ClientRect.top)
	    {
	      W32kSetEmptyRect(Rect);
	      return(FALSE);
	    }
	  Rect->left = max(Rect->left, Window->ClientRect.left);
	  Rect->right = min(Rect->right, Window->ClientRect.right);
	  Rect->top = max(Rect->top, Window->ClientRect.top);
	  Rect->bottom = min(Rect->bottom, Window->ClientRect.bottom);
	}
      W32kOffsetRect(Rect, -XOffset, -YOffset);
      return(TRUE);
    }
  W32kSetEmptyRect(Rect);
  return(FALSE);
}

BOOL
DceAddClipRects(PWINDOW_OBJECT Parent, PWINDOW_OBJECT End, 
		HRGN ClipRgn, PRECT Rect, INT XOffset, INT YOffset)
{
  PLIST_ENTRY ChildListEntry;
  PWINDOW_OBJECT Child;
  RECT Rect1;

  ChildListEntry = Parent->ChildrenListHead.Flink;
  while (ChildListEntry != &Parent->ChildrenListHead)
    {
      Child = CONTAINING_RECORD(ChildListEntry, WINDOW_OBJECT, 
				SiblingListEntry);
      if (Child == End)
	{
	  return(TRUE);
	}
      if (Child->Style & WS_VISIBLE)
	{
	  Rect1.left = Child->WindowRect.left + XOffset;
	  Rect1.top = Child->WindowRect.top + YOffset;
	  Rect1.right = Child->WindowRect.right + XOffset;
	  Rect1.bottom = Child->WindowRect.bottom + YOffset;

	  if (W32kIntersectRect(&Rect1, &Rect1, Rect))
	    {
	      W32kUnionRectWithRgn(ClipRgn, &Rect1);
	    }
	}
      ChildListEntry = ChildListEntry->Flink;
    }
  return(FALSE);
}

HRGN 
DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT Child;
  HRGN VisRgn;
  RECT Rect;

  Window = W32kGetWindowObject(hWnd);
  Child = W32kGetWindowObject(hWndChild);

  if (Window != NULL && DceGetVisRect(Window, !(Flags & DCX_WINDOW), &Rect))
    {
      if ((VisRgn = W32kCreateRectRgnIndirect(&Rect)) != NULL)
	{
	  HRGN ClipRgn = W32kCreateRectRgn(0, 0, 0, 0);
	  INT XOffset, YOffset;

	  if (ClipRgn != NULL)
	    {
	      if (Flags & DCX_CLIPCHILDREN && 
		  !IsListEmpty(&Window->ChildrenListHead))
		{
		  if (Flags & DCX_WINDOW)
		    {
		      XOffset = Window->ClientRect.left -
			Window->WindowRect.left;
		      YOffset = Window->ClientRect.top -
			Window->WindowRect.top;
		    }		
		  else
		    {
		      XOffset = YOffset = 0;
		    }
		  DceAddClipRects(Window, NULL, ClipRgn, &Rect,
				  XOffset, YOffset);
		}

	      if (CFlags & DCX_CLIPCHILDREN && Child &&
		  !IsListEmpty(&Child->ChildrenListHead))
		{
		  if (Flags & DCX_WINDOW)
		    {
		      XOffset = Window->ClientRect.left -
			Window->WindowRect.left;
		      YOffset = Window->ClientRect.top -
			Window->WindowRect.top;
		    }
		  else
		    {
		      XOffset = YOffset = 0;
		    }

		  XOffset += Child->ClientRect.left;
		  YOffset += Child->ClientRect.top;

		  DceAddClipRects(Child, NULL, ClipRgn, &Rect,
				  XOffset, YOffset);
		}

	      if (Flags & DCX_WINDOW)
		{
		  XOffset = -Window->WindowRect.left;
		  YOffset = -Window->WindowRect.top;
		}
	      else
		{
		  XOffset = -Window->ClientRect.left;
		  YOffset = -Window->ClientRect.top;
		}

	      if (Flags & DCX_CLIPSIBLINGS && Window->Parent != NULL)
		{
		  DceAddClipRects(Window->Parent, Window, ClipRgn,
				  &Rect, XOffset, YOffset);
		}
	      
	      while (Window->Style & WS_CHILD)
		{
		  Window = Window->Parent;
		  XOffset -= Window->ClientRect.left;
		  YOffset -= Window->ClientRect.top;
		  if (Window->Style & WS_CLIPSIBLINGS && 
		      Window->Parent != NULL)
		    {
		      DceAddClipRects(Window->Parent, Window, ClipRgn,
				      &Rect, XOffset, YOffset);
		    }
		}

	      W32kCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
	      W32kDeleteObject(ClipRgn);
	    }
	  else
	    {
	      W32kDeleteObject(VisRgn);
	      VisRgn = 0;
	    }
	}
    }
  else
    {
      VisRgn = W32kCreateRectRgn(0, 0, 0, 0);
    }
  W32kReleaseWindowObject(Window);
  W32kReleaseWindowObject(Child);
  return(VisRgn);
}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  
}

DWORD
STDCALL
NtUserGetDC(HWND hWnd)
{
    if (!hWnd)
        return NtUserGetDCEx(0, 0, DCX_CACHE | DCX_WINDOW);
    return NtUserGetDCEx(hWnd, 0, DCX_USESTYLE);
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE hRegion, ULONG Flags)
{
    HDC 	hdc = 0;
    HDCE	hdce;
    PDCE	dce;
    DWORD 	dcxFlags = 0;
    BOOL	bUpdateVisRgn = TRUE;
    BOOL	bUpdateClipOrigin = FALSE;
    HWND parent, full;
    PWINDOW_OBJECT Window;

    DPRINT("hWnd %04x, hRegion %04x, Flags %08x\n", hWnd, hRegion, (unsigned)Flags);

    if (!hWnd) hWnd = W32kGetDesktopWindow();
    if (!(Window = W32kGetWindowObject(hWnd))) return 0;

    // fixup flags

    if (Flags & (DCX_WINDOW | DCX_PARENTCLIP)) Flags |= DCX_CACHE;

    if (Flags & DCX_USESTYLE)
    {
	Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if(Window->Style & WS_CLIPSIBLINGS)
            Flags |= DCX_CLIPSIBLINGS;

	if (!(Flags & DCX_WINDOW))
	{
            if (Window->ExStyle & CS_PARENTDC) Flags |= DCX_PARENTCLIP;

	    if (Window->Style & WS_CLIPCHILDREN &&
                     !(Window->Style & WS_MINIMIZE)) Flags |= DCX_CLIPCHILDREN;
            if (!Window->dce) Flags |= DCX_CACHE;
	}
    }

    if (Flags & DCX_WINDOW) Flags &= ~DCX_CLIPCHILDREN;

    parent = W32kGetParentWindow(hWnd);
    if (!parent || (parent == W32kGetDesktopWindow()))
        Flags = (Flags & ~DCX_PARENTCLIP) | DCX_CLIPSIBLINGS;

    // it seems parent clip is ignored when clipping siblings or children
    if (Flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) Flags &= ~DCX_PARENTCLIP;

    if(Flags & DCX_PARENTCLIP)
    {
        LONG parent_style = NtUserGetWindowLong(parent, GWL_STYLE);
        if((Window->Style & WS_VISIBLE) && (parent_style & WS_VISIBLE))
        {
            Flags &= ~DCX_CLIPCHILDREN;
            if (parent_style & WS_CLIPSIBLINGS) Flags |= DCX_CLIPSIBLINGS;
        }
    }

    // find a suitable DCE

    dcxFlags = Flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
		        DCX_CACHE | DCX_WINDOW);

    if (Flags & DCX_CACHE)
    {
	PDCE dceEmpty;
	PDCE dceUnused;

	dceEmpty = dceUnused = NULL;

	/* Strategy: First, we attempt to find a non-empty but unused DCE with
	 * compatible flags. Next, we look for an empty entry. If the cache is
	 * full we have to purge one of the unused entries.
	 */

	for (dce = firstDCE; (dce); dce = dce->next)
	{
	    if ((dce->DCXflags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
	    {
		dceUnused = dce;

		if (dce->DCXflags & DCX_DCEEMPTY)
		    dceEmpty = dce;
		else
		if ((dce->hwndCurrent == hWnd) &&
		   ((dce->DCXflags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
				      DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)) == dcxFlags))
		{
		    DPRINT("Found valid %08x dce [%04x], flags %08x\n",
					(unsigned)dce, hWnd, (unsigned)dcxFlags);
		    bUpdateVisRgn = FALSE;
		    bUpdateClipOrigin = TRUE;
		    break;
		}
	    }
	}

	if (!dce) dce = (dceEmpty) ? dceEmpty : dceUnused;

        // if there's no dce empty or unused, allocate a new one
        if (!dce)
        {
            hdce = DCEOBJ_AllocDCE();
            if (hdce == NULL)
            {
                return 0;
            }
            dce = DCEOBJ_LockDCE(hdce);
            dce->type = DCE_CACHE_DC;
            dce->hDC = hdce;
        }
    }
    else
    {
        dce = Window->dce;
        if (dce && dce->hwndCurrent == hWnd)
	{
	    DPRINT("skipping hVisRgn update\n");
	    bUpdateVisRgn = FALSE; // updated automatically, via DCHook()
	}
    }
    if (!dce)
    {
        hdc = 0;
        goto END;
    }

    if (!(Flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))) hRegion = 0;

    if (((Flags ^ dce->DCXflags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->hClipRgn != hRegion))
    {
        // if the extra clip region has changed, get rid of the old one
/*        DCE_DeleteClipRgn(dce); */
    }

    dce->hwndCurrent = hWnd;
    dce->hClipRgn = hRegion;
    dce->DCXflags = Flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                             DCX_CACHE | DCX_WINDOW | DCX_WINDOWPAINT |
                             DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
    dce->DCXflags |= DCX_DCEBUSY;
    dce->DCXflags &= ~DCX_DCEDIRTY;
    hdc = dce->hDC;

/*    if (bUpdateVisRgn) SetHookFlags(hdc, DCHF_INVALIDATEVISRGN); // force update */

/*    if (!USER_Driver.pGetDC(hWnd, hdc, hRegion, Flags)) hdc = 0; */

    DPRINT("(%04x,%04x,0x%lx): returning %04x\n", hWnd, hRegion, Flags, hdc);

END:
/*    WIN_ReleasePtr(Window); */
    return hdc;
}

/* EOF */
