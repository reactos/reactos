/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/windc.c
 * PURPOSE:         Window DC management
 * COPYRIGHT:       Copyright 2007 ReactOS Team
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDce);

/* GLOBALS *******************************************************************/

/* NOTE: I think we should store this per window station (including GDI objects) */
/* Answer: No, use the DCE pMonitor to compare with! */

static LIST_ENTRY LEDce;
static INT DCECount = 0; // Count of DCE in system.

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_NORESETATTRS | DCX_LOCKWINDOWUPDATE | \
                              DCX_LAYEREDWIN | DCX_CACHE | DCX_WINDOW | \
                              DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
InitDCEImpl(VOID)
{
    InitializeListHead(&LEDce);
    return STATUS_SUCCESS;
}

//
// This should be moved to dc.c or dcutil.c.
//
HDC FASTCALL
DceCreateDisplayDC(VOID)
{
  UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"DISPLAY");

  co_IntGraphicsCheck(TRUE);

  return IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
}

static
HRGN FASTCALL
DceGetVisRgn(PWND Window, ULONG Flags, HWND hWndChild, ULONG CFlags)
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
DceAllocDCE(PWND Window OPTIONAL, DCE_TYPE Type)
{
  PDCE pDce;

  pDce = ExAllocatePoolWithTag(PagedPool, sizeof(DCE), USERTAG_DCE);
  if(!pDce)
        return NULL;

  pDce->hDC = DceCreateDisplayDC();
  if (!pDce->hDC)
  {
      ExFreePoolWithTag(pDce, USERTAG_DCE);
      return NULL;
  }
  DCECount++;
  TRACE("Alloc DCE's! %d\n",DCECount);
  pDce->hwndCurrent = (Window ? Window->head.h : NULL);
  pDce->pwndOrg  = Window;
  pDce->pwndClip = Window;
  pDce->hrgnClip = NULL;
  pDce->hrgnClipPublic = NULL;
  pDce->hrgnSavedVis = NULL;
  pDce->ppiOwner = NULL;

  InsertTailList(&LEDce, &pDce->List);

  DCU_SetDcUndeletable(pDce->hDC);

  if (Type == DCE_WINDOW_DC || Type == DCE_CLASS_DC) // Window DCE have ownership.
  {
     pDce->ptiOwner = GetW32ThreadInfo();
  }
  else
  {
     TRACE("FREE DCATTR!!!! NOT DCE_WINDOW_DC!!!!! hDC-> %x\n", pDce->hDC);
     GreSetDCOwner(pDce->hDC, GDI_OBJ_HMGR_NONE);
     pDce->ptiOwner = NULL;
  }

  if (Type == DCE_CACHE_DC)
  {
     pDce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
  }
  else
  {
     pDce->DCXFlags = DCX_DCEBUSY;
     if (Window)
     {
        if (Type == DCE_WINDOW_DC)
        {
          if (Window->style & WS_CLIPCHILDREN) pDce->DCXFlags |= DCX_CLIPCHILDREN;
          if (Window->style & WS_CLIPSIBLINGS) pDce->DCXFlags |= DCX_CLIPSIBLINGS;
        }
     }
  }
  return(pDce);
}

static VOID APIENTRY
DceSetDrawable( PWND Window OPTIONAL,
                HDC hDC,
                ULONG Flags,
                BOOL SetClipOrigin)
{
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
      if (Flags & DCX_WINDOW)
      {
         dc->ptlDCOrig.x = Window->rcWindow.left;
         dc->ptlDCOrig.y = Window->rcWindow.top;
      }
      else
      {
         dc->ptlDCOrig.x = Window->rcClient.left;
         dc->ptlDCOrig.y = Window->rcClient.top;
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
      GreDeleteObject(Dce->hrgnClip);
   }

   Dce->hrgnClip = NULL;

   /* Make it dirty so that the vis rgn gets recomputed next time */
   Dce->DCXFlags |= DCX_DCEDIRTY;
}

