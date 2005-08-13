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
/* $Id$
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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define DCX_USESTYLE 0x10000

/* GLOBALS *******************************************************************/

/* NOTE - I think we should store this per window station (including gdi objects) */

static FAST_MUTEX DceListLock;
static PDCE FirstDce = NULL;
static HDC defaultDCstate;

#if 0

#define DCE_LockList() \
  ExAcquireFastMutex(&DceListLock)
#define DCE_UnlockList() \
  ExReleaseFastMutex(&DceListLock)

#else
#define DCE_LockList()
#define DCE_UnlockList()
#endif

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

VOID FASTCALL
DceInit(VOID)
{
  ExInitializeFastMutex(&DceListLock);
}

HRGN STDCALL
DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  PWINDOW_OBJECT Window;
  HRGN VisRgn;

  Window = IntGetWindowObject(hWnd);

  if (NULL == Window)
    {
      return NULL;
    }

  VisRgn = VIS_ComputeVisibleRegion(Window,
                                    0 == (Flags & DCX_WINDOW),
                                    0 != (Flags & DCX_CLIPCHILDREN),
                                    0 != (Flags & DCX_CLIPSIBLINGS));

  IntReleaseWindowObject(Window);

  return VisRgn;
}

/*
 * NtUserGetWindowDC
 *
 * The NtUserGetWindowDC function retrieves the device context (DC) for the
 * entire window, including title bar, menus, and scroll bars. A window device
 * context permits painting anywhere in a window, because the origin of the
 * device context is the upper-left corner of the window instead of the client
 * area.
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserGetWindowDC(HWND hWnd)
{
   return (DWORD)NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
   return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

PDCE FASTCALL
DceAllocDCE(HWND hWnd, DCE_TYPE Type)
{
  HDCE DceHandle;
  DCE* Dce;
  UNICODE_STRING DriverName;

  DceHandle = DCEOBJ_AllocDCE();
  if(!DceHandle)
    return NULL;

  RtlInitUnicodeString(&DriverName, L"DISPLAY");

  Dce = DCEOBJ_LockDCE(DceHandle);
  /* No real locking, just get the pointer */
  DCEOBJ_UnlockDCE(Dce);
  Dce->Self = DceHandle;
  Dce->hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
  if (NULL == defaultDCstate)
    {
      defaultDCstate = NtGdiGetDCState(Dce->hDC);
      GDIOBJ_SetOwnership(defaultDCstate, NULL);
    }
  GDIOBJ_SetOwnership(Dce->Self, NULL);
  DC_SetOwnership(Dce->hDC, NULL);
  Dce->hwndCurrent = hWnd;
  Dce->hClipRgn = NULL;
  DCE_LockList();
  Dce->next = FirstDce;
  FirstDce = Dce;
  DCE_UnlockList();

  if (Type != DCE_CACHE_DC)
    {
      Dce->DCXFlags = DCX_DCEBUSY;
      if (hWnd != NULL)
	{
	  PWINDOW_OBJECT WindowObject;

	  WindowObject = IntGetWindowObject(hWnd);
	  if (WindowObject->Style & WS_CLIPCHILDREN)
	    {
	      Dce->DCXFlags |= DCX_CLIPCHILDREN;
	    }
	  if (WindowObject->Style & WS_CLIPSIBLINGS)
	    {
	      Dce->DCXFlags |= DCX_CLIPSIBLINGS;
	    }
	  IntReleaseWindowObject(WindowObject);
	}
    }
  else
    {
      Dce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
    }

  return(Dce);
}

VOID STATIC STDCALL
DceSetDrawable(PWINDOW_OBJECT WindowObject, HDC hDC, ULONG Flags,
	       BOOL SetClipOrigin)
{
  DC *dc = DC_LockDc(hDC);
  if(!dc)
    return;

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
    }
  DC_UnlockDc(dc);
}


STATIC VOID FASTCALL
DceDeleteClipRgn(DCE* Dce)
{
  Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

  if (Dce->DCXFlags & DCX_KEEPCLIPRGN )
    {
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
    }
  else if (Dce->hClipRgn != NULL)
    {
      GDIOBJ_SetOwnership(Dce->hClipRgn, PsGetCurrentProcess());
      NtGdiDeleteObject(Dce->hClipRgn);
    }

  Dce->hClipRgn = NULL;

  /* make it dirty so that the vis rgn gets recomputed next time */
  Dce->DCXFlags |= DCX_DCEDIRTY;
}

