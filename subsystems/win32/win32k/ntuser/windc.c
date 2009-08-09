/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/windc.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 ReactOS
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* NOTE - I think we should store this per window station (including gdi objects) */

static PDCE FirstDce = NULL;
//static INT DCECount = 0; // Count of DCE in system.

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

//
// This should be moved to dc.c or dcutil.c.
//
HDC FASTCALL
DceCreateDisplayDC(VOID)
{
  HDC hDC;
  UNICODE_STRING DriverName;
  RtlInitUnicodeString(&DriverName, L"DISPLAY");
  hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
//
// If NULL, first time through! Build the default window dc!
//
  if (hDC && !defaultDCstate) // Ultra HAX! Dedicated to GvG!
  { // This is a cheesy way to do this.
      PDC dc = DC_LockDc ( hDC );
      defaultDCstate = ExAllocatePoolWithTag(PagedPool, sizeof(DC), TAG_DC);
      RtlZeroMemory(defaultDCstate, sizeof(DC));
      IntGdiCopyToSaveState(dc, defaultDCstate);
      DC_UnlockDc( dc );
  }
  return hDC;
}

static
HRGN FASTCALL
DceGetVisRgn(PWINDOW_OBJECT Window, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  HRGN VisRgn;

  VisRgn = VIS_ComputeVisibleRegion( Window,
                  0 == (Flags & DCX_WINDOW),
            0 != (Flags & DCX_CLIPCHILDREN),
            0 != (Flags & DCX_CLIPSIBLINGS));

  if (VisRgn == NULL)
      VisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

  return VisRgn;
}


PDCE FASTCALL
DceAllocDCE(PWINDOW_OBJECT Window OPTIONAL, DCE_TYPE Type)
{
  PDCE pDce;
  PWINDOW Wnd = NULL;

  if (Window) Wnd = Window->Wnd;

  pDce = ExAllocatePoolWithTag(PagedPool, sizeof(DCE), TAG_PDCE);
  if(!pDce)
        return NULL;

  pDce->hDC = DceCreateDisplayDC();
  if (!pDce->hDC)
  {
      ExFreePoolWithTag(pDce, TAG_PDCE);
      return NULL;
  }

  pDce->hwndCurrent = (Window ? Window->hSelf : NULL);
  pDce->hClipRgn = NULL;
  pDce->pProcess = NULL;

  KeEnterCriticalRegion();
  if (FirstDce == NULL)
  {
     FirstDce = pDce;
     InitializeListHead(&FirstDce->List);
  }
  else
     InsertTailList(&FirstDce->List, &pDce->List);
  KeLeaveCriticalRegion();

  DCU_SetDcUndeletable(pDce->hDC);

  if (Type == DCE_WINDOW_DC || Type == DCE_CLASS_DC) //Window DCE have ownership.
  { // Process should already own it.
    pDce->pProcess = PsGetCurrentProcess();
  }
  else
  {
     DPRINT("FREE DCATTR!!!! NOT DCE_WINDOW_DC!!!!! hDC-> %x\n", pDce->hDC);
     IntGdiSetDCOwnerEx( pDce->hDC, GDI_OBJ_HMGR_NONE, FALSE);
  }

  if (Type == DCE_CACHE_DC)
  {
     pDce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
  }
  else
  {
     pDce->DCXFlags = DCX_DCEBUSY;
     if (Wnd)
     {
        if (Type == DCE_WINDOW_DC)
        {
          if (Wnd->Style & WS_CLIPCHILDREN) pDce->DCXFlags |= DCX_CLIPCHILDREN;
          if (Wnd->Style & WS_CLIPSIBLINGS) pDce->DCXFlags |= DCX_CLIPSIBLINGS;
        }
     }
  }
  return(pDce);
}

