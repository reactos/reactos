/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/windc.c
 * PURPOSE:         Window DC management
 * COPYRIGHT:       Copyright 2007 ReactOS
 *
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

int FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);

/* GLOBALS *******************************************************************/

/* NOTE - I think we should store this per window station (including gdi objects) */
/* Answer: No, use the DCE pMonitor to compare with! */

static LIST_ENTRY LEDce;
static INT DCECount = 0; // Count of DCE in system.

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_NORESETATTRS | DCX_LOCKWINDOWUPDATE | \
                              DCX_LAYEREDWIN | DCX_CACHE | DCX_WINDOW | \
                              DCX_PARENTCLIP)

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
      defaultDCstate->pdcattr = &defaultDCstate->dcattr;
      DC_vCopyState(dc, defaultDCstate, TRUE);
      DC_UnlockDc( dc );
      InitializeListHead(&LEDce);
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
      VisRgn = IntSysCreateRectRgn(0, 0, 0, 0);

  return VisRgn;
}

PDCE FASTCALL
DceAllocDCE(PWINDOW_OBJECT Window OPTIONAL, DCE_TYPE Type)
{
  PDCE pDce;
  PWND Wnd = NULL;

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
  DCECount++;
  DPRINT("Alloc DCE's! %d\n",DCECount);
  pDce->hwndCurrent = (Window ? Window->hSelf : NULL);
  pDce->pwndOrg  = Wnd;
  pDce->pwndClip = Wnd;
  pDce->hrgnClip = NULL;
  pDce->hrgnClipPublic = NULL;
  pDce->hrgnSavedVis = NULL;
  pDce->ppiOwner = NULL;

  InsertTailList(&LEDce, &pDce->List);

  DCU_SetDcUndeletable(pDce->hDC);

  if (Type == DCE_WINDOW_DC || Type == DCE_CLASS_DC) //Window DCE have ownership.
  {
     pDce->ptiOwner = GetW32ThreadInfo();
  }
  else
  {
     DPRINT("FREE DCATTR!!!! NOT DCE_WINDOW_DC!!!!! hDC-> %x\n", pDce->hDC);
     IntGdiSetDCOwnerEx( pDce->hDC, GDI_OBJ_HMGR_NONE, FALSE);
     pDce->ptiOwner = NULL;
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
          if (Wnd->style & WS_CLIPCHILDREN) pDce->DCXFlags |= DCX_CLIPCHILDREN;
          if (Wnd->style & WS_CLIPSIBLINGS) pDce->DCXFlags |= DCX_CLIPSIBLINGS;
        }
     }
  }
  return(pDce);
}

static VOID APIENTRY
DceSetDrawable( PWINDOW_OBJECT Window OPTIONAL,
                HDC hDC,
                ULONG Flags,
                BOOL SetClipOrigin)
{
  PWND Wnd;
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
         dc->ptlDCOrig.x = Wnd->rcWindow.left;
         dc->ptlDCOrig.y = Wnd->rcWindow.top;
      }
      else
      {
         dc->ptlDCOrig.x = Wnd->rcClient.left;
         dc->ptlDCOrig.y = Wnd->rcClient.top;
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
   else if (Dce->hrgnClip != NULL)
   {
      GDIOBJ_FreeObjByHandle(Dce->hrgnClip, GDI_OBJECT_TYPE_REGION|GDI_OBJECT_TYPE_SILENT);
   }

   Dce->hrgnClip = NULL;

   /* make it dirty so that the vis rgn gets recomputed next time */
   Dce->DCXFlags |= DCX_DCEDIRTY;
}

static INT FASTCALL
DceReleaseDC(DCE* dce, BOOL EndPaint)
{
   if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_INDESTROY | DCX_DCEEMPTY | DCX_DCEBUSY)))
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
      dce->ptiOwner = NULL; // Reset ownership.
      dce->ppiOwner = NULL;

#if 0 // Need to research and fix before this is a "growing" issue.
      if (++DCECache > 32)
      {
         pLE = LEDce.Flink;
         pDCE = CONTAINING_RECORD(pLE, DCE, List);
         do
         {
            if (!(pDCE->DCXFlags & DCX_DCEBUSY))
            {  /* Free the unused cache DCEs. */
               pDCE = DceFreeDCE(pDCE, TRUE);
               if (!pDCE) break;
               continue;
            }
         }
         while (pLE != &LEDce );
      }