STATIC INT FASTCALL
DceReleaseDC(DCE* dce)
{
  if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_DCEEMPTY | DCX_DCEBUSY)))
    {
      return 0;
    }

  /* restore previous visible region */

  if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
      (dce->DCXFlags & (DCX_CACHE | DCX_WINDOWPAINT)) )
    {
      DceDeleteClipRgn(dce);
    }

  if (dce->DCXFlags & DCX_CACHE)
    {
      /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
      NtGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);
      NtGdiSetDCState(dce->hDC, defaultDCstate);
      dce->DCXFlags &= ~DCX_DCEBUSY;
      if (dce->DCXFlags & DCX_DCEDIRTY)
	{
	  /* don't keep around invalidated entries
	   * because SetDCState() disables hVisRgn updates
	   * by removing dirty bit. */
	  dce->hwndCurrent = 0;
	  dce->DCXFlags &= DCX_CACHE;
	  dce->DCXFlags |= DCX_DCEEMPTY;
	}
    }

  return 1;
}

STATIC VOID FASTCALL
DceUpdateVisRgn(DCE *Dce, PWINDOW_OBJECT Window, ULONG Flags)
{
   HANDLE hRgnVisible = NULL;
   ULONG DcxFlags;
   PWINDOW_OBJECT DesktopWindow;

   if (Flags & DCX_PARENTCLIP)
   {
      PWINDOW_OBJECT Parent;

      Parent = IntGetParentObject(Window);
      if(!Parent)
      {
        hRgnVisible = NULL;
        goto noparent;
      }

      if (Parent->Style & WS_CLIPSIBLINGS)
      {
         DcxFlags = DCX_CLIPSIBLINGS |
            (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
      }
      else
      {
         DcxFlags = Flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
      }
      hRgnVisible = DceGetVisRgn(Parent->Self, DcxFlags, Window->Self, Flags);
      if (hRgnVisible == NULL)
      {
         hRgnVisible = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
      else
      {
         if (0 == (Flags & DCX_WINDOW))
         {
            NtGdiOffsetRgn(
               hRgnVisible,
               Parent->ClientRect.left - Window->ClientRect.left,
               Parent->ClientRect.top - Window->ClientRect.top);
         }
         else
         {
            NtGdiOffsetRgn(
               hRgnVisible,
               Parent->WindowRect.left - Window->WindowRect.left,
               Parent->WindowRect.top - Window->WindowRect.top);
         }
      }
   }
   else if (Window == NULL)
   {
      DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
      if (NULL != DesktopWindow)
      {
         hRgnVisible = UnsafeIntCreateRectRgnIndirect(&DesktopWindow->WindowRect);
         IntReleaseWindowObject(DesktopWindow);
      }
      else
      {
         hRgnVisible = NULL;
      }
   }
   else
   {
      hRgnVisible = DceGetVisRgn(Window->Self, Flags, 0, 0);
   }

noparent:
   if (Flags & DCX_INTERSECTRGN)
   {
      if(Dce->hClipRgn != NULL)
      {
         NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_AND);
      }
      else
      {
         if(hRgnVisible != NULL)
         {
            NtGdiDeleteObject(hRgnVisible);
         }
         hRgnVisible = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
   }

   if (Flags & DCX_EXCLUDERGN && Dce->hClipRgn != NULL)
   {
      NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_DIFF);
   }

   Dce->DCXFlags &= ~DCX_DCEDIRTY;
   NtGdiSelectVisRgn(Dce->hDC, hRgnVisible);

   if (Window != NULL)
   {
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
   }

   if (hRgnVisible != NULL)
   {
      NtGdiDeleteObject(hRgnVisible);
   }
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE ClipRegion, ULONG Flags)
{
  PWINDOW_OBJECT Window, Parent;
  ULONG DcxFlags;
  DCE* Dce;
  BOOL UpdateVisRgn = TRUE;
  BOOL UpdateClipOrigin = FALSE;

  if (NULL == hWnd)
    {
      Flags &= ~DCX_USESTYLE;
      Window = NULL;
    }
  else if (NULL == (Window = IntGetWindowObject(hWnd)))
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
	  if (Window->Class->style & CS_PARENTDC)
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

  Parent = (Window ? IntGetParentObject(Window) : NULL);

  if (NULL == Window || !(Window->Style & WS_CHILD) || NULL == Parent)
    {
      Flags &= ~DCX_PARENTCLIP;
    }
  else if (Flags & DCX_PARENTCLIP)
    {
      Flags |= DCX_CACHE;
      if ((Window->Style & WS_VISIBLE) &&
          (Parent->Style & WS_VISIBLE))
        {
          Flags &= ~DCX_CLIPCHILDREN;
          if (Parent->Style & WS_CLIPSIBLINGS)
            {
              Flags |= DCX_CLIPSIBLINGS;
            }
        }
    }

  if(Parent)
    IntReleaseWindowObject(Parent);

  DcxFlags = Flags & DCX_CACHECOMPAREMASK;

  if (Flags & DCX_CACHE)
    {
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;

      DCE_LockList();

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
#if 0 /* FIXME */
		  UpdateVisRgn = FALSE;
#endif
		  UpdateClipOrigin = TRUE;
		  break;
		}
	    }
	}

      DCE_UnlockList();

      if (Dce == NULL)
	{
	  Dce = (DceEmpty == NULL) ? DceUnused : DceEmpty;
	}

      if (Dce == NULL)
	{
	  Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
	}
    }
  else
    {
      Dce = Window->Dce;
      if (NULL != Dce && Dce->hwndCurrent == hWnd)
        {
          UpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
        }
#if 1 /* FIXME */
      UpdateVisRgn = TRUE;
#endif
    }

  if (NULL == Dce)
    {
      if(NULL != Window)
        IntReleaseWindowObject(Window);
      return(NULL);
    }

  Dce->hwndCurrent = hWnd;
  Dce->DCXFlags = DcxFlags | (Flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;

  if (0 == (Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) && NULL != ClipRegion)
    {
      NtGdiDeleteObject(ClipRegion);
      ClipRegion = NULL;
    }

  if (NULL != Dce->hClipRgn)
    {
      DceDeleteClipRgn(Dce);
      Dce->hClipRgn = NULL;
    }

  if (0 != (Flags & DCX_INTERSECTUPDATE) && NULL == ClipRegion)
    {
      Dce->hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      if (Dce->hClipRgn && Window->UpdateRegion)
        {
          GDIOBJ_SetOwnership(Dce->hClipRgn, NULL);
          NtGdiCombineRgn(Dce->hClipRgn, Window->UpdateRegion, NULL, RGN_COPY);
          if(Window->WindowRegion && !(Window->Style & WS_MINIMIZE))
            NtGdiCombineRgn(Dce->hClipRgn, Dce->hClipRgn, Window->WindowRegion, RGN_AND);
          if (!(Flags & DCX_WINDOW))
            {
              NtGdiOffsetRgn(Dce->hClipRgn,
                Window->WindowRect.left - Window->ClientRect.left,
                Window->WindowRect.top - Window->ClientRect.top);
            }
        }
      Flags |= DCX_INTERSECTRGN;
    }

  if (ClipRegion == (HRGN) 1)
    {
      if (!(Flags & DCX_WINDOW))
        {
          Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
          GDIOBJ_SetOwnership(Dce->hClipRgn, NULL);
          if(!Window->WindowRegion || (Window->Style & WS_MINIMIZE))
          {
            NtGdiOffsetRgn(Dce->hClipRgn, -Window->ClientRect.left, -Window->ClientRect.top);
          }
          else
          {
            NtGdiOffsetRgn(Dce->hClipRgn, -Window->WindowRect.left, -Window->WindowRect.top);
            NtGdiCombineRgn(Dce->hClipRgn, Dce->hClipRgn, Window->WindowRegion, RGN_AND);
            NtGdiOffsetRgn(Dce->hClipRgn, -(Window->ClientRect.left - Window->WindowRect.left),
                                          -(Window->ClientRect.top - Window->WindowRect.top));
          }
        }
      else
        {
          Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
          GDIOBJ_SetOwnership(Dce->hClipRgn, NULL);
          NtGdiOffsetRgn(Dce->hClipRgn, -Window->WindowRect.left,
             -Window->WindowRect.top);
          if(Window->WindowRegion && !(Window->Style & WS_MINIMIZE))
            NtGdiCombineRgn(Dce->hClipRgn, Dce->hClipRgn, Window->WindowRegion, RGN_AND);
        }
    }
  else if (NULL != ClipRegion)
    {
      Dce->hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      if (Dce->hClipRgn)
        {
          GDIOBJ_SetOwnership(Dce->hClipRgn, NULL);
          if(!Window->WindowRegion || (Window->Style & WS_MINIMIZE))
            NtGdiCombineRgn(Dce->hClipRgn, ClipRegion, NULL, RGN_COPY);
          else
            NtGdiCombineRgn(Dce->hClipRgn, ClipRegion, Window->WindowRegion, RGN_AND);
        }
      NtGdiDeleteObject(ClipRegion);
    }

  DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

//  if (UpdateVisRgn)
    {
      DceUpdateVisRgn(Dce, Window, Flags);
    }

  if (NULL != Window)
    {
      IntReleaseWindowObject(Window);
    }

  return(Dce->hDC);
}