static VOID APIENTRY
DceSetDrawable(PWINDOW_OBJECT Window OPTIONAL, HDC hDC, ULONG Flags,
               BOOL SetClipOrigin)
{
  PWINDOW Wnd;
  DC *dc = DC_LockDc(hDC);
  if(!dc)
      return;

  if (Window == NULL)
  {
      dc->ptlDCOrig.x = 0;
      dc->ptlDCOrig.y = 0;
  }
  else
  {
      Wnd = Window->Wnd;
      if (Flags & DCX_WINDOW)
      {
         dc->ptlDCOrig.x = Wnd->WindowRect.left;
         dc->ptlDCOrig.y = Wnd->WindowRect.top;
      }
      else
      {
         dc->ptlDCOrig.x = Wnd->ClientRect.left;
         dc->ptlDCOrig.y = Wnd->ClientRect.top;
      }
  }
  DC_UnlockDc(dc);
}


static VOID FASTCALL
DceDeleteClipRgn(DCE* Dce)
{
   Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);

   if (Dce->DCXFlags & DCX_KEEPCLIPRGN )
   {
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (Dce->hClipRgn != NULL)
   {
      NtGdiDeleteObject(Dce->hClipRgn);
   }

   Dce->hClipRgn = NULL;

   /* make it dirty so that the vis rgn gets recomputed next time */
   Dce->DCXFlags |= DCX_DCEDIRTY;
}

static INT FASTCALL
DceReleaseDC(DCE* dce, BOOL EndPaint)
{
   if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_DCEEMPTY | DCX_DCEBUSY)))
   {
      return 0;
   }

   /* restore previous visible region */

   if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
         ((dce->DCXFlags & DCX_CACHE) || EndPaint))
   {
      DceDeleteClipRgn(dce);
   }

   if (dce->DCXFlags & DCX_CACHE)
   {
     if (!(dce->DCXFlags & DCX_NORESETATTRS))
     {
       /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
       IntGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);

       // Clean the DC
       if (!IntGdiCleanDC(dce->hDC)) return 0;

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
     dce->DCXFlags &= ~DCX_DCEBUSY;
     DPRINT("Exit!!!!! DCX_CACHE!!!!!!   hDC-> %x \n", dce->hDC);
     if (!IntGdiSetDCOwnerEx( dce->hDC, GDI_OBJ_HMGR_NONE, FALSE))
        return 0;
     dce->pProcess = NULL;            // Reset ownership.
   }
   return 1; // Released!
}