#endif      
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
      PWND ParentWnd;

      Parent = Window->spwndParent;
      if(!Parent)
      {
         hRgnVisible = NULL;
         goto noparent;
      }

      ParentWnd = Parent->Wnd;

      if (ParentWnd->style & WS_CLIPSIBLINGS)
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
         hRgnVisible = IntSysCreateRectRgnIndirect(&DesktopWindow->Wnd->rcWindow);
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
      if(Dce->hrgnClip != NULL)
      {
         NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hrgnClip, RGN_AND);
      }
      else
      {
         if(hRgnVisible != NULL)
         {
            REGION_FreeRgnByHandle(hRgnVisible);
         }
         hRgnVisible = IntSysCreateRectRgn(0, 0, 0, 0);
      }
   }
   else if (Flags & DCX_EXCLUDERGN && Dce->hrgnClip != NULL)
   {
      NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hrgnClip, RGN_DIFF);
   }

   Dce->DCXFlags &= ~DCX_DCEDIRTY;
   GdiSelectVisRgn(Dce->hDC, hRgnVisible);

   if (Window != NULL)
   {
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
   }

   if (hRgnVisible != NULL)
   {
      REGION_FreeRgnByHandle(hRgnVisible);
   }
}

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
   PWINDOW_OBJECT Parent;
   ULONG DcxFlags;
   DCE* Dce = NULL;
   BOOL UpdateClipOrigin = FALSE;
   PWND Wnd = NULL;
   HDC hDC = NULL;
   PPROCESSINFO ppi;
   PLIST_ENTRY pLE;

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
         if (Wnd->pcls->style & CS_PARENTDC)
         {
            Flags |= DCX_PARENTCLIP;
         }

         if (!(Flags & DCX_CACHE) && // Not on the cheap wine list.
             !(Wnd->pcls->style & CS_OWNDC) )
         {
            if (!(Wnd->pcls->style & CS_CLASSDC))
            // The window is not POWNED or has any CLASS, so we are looking for cheap wine.
               Flags |= DCX_CACHE;
            else
            {
               if (Wnd->pcls->pdce) hDC = ((PDCE)Wnd->pcls->pdce)->hDC;
               DPRINT("We have CLASS!!\n");
            }
         }