BOOL INTERNAL_CALL
DCE_Cleanup(PVOID ObjectBody)
{
  PDCE PrevInList;
  PDCE pDce = (PDCE)ObjectBody;

  DCE_LockList();

  if (pDce == FirstDce)
    {
      FirstDce = pDce->next;
      PrevInList = pDce;
    }
  else
    {
      for (PrevInList = FirstDce; NULL != PrevInList; PrevInList = PrevInList->next)
	{
	  if (pDce == PrevInList->next)
	    {
	      PrevInList->next = pDce->next;
	      break;
	    }
	}
      assert(NULL != PrevInList);
    }

  DCE_UnlockList();

  return NULL != PrevInList;
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;

  DCE_LockList();
  for (Dce = FirstDce; Dce != NULL; Dce = Dce->next)
  {
    if(Dce->hDC == hDc)
    {
      DCE_UnlockList();
      return Dce->hwndCurrent;
    }
  }
  DCE_UnlockList();
  return 0;
}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  DCE *dce;
  INT nRet = 0;

  DCE_LockList();

  dce = FirstDce;

  DPRINT("%p %p\n", hWnd, hDc);

  while (dce && (dce->hDC != hDc))
    {
      dce = dce->next;
    }

  if (dce && (dce->DCXFlags & DCX_DCEBUSY))
    {
      nRet = DceReleaseDC(dce);
    }

  DCE_UnlockList();

  return nRet;
}

