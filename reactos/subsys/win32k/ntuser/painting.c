/* $Id: painting.c,v 1.1 2002/07/04 20:12:27 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Painting
 * FILE:             subsys/win32k/ntuser/painting.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
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
#include <include/painting.h>
#include <user32/wininternal.h>
#include <include/rect.h>
#include <win32k/coord.h>


#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define UNC_DELAY_NCPAINT                      (0x00000001)
#define UNC_IN_BEGINPAINT                      (0x00000002)
#define UNC_CHECK                              (0x00000004)
#define UNC_REGION                             (0x00000008)
#define UNC_ENTIRE                             (0x00000010)
#define UNC_UPDATE                             (0x00000020)

/* FUNCTIONS *****************************************************************/

HRGN STATIC
PaintDoPaint(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags, ULONG ExFlags)
{
  HDC hDC;
  HWND hWnd = Window->Self;
  BOOL bIcon = (Window->Style & WS_MINIMIZE) &&
    NtUserGetClassLong(hWnd, GCL_HICON);

  if (ExFlags & RDW_EX_DELAY_NCPAINT || 
      PaintHaveToDelayNCPaint(Window, 0))
    {
      ExFlags |= RDW_EX_DELAY_NCPAINT;
    }

  if (Flags & RDW_UPDATENOW)
    {
      if (Window->UpdateRegion != NULL)
	{
	  NtUserSendMessage(hWnd, bIcon ? WM_PAINTICON : WM_PAINT, bIcon, 0);
	}
    }
  else if (Flags & RDW_ERASENOW || ExFlags & RDW_EX_TOPFRAME)
    {
      UINT Dcx = DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN |
	DCX_WINDOWPAINT | DCX_CACHE;
      HRGN hRgnRet;

      hRgnRet = 
	PaintUpdateNCRegion(Window,
			 hRgn,
			 UNC_REGION | UNC_CHECK |
			 ((ExFlags & RDW_EX_TOPFRAME) ? UNC_ENTIRE : 0) |
			 ((ExFlags & RDW_EX_DELAY_NCPAINT) ? 
			  UNC_DELAY_NCPAINT : 0));
      if (hRgnRet != NULL)
	{
	  if (hRgnRet != (HRGN)1)
	    {
	      hRgn = hRgnRet;
	    }
	  else
	    {
	      hRgnRet = NULL;
	    }
	  if (Window->Flags & WINDOWOBJECT_NEED_ERASEBACKGRD)
	    {
	      if (bIcon)
		{
		  Dcx |= DCX_WINDOW;
		}
	      if (hRgnRet)
		{
		  W32kOffsetRgn(hRgnRet, 
				Window->WindowRect.left - 
				Window->ClientRect.left,
				Window->WindowRect.top -
				Window->ClientRect.top);
		}
	      else
		{
		  Dcx &= ~DCX_INTERSECTRGN;
		}
	      if ((hDC = NtUserGetDCEx(hWnd, hRgnRet, Dcx)) != NULL)
		{
		  LRESULT Result;
		  Result = NtUserSendMessage(hWnd, bIcon ? WM_ICONERASEBKGND :
					     WM_ERASEBKGND, (WPARAM)hDC, 0);
		  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
		  NtUserReleaseDC(hWnd, hDC);
		}
	    }
	}
    }

  /* FIXME: Check that the window is still valid at this point. */

  ExFlags &= ~RDW_EX_TOPFRAME;

  /* FIXME: Paint child windows. */

  return(hRgn);
}

VOID STATIC
PaintUpdateRgns(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags, 
		BOOL First)
{

}

