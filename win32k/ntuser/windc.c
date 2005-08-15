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

extern GDIDEVICE PrimarySurface;

BOOL FASTCALL
SetupDevMode(PDEVMODEW DevMode, ULONG DisplayNumber);
BOOL FASTCALL
FindDriverFileNames(PUNICODE_STRING DriverFileNames, ULONG DisplayNumber);


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

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd)
{
   return (DWORD)UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);
}


HDC STDCALL
NtUserGetDC(HWND hWnd)
{
   return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

PDCE FASTCALL
UserDceAllocDCE(HWND hWnd, DCE_TYPE Type)
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
  Dce->hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE, TRUE  );
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

      Parent = Window->ParentWnd;
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
      hRgnVisible = DceGetVisRgn(Parent->hSelf, DcxFlags, Window->hSelf, Flags);
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
      DesktopWindow = UserGetDesktopWindow();
      if (NULL != DesktopWindow)
      {
         hRgnVisible = UnsafeIntCreateRectRgnIndirect(&DesktopWindow->WindowRect);
      }
      else
      {
         hRgnVisible = NULL;
      }
   }
   else
   {
      hRgnVisible = DceGetVisRgn(Window->hSelf, Flags, 0, 0);
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
  PWINDOW_OBJECT Wnd=NULL;
  DECLARE_RETURN(HDC);
  
  DPRINT("Enter NtUserGetDCEx\n");
  UserEnterExclusive();

  if (hWnd)
  {
     Wnd = IntGetWindowObject(hWnd);
     if (!Wnd)
     {
        //FIXME: last error
        RETURN(NULL);
     }
  }

  RETURN(UserGetDCEx(Wnd, ClipRegion, Flags) );
  
CLEANUP:
  DPRINT("Leave NtUserGetDCEx, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}






HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
  PWINDOW_OBJECT Parent;
  ULONG DcxFlags;
  DCE* Dce;
  BOOL UpdateVisRgn = TRUE;
  BOOL UpdateClipOrigin = FALSE;


  if (NULL == Window)
    {
      Flags &= ~DCX_USESTYLE;
//      Window = NULL;
    }
//  else if (NULL == (Window = IntGetWindowObject(hWnd)))
//    {
//      return(0);
//    }

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

   /* no parent? desktop? */
  Parent = (Window ? Window->ParentWnd : NULL);

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
         else if (Dce->hwndCurrent == GetHwnd(Window)/*hWnd*/ &&
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
     Dce = UserDceAllocDCE(NULL, DCE_CACHE_DC);
   }
    }
  else
    {
      Dce = Window->Dce;
      if (NULL != Dce && Dce->hwndCurrent == GetHwnd(Window)/*hWnd*/)
        {
          UpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
        }
#if 1 /* FIXME */
      UpdateVisRgn = TRUE;
#endif
    }

  if (NULL == Dce)
    {
      return(NULL);
    }

  Dce->hwndCurrent = GetHwnd(Window);//hWnd;
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
  DECLARE_RETURN(INT);
  
  DPRINT("Enter NtUserReleaseDC\n");
  UserEnterExclusive();
  
  RETURN(UserReleaseDC(NULL, hDc));
  
CLEANUP:
  DPRINT("Leave NtUserReleaseDC, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
  
}




INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Wnd, HDC hDc)
{
  DCE *dce;
  INT nRet = 0;
  
  DCE_LockList();

  dce = FirstDce;

//  DPRINT("%p %p\n", hWnd, hDc);

  while (dce && (dce->hDC != hDc))
    {
      dce = dce->next;
    }

  if (dce && (dce->DCXFlags & DCX_DCEBUSY))
    {
      nRet = DceReleaseDC(dce);
    }

  DCE_UnlockList();

//  RETURN( nRet);
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
      if (pDCE->hwndCurrent == Window->hSelf)
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
                  DPRINT1("[%p] GetDC() without ReleaseDC()!\n", Window->hSelf);
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
          if (Window->hSelf == pDCE->hwndCurrent)
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
              pDCE = pDCE->next;
              continue;
            }
          if (Window == CurrentWindow || UserIsChildWindow(Window, CurrentWindow))
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

          if (Window->hSelf != pDCE->hwndCurrent)
            {
//              IntEngWindowChanged(CurrentWindow, WOC_RGN_CLIENT);
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
  DECLARE_RETURN(BOOL);
  
  DPRINT("Enter NtUserEnumDisplaySettings\n");
  UserEnterExclusive();

  /* Copy the devmode */
  Status = MmCopyFromCaller(&Size, &lpDevMode->dmSize, sizeof (Size));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    RETURN( FALSE);
  }
  Status = MmCopyFromCaller(&ExtraSize, &lpDevMode->dmDriverExtra, sizeof (ExtraSize));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    RETURN( FALSE);
  }
  pSafeDevMode = ExAllocatePool(PagedPool, Size + ExtraSize);
  if (pSafeDevMode == NULL)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    RETURN( FALSE);
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
      RETURN( FALSE);
    }
    pSafeDeviceName = &SafeDeviceName;
  }

  /* Call internal function */
  if (!UserEnumDisplaySettings(pSafeDeviceName, iModeNum, pSafeDevMode, dwFlags))
  {
    if (pSafeDeviceName != NULL)
      RtlFreeUnicodeString(pSafeDeviceName);
    ExFreePool(pSafeDevMode);
    RETURN( FALSE);
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
      RETURN(  FALSE);
    }
  }

  ExFreePool(pSafeDevMode);
  RETURN( TRUE);