static VOID FASTCALL
DceUpdateVisRgn(DCE *Dce, PWINDOW_OBJECT Window, ULONG Flags)
{
   HANDLE hRgnVisible = NULL;
   ULONG DcxFlags;
   PWINDOW_OBJECT DesktopWindow;

   if (Flags & DCX_PARENTCLIP)
   {
      PWINDOW_OBJECT Parent;
      PWINDOW ParentWnd;

      Parent = Window->Parent;
      if(!Parent)
      {
         hRgnVisible = NULL;
         goto noparent;
      }

      ParentWnd = Parent->Wnd;

      if (ParentWnd->Style & WS_CLIPSIBLINGS)
      {
         DcxFlags = DCX_CLIPSIBLINGS |
                    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
      }
      else
      {
         DcxFlags = Flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
      }
      hRgnVisible = DceGetVisRgn(Parent, DcxFlags, Window->hSelf, Flags);
   }
   else if (Window == NULL)
   {
      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if (NULL != DesktopWindow)
      {
         hRgnVisible = UnsafeIntCreateRectRgnIndirect(&DesktopWindow->Wnd->WindowRect);
      }
      else
      {
         hRgnVisible = NULL;
      }
   }
   else
   {
      hRgnVisible = DceGetVisRgn(Window, Flags, 0, 0);
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
   GdiSelectVisRgn(Dce->hDC, hRgnVisible);

   if (Window != NULL)
   {
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
   }

   if (hRgnVisible != NULL)
   {
      NtGdiDeleteObject(hRgnVisible);
   }
}

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
   PWINDOW_OBJECT Parent;
   ULONG DcxFlags;
   DCE* Dce = NULL;
   BOOL UpdateVisRgn = TRUE;
   BOOL UpdateClipOrigin = FALSE;
   PWINDOW Wnd = NULL;
   HDC hDC = NULL;   

   if (NULL == Window)
   {
      Flags &= ~DCX_USESTYLE;
      Flags |= DCX_CACHE;
   }
   else
       Wnd = Window->Wnd;

   if (Flags & (DCX_WINDOW | DCX_PARENTCLIP)) Flags |= DCX_CACHE;

   // When GetDC is called with hWnd nz, DCX_CACHE & _WINDOW are clear w _USESTYLE set.
   if (Flags & DCX_USESTYLE)
   {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);
      if (!(Flags & DCX_WINDOW)) // not window rectangle
      {
         if (Wnd->Class->Style & CS_PARENTDC)
         {
            Flags |= DCX_PARENTCLIP;
         }

         if (!(Flags & DCX_CACHE) && // Not on the cheap wine list.
             !(Wnd->Class->Style & CS_OWNDC) )
         {
            if (!(Wnd->Class->Style & CS_CLASSDC))
            // The window is not POWNED or has any CLASS, so we are looking for cheap wine.
               Flags |= DCX_CACHE;
            else
            {
               if (Wnd->Class->Dce) hDC = ((PDCE)Wnd->Class->Dce)->hDC;
               DPRINT("We have CLASS!!\n");
            }
         }
/*         else // For Testing!
         {
            DPRINT1("We have POWNER!!\n");
            if (Window->Dce) DPRINT1("We have POWNER with DCE!!\n");
         }
*/
         if (Wnd->Style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }

         if (Wnd->Style & WS_CLIPCHILDREN &&
             !(Wnd->Style & WS_MINIMIZE))
         {
            Flags |= DCX_CLIPCHILDREN;
         }
      }
      else
      {
         if (Wnd->Style & WS_CLIPSIBLINGS) Flags |= DCX_CLIPSIBLINGS;
         Flags |= DCX_CACHE;
      }
   }

   if (Flags & DCX_WINDOW) Flags &= ~DCX_CLIPCHILDREN;

   if (Flags & DCX_NOCLIPCHILDREN)
   {
      Flags |= DCX_CACHE;
      Flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
   }

   Parent = (Window ? Window->Parent : NULL);

   if (NULL == Window || !(Wnd->Style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
      Flags |= DCX_CLIPSIBLINGS;
   }

   /* it seems parent clip is ignored when clipping siblings or children */
   if (Flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) Flags &= ~DCX_PARENTCLIP;

   if (Flags & DCX_PARENTCLIP)
   {
      if ((Wnd->Style & WS_VISIBLE) &&
          (Parent->Wnd->Style & WS_VISIBLE))
      {
         Flags &= ~DCX_CLIPCHILDREN;
         if (Parent->Wnd->Style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }
      }
   }

   // Window nz, check to see if we still own this or it is just cheap wine tonight.
   if (!(Flags & DCX_CACHE))
   {
      if ( Wnd->ti != GetW32ThreadInfo())
         Flags |= DCX_CACHE; // Ah~ Not Powned! Forced to be cheap~
   }

   DcxFlags = Flags & DCX_CACHECOMPAREMASK;

   if (Flags & DCX_CACHE)
   { // Scan the cheap wine list for our match.
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;
      KeEnterCriticalRegion();
      Dce = FirstDce;
      do
      {
// The reason for this you may ask?
// Well, it seems ReactOS calls GetDC with out first creating a desktop DC window!
// Need to test for null here. Not sure if this is a bug or a feature.
// First time use hax, need to use DceAllocDCE during window display init.
         if (!Dce) break;
//
// The way I understand this, you can have more than one DC per window.
// Only one Owned if one was requested and saved and one Cached. 
//
         if ((Dce->DCXFlags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
         {
            DceUnused = Dce;
            if (Dce->DCXFlags & DCX_DCEEMPTY)
            {
               DceEmpty = Dce;
            }
            else if (Dce->hwndCurrent == (Window ? Window->hSelf : NULL) &&
                     ((Dce->DCXFlags & DCX_CACHECOMPAREMASK) == DcxFlags))
            {
#if 0 /* FIXME */
               UpdateVisRgn = FALSE;
#endif
               UpdateClipOrigin = TRUE;
               break;
            }
         }
        Dce = (PDCE)Dce->List.Flink;
      } while (Dce != FirstDce);
      KeLeaveCriticalRegion();

      Dce = (DceEmpty == NULL) ? DceUnused : DceEmpty;

      if (Dce == NULL)
      {
         Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
      }
      if (!Dce) return NULL;

      Dce->hwndCurrent = (Window ? Window->hSelf : NULL);
   }
   else // If we are here, we are POWNED or having CLASS.
   {
      KeEnterCriticalRegion();
      Dce = FirstDce;
      do
      {   // Check for Window handle than HDC match for CLASS.
          if ((Dce->hwndCurrent == Window->hSelf) || (Dce->hDC == hDC)) break;
          Dce = (PDCE)Dce->List.Flink;
      } while (Dce != FirstDce);
      KeLeaveCriticalRegion();

      DPRINT("DCX:Flags -> %x:%x\n",Dce->DCXFlags & DCX_CACHE, Flags & DCX_CACHE);

      if (Dce->hwndCurrent == Window->hSelf)
      {
          DPRINT("Owned DCE!\n");
          UpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
      }
      else
      {
          DPRINT("Owned/Class DCE\n");
      /* we should free dce->clip_rgn here, but Windows apparently doesn't */
          Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
          Dce->hClipRgn = NULL;
      }

#if 1 /* FIXME */
      UpdateVisRgn = TRUE;
#endif
   }

// First time use hax, need to use DceAllocDCE during window display init.
   if (NULL == Dce)
   {
      return(NULL);
   }

   if (!GDIOBJ_ValidateHandle(Dce->hDC, GDI_OBJECT_TYPE_DC))
   {
      DPRINT1("FIXME: Got DCE with invalid hDC! 0x%x\n", Dce->hDC);
      Dce->hDC = DceCreateDisplayDC();
      /* FIXME: Handle error */
   }

   if (!(Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) && ClipRegion)
   {
      if (!(Flags & DCX_KEEPCLIPRGN))
         NtGdiDeleteObject(ClipRegion);
      ClipRegion = NULL;
   }

#if 0
   if (NULL != Dce->hClipRgn)
   {
      DceDeleteClipRgn(Dce);
   }
#endif

   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   if (0 != (Flags & DCX_INTERSECTUPDATE) && NULL == ClipRegion)
   {
      Flags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      Dce->DCXFlags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      ClipRegion = Window->UpdateRegion;
   }

   if (ClipRegion == (HRGN) 1)
   {
      if (!(Flags & DCX_WINDOW))
      {
         Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->ClientRect);
      }
      else
      {
         Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->WindowRect);
      }
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (ClipRegion != NULL)
   {
      Dce->hClipRgn = ClipRegion;
   }

   DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

   //  if (UpdateVisRgn)
   {
      DceUpdateVisRgn(Dce, Window, Flags);
   }

   if (Dce->DCXFlags & DCX_CACHE)
   {
      DPRINT("ENTER!!!!!! DCX_CACHE!!!!!!   hDC-> %x\n", Dce->hDC);
      // Need to set ownership so Sync dcattr will work.
      IntGdiSetDCOwnerEx( Dce->hDC, GDI_OBJ_HMGR_POWNED, FALSE);
      Dce->pProcess = PsGetCurrentProcess(); // Set the temp owning process
   }
   return(Dce->hDC);
}

/***********************************************************************
 *           DceFreeDCE
 */
PDCE FASTCALL
DceFreeDCE(PDCE pdce, BOOLEAN Force)
{
  DCE *ret;
  BOOL Hit = FALSE;

  if (NULL == pdce) return NULL;

  ret = (PDCE) pdce->List.Flink;

#if 0 /* FIXME */
  SetDCHook(pdce->hDC, NULL, 0L);
#endif

  if (Force && !GDIOBJ_OwnedByCurrentProcess(pdce->hDC))
  {
     DPRINT("Change ownership for DCE! -> %x\n" , pdce);
     // Note: Windows sets W32PF_OWNDCCLEANUP and moves on.
     if (!IsObjectDead((HGDIOBJ) pdce->hDC))
     {
         DC_SetOwnership( pdce->hDC, PsGetCurrentProcess());
     }
     else
     {
         DPRINT1("Attempted to change ownership of an DCEhDC 0x%x currently being destroyed!!!\n",pdce->hDC);
         Hit = TRUE;
     }
  }

  if (!Hit) IntGdiDeleteDC(pdce->hDC, TRUE);

  if (pdce->hClipRgn && ! (pdce->DCXFlags & DCX_KEEPCLIPRGN))
  {
      NtGdiDeleteObject(pdce->hClipRgn);
  }

  RemoveEntryList(&pdce->List);

  if (IsListEmpty(&pdce->List))
  {
      DPRINT1("List is Empty! DCE! -> %x\n" , pdce);
      FirstDce = NULL;
      ret = NULL;
  }

  ExFreePoolWithTag(pdce, TAG_PDCE);

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
  PDCE pDCE;

  pDCE = FirstDce;
  KeEnterCriticalRegion();
  do
  {
      if (!pDCE) break;
      if (pDCE->hwndCurrent == Window->hSelf)
      {
         if (!(pDCE->DCXFlags & DCX_CACHE)) /* owned or Class DCE*/
         {
            if (Window->Wnd->Class->Style & CS_CLASSDC ||
                Window->Wnd->Style & CS_CLASSDC) /* Test Class first */
            {
               if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) /* Class DCE*/
                   DceDeleteClipRgn(pDCE);
               // Update and reset Vis Rgn and clear the dirty bit.
               // Should release VisRgn than reset it to default.
               DceUpdateVisRgn(pDCE, Window, pDCE->DCXFlags);
               pDCE->DCXFlags = DCX_DCEEMPTY;
               pDCE->hwndCurrent = 0;
            }
            else if (Window->Wnd->Class->Style & CS_OWNDC ||
                     Window->Wnd->Style & CS_OWNDC) /* owned DCE*/
            {
               pDCE = DceFreeDCE(pDCE, FALSE);
               if (!pDCE) break;
               Window->Dce = NULL;
               continue;
            }
            else
            {
               ASSERT(FALSE);
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
               DceReleaseDC(pDCE, FALSE);
            }
            pDCE->DCXFlags |= DCX_DCEEMPTY;
            pDCE->hwndCurrent = 0;
         }
      }
      pDCE = (PDCE) pDCE->List.Flink;
  } while (pDCE != FirstDce);
  KeLeaveCriticalRegion();
}