BOOL
PaintRedrawWindow(HWND hWnd, const RECT* UpdateRect, HRGN UpdateRgn,
		  ULONG Flags, ULONG ExFlags)
{
  PWINDOW_OBJECT Window;
  RECT Rect, Rect2;
  POINT Pt;
  HRGN hRgn;

  /* FIXME: Return if this is for a desktop. */

  Window = W32kGetWindowObject(hWnd);
  if (Window == NULL)
    {
      return(FALSE);
    }

  if (Flags & RDW_FRAME)
    {
      Rect = Window->WindowRect;
    }
  else
    {
      Rect = Window->ClientRect;
    }

  if (ExFlags & RDW_EX_XYWINDOW)
    {
      Pt.x = Pt.y = 0;
      W32kOffsetRect(&Rect, -Window->WindowRect.left, -Window->WindowRect.top);
    }
  else
    {
      Pt.x = Window->ClientRect.left - Window->WindowRect.left;
      Pt.y = Window->ClientRect.top - Window->WindowRect.top;
      W32kOffsetRect(&Rect, -Window->ClientRect.left, -Window->WindowRect.top);
    }

  if (Flags & RDW_INVALIDATE)
    {
      if (UpdateRgn != NULL)
	{
	  if (Window->UpdateRegion != NULL)
	    {
	      hRgn = W32kCropRgn(0, UpdateRgn, NULL, &Pt);
	    }
	  else
	    {
	      Window->UpdateRegion = W32kCropRgn(0, UpdateRgn, &Rect, &Pt);
	    }
	}
      else if (UpdateRect != NULL)
	{
	  if (!W32kIntersectRect(&Rect2, &Rect, UpdateRect))
	    {
	      W32kReleaseWindowObject(Window);
	      if (hRgn != NULL && hRgn != (HRGN)1)
		{
		  W32kDeleteObject(hRgn);
		}
	      return(TRUE);
	    }
	  if (Window->UpdateRegion == NULL)
	    {
	      Window->UpdateRegion = W32kCreateRectRgnIndirect(&Rect2);
	    }
	  else
	    {
	      hRgn = W32kCreateRectRgnIndirect(&Rect2);
	    }
	}
      else
	{
	  if (Flags & RDW_FRAME)
	    {
	      hRgn = (HRGN)1;
	    }
	  else
	    {
	      W32kGetClientRect(hWnd, &Rect2);
	      hRgn = W32kCreateRectRgnIndirect(&Rect2);
	    }
	}
    }

  PaintUpdateRgns(Window, hRgn, Flags, TRUE);

  hRgn = PaintDoPaint(Window, hRgn == 1 ? 0 : hRgn, Flags, ExFlags);

  if (hRgn != (HANDLE)1 && hRgn != UpdateRgn)
    {
      W32kDeleteObject(hRgn);
    }
  W32kReleaseWindowObject(Window);
  return(TRUE);
}

BOOL STDCALL
PaintHaveToDelayNCPaint(PWINDOW_OBJECT Window, ULONG Flags)
{
  if (Flags & UNC_DELAY_NCPAINT)
    {
      return(TRUE);
    }

  if (Flags & UNC_IN_BEGINPAINT)
    {
      return(FALSE);
    }

  Window = Window->Parent;
  while (Window != NULL)
    {
      if (Window->Style & WS_CLIPCHILDREN && Window->UpdateRegion != NULL)
	{
	  return(TRUE);
	}
      Window = Window->Parent;
    }
  return(FALSE);
}