CLEANUP:
  DPRINT("Leave NtUserEnumDisplaySettings, ret=%i\n", _ret_);
  UserLeave();
  END_CLEANUP;
}



#undef COPY_DEVMODE_VALUE_TO_CALLER


#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

/*! \brief Enumerate possible display settings for the given display...
 *
 * \todo Make thread safe!?
 * \todo Don't ignore pDeviceName
 * \todo Implement non-raw mode (only return settings valid for driver and monitor)
 */
BOOL FASTCALL
UserEnumDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN DWORD iModeNum,
  IN OUT LPDEVMODEW pDevMode,
  IN DWORD dwFlags)
{
  static DEVMODEW *CachedDevModes = NULL, *CachedDevModesEnd = NULL;
  static DWORD SizeOfCachedDevModes = 0;
  PDEVMODEW CachedMode = NULL;
  DEVMODEW DevMode;
  INT Size, OldSize;
  ULONG DisplayNumber = 0; /* only default display supported */

  DPRINT("DevMode->dmSize = %d\n", pDevMode->dmSize);
  DPRINT("DevMode->dmExtraSize = %d\n", pDevMode->dmDriverExtra);
  if (pDevMode->dmSize != SIZEOF_DEVMODEW_300 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_400 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_500)
  {
    SetLastWin32Error(STATUS_INVALID_PARAMETER);
    return FALSE;
  }

  if (iModeNum == ENUM_CURRENT_SETTINGS)
  {
    CachedMode = &PrimarySurface.DMW;
    ASSERT(CachedMode->dmSize > 0);
  }
  else if (iModeNum == ENUM_REGISTRY_SETTINGS)
  {
    RtlZeroMemory(&DevMode, sizeof (DevMode));
    DevMode.dmSize = sizeof (DevMode);
    DevMode.dmDriverExtra = 0;
    if (SetupDevMode(&DevMode, DisplayNumber))
      CachedMode = &DevMode;
    else
    {
      SetLastWin32Error(0); /* FIXME: use error code */
      return FALSE;
    }
    /* FIXME: Maybe look for the matching devmode supplied by the
     *        driver so we can provide driver private/extra data?
     */
  }
  else
  {
    if (iModeNum == 0 || CachedDevModes == NULL) /* query modes from drivers */
    {
      BOOL PrimarySurfaceCreated = FALSE;
      UNICODE_STRING DriverFileNames;
      LPWSTR CurrentName;
      DRVENABLEDATA DrvEnableData;

      /* Retrieve DDI driver names from registry */
      RtlInitUnicodeString(&DriverFileNames, NULL);
      if (!FindDriverFileNames(&DriverFileNames, DisplayNumber))
      {
        DPRINT1("FindDriverFileNames failed\n");
        return FALSE;
      }

      if (!HalQueryDisplayOwnership())
      {
        IntCreatePrimarySurface();
        PrimarySurfaceCreated = TRUE;
      }

      /*
       * DriverFileNames may be a list of drivers in REG_SZ_MULTI format,
       * scan all of them until a good one found.
       */
      CurrentName = DriverFileNames.Buffer;
      for (;CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR));
           CurrentName += wcslen(CurrentName) + 1)
      {
        INT i;
        PGD_ENABLEDRIVER GDEnableDriver;

        /* Get the DDI driver's entry point */
        GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
        if (NULL == GDEnableDriver)
        {
          DPRINT("FindDDIDriver failed for %S\n", CurrentName);
          continue;
        }

        /*  Call DDI driver's EnableDriver function  */
        RtlZeroMemory(&DrvEnableData, sizeof (DrvEnableData));

        if (!GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof (DrvEnableData), &DrvEnableData))
        {
          DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
          continue;
        }

        CachedDevModesEnd = CachedDevModes;

        /* find DrvGetModes function */
        for (i = 0; i < DrvEnableData.c; i++)
        {
          PDRVFN DrvFn = DrvEnableData.pdrvfn + i;
          PGD_GETMODES GetModes;
          INT SizeNeeded, SizeUsed;

          if (DrvFn->iFunc != INDEX_DrvGetModes)
            continue;

          GetModes = (PGD_GETMODES)DrvFn->pfn;

          /* make sure we have enough memory to hold the modes */
          SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject), 0, NULL);
          if (SizeNeeded <= 0)
          {
            DPRINT("DrvGetModes failed for %S\n", CurrentName);
            break;
          }

          SizeUsed = CachedDevModesEnd - CachedDevModes;
          if (SizeOfCachedDevModes - SizeUsed < SizeNeeded)
          {
            PVOID NewBuffer;

            SizeOfCachedDevModes += SizeNeeded;
            NewBuffer = ExAllocatePool(PagedPool, SizeOfCachedDevModes);
            if (NewBuffer == NULL)
            {
              /* clean up */
              ExFreePool(CachedDevModes);
              SizeOfCachedDevModes = 0;
              CachedDevModes = NULL;
              CachedDevModesEnd = NULL;
              if (PrimarySurfaceCreated)
              {
                IntDestroyPrimarySurface();
              }
              SetLastWin32Error(STATUS_NO_MEMORY);
              return FALSE;
            }
            if (CachedDevModes != NULL)
            {
              RtlCopyMemory(NewBuffer, CachedDevModes, SizeUsed);
              ExFreePool(CachedDevModes);
            }
            CachedDevModes = NewBuffer;
            CachedDevModesEnd = (DEVMODEW *)((PCHAR)NewBuffer + SizeUsed);
          }

          /* query modes */
          SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject),
                                SizeOfCachedDevModes - SizeUsed,
                                CachedDevModesEnd);
          if (SizeNeeded <= 0)
          {
            DPRINT("DrvGetModes failed for %S\n", CurrentName);
          }
          else
          {
            CachedDevModesEnd = (DEVMODEW *)((PCHAR)CachedDevModesEnd + SizeNeeded);
          }
          break;
        }
      }

      if (PrimarySurfaceCreated)
      {
        IntDestroyPrimarySurface();
      }

      RtlFreeUnicodeString(&DriverFileNames);
    }

    /* return cached info */
    CachedMode = CachedDevModes;
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
    while (iModeNum-- > 0 && CachedMode < CachedDevModesEnd)
    {
      assert(CachedMode->dmSize > 0);
      CachedMode = (DEVMODEW *)((PCHAR)CachedMode + CachedMode->dmSize + CachedMode->dmDriverExtra);
    }
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
  }

  ASSERT(CachedMode != NULL);

  Size = OldSize = pDevMode->dmSize;
  if (Size > CachedMode->dmSize)
    Size = CachedMode->dmSize;
  RtlCopyMemory(pDevMode, CachedMode, Size);
  RtlZeroMemory((PCHAR)pDevMode + Size, OldSize - Size);
  pDevMode->dmSize = OldSize;

  Size = OldSize = pDevMode->dmDriverExtra;
  if (Size > CachedMode->dmDriverExtra)
    Size = CachedMode->dmDriverExtra;
  RtlCopyMemory((PCHAR)pDevMode + pDevMode->dmSize,
                (PCHAR)CachedMode + CachedMode->dmSize, Size);
  RtlZeroMemory((PCHAR)pDevMode + pDevMode->dmSize + Size, OldSize - Size);
  pDevMode->dmDriverExtra = OldSize;

  return TRUE;
}



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
  DECLARE_RETURN(LONG);
  
  DPRINT("Enter NtUserChangeDisplaySettings\n");
  UserEnterExclusive();

  /* Check arguments */