/***********************************************************************
 *           DceFreeDCE
 */
PDCE FASTCALL
DceFreeDCE(PDCE dce, BOOLEAN Force)
{
  DCE *ret;

  if (NULL == dce)
    {
      return NULL;
    }

  ret = dce->next;

#if 0 /* FIXME */
  SetDCHook(dce->hDC, NULL, 0L);
#endif

  if(Force && !GDIOBJ_OwnedByCurrentProcess(dce->hDC))
  {
    GDIOBJ_SetOwnership(dce->Self, PsGetCurrentProcess());
    DC_SetOwnership(dce->hDC, PsGetCurrentProcess());
  }

  NtGdiDeleteDC(dce->hDC);
  if (dce->hClipRgn && ! (dce->DCXFlags & DCX_KEEPCLIPRGN))
    {
      GDIOBJ_SetOwnership(dce->hClipRgn, PsGetCurrentProcess());
      NtGdiDeleteObject(dce->hClipRgn);
    }

  DCEOBJ_FreeDCE(dce->Self);

  return ret;
}


/***********************************************************************
 *           DceFreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void FASTCALL
DceFreeWindowDCE(PWINDOW_OBJECT Window)
{
  DCE *pDCE;

  DCE_LockList();

  pDCE = FirstDce;
  while (pDCE)
    {
      if (pDCE->hwndCurrent == Window->Self)
        {
          if (pDCE == Window->Dce) /* owned or Class DCE*/
            {
              if (Window->Class->style & CS_OWNDC) /* owned DCE*/
                {
                  pDCE = DceFreeDCE(pDCE, FALSE);
                  Window->Dce = NULL;
                  continue;
                }
              else if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))	/* Class DCE*/
		{
                  DceDeleteClipRgn(pDCE);
                  pDCE->hwndCurrent = 0;
                }
            }
          else
            {
              if (pDCE->DCXFlags & DCX_DCEBUSY) /* shared cache DCE */
                {
                  /* FIXME: AFAICS we are doing the right thing here so
                   * this should be a DPRINT. But this is best left as an ERR
                   * because the 'application error' is likely to come from
                   * another part of Wine (i.e. it's our fault after all).
                   * We should change this to DPRINT when ReactOS is more stable
                   * (for 1.0?).
                   */
                  DPRINT1("[%p] GetDC() without ReleaseDC()!\n", Window->Self);
                  DceReleaseDC(pDCE);
                }

              pDCE->DCXFlags &= DCX_CACHE;
              pDCE->DCXFlags |= DCX_DCEEMPTY;
              pDCE->hwndCurrent = 0;
            }
        }
      pDCE = pDCE->next;
    }
    DCE_UnlockList();
}