HWND STDCALL
PaintingFindWinToRepaint(HWND hWnd, PW32THREAD Thread)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT BaseWindow;
  PLIST_ENTRY current_entry;
  HWND hFoundWnd = NULL;
  NTSTATUS Status;

  if (hWnd == NULL)
    {
      ExAcquireFastMutex(&Thread->WindowListLock);
      current_entry = Thread->WindowListHead.Flink;
      while (current_entry != &Thread->WindowListHead)
	{
	  Window = CONTAINING_RECORD(current_entry, WINDOW_OBJECT,
				     ThreadListEntry);
	  if (Window->Style & WS_VISIBLE)
	    {
	      hFoundWnd = PaintingFindWinToRepaint(hWnd, Thread);
	      if (hFoundWnd != NULL)
		{
		  break;
		}
	    }
	  current_entry = current_entry->Flink;
	}
      ExReleaseFastMutex(&Thread->WindowListLock);
      return(hFoundWnd);
    }

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&BaseWindow);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  if (BaseWindow->Style & WS_VISIBLE && BaseWindow->UpdateRegion != NULL)
    {
      return(BaseWindow->Self);
    }

  ExAcquireFastMutex(&BaseWindow->ChildrenListLock);
  current_entry = Thread->WindowListHead.Flink;
  while (current_entry != &Thread->WindowListHead)
    {
      Window = CONTAINING_RECORD(current_entry, WINDOW_OBJECT,
				 ThreadListEntry);
      if (Window->Style & WS_VISIBLE)
	{
	  hFoundWnd = PaintingFindWinToRepaint(hWnd, Thread);
	  if (hFoundWnd != NULL)
	    {
	      break;
	    }
	}
      current_entry = current_entry->Flink;
    }
  ExReleaseFastMutex(&BaseWindow->ChildrenListLock);
  return(hFoundWnd);
}

HRGN STDCALL
PaintUpdateNCRegion(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags)
{
  HRGN hRgnRet;
  RECT ClientRect;
  HRGN hClip = NULL;

  /* Desktop has no parent. */
  if (Window->Parent == NULL)
    {
      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      if (Window->UpdateRegion > (HANDLE)1)
	{
	  hRgnRet = W32kCropRgn(hRgn, Window->UpdateRegion, NULL, NULL);
	}
      else
	{
	  hRgnRet = Window->UpdateRegion;
	}
      return(hRgnRet);
    }

  if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT && 
      !PaintHaveToDelayNCPaint(Window, Flags))
    {
      RECT UpdateRegionBox;
      RECT Rect;

      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      W32kGetClientRect(Window, &ClientRect);

      if (Window->UpdateRegion > (HRGN)1)
	{
	  W32kGetRgnBox(Window->UpdateRegion, &UpdateRegionBox);
	  W32kUnionRect(&Rect, &ClientRect, &UpdateRegionBox);
	  if (Rect.left != ClientRect.left || Rect.top != ClientRect.top ||
	      Rect.right != ClientRect.right || Rect.right != ClientRect.right)
	    {
	      hClip = Window->UpdateRegion;
	      Window->UpdateRegion = W32kCropRgn(hRgn, hClip,
						 &Rect, NULL);
	      if (Flags & UNC_REGION)
		{
		  hRgnRet = hClip;
		}
	    }

	  if (Flags & UNC_CHECK)
	    {
	      W32kGetBoxRgn(Window->UpdateRegion, &UpdateRegionBox);
	      if (W32kIsEmptyRect(&UpdateRegionBox))
		{
		  W32kDeleteObject(Window->UpdateRegion);
		  Window->UpdateRegion = NULL;
		  MsqDecPaintCountQueue(Window->MessageQueue);
		  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
		}
	    }

	  if (!hClip && Window->UpdateRegion && Flags & UNC_REGION)
	    {
	      hRgnRet = W32kCropRgn(hRgn, Window->UpdateRegion, NULL,
				    NULL);
	    }
	}
      else if (Window->UpdateRegion == (HRGN)1)
	{
	  if (Flags & UNC_UPDATE)
	    {
	      Window->UpdateRegion = W32kCreateRectRgnIndirect(&ClientRect);
	    }
	  if (Flags & UNC_REGION)
	    {
	      hRgnRet = (HANDLE)1;
	    }
	  Flags |= UNC_ENTIRE;
	}
    }
  else
    {
      if (Window->UpdateRegion > (HANDLE)1 && Flags & UNC_REGION)
	{
	  hRgnRet = W32kCropRgn(hRgn, Window->UpdateRegion, NULL, NULL);
	}
      else if (Window->UpdateRegion == (HANDLE)1 && Flags & UNC_UPDATE)
	{
	  W32kGetClientRect(Window, &ClientRect);
	  Window->UpdateRegion = W32kCreateRectRgnIndirect(&ClientRect);
	  if (Flags & UNC_REGION)
	    {
	      hRgnRet = (HANDLE)1;
	    }
	}
    }

  if (hClip == NULL && Flags & UNC_ENTIRE)
    {
      if (RtlCompareMemory(&Window->WindowRect, &Window->ClientRect, 
			   sizeof(RECT)) == sizeof(RECT))
	{
	  hClip = (HANDLE)1;
	}
      else
	{
	  hClip = 0;
	}
    }

  if (hClip != 0)
    {
      if (hClip == hRgnRet && hRgnRet > (HANDLE)1)
	{
	  hClip = W32kCreateRectRgn(0, 0, 0, 0);
	  W32kCombineRgn(hClip, hRgnRet, 0, RGN_COPY);
	}
      NtUserSendMessage(Window->Self, WM_NCPAINT, (WPARAM)hClip, 0);
      if (hClip > (HANDLE)1 && hClip != hRgn && hClip != hRgnRet)
	{
	  W32kDeleteObject(hClip);
	  /* FIXME: Need to check the window is still valid. */
	}
    }
  return(hRgnRet);
}

