/* $Id: windc.c,v 1.9 2003/05/03 14:12:14 gvg Exp $
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
#include <win32k/region.h>
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

/* GLOBALS *******************************************************************/

static PDCE FirstDce = NULL;

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

VOID STATIC
DceOffsetVisRgn(HDC hDC, HRGN hVisRgn)
{
  DC *dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
    {
      return;
    }
  W32kOffsetRgn(hVisRgn, dc->w.DCOrgX, dc->w.DCOrgY);
  DC_ReleasePtr(hDC);
}

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
      if ((VisRgn = UnsafeW32kCreateRectRgnIndirect(&Rect)) != NULL)
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

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
    return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

DCE* DceAllocDCE(HWND hWnd, DCE_TYPE Type)
{
  HDCE DceHandle;
  DCE* Dce;

  DceHandle = DCEOBJ_AllocDCE();
  Dce = DCEOBJ_LockDCE(DceHandle);
  Dce->hDC = W32kCreateDC(L"DISPLAY", NULL, NULL, NULL);
  Dce->hwndCurrent = hWnd;
  Dce->hClipRgn = NULL;
  Dce->next = FirstDce;
  FirstDce = Dce;

  if (Type != DCE_CACHE_DC)
    {
      Dce->DCXFlags = DCX_DCEBUSY;
      if (hWnd != NULL)
	{
	  PWINDOW_OBJECT WindowObject;

	  WindowObject = W32kGetWindowObject(hWnd);
	  if (WindowObject->Style & WS_CLIPCHILDREN)
	    {
	      Dce->DCXFlags |= DCX_CLIPCHILDREN;
	    }
	  if (WindowObject->Style & WS_CLIPSIBLINGS)
	    {
	      Dce->DCXFlags |= DCX_CLIPSIBLINGS;
	    }
	  W32kReleaseWindowObject(WindowObject);
	}
    }
  else
    {
      Dce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
    }

  return(Dce);
}