#ifdef CDS_VIDEOPARAMETERS
  if (dwflags != CDS_VIDEOPARAMETERS && lParam != NULL)
#else
  if (lParam != NULL)
#endif
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    RETURN( DISP_CHANGE_BADPARAM);
  }
  if (hwnd != NULL)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    RETURN( DISP_CHANGE_BADPARAM);
  }

  /* Copy devmode */
  Status = MmCopyFromCaller(&DevMode.dmSize, &lpDevMode->dmSize, sizeof (DevMode.dmSize));
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    RETURN( DISP_CHANGE_BADPARAM);
  }
  DevMode.dmSize = min(sizeof (DevMode), DevMode.dmSize);
  Status = MmCopyFromCaller(&DevMode, lpDevMode, DevMode.dmSize);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    RETURN( DISP_CHANGE_BADPARAM);
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
      RETURN( DISP_CHANGE_BADPARAM);
    }
    pSafeDeviceName = &SafeDeviceName;
  }

  /* Call internal function */
  Ret = UserChangeDisplaySettings(pSafeDeviceName, &DevMode, dwflags, lParam);

  if (pSafeDeviceName != NULL)
    RtlFreeUnicodeString(pSafeDeviceName);

  RETURN( Ret);
  
CLEANUP:
  DPRINT("Leave NtUserChangeDisplaySettings, ret=%i\n", _ret_);
  UserLeave();
  END_CLEANUP;
}