BOOL STDCALL
NtUserEndPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
  NtUserReleaseDC(hWnd, lPs->hdc);
  /* FIXME: Show claret. */
  return(TRUE);
}

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
  BOOL IsIcon;
  PWINDOW_OBJECT Window;
  HRGN UpdateRegion;
  RECT ClientRect;
  RECT ClipRect;
  NTSTATUS Status;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }

  IsIcon = Window->Style & WS_MINIMIZE && 
    NtUserGetClassLong(Window->Self, GCL_HICON);
  
  Window->Flags &= ~WINDOWOBJECT_NEED_BEGINPAINT;

  /* Send WM_NCPAINT */
  PaintUpdateNCRegion(Window, 0, UNC_UPDATE | UNC_IN_BEGINPAINT);

  /* FIXME: Check the window is still valid. */

  /* FIXME: Decrement the paint count if the update region is non-zero. */

  UpdateRegion = Window->UpdateRegion;
  Window->UpdateRegion = 0;

  /* FIXME: Hide claret. */

  if (NtUserGetClassLong(Window->Self, GCL_STYLE) & CS_PARENTDC)
    {
      if (UpdateRegion != NULL)
	{
	  W32kDeleteObject(UpdateRegion);
	}
      lPs->hdc = NtUserGetDCEx(hWnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
			       (IsIcon ? DCX_WINDOW : 0));
    }
  else
    {
      if (UpdateRegion != NULL)
	{
	  W32kOffsetRgn(UpdateRegion, 
			Window->WindowRect.left - 
			Window->ClientRect.left,
			Window->WindowRect.top -
			Window->ClientRect.top);
	}
      lPs->hdc = NtUserGetDCEx(hWnd, UpdateRegion, DCX_INTERSECTRGN |
			       DCX_WINDOWPAINT | DCX_USESTYLE |
			       (IsIcon ? DCX_WINDOW : 0));
    }

  /* FIXME: Check for DC creation failure. */

  W32kGetClientRect(hWnd, &ClientRect);
  W32kGetClipBox(lPs->hdc, &ClipRect);
  W32kLPtoDP(lPs->hdc, (LPPOINT)&ClipRect, 2);
  W32kIntersectRect(&lPs->rcPaint, &ClientRect, &ClipRect);
  W32kDPtoLP(lPs->hdc, (LPPOINT)&lPs->rcPaint, 2);

  if (Window->Flags & WINDOWOBJECT_NEED_ERASEBACKGRD)
    {
      BOOLEAN Result;
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
      Result = NtUserSendMessage(hWnd, 
				 IsIcon ? WM_ICONERASEBKGND : WM_ERASEBKGND,
				 (WPARAM)lPs->hdc,
				 0);
      lPs->fErase = !Result;
    }
  else
    {
      lPs->fErase = FALSE;
    }

  ObmDereferenceObject(Window);
  return(lPs->hdc);
}