VOID STATIC
DceSetDrawable(PWINDOW_OBJECT WindowObject, HDC hDC, ULONG Flags,
	       BOOL SetClipOrigin)
{
  DC *dc = DC_HandleToPtr(hDC);
  if (WindowObject == NULL)
    {
      dc->w.DCOrgX = 0;
      dc->w.DCOrgY = 0;
    }
  else
    {
      if (Flags & DCX_WINDOW)
	{
	  dc->w.DCOrgX = WindowObject->WindowRect.left;
	  dc->w.DCOrgY = WindowObject->WindowRect.top;
	}
      else
	{
	  dc->w.DCOrgX = WindowObject->ClientRect.left;
	  dc->w.DCOrgY = WindowObject->ClientRect.top;
	}
      /* FIXME: Offset by parent's client rectangle. */
    }
  DC_ReleasePtr(hDC);
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE hRegion, ULONG Flags)
{
  PWINDOW_OBJECT Window;
  ULONG DcxFlags;
  DCE* Dce;
  BOOL UpdateVisRgn = TRUE;
  BOOL UpdateClipOrigin = FALSE;
  HANDLE hRgnVisible = NULL;

if (NULL == hWnd)
__asm__("int $3\n");
  if (NULL == hWnd)
    {
      Flags &= ~DCX_USESTYLE;
      Window = NULL;
    }
  else if (NULL == (Window = W32kGetWindowObject(hWnd)))
    {
      return(0);
    }

  if (NULL == Window || NULL == Window->Dce)
    {
      Flags |= DCX_CACHE;
    }


  if (Flags & DCX_USESTYLE)
    {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

      if (Window->Style & WS_CLIPSIBLINGS)
	{
	  Flags |= DCX_CLIPSIBLINGS;
	}

      if (!(Flags & DCX_WINDOW))
	{
	  if (Window->Class->Class.style & CS_PARENTDC)
	    {
	      Flags |= DCX_PARENTCLIP;
	    }

	  if (Window->Style & WS_CLIPCHILDREN &&
	      !(Window->Style & WS_MINIMIZE))
	    {
	      Flags |= DCX_CLIPCHILDREN;
	    }
	}
      else
	{
	  Flags |= DCX_CACHE;
	}
    }

  if (Flags & DCX_NOCLIPCHILDREN)
    {
      Flags |= DCX_CACHE;
      Flags |= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
    }

  if (Flags & DCX_WINDOW)
    {
      Flags = (Flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;
    }

  if (NULL == Window || !(Window->Style & WS_CHILD) || NULL == Window->Parent)
    {
      Flags &= ~DCX_PARENTCLIP;
    }
  else if (Flags & DCX_PARENTCLIP)
    {
      Flags |= DCX_CACHE;
      if (!(Flags & (DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS)))
	{
	  if ((Window->Style & WS_VISIBLE) && 
	      (Window->Parent->Style & WS_VISIBLE))
	    {
	      Flags &= ~DCX_CLIPCHILDREN;
	      if (Window->Parent->Style & WS_CLIPSIBLINGS)
		{
		  Flags |= DCX_CLIPSIBLINGS;
		}
	    }
	}
    }

  DcxFlags = Flags & DCX_CACHECOMPAREMASK;

  if (Flags & DCX_CACHE)
    {
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;

      for (Dce = FirstDce; Dce != NULL; Dce = Dce->next)
	{
	  if ((Dce->DCXFlags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
	    {
	      DceUnused = Dce;
	      if (Dce->DCXFlags & DCX_DCEEMPTY)
		{
		  DceEmpty = Dce;
		}
	      else if (Dce->hwndCurrent == hWnd &&
		       ((Dce->DCXFlags & DCX_CACHECOMPAREMASK) == DcxFlags))
		{
		  UpdateVisRgn = FALSE;
		  UpdateClipOrigin = TRUE;
		  break;
		}
	    }
	}

      if (Dce == NULL)
	{
	  Dce = (DceEmpty == NULL) ? DceEmpty : DceUnused;
	}

      if (Dce == NULL)
	{
	  Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
	}
    }
  else
    {
      Dce = Window->Dce;
      /* FIXME: Implement this. */
      DbgBreakPoint();
    }

  if (NULL == Dce && NULL != Window)
    {
      W32kReleaseWindowObject(Window);
      return(NULL);
    }

  Dce->hwndCurrent = hWnd;
  Dce->hClipRgn = NULL;
  Dce->DCXFlags = DcxFlags | (Flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;

  DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

  if (UpdateVisRgn)
    {
      if (Flags & DCX_PARENTCLIP)
	{
	  PWINDOW_OBJECT Parent;

	  Parent = Window->Parent;

	  if (Window->Style & WS_VISIBLE &&
	      !(Parent->Style & WS_MINIMIZE))
	    {
	      if (Parent->Style & WS_CLIPSIBLINGS)
		{
		  DcxFlags = DCX_CLIPSIBLINGS | 
		    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
		}
	      else
		{
		  DcxFlags = Flags & 
		    ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
		}
	      hRgnVisible = DceGetVisRgn(Parent->Self, DcxFlags, 
					 Window->Self, Flags);
	      if (Flags & DCX_WINDOW)
		{
		  W32kOffsetRgn(hRgnVisible, -Window->WindowRect.left,
				-Window->WindowRect.top);
		}
	      else
		{
		  W32kOffsetRgn(hRgnVisible, -Window->ClientRect.left,
				-Window->ClientRect.top);
		}
	      DceOffsetVisRgn(Dce->hDC, hRgnVisible);
	    }
	  else
	    {
	      hRgnVisible = W32kCreateRectRgn(0, 0, 0, 0);
	    }
	}
      else
	{
	  if (hWnd == W32kGetDesktopWindow())
	    {
	      hRgnVisible = 
		W32kCreateRectRgn(0, 0, 
				  NtUserGetSystemMetrics(SM_CXSCREEN),
				  NtUserGetSystemMetrics(SM_CYSCREEN));
	    }
	  else
	    {
	      hRgnVisible = DceGetVisRgn(hWnd, Flags, 0, 0);
	      DceOffsetVisRgn(Dce->hDC, hRgnVisible);
	    }

	  Dce->DCXFlags &= ~DCX_DCEDIRTY;
	  W32kSelectVisRgn(Dce->hDC, hRgnVisible);
	}
    }

  if (Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
    {
      DPRINT1("FIXME.\n");
    }

  if (hRgnVisible != NULL)
    {
      W32kDeleteObject(hRgnVisible);
    }
  if (NULL != Window)
    {
      W32kReleaseWindowObject(Window);
    }

  return(Dce->hDC);
}

BOOL
DCE_InternalDelete(PDCE Dce)
{
  PDCE PrevInList;

  if (Dce == FirstDce)
    {
      FirstDce = Dce->next;
      PrevInList = Dce;
    }
  else
    {
      for (PrevInList = FirstDce; NULL != PrevInList; PrevInList = PrevInList->next)
	{
	  if (Dce == PrevInList->next)
	    {
	      PrevInList->next = Dce->next;
	      break;
	    }
	}
      assert(NULL != PrevInList);
    }

  return NULL != PrevInList;
}
  

/* EOF */