void FASTCALL
DceEmptyCache()
{
  DCE_LockList();
  while (FirstDce != NULL)
    {
      DceFreeDCE(FirstDce, TRUE);
    }
  DCE_UnlockList();
}

VOID FASTCALL
DceResetActiveDCEs(PWINDOW_OBJECT Window)
{
  DCE *pDCE;
  PDC dc;
  PWINDOW_OBJECT CurrentWindow;
  INT DeltaX;
  INT DeltaY;

  if (NULL == Window)
    {
      return;
    }

  DCE_LockList();

  pDCE = FirstDce;
  while (pDCE)
    {
      if (0 == (pDCE->DCXFlags & DCX_DCEEMPTY))
        {
          if (Window->Self == pDCE->hwndCurrent)
            {
              CurrentWindow = Window;
            }
          else
            {
              CurrentWindow = IntGetWindowObject(pDCE->hwndCurrent);
              if (NULL == CurrentWindow)
                {
                  pDCE = pDCE->next;
                  continue;
                }
            }

          dc = DC_LockDc(pDCE->hDC);
          if (dc == NULL)
            {
              if (Window->Self != pDCE->hwndCurrent)
                {
                  IntReleaseWindowObject(CurrentWindow);
                }
              pDCE = pDCE->next;
              continue;
            }
          if (Window == CurrentWindow || IntIsChildWindow(Window->Self, CurrentWindow->Self))
            {
              if (pDCE->DCXFlags & DCX_WINDOW)
                {
                  DeltaX = CurrentWindow->WindowRect.left - dc->w.DCOrgX;
                  DeltaY = CurrentWindow->WindowRect.top - dc->w.DCOrgY;
                  dc->w.DCOrgX = CurrentWindow->WindowRect.left;
                  dc->w.DCOrgY = CurrentWindow->WindowRect.top;
                }
              else
                {
                  DeltaX = CurrentWindow->ClientRect.left - dc->w.DCOrgX;
                  DeltaY = CurrentWindow->ClientRect.top - dc->w.DCOrgY;
                  dc->w.DCOrgX = CurrentWindow->ClientRect.left;
                  dc->w.DCOrgY = CurrentWindow->ClientRect.top;
                }
              if (NULL != dc->w.hClipRgn)
                {
                  NtGdiOffsetRgn(dc->w.hClipRgn, DeltaX, DeltaY);
                }
              if (NULL != pDCE->hClipRgn)
                {
                  NtGdiOffsetRgn(pDCE->hClipRgn, DeltaX, DeltaY);
                }
            }
          DC_UnlockDc(dc);

          DceUpdateVisRgn(pDCE, CurrentWindow, pDCE->DCXFlags);

          if (Window->Self != pDCE->hwndCurrent)
            {
//              IntEngWindowChanged(CurrentWindow, WOC_RGN_CLIENT);
              IntReleaseWindowObject(CurrentWindow);
            }
        }

      pDCE = pDCE->next;
    }

  DCE_UnlockList();
}


#define COPY_DEVMODE_VALUE_TO_CALLER(dst, src, member) \
    Status = MmCopyToCaller(&(dst)->member, &(src)->member, sizeof ((src)->member)); \
    if (!NT_SUCCESS(Status)) \
    { \
      SetLastNtError(Status); \
      ExFreePool(src); \
      return FALSE; \
    }