void FASTCALL
DceFreeClassDCE(HDC hDC)
{
   PDCE pDCE = FirstDce;
   KeEnterCriticalRegion();
   do
   {
       if(!pDCE) break;
       if (pDCE->hDC == hDC)
       {
          pDCE = DceFreeDCE(pDCE, FALSE);
          if(!pDCE) break;
          continue;
       }
       pDCE = (PDCE)pDCE->List.Flink;
   } while (pDCE != FirstDce);
   KeLeaveCriticalRegion();
}

VOID FASTCALL
DceEmptyCache()
{
   PDCE pDCE = FirstDce;
   KeEnterCriticalRegion();
   do
   {
      if(!pDCE) break;
      pDCE = DceFreeDCE(pDCE, TRUE);
      if(!pDCE) break;
   } while (pDCE != FirstDce);
   KeLeaveCriticalRegion();
   FirstDce = NULL;
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
   pDCE = FirstDce;
   if(!pDCE) return; // Another null test!
   do
   {
      if(!pDCE) break;
      if (0 == (pDCE->DCXFlags & DCX_DCEEMPTY))
      {
         if (Window->hSelf == pDCE->hwndCurrent)
         {
            CurrentWindow = Window;
         }
         else
         {
            CurrentWindow = UserGetWindowObject(pDCE->hwndCurrent);
            if (NULL == CurrentWindow)
            {
               pDCE = (PDCE) pDCE->List.Flink;
               continue;
            }
         }

         if (!GDIOBJ_ValidateHandle(pDCE->hDC, GDI_OBJECT_TYPE_DC) ||
             (dc = DC_LockDc(pDCE->hDC)) == NULL)
         {
            pDCE = (PDCE) pDCE->List.Flink;
            continue;
         }
         if (Window == CurrentWindow || IntIsChildWindow(Window, CurrentWindow))
         {
            if (pDCE->DCXFlags & DCX_WINDOW)
            {
               DeltaX = CurrentWindow->Wnd->WindowRect.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->Wnd->WindowRect.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->Wnd->WindowRect.left;
               dc->ptlDCOrig.y = CurrentWindow->Wnd->WindowRect.top;
            }
            else
            {
               DeltaX = CurrentWindow->Wnd->ClientRect.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->Wnd->ClientRect.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->Wnd->ClientRect.left;
               dc->ptlDCOrig.y = CurrentWindow->Wnd->ClientRect.top;
            }
            if (NULL != dc->w.hClipRgn)
            {
               int FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);
               NtGdiOffsetRgn(dc->w.hClipRgn, DeltaX, DeltaY);
               CLIPPING_UpdateGCRegion(dc);
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
//            IntEngWindowChanged(CurrentWindow, WOC_RGN_CLIENT);
//            UserDerefObject(CurrentWindow);
         }
      }
      pDCE =(PDCE) pDCE->List.Flink;
   } while (pDCE != FirstDce);
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;
  HWND Ret = NULL;
  KeEnterCriticalRegion();
  Dce = FirstDce;
  do
  {
      if(Dce->hDC == hDc)
      {
         Ret = Dce->hwndCurrent;
         break;
      }
      Dce = (PDCE)Dce->List.Flink;
  } while (Dce != FirstDce);
  KeLeaveCriticalRegion();
  return Ret;
}

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Window, HDC hDc, BOOL EndPaint)
{
  PDCE dce;
  INT nRet = 0;
  BOOL Hit = FALSE;

  DPRINT("%p %p\n", Window, hDc);
  dce = FirstDce;
  KeEnterCriticalRegion();
  do
  {
     if(!dce) break;
     if (dce->hDC == hDc)
     {
        Hit = TRUE;
        break;
     }

     dce = (PDCE) dce->List.Flink;
  }
  while (dce != FirstDce );
  KeLeaveCriticalRegion();

  if ( Hit && (dce->DCXFlags & DCX_DCEBUSY))
  {
     nRet = DceReleaseDC(dce, EndPaint);
  }

  return nRet;
}