static INT FASTCALL
DceReleaseDC(DCE* dce, BOOL EndPaint)
{
   if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_INDESTROY | DCX_DCEEMPTY | DCX_DCEBUSY)))
   {
      return 0;
   }

   /* Restore previous visible region */
   if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
         ((dce->DCXFlags & DCX_CACHE) || EndPaint))
   {
      DceDeleteClipRgn(dce);
   }

   if (dce->DCXFlags & DCX_CACHE)
   {
      if (!(dce->DCXFlags & DCX_NORESETATTRS))
      {
         /* Make the DC clean so that SetDCState doesn't try to update the vis rgn */
         IntGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);

         // Clean the DC
         if (!IntGdiCleanDC(dce->hDC)) return 0;

         if (dce->DCXFlags & DCX_DCEDIRTY)
         {
           /* Don't keep around invalidated entries
            * because SetDCState() disables hVisRgn updates
            * by removing dirty bit. */
           dce->hwndCurrent = 0;
           dce->DCXFlags &= DCX_CACHE;
           dce->DCXFlags |= DCX_DCEEMPTY;
         }
      }
      dce->DCXFlags &= ~DCX_DCEBUSY;
      TRACE("Exit!!!!! DCX_CACHE!!!!!!   hDC-> %x \n", dce->hDC);
      if (!GreSetDCOwner(dce->hDC, GDI_OBJ_HMGR_NONE))
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
DceUpdateVisRgn(DCE *Dce, PWND Window, ULONG Flags)
{
   HANDLE hRgnVisible = NULL;
   ULONG DcxFlags;
   PWND DesktopWindow;

   if (Flags & DCX_PARENTCLIP)
   {
      PWND Parent;

      Parent = Window->spwndParent;
      if(!Parent)
      {
         hRgnVisible = NULL;
         goto noparent;
      }

      if (Parent->style & WS_CLIPSIBLINGS)
      {
         DcxFlags = DCX_CLIPSIBLINGS |
                    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
      }
      else
      {
         DcxFlags = Flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
      }
      hRgnVisible = DceGetVisRgn(Parent, DcxFlags, Window->head.h, Flags);
   }
   else if (Window == NULL)
   {
      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if (NULL != DesktopWindow)
      {
         hRgnVisible = IntSysCreateRectRgnIndirect(&DesktopWindow->rcWindow);
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
            GreDeleteObject(hRgnVisible);
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
      GreDeleteObject(hRgnVisible);
   }
}

HDC FASTCALL
UserGetDCEx(PWND Wnd OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
   PWND Parent;
   ULONG DcxFlags;
   DCE* Dce = NULL;
   BOOL UpdateClipOrigin = FALSE;
   HDC hDC = NULL;
   PPROCESSINFO ppi;
   PLIST_ENTRY pLE;

   if (NULL == Wnd)
   {
      Flags &= ~DCX_USESTYLE;
      Flags |= DCX_CACHE;
   }

   if (Flags & (DCX_WINDOW | DCX_PARENTCLIP)) Flags |= DCX_CACHE;

   // When GetDC is called with hWnd nz, DCX_CACHE & _WINDOW are clear w _USESTYLE set.
   if (Flags & DCX_USESTYLE)
   {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);
      if (!(Flags & DCX_WINDOW)) // Not window rectangle
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
               TRACE("We have CLASS!!\n");
            }
         }