BOOL
STDCALL
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags )
{
  NTSTATUS Status;
  LPDEVMODEW pSafeDevMode;
  PUNICODE_STRING pSafeDeviceName = NULL;
  UNICODE_STRING SafeDeviceName;
  USHORT Size = 0, ExtraSize = 0;

  /* Copy the devmode */
  Status = MmCopyFromCaller(&Size, &lpDevMode->dmSize, sizeof (Size));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  Status = MmCopyFromCaller(&ExtraSize, &lpDevMode->dmDriverExtra, sizeof (ExtraSize));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  pSafeDevMode = ExAllocatePool(PagedPool, Size + ExtraSize);
  if (pSafeDevMode == NULL)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  pSafeDevMode->dmSize = Size;
  pSafeDevMode->dmDriverExtra = ExtraSize;

  /* Copy the device name */
  if (lpszDeviceName != NULL)
  {
    Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
    if (!NT_SUCCESS(Status))
    {
      ExFreePool(pSafeDevMode);
      SetLastNtError(Status);
      return FALSE;
    }
    pSafeDeviceName = &SafeDeviceName;
  }

  /* Call internal function */
  if (!IntEnumDisplaySettings(pSafeDeviceName, iModeNum, pSafeDevMode, dwFlags))
  {
    if (pSafeDeviceName != NULL)
      RtlFreeUnicodeString(pSafeDeviceName);
    ExFreePool(pSafeDevMode);
    return FALSE;
  }
  if (pSafeDeviceName != NULL)
    RtlFreeUnicodeString(pSafeDeviceName);

  /* Copy some information back */
  COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmPelsWidth);
  COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmPelsHeight);
  COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmBitsPerPel);
  COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmDisplayFrequency);
  COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmDisplayFlags);

  /* output private/extra driver data */
  if (ExtraSize > 0)
  {
    Status = MmCopyToCaller((PCHAR)lpDevMode + Size, (PCHAR)pSafeDevMode + Size, ExtraSize);
    if (!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      ExFreePool(pSafeDevMode);
      return FALSE;
    }
  }

  ExFreePool(pSafeDevMode);
  return TRUE;
}

#undef COPY_DEVMODE_VALUE_TO_CALLER


LONG
STDCALL
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  NTSTATUS Status;
  DEVMODEW DevMode;
  PUNICODE_STRING pSafeDeviceName = NULL;
  UNICODE_STRING SafeDeviceName;
  LONG Ret;

  /* Check arguments */
#ifdef CDS_VIDEOPARAMETERS
  if (dwflags != CDS_VIDEOPARAMETERS && lParam != NULL)
#else
  if (lParam != NULL)
#endif
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return DISP_CHANGE_BADPARAM;
  }
  if (hwnd != NULL)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return DISP_CHANGE_BADPARAM;
  }

  /* Copy devmode */
  Status = MmCopyFromCaller(&DevMode.dmSize, &lpDevMode->dmSize, sizeof (DevMode.dmSize));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return DISP_CHANGE_BADPARAM;
  }
  DevMode.dmSize = min(sizeof (DevMode), DevMode.dmSize);
  Status = MmCopyFromCaller(&DevMode, lpDevMode, DevMode.dmSize);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return DISP_CHANGE_BADPARAM;
  }
  if (DevMode.dmDriverExtra > 0)
  {
    DbgPrint("(%s:%i) WIN32K: %s lpDevMode->dmDriverExtra is IGNORED!\n", __FILE__, __LINE__, __FUNCTION__);
    DevMode.dmDriverExtra = 0;
  }

  /* Copy the device name */
  if (lpszDeviceName != NULL)
  {
    Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
    if (!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return DISP_CHANGE_BADPARAM;
    }
    pSafeDeviceName = &SafeDeviceName;
  }

  /* Call internal function */
  Ret = IntChangeDisplaySettings(pSafeDeviceName, &DevMode, dwflags, lParam);

  if (pSafeDeviceName != NULL)
    RtlFreeUnicodeString(pSafeDeviceName);

  return Ret;
}


/* EOF */