LONG
FASTCALL
UserChangeDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN LPDEVMODEW DevMode,
  IN DWORD dwflags,
  IN PVOID lParam  OPTIONAL)
{
  BOOLEAN Global = FALSE;
  BOOLEAN NoReset = FALSE;
  BOOLEAN Reset = FALSE;
  BOOLEAN SetPrimary = FALSE;
  LONG Ret=0;
  NTSTATUS Status ;

  DPRINT1("display flag : %x\n",dwflags);

  if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
  {
    /* Check global, reset and noreset flags */
    if ((dwflags & CDS_GLOBAL) == CDS_GLOBAL)
      Global = TRUE;
    if ((dwflags & CDS_NORESET) == CDS_NORESET)
      NoReset = TRUE;
    dwflags &= ~(CDS_GLOBAL | CDS_NORESET);
  }
  if ((dwflags & CDS_RESET) == CDS_RESET)
    Reset = TRUE;
  if ((dwflags & CDS_SET_PRIMARY) == CDS_SET_PRIMARY)
    SetPrimary = TRUE;
  dwflags &= ~(CDS_RESET | CDS_SET_PRIMARY);

  if (Reset && NoReset)
    return DISP_CHANGE_BADFLAGS;

  if (dwflags == 0)
  {
   /* Dynamically change graphics mode */
   DPRINT1("flag 0 UNIMPLEMENT \n");
   return DISP_CHANGE_FAILED;
  }

  if ((dwflags & CDS_TEST) == CDS_TEST)
  {
   /* Test reslution */
   dwflags &= ~CDS_TEST;
   DPRINT1("flag CDS_TEST UNIMPLEMENT");
   Ret = DISP_CHANGE_FAILED;
  }
  
  if ((dwflags & CDS_FULLSCREEN) == CDS_FULLSCREEN)
  {
   DEVMODE lpDevMode;
   /* Full Screen */
   dwflags &= ~CDS_FULLSCREEN;
   DPRINT1("flag CDS_FULLSCREEN partially implemented");
   Ret = DISP_CHANGE_FAILED;

   lpDevMode.dmBitsPerPel =0;
   lpDevMode.dmPelsWidth  =0;
   lpDevMode.dmPelsHeight =0;
   lpDevMode.dmDriverExtra =0;

   lpDevMode.dmSize = sizeof(DEVMODE);
   Status = UserEnumDisplaySettings(pDeviceName,  ENUM_CURRENT_SETTINGS, &lpDevMode, 0);
   if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;

   DPRINT1("Req Mode     : %d x %d x %d\n", DevMode->dmPelsWidth,DevMode->dmPelsHeight,DevMode->dmBitsPerPel);
   DPRINT1("Current Mode : %d x %d x %d\n", lpDevMode.dmPelsWidth,lpDevMode.dmPelsHeight, lpDevMode.dmBitsPerPel);


   if ((lpDevMode.dmBitsPerPel == DevMode->dmBitsPerPel) &&
       (lpDevMode.dmPelsWidth  == DevMode->dmPelsWidth) &&
       (lpDevMode.dmPelsHeight == DevMode->dmPelsHeight))
      Ret = DISP_CHANGE_SUCCESSFUL; 
  }

  if ((dwflags & CDS_VIDEOPARAMETERS) == CDS_VIDEOPARAMETERS)
  {  
    dwflags &= ~CDS_VIDEOPARAMETERS;
    if (lParam == NULL) Ret=DISP_CHANGE_BADPARAM;
   else
   {
      DPRINT1("flag CDS_VIDEOPARAMETERS UNIMPLEMENT");
      Ret = DISP_CHANGE_FAILED;
   }

  }    

  if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
  {  
  
      UNICODE_STRING ObjectName;
      UNICODE_STRING KernelModeName;
      WCHAR KernelModeNameBuffer[256];
      UNICODE_STRING RegistryKey;
      WCHAR RegistryKeyBuffer[512];
      PDEVICE_OBJECT DeviceObject;     
      ULONG LastSlash;
      OBJECT_ATTRIBUTES ObjectAttributes;
      HANDLE DevInstRegKey;
      ULONG NewValue;
      

      DPRINT1("set CDS_UPDATEREGISTRY \n");
      
      dwflags &= ~CDS_UPDATEREGISTRY;  

      /* Get device name (pDeviceName is "\.\xxx") */
      for (LastSlash = pDeviceName->Length / sizeof(WCHAR); LastSlash > 0; LastSlash--)
      {
         if (pDeviceName->Buffer[LastSlash - 1] == L'\\')
            break;
      }
      
      if (LastSlash == 0) return DISP_CHANGE_RESTART;
      ObjectName = *pDeviceName;
      ObjectName.Length -= LastSlash * sizeof(WCHAR);
      ObjectName.MaximumLength -= LastSlash * sizeof(WCHAR);
      ObjectName.Buffer += LastSlash;

      KernelModeName.Length = 0;
      KernelModeName.MaximumLength = sizeof(KernelModeNameBuffer);
      KernelModeName.Buffer = KernelModeNameBuffer;

      /* Open \??\xxx (ex: "\??\DISPLAY1") */
      Status = RtlAppendUnicodeToString(&KernelModeName, L"\\??\\");
      
      if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
      Status = RtlAppendUnicodeStringToString(&KernelModeName, &ObjectName);
      
      if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
      Status = ObReferenceObjectByName(
         &KernelModeName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         0,
         IoDeviceObjectType,
         KernelMode,
         NULL,
         (PVOID*)&DeviceObject);

      if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
      /* Get associated driver name (ex: "VBE") */
      for (LastSlash = DeviceObject->DriverObject->DriverName.Length / sizeof(WCHAR); LastSlash > 0; LastSlash--)
      {
         if (DeviceObject->DriverObject->DriverName.Buffer[LastSlash - 1] == L'\\')
            break;
      }

      if (LastSlash == 0) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
      ObjectName = DeviceObject->DriverObject->DriverName;
      ObjectName.Length -= LastSlash * sizeof(WCHAR);
      ObjectName.MaximumLength -= LastSlash * sizeof(WCHAR);
      ObjectName.Buffer += LastSlash;

      RegistryKey.Length = 0;
      RegistryKey.MaximumLength = sizeof(RegistryKeyBuffer);
      RegistryKey.Buffer = RegistryKeyBuffer;

      /* Open registry key */
      Status = RtlAppendUnicodeToString(&RegistryKey,
         L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\");
      
      if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
      Status = RtlAppendUnicodeStringToString(&RegistryKey, &ObjectName);

      if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
      Status = RtlAppendUnicodeToString(&RegistryKey,
         L"\\Device0");

      if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }

      InitializeObjectAttributes(&ObjectAttributes, &RegistryKey,
         OBJ_CASE_INSENSITIVE, NULL, NULL);
      Status = ZwOpenKey(&DevInstRegKey, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes);
      ObDereferenceObject(DeviceObject);
      if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;

      /* Update needed fields */
      if (NT_SUCCESS(Status) && DevMode->dmFields & DM_BITSPERPEL)
      {
         RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.BitsPerPel");
         NewValue = DevMode->dmBitsPerPel;
         Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));       
      }

      if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSWIDTH)
      {
         RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.XResolution");
         NewValue = DevMode->dmPelsWidth;       
         Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));       
      }

      if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSHEIGHT)
      {
         RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.YResolution");
         NewValue = DevMode->dmPelsHeight;
         Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));       
      }

      ZwClose(DevInstRegKey);
      if (NT_SUCCESS(Status))
         Ret = DISP_CHANGE_RESTART;
      else
         /* return DISP_CHANGE_NOTUPDATED when we can save to reg only vaild for NT */ 
         Ret = DISP_CHANGE_NOTUPDATED;
      
    }
 
 if (dwflags != 0)  
    Ret = DISP_CHANGE_BADFLAGS;

  return Ret;
}
/* EOF */