/*         else // For Testing!
         {
            DPRINT1("We have POWNER!!\n");
            if (Window->Dce) DPRINT1("We have POWNER with DCE!!\n");
         }
*/
         if (Wnd->style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }

         if (Wnd->style & WS_CLIPCHILDREN &&
             !(Wnd->style & WS_MINIMIZE))
         {
            Flags |= DCX_CLIPCHILDREN;
         }
         /* If minized with icon in the set, we are forced to be cheap! */
         if (Wnd->style & WS_MINIMIZE &&
             Wnd->pcls->hIcon)
         {
            Flags |= DCX_CACHE;
         }
      }
      else
      {
         if (Wnd->style & WS_CLIPSIBLINGS) Flags |= DCX_CLIPSIBLINGS;
         Flags |= DCX_CACHE;
      }
   }

   if (Flags & DCX_WINDOW) Flags &= ~DCX_CLIPCHILDREN;

   if (Flags & DCX_NOCLIPCHILDREN)
   {
      Flags |= DCX_CACHE;
      Flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
   }

   Parent = (Window ? Window->spwndParent : NULL);

   if (NULL == Window || !(Wnd->style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
      Flags |= DCX_CLIPSIBLINGS;
   }

   /* it seems parent clip is ignored when clipping siblings or children */
   if (Flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) Flags &= ~DCX_PARENTCLIP;

   if (Flags & DCX_PARENTCLIP)
   {
      if ((Wnd->style & WS_VISIBLE) &&
          (Parent->Wnd->style & WS_VISIBLE))
      {
         Flags &= ~DCX_CLIPCHILDREN;
         if (Parent->Wnd->style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }
      }
   }

   // Window nz, check to see if we still own this or it is just cheap wine tonight.
   if (!(Flags & DCX_CACHE))
   {
      if ( Wnd->head.pti != GetW32ThreadInfo())
         Flags |= DCX_CACHE; // Ah~ Not Powned! Forced to be cheap~
   }

   DcxFlags = Flags & DCX_CACHECOMPAREMASK;

   if (Flags & DCX_CACHE)
   { // Scan the cheap wine list for our match.
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;
      KeEnterCriticalRegion();
      pLE = LEDce.Flink;
      Dce = CONTAINING_RECORD(pLE, DCE, List);
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
               UpdateClipOrigin = TRUE;
               break;
            }
         }
         pLE = Dce->List.Flink;
         Dce = CONTAINING_RECORD(pLE, DCE, List);
      }
      while (pLE != &LEDce);
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
      pLE = LEDce.Flink;
      Dce = CONTAINING_RECORD(pLE, DCE, List);
      do
      {   // Check for Window handle than HDC match for CLASS.
          if ((Dce->hwndCurrent == Window->hSelf) ||
              (Dce->hDC == hDC))
             break;
          pLE = Dce->List.Flink;
          Dce = CONTAINING_RECORD(pLE, DCE, List);
      }
      while (pLE != &LEDce);
      KeLeaveCriticalRegion();

      if ( (Flags & (DCX_INTERSECTRGN|DCX_EXCLUDERGN)) &&
           (Dce->DCXFlags & (DCX_INTERSECTRGN|DCX_EXCLUDERGN)) )          
      {
          DceDeleteClipRgn(Dce);
      }
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

   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   /*
      Bump it up! This prevents the random errors in wine dce tests and with
      proper bits set in DCX_CACHECOMPAREMASK.
      Reference:
        http://www.reactos.org/archives/public/ros-dev/2008-July/010498.html
        http://www.reactos.org/archives/public/ros-dev/2008-July/010499.html
    */
   if (pLE != &LEDce)
   {
      RemoveEntryList(&Dce->List);
      InsertHeadList(&LEDce, &Dce->List);
   }

   /* Introduced in rev 6691 and modified later. */
   if ( (Flags & DCX_INTERSECTUPDATE) && !ClipRegion )
   {
      Flags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      Dce->DCXFlags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      ClipRegion = Window->hrgnUpdate;
   }

   if (ClipRegion == (HRGN) 1)
   {
      if (!(Flags & DCX_WINDOW))
      {
         Dce->hrgnClip = IntSysCreateRectRgnIndirect(&Window->Wnd->rcClient);
      }
      else
      {
         Dce->hrgnClip = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
      }
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (ClipRegion != NULL)
   {
      if (Dce->hrgnClip != NULL)
      {
         DPRINT1("Should not be called!!\n");
         GDIOBJ_FreeObjByHandle(Dce->hrgnClip, GDI_OBJECT_TYPE_REGION|GDI_OBJECT_TYPE_SILENT);
         Dce->hrgnClip = NULL;
      }
      Dce->hrgnClip = ClipRegion;
   }

   DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

   DceUpdateVisRgn(Dce, Window, Flags);

   if (Dce->DCXFlags & DCX_CACHE)
   {
      DPRINT("ENTER!!!!!! DCX_CACHE!!!!!!   hDC-> %x\n", Dce->hDC);
      // Need to set ownership so Sync dcattr will work.
      IntGdiSetDCOwnerEx( Dce->hDC, GDI_OBJ_HMGR_POWNED, FALSE);
      Dce->ptiOwner = GetW32ThreadInfo(); // Set the temp owning
   }

   if ( Wnd &&
        Wnd->ExStyle & WS_EX_LAYOUTRTL &&
       !(Flags & DCX_KEEPLAYOUT) )
   {
      NtGdiSetLayout(Dce->hDC, -1, LAYOUT_RTL);
   }

   if (Dce->DCXFlags & DCX_PROCESSOWNED) 
   {
      ppi = PsGetCurrentProcessWin32Process();
      ppi->W32PF_flags |= W32PF_OWNDCCLEANUP;
      Dce->ptiOwner = NULL;
      Dce->ppiOwner = ppi;
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
  PLIST_ENTRY pLE;
  BOOL Hit = FALSE;

  if (NULL == pdce) return NULL;

  pLE = pdce->List.Flink;
  ret = CONTAINING_RECORD(pLE, DCE, List);

  pdce->DCXFlags |= DCX_INDESTROY;

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
  else
  {
     if (!GreGetObjectOwner(pdce->hDC, GDIObjType_DC_TYPE))
        DC_SetOwnership( pdce->hDC, PsGetCurrentProcess());
  }

  if (!Hit) IntGdiDeleteDC(pdce->hDC, TRUE);

  if (pdce->hrgnClip && !(pdce->DCXFlags & DCX_KEEPCLIPRGN))
  {
      GDIOBJ_FreeObjByHandle(pdce->hrgnClip, GDI_OBJECT_TYPE_REGION|GDI_OBJECT_TYPE_SILENT);
      pdce->hrgnClip = NULL;
  }

  RemoveEntryList(&pdce->List);

  if (IsListEmpty(&pdce->List))
  {
      DPRINT1("List is Empty! DCE! -> %x\n" , pdce);
      return NULL;
  }

  ExFreePoolWithTag(pdce, TAG_PDCE);

  DCECount--;
  DPRINT("Freed DCE's! %d \n", DCECount);

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
  PLIST_ENTRY pLE;

  if (DCECount <= 0)
  {
     DPRINT1("FreeWindowDCE No Entry! %d\n",DCECount);
     return;
  }

  pLE = LEDce.Flink;
  pDCE = CONTAINING_RECORD(pLE, DCE, List);
  do
  {
     if (!pDCE)
     {
        DPRINT1("FreeWindowDCE No DCE Pointer!\n");
        break;
     }
     if (IsListEmpty(&pDCE->List))
     {
        DPRINT1("FreeWindowDCE List is Empty!!!!\n");
        break;
     }
     if ( pDCE->hwndCurrent == Window->hSelf &&
          !(pDCE->DCXFlags & DCX_DCEEMPTY) )
     {
        if (!(pDCE->DCXFlags & DCX_CACHE)) /* owned or Class DCE*/
        {
           if (Window->Wnd->pcls->style & CS_CLASSDC) /* Test Class first */
           {
              if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) /* Class DCE*/
                 DceDeleteClipRgn(pDCE);
              // Update and reset Vis Rgn and clear the dirty bit.
              // Should release VisRgn than reset it to default.
              DceUpdateVisRgn(pDCE, Window, pDCE->DCXFlags);
              pDCE->DCXFlags = DCX_DCEEMPTY|DCX_CACHE;
              pDCE->hwndCurrent = 0;

              DPRINT("POWNED DCE going Cheap!! DCX_CACHE!! hDC-> %x \n", pDCE->hDC);
              if (!IntGdiSetDCOwnerEx( pDCE->hDC, GDI_OBJ_HMGR_NONE, FALSE))
              {
                  DPRINT1("Fail Owner Switch hDC-> %x \n", pDCE->hDC);
                  break;
              }
              /* Do not change owner so thread can clean up! */
           }
           else if (Window->Wnd->pcls->style & CS_OWNDC) /* owned DCE*/
           {
              pDCE = DceFreeDCE(pDCE, FALSE);
              if (!pDCE) break;
              continue;
           }
           else
           {
              DPRINT1("Not POWNED or CLASSDC hwndCurrent -> %x \n", pDCE->hwndCurrent);
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
     pLE = pDCE->List.Flink;
     pDCE = CONTAINING_RECORD(pLE, DCE, List);
  }
  while (pLE != &LEDce);
}

void FASTCALL
DceFreeClassDCE(HDC hDC)
{
   PDCE pDCE;
   PLIST_ENTRY pLE;
   pLE = LEDce.Flink;
   pDCE = CONTAINING_RECORD(pLE, DCE, List);

   do
   {
       if(!pDCE) break;
       if (pDCE->hDC == hDC)
       {
          pDCE = DceFreeDCE(pDCE, TRUE); // Might have gone cheap!
          if (!pDCE) break;
          continue;
       }
       pLE = pDCE->List.Flink;
       pDCE = CONTAINING_RECORD(pLE, DCE, List);
   }
   while (pLE != &LEDce);
}

void FASTCALL
DceFreeThreadDCE(PTHREADINFO pti)
{
   PDCE pDCE;
   PLIST_ENTRY pLE;
   pLE = LEDce.Flink;
   pDCE = CONTAINING_RECORD(pLE, DCE, List);

   do
   {
       if(!pDCE) break;
       if (pDCE->ptiOwner == pti)
       {
          if (pDCE->DCXFlags & DCX_CACHE)
          {
             pDCE = DceFreeDCE(pDCE, TRUE);
             if (!pDCE) break;
             continue;
          }
       }
       pLE = pDCE->List.Flink;
       pDCE = CONTAINING_RECORD(pLE, DCE, List);
   }
   while (pLE != &LEDce);
}

VOID FASTCALL
DceEmptyCache(VOID)
{
   PDCE pDCE;
   PLIST_ENTRY pLE;
   pLE = LEDce.Flink;
   pDCE = CONTAINING_RECORD(pLE, DCE, List);

   do
   {
      if(!pDCE) break;
      pDCE = DceFreeDCE(pDCE, TRUE);
      if(!pDCE) break;
   }
   while (pLE != &LEDce);
}

VOID FASTCALL
DceResetActiveDCEs(PWINDOW_OBJECT Window)
{
   DCE *pDCE;
   PDC dc;
   PWINDOW_OBJECT CurrentWindow;
   INT DeltaX;
   INT DeltaY;
   PLIST_ENTRY pLE;

   if (NULL == Window)
   {
      return;
   }
   pLE = LEDce.Flink;
   pDCE = CONTAINING_RECORD(pLE, DCE, List);
   if(!pDCE) return; // Another null test!
   do
   {
      if(!pDCE) break;
      if(pLE == &LEDce) break;
      if (0 == (pDCE->DCXFlags & (DCX_DCEEMPTY|DCX_INDESTROY)))
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
               pLE = pDCE->List.Flink;
               pDCE = CONTAINING_RECORD(pLE, DCE, List);
               continue;
            }
         }

         if (!GDIOBJ_ValidateHandle(pDCE->hDC, GDI_OBJECT_TYPE_DC) ||
             (dc = DC_LockDc(pDCE->hDC)) == NULL)
         {
            pLE = pDCE->List.Flink;
            pDCE = CONTAINING_RECORD(pLE, DCE, List);
            continue;
         }
         if (Window == CurrentWindow || IntIsChildWindow(Window, CurrentWindow))
         {
            if (pDCE->DCXFlags & DCX_WINDOW)
            {
               DeltaX = CurrentWindow->Wnd->rcWindow.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->Wnd->rcWindow.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->Wnd->rcWindow.left;
               dc->ptlDCOrig.y = CurrentWindow->Wnd->rcWindow.top;
            }
            else
            {
               DeltaX = CurrentWindow->Wnd->rcClient.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->Wnd->rcClient.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->Wnd->rcClient.left;
               dc->ptlDCOrig.y = CurrentWindow->Wnd->rcClient.top;
            }
            if (NULL != dc->rosdc.hClipRgn)
            {
               NtGdiOffsetRgn(dc->rosdc.hClipRgn, DeltaX, DeltaY);
               CLIPPING_UpdateGCRegion(dc);
            }
            if (NULL != pDCE->hrgnClip)
            {
               NtGdiOffsetRgn(pDCE->hrgnClip, DeltaX, DeltaY);
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
      pLE = pDCE->List.Flink;
      pDCE = CONTAINING_RECORD(pLE, DCE, List);
   }
   while (pLE != &LEDce);
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;
  PLIST_ENTRY pLE;
  HWND Ret = NULL;

  pLE = LEDce.Flink;
  Dce = CONTAINING_RECORD(pLE, DCE, List);
  do
  {
      if (Dce->hDC == hDc)
      {
         if (Dce->DCXFlags & DCX_INDESTROY)
            Ret = NULL;
         else
            Ret = Dce->hwndCurrent;
         break;
      }
      pLE = Dce->List.Flink;
      Dce = CONTAINING_RECORD(pLE, DCE, List);
  }
  while (pLE != &LEDce);
  return Ret;
}

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Window, HDC hDc, BOOL EndPaint)
{
  PDCE dce;
  PLIST_ENTRY pLE;
  INT nRet = 0;
  BOOL Hit = FALSE;

  DPRINT("%p %p\n", Window, hDc);
  pLE = LEDce.Flink;
  dce = CONTAINING_RECORD(pLE, DCE, List);
  do
  {
     if(!dce) break;
     if (dce->hDC == hDc)
     {
        Hit = TRUE;
        break;
     }
     pLE = dce->List.Flink;
     dce = CONTAINING_RECORD(pLE, DCE, List);
  }
  while (pLE != &LEDce );

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

HWND FASTCALL
UserGethWnd( HDC hdc, PWNDOBJ *pwndo)
{
  PWNDGDI pWndgdi;
  PWINDOW_OBJECT Wnd;
  HWND hWnd;

  hWnd = IntWindowFromDC(hdc);

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
     pWndgdi = (WNDGDI *)IntGetProp(Wnd, AtomWndObj);

     if ( pWndgdi && pWndgdi->Hwnd == hWnd )
     {
        if (pwndo) *pwndo = (PWNDOBJ)pWndgdi;
     }
  }
  return hWnd;
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
