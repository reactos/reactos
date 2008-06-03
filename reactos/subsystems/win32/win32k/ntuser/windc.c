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
static PDC defaultDCstate = NULL;
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
  PVOID Class = NULL;

  if (Window) Wnd = Window->Wnd;

  if (Wnd) Class = Wnd->Class;

  if (Type == DCE_CLASS_DC)
  {
     PDCE pdce;
     KeEnterCriticalRegion();
     pdce = FirstDce;
     do
     {
        if (pdce->Class == Class)
        {
           pdce->Count++;
           KeLeaveCriticalRegion();
           return pdce;
        }
        pdce = (PDCE)pdce->List.Flink;
     } while (pdce != FirstDce);
     KeLeaveCriticalRegion();
  }

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
  pDce->Class = Class;
  pDce->Count = 1;

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

static VOID STDCALL
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
       PDC dc;
       /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
       IntGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);

       dc = DC_LockDc ( dce->hDC );
       // Clean the DC
       IntGdiCopyFromSaveState(dc, defaultDCstate, dce->hDC ); // Was SetDCState.

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
     else
     { // Save Users Dc_Attr.
       PDC dc = DC_LockDc(dce->hDC);
       if(dc)
       {
         PDC_ATTR Dc_Attr = dc->pDc_Attr;
         if(Dc_Attr) MmCopyFromCaller(&dc->Dc_Attr, Dc_Attr, sizeof(DC_ATTR));
         DC_UnlockDc(dc);
       }
     }
     DPRINT("Exit!!!!! DCX_CACHE!!!!!!   hDC-> %x \n", dce->hDC);
     if (!IntGdiSetDCOwnerEx( dce->hDC, GDI_OBJ_HMGR_NONE, FALSE))
        return 0;
     dce->pProcess = NULL;            // Reset ownership.
   }
   return 1;
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
   DCE* Dce;
   BOOL UpdateVisRgn = TRUE;
   BOOL UpdateClipOrigin = FALSE;
   PWINDOW Wnd = NULL;

   if (NULL == Window)
   {  // Do the same as GetDC with a NULL.
//      Window = UserGetWindowObject(IntGetDesktopWindow());
//      if (Window) Wnd = Window->Wnd;
//      else
         Flags &= ~DCX_USESTYLE;
   }
   else
       Wnd = Window->Wnd;

   if (NULL == Window || NULL == Window->Dce)
   {
      Flags |= DCX_CACHE;
   }

   if (Flags & DCX_USESTYLE)
   {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

      if (Wnd->Style & WS_CLIPSIBLINGS)
      {
         Flags |= DCX_CLIPSIBLINGS;
      }

      if (!(Flags & DCX_WINDOW))
      {
         if (Wnd->Class->Style & CS_PARENTDC)
         {
            Flags |= DCX_PARENTCLIP;
         }

         if (Wnd->Style & WS_CLIPCHILDREN &&
             !(Wnd->Style & WS_MINIMIZE))
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
      Flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
   }

   if (Flags & DCX_WINDOW)
   {
      Flags = (Flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;
   }

   Parent = (Window ? Window->Parent : NULL);

   if (NULL == Window || !(Wnd->Style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
   }
   else if (Flags & DCX_PARENTCLIP)
   {
      Flags |= DCX_CACHE;
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

   DcxFlags = Flags & DCX_CACHECOMPAREMASK;

   if (Flags & DCX_CACHE)
   {
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;
      KeEnterCriticalRegion();
      Dce = FirstDce;
      do
      {
// The reason for this you may ask?
// Well, it seems ReactOS calls GetDC with out first creating a desktop DC window!
// Need to test for null here. Not sure if this is a bug or a feature.
         if (!Dce) break;

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
   }
   else
   {
      Dce = Window->Dce;
      if (Dce->hwndCurrent == Window->hSelf)
      {
         UpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
      }
      else
      {
      /* we should free dce->clip_rgn here, but Windows apparently doesn't */
         Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
         Dce->hClipRgn = NULL;
      }
#if 1 /* FIXME */
      UpdateVisRgn = TRUE;
#endif

   }

   if (NULL == Dce)
   {
      return(NULL);
   }

   if (!GDIOBJ_ValidateHandle(Dce->hDC, GDI_OBJECT_TYPE_DC))
   {
      DPRINT1("FIXME: Got DCE with invalid hDC!\n");
      Dce->hDC = DceCreateDisplayDC();
      /* FIXME: Handle error */
   }

   Dce->hwndCurrent = (Window ? Window->hSelf : NULL);
   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   if (0 == (Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) && NULL != ClipRegion)
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
  NTSTATUS Status = STATUS_SUCCESS;

  if (NULL == pdce) return NULL;

  ret = (PDCE) pdce->List.Flink;

#if 0 /* FIXME */
  SetDCHook(pdce->hDC, NULL, 0L);
#endif

  if (Force && !GDIOBJ_OwnedByCurrentProcess(pdce->hDC))
  {
     DPRINT1("Change ownership for DCE!\n");

     if (!IsObjectDead((HGDIOBJ) pdce->hDC))
         DC_SetOwnership( pdce->hDC, PsGetCurrentProcess());
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
  // Temp fix until we know where the problem is, most likely in windc.
  _SEH_TRY
  {            
      RemoveEntryList(&pdce->List);
  }
  _SEH_HANDLE
  {
      Status = _SEH_GetExceptionCode();
  }
  _SEH_END
  if (!NT_SUCCESS(Status))
  {
     SetLastNtError(Status);
     DPRINT1("CRASHED DCE! -> %x\n" , pdce);
     return 0; // Give it up and bail~!
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

HDC
FASTCALL
IntGetDC(PWINDOW_OBJECT Window)
{
  if (!Window)
  {  // MSDN:
     //"hWnd [in] Handle to the window whose DC is to be retrieved.
     // If this value is NULL, GetDC retrieves the DC for the entire screen."
     Window = UserGetWindowObject(IntGetDesktopWindow());
     if (Window)
        return UserGetDCEx(Window, NULL, DCX_CACHE | DCX_WINDOW);
     else
        return NULL;
  }
  return UserGetDCEx(Window, NULL, DCX_USESTYLE);
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

HDC STDCALL
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
HDC STDCALL
NtUserGetWindowDC(HWND hWnd)
{
  return NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
// We have a problem here! Should call IntGetDC.
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
HPALETTE STDCALL NtUserSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
    PDC dc;
    HPALETTE oldPal = NULL;
    PPALGDI PalGDI;

    // FIXME: mark the palette as a [fore\back]ground pal
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        return NULL;
    }

    /* Check if this is a valid palette handle */
    PalGDI = PALETTE_LockPalette(hpal);
    if (!PalGDI)
    {
        DC_UnlockDc(dc);
        return NULL;
    }

    /* Is this a valid palette for this depth? */
    if ((dc->w.bitsPerPixel <= 8 && PalGDI->Mode == PAL_INDEXED) ||
        (dc->w.bitsPerPixel > 8  && PalGDI->Mode != PAL_INDEXED))
    {
        oldPal = dc->DcLevel.hpal;
        dc->DcLevel.hpal = hpal;
    }
    else if (8 < dc->w.bitsPerPixel && PAL_INDEXED == PalGDI->Mode)
    {
        oldPal = dc->DcLevel.hpal;
        dc->DcLevel.hpal = hpal;
    }

    PALETTE_UnlockPalette(PalGDI);
    DC_UnlockDc(dc);

    return oldPal;
}


/* EOF */