HDC FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd)
{
  return UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC APIENTRY
NtUserGetDCEx(HWND hWnd OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
  PWINDOW_OBJECT Wnd=NULL;
  DECLARE_RETURN(HDC);

  DPRINT("Enter NtUserGetDCEx\n");
  UserEnterExclusive();

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
      RETURN(NULL);
  }
  RETURN( UserGetDCEx(Wnd, ClipRegion, Flags));

CLEANUP:
  DPRINT("Leave NtUserGetDCEx, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
HDC APIENTRY
NtUserGetWindowDC(HWND hWnd)
{
  return NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC APIENTRY
NtUserGetDC(HWND hWnd)
{
 DPRINT("NtUGetDC -> %x:%x\n", hWnd, !hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE );

  return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

/*!
 * Select logical palette into device context.
 * \param	hDC 				handle to the device context
 * \param	hpal				handle to the palette
 * \param	ForceBackground 	If this value is FALSE the logical palette will be copied to the device palette only when the applicatioon
 * 								is in the foreground. If this value is TRUE then map the colors in the logical palette to the device
 * 								palette colors in the best way.
 * \return	old palette
 *
 * \todo	implement ForceBackground == TRUE
*/
HPALETTE
APIENTRY
NtUserSelectPalette(HDC  hDC,
              HPALETTE  hpal,
       BOOL  ForceBackground)
{
    HPALETTE oldPal;
    UserEnterExclusive();
    // Implement window checks
    oldPal = GdiSelectPalette( hDC, hpal, ForceBackground);
    UserLeave();
    return oldPal;
}


/* EOF */