/*         else // For Testing!
         {
            ERR("We have POWNER!!\n");
            if (Window->Dce) ERR("We have POWNER with DCE!!\n");
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

   Parent = (Wnd ? Wnd->spwndParent : NULL);

   if (NULL == Wnd || !(Wnd->style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
      Flags |= DCX_CLIPSIBLINGS;
   }

   /* It seems parent clip is ignored when clipping siblings or children */
   if (Flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) Flags &= ~DCX_PARENTCLIP;

   if (Flags & DCX_PARENTCLIP)
   {
      if ((Wnd->style & WS_VISIBLE) &&
          (Parent->style & WS_VISIBLE))
      {
         Flags &= ~DCX_CLIPCHILDREN;
         if (Parent->style & WS_CLIPSIBLINGS)
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
            else if (Dce->hwndCurrent == (Wnd ? Wnd->head.h : NULL) &&
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

      Dce->hwndCurrent = (Wnd ? Wnd->head.h : NULL);
   }
   else // If we are here, we are POWNED or having CLASS.
   {
      KeEnterCriticalRegion();
      pLE = LEDce.Flink;
      Dce = CONTAINING_RECORD(pLE, DCE, List);
      do
      {   // Check for Window handle than HDC match for CLASS.
          if ((Dce->hwndCurrent == Wnd->head.h) ||
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

   if (!GreIsHandleValid(Dce->hDC))
   {
      ERR("FIXME: Got DCE with invalid hDC! 0x%x\n", Dce->hDC);
      Dce->hDC = DceCreateDisplayDC();
      /* FIXME: Handle error */
   }

   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   /*
    * Bump it up! This prevents the random errors in wine dce tests and with
    * proper bits set in DCX_CACHECOMPAREMASK.
    * Reference:
    *   http://www.reactos.org/archives/public/ros-dev/2008-July/010498.html
    *   http://www.reactos.org/archives/public/ros-dev/2008-July/010499.html
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
      ClipRegion = Wnd->hrgnUpdate;
   }

   if (ClipRegion == HRGN_WINDOW)
   {
      if (!(Flags & DCX_WINDOW))
      {
         Dce->hrgnClip = IntSysCreateRectRgnIndirect(&Wnd->rcClient);
      }
      else
      {
         Dce->hrgnClip = IntSysCreateRectRgnIndirect(&Wnd->rcWindow);
      }
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (ClipRegion != NULL)
   {
      if (Dce->hrgnClip != NULL)
      {
         ERR("Should not be called!!\n");
         GreDeleteObject(Dce->hrgnClip);
         Dce->hrgnClip = NULL;
      }
      Dce->hrgnClip = ClipRegion;
   }

   DceSetDrawable(Wnd, Dce->hDC, Flags, UpdateClipOrigin);

   DceUpdateVisRgn(Dce, Wnd, Flags);

   if (Dce->DCXFlags & DCX_CACHE)
   {
      TRACE("ENTER!!!!!! DCX_CACHE!!!!!!   hDC-> %x\n", Dce->hDC);
      // Need to set ownership so Sync dcattr will work.
      GreSetDCOwner(Dce->hDC, GDI_OBJ_HMGR_POWNED);
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

  if (Force &&
      GreGetObjectOwner(pdce->hDC) != GDI_OBJ_HMGR_POWNED)
  {
     TRACE("Change ownership for DCE! -> %x\n" , pdce);
     // NOTE: Windows sets W32PF_OWNDCCLEANUP and moves on.
     if (GreIsHandleValid(pdce->hDC))
     {
         GreSetDCOwner(pdce->hDC, GDI_OBJ_HMGR_POWNED);
     }
     else
     {
         ERR("Attempted to change ownership of an DCEhDC 0x%x currently being destroyed!!!\n",pdce->hDC);
         Hit = TRUE;
     }
  }
  else
  {
     if (GreGetObjectOwner(pdce->hDC) == GDI_OBJ_HMGR_PUBLIC)
        GreSetDCOwner(pdce->hDC, GDI_OBJ_HMGR_POWNED);
  }

  if (!Hit) IntGdiDeleteDC(pdce->hDC, TRUE);

  if (pdce->hrgnClip && !(pdce->DCXFlags & DCX_KEEPCLIPRGN))
  {
      GreDeleteObject(pdce->hrgnClip);
      pdce->hrgnClip = NULL;
  }

  RemoveEntryList(&pdce->List);

  if (IsListEmpty(&pdce->List))
  {
      ERR("List is Empty! DCE! -> %x\n" , pdce);
      return NULL;
  }

  ExFreePoolWithTag(pdce, USERTAG_DCE);

  DCECount--;
  TRACE("Freed DCE's! %d \n", DCECount);

  return ret;
}

/***********************************************************************
 *           DceFreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void FASTCALL
DceFreeWindowDCE(PWND Window)
{
  PDCE pDCE;
  PLIST_ENTRY pLE;

  if (DCECount <= 0)
  {
     ERR("FreeWindowDCE No Entry! %d\n",DCECount);
     return;
  }

  pLE = LEDce.Flink;
  pDCE = CONTAINING_RECORD(pLE, DCE, List);
  do
  {
     if (!pDCE)
     {
        ERR("FreeWindowDCE No DCE Pointer!\n");
        break;
     }
     if (IsListEmpty(&pDCE->List))
     {
        ERR("FreeWindowDCE List is Empty!!!!\n");
        break;
     }
     if ( pDCE->hwndCurrent == Window->head.h &&
          !(pDCE->DCXFlags & DCX_DCEEMPTY) )
     {
        if (!(pDCE->DCXFlags & DCX_CACHE)) /* Owned or Class DCE */
        {
           if (Window->pcls->style & CS_CLASSDC) /* Test Class first */
           {
              if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) /* Class DCE */
                 DceDeleteClipRgn(pDCE);
              // Update and reset Vis Rgn and clear the dirty bit.
              // Should release VisRgn than reset it to default.
              DceUpdateVisRgn(pDCE, Window, pDCE->DCXFlags);
              pDCE->DCXFlags = DCX_DCEEMPTY|DCX_CACHE;
              pDCE->hwndCurrent = 0;

              TRACE("POWNED DCE going Cheap!! DCX_CACHE!! hDC-> %x \n", pDCE->hDC);
              if (!GreSetDCOwner( pDCE->hDC, GDI_OBJ_HMGR_NONE))
              {
                  ERR("Fail Owner Switch hDC-> %x \n", pDCE->hDC);
                  break;
              }
              /* Do not change owner so thread can clean up! */
           }
           else if (Window->pcls->style & CS_OWNDC) /* Owned DCE */
           {
              pDCE = DceFreeDCE(pDCE, FALSE);
              if (!pDCE) break;
              continue;
           }
           else
           {
              ERR("Not POWNED or CLASSDC hwndCurrent -> %x \n", pDCE->hwndCurrent);
              // ASSERT(FALSE); /* bug 5320 */
           }
        }
        else
        {
           if (pDCE->DCXFlags & DCX_DCEBUSY) /* Shared cache DCE */
           {
              /* FIXME: AFAICS we are doing the right thing here so
               * this should be a TRACE. But this is best left as an ERR
               * because the 'application error' is likely to come from
               * another part of Wine (i.e. it's our fault after all).
               * We should change this to TRACE when ReactOS is more stable
               * (for 1.0?).
               */
              ERR("[%p] GetDC() without ReleaseDC()!\n", Window->head.h);
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
DceResetActiveDCEs(PWND Window)
{
   DCE *pDCE;
   PDC dc;
   PWND CurrentWindow;
   INT DeltaX;
   INT DeltaY;
   PLIST_ENTRY pLE;

   if (NULL == Window)
   {
      return;
   }
   pLE = LEDce.Flink;
   pDCE = CONTAINING_RECORD(pLE, DCE, List);

   do
   {
      if(!pDCE) break;
      if(pLE == &LEDce) break;
      if (0 == (pDCE->DCXFlags & (DCX_DCEEMPTY|DCX_INDESTROY)))
      {
         if (Window->head.h == pDCE->hwndCurrent)
         {
            CurrentWindow = Window;
         }
         else
         {
            if (!pDCE->hwndCurrent)
               CurrentWindow = NULL;
            else 
               CurrentWindow = UserGetWindowObject(pDCE->hwndCurrent);
            if (NULL == CurrentWindow)
            {
               pLE = pDCE->List.Flink;
               pDCE = CONTAINING_RECORD(pLE, DCE, List);
               continue;
            }
         }

         if (!GreIsHandleValid(pDCE->hDC) ||
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
               DeltaX = CurrentWindow->rcWindow.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->rcWindow.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->rcWindow.left;
               dc->ptlDCOrig.y = CurrentWindow->rcWindow.top;
            }
            else
            {
               DeltaX = CurrentWindow->rcClient.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->rcClient.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->rcClient.left;
               dc->ptlDCOrig.y = CurrentWindow->rcClient.top;
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

         if (Window->head.h != pDCE->hwndCurrent)
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
UserReleaseDC(PWND Window, HDC hDc, BOOL EndPaint)
{
  PDCE dce;
  PLIST_ENTRY pLE;
  INT nRet = 0;
  BOOL Hit = FALSE;

  TRACE("%p %p\n", Window, hDc);
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
UserGetWindowDC(PWND Wnd)
{
  return UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HWND FASTCALL
UserGethWnd( HDC hdc, PWNDOBJ *pwndo)
{
  PWNDGDI pWndgdi;
  PWND Wnd;
  HWND hWnd;
  PPROPERTY pprop;

  hWnd = IntWindowFromDC(hdc);

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
     pprop = IntGetProp(Wnd, AtomWndObj);

     pWndgdi = (WNDGDI *)pprop->Data;

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
  PWND Wnd=NULL;
  DECLARE_RETURN(HDC);

  TRACE("Enter NtUserGetDCEx\n");
  UserEnterExclusive();

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
      RETURN(NULL);
  }
  RETURN( UserGetDCEx(Wnd, ClipRegion, Flags));

CLEANUP:
  TRACE("Leave NtUserGetDCEx, ret=%i\n",_ret_);
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
 TRACE("NtUGetDC -> %x:%x\n", hWnd, !hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE );

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
