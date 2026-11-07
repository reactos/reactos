/*
 * PROJECT:          ReactOS Win32k subsystem
 * LICENSE:          GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:          Layered window support
 * COPYRIGHT:        Copyright 2015 James Tabor <james.tabor@reactos.org>
 *                   Copyright 2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

typedef struct _LRD_PROP
{
   HBITMAP  hbmSurface, hbmBackground, hbmComp;
   HBITMAP  hbmOld, hbmBackgroundOld, hbmCompOld;
   HDC      hdcSurface, hdcBackground, hdcComp;
   BOOL     BackgroundValid, BackgroundClean;
   BOOL     IsLayered;
   SIZE     Size;
   COLORREF crKey;
   DWORD    dwFlags;
   BLENDFUNCTION Blend;
} LRD_PROP, *PLRD_PROP;

static
PLRD_PROP
IntLayeredGetProp(
   _In_ PWND pWnd,
   _In_ BOOL Create)
{
   PLRD_PROP LrdProp = UserGetProp(pWnd, AtomLayer, TRUE);
   if (!LrdProp && Create)
   {
      LrdProp = ExAllocatePoolZero(PagedPool, sizeof(LRD_PROP), USERTAG_REDIRECT);
      if (!LrdProp)
      {
         return NULL;
      }
      UserSetProp(pWnd, AtomLayer, (HANDLE)LrdProp, TRUE);
   }
   return LrdProp;
}

BOOL
FASTCALL
IntLayeredGetState(
   _In_ PWND pWnd)
{
   PLRD_PROP pLrdProp = IntLayeredGetProp(pWnd, FALSE);
   if (pLrdProp)
   {
      /* Get current layered state */
      return pLrdProp->IsLayered;
   }
   return FALSE;
}

BOOL
FASTCALL
IntLayeredSetState(
   _In_ PWND pWnd,
   _In_ BOOL Set)
{
   PLRD_PROP pLrdProp = IntLayeredGetProp(pWnd, FALSE);
   if (pLrdProp)
   {
      /* Set new layered state */
      pLrdProp->IsLayered = Set;
      return TRUE;
   }
   return FALSE;
}

/**
 * @brief
 *
 * Checks whether specified window is layered and currently composited.
 **/
BOOL
FASTCALL
IntIsLayeredWindow(
   _In_ PWND pWnd)
{
   PLRD_PROP LrdProp;

   if (pWnd->ExStyle & WS_EX_LAYERED)
   {
      LrdProp = IntLayeredGetProp(pWnd, FALSE);
      return LrdProp && LrdProp->hdcSurface;
   }
   return FALSE;
}

static
HDC
IntLayeredMakeMemoryDC(
   _In_ HDC ref,
   _In_ INT cx,
   _In_ INT cy,
   _Out_ HBITMAP *phbm,
   _Out_ HBITMAP *phbmOld)
{
   HBITMAP hbm;
   HDC hdc;

   hbm = NtGdiCreateCompatibleBitmap(ref, cx, cy);
   if (!hbm) return NULL;

   hdc = NtGdiCreateCompatibleDC(ref);
   if (!hdc)
   {
      GreDeleteObject(hbm);
      return NULL;
   }
   GreSetDCOwner(hdc, GDI_OBJ_HMGR_PUBLIC);

   *phbmOld = NtGdiSelectBitmap(hdc, hbm);
   *phbm = hbm;
   return hdc;
}

static
VOID
IntLayeredFreeMemoryDC(
   _Inout_ HDC *phdc,
   _Inout_ HBITMAP *phbm,
   _Inout_ HBITMAP *phbmOld)
{
   if (*phdc)
   {
      if (*phbmOld)
         NtGdiSelectBitmap(*phdc, *phbmOld);
      GreSetDCOwner(*phdc, GDI_OBJ_HMGR_POWNED);
      IntGdiDeleteDC(*phdc, FALSE);
      *phdc = NULL;
   }
   if (*phbm)
   {
      GreDeleteObject(*phbm);
      *phbm = NULL;
   }
   *phbmOld = NULL;
}

/**
 * @brief
 *
 * Performs the cleanup of the previous or no longer needed bitmap surface.
 **/
static
VOID
IntLayeredFreeSurface(
   _In_ PLRD_PROP LrdProp)
{
   if (LrdProp->hdcSurface)
   {
      if (LrdProp->hbmOld)
         NtGdiSelectBitmap(LrdProp->hdcSurface, LrdProp->hbmOld);
      GreSetDCOwner(LrdProp->hdcSurface, GDI_OBJ_HMGR_POWNED);
      IntGdiDeleteDC(LrdProp->hdcSurface, FALSE);
      LrdProp->hdcSurface = NULL;
   }
   if (LrdProp->hbmSurface)
   {
      GreDeleteObject(LrdProp->hbmSurface);
      LrdProp->hbmSurface = NULL;
   }
   LrdProp->hbmOld = NULL;
   if (LrdProp->hdcBackground)
      IntLayeredFreeMemoryDC(&LrdProp->hdcBackground,  &LrdProp->hbmBackground,  &LrdProp->hbmBackgroundOld);
   if (LrdProp->hdcComp)
      IntLayeredFreeMemoryDC(&LrdProp->hdcComp, &LrdProp->hbmComp, &LrdProp->hbmCompOld);
   LrdProp->BackgroundValid = FALSE;
   LrdProp->BackgroundClean = FALSE;
}

/**
 * @brief
 *
 * Performs freeing the data associated with the layered window.
 * Called from co_UserFreeWindow and when WS_EX_LAYERED is removed.
 **/
VOID
FASTCALL
IntLayeredFreeWindow(
   _In_ PWND pWnd)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   if (!LrdProp) return;
   IntLayeredFreeSurface(LrdProp);
   UserRemoveProp(pWnd, AtomLayer, TRUE);
   ExFreePoolWithTag(LrdProp, USERTAG_REDIRECT);
}

/**
 * @brief
 *
 * Captures the background bits to the backbuffer of the layered window,
 * for the specified region.
 **/
static
VOID
IntLayeredCaptureBackground(
   _In_ PWND pWnd,
   _In_ HRGN hrgn)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   RECTL RgnBox;
   HRGN hcopy;
   INT type;
   HDC hdc;

   /* Validate parameters */
   if (!LrdProp ||
       !LrdProp->hdcBackground ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Get region rectangle */
   type = IntGdiGetRgnBox(hrgn, &RgnBox);
   if (type == ERROR || type == NULLREGION) return;

   /* Check whether region is still valid (intersects with window) at this point */
   if (!RECTL_bIntersectRect(&RgnBox, &RgnBox, &pWnd->rcWindow)) return;

   /* Create a copy of region */
   hcopy = NtGdiCreateRectRgn(0, 0, 0, 0);
   if (!hcopy) return;
   NtGdiCombineRgn(hcopy, hrgn, NULL, RGN_COPY);

   /*
    * Get the DC of the underlying intersecting region of the desktop.
    * DC takes ownership of hcopy (no KEEPCLIPRGN)
    */
   hdc = UserGetDCEx(NULL, hcopy, DCX_USESTYLE | DCX_WINDOW | DCX_INTERSECTRGN);
   if (hdc)
   {
      /* Copy the background bits to the buffer */
      if (!NtGdiBitBlt(LrdProp->hdcBackground,
                       RgnBox.left - pWnd->rcWindow.left,
                       RgnBox.top - pWnd->rcWindow.top,
                       RgnBox.right - RgnBox.left,
                       RgnBox.bottom - RgnBox.top,
                       hdc,
                       RgnBox.left,
                       RgnBox.top,
                       SRCCOPY,
                       CLR_INVALID,
                       0))
      {
         ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
         UserReleaseDC(NULL, hdc, FALSE);
         return;
      }

      UserReleaseDC(NULL, hdc, FALSE);
   }
   else
      GreDeleteObject(hcopy);

   LrdProp->BackgroundValid = TRUE;
   LrdProp->BackgroundClean = FALSE;
}

/**
 * @brief
 *
 * Restores previously captured background bits from backbuffer back to the screeen,
 * removing the current contents of the layered window.
 **/
static
VOID
IntLayeredRestoreBackground(
   _In_ PWND pWnd)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   HRGN hrgn;
   HDC hdc;

   /* Validate parameters */
   if (!LrdProp ||
       !LrdProp->hdcBackground ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Create window region */
   hrgn = GreCreateRectRgnIndirect(&pWnd->rcWindow);
   if (!hrgn) return;

   /* Capture background if it's not valid yet */
   if (!LrdProp->BackgroundValid) IntLayeredCaptureBackground(pWnd, hrgn);

   /* Get the DC of the underlying intersecting region of the desktop */
   hdc = UserGetDCEx(NULL, hrgn, DCX_CACHE | DCX_WINDOW | DCX_INTERSECTRGN);
   if (hdc)
   {
      /* Copy the background buffer to the screen */
      if (!NtGdiBitBlt(hdc,
                       pWnd->rcWindow.left,
                       pWnd->rcWindow.top,
                       LrdProp->Size.cx,
                       LrdProp->Size.cy,
                       LrdProp->hdcBackground,
                       0,
                       0,
                       SRCCOPY,
                       CLR_INVALID,
                       0))
      {
         ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
         UserReleaseDC(NULL, hdc, FALSE);
         return;
      }

      UserReleaseDC(NULL, hdc, FALSE);
   }
   else
      GreDeleteObject(hrgn);

   LrdProp->BackgroundValid = FALSE;
   LrdProp->BackgroundClean = TRUE;
}

/**
 * @brief
 *
 * Performs an initialization of the new bitmap surface for the layered window,
 * or simply copies the initial source bits, in case the shape of the window
 * is not changing.
 **/
static
BOOL
IntLayeredSetSurface(
   _In_ PWND pWnd,
   _In_ PLRD_PROP LrdProp,
   _Inout_ UPDATELAYEREDWINDOWINFO *info,
   _In_ INT cx,
   _In_ INT cy)
{
   HBITMAP hBitmap;
   HDC hdc;

   /* Get custom coordinate, if any */
   INT sx = info->pptSrc ? info->pptSrc->x : 0;
   INT sy = info->pptSrc ? info->pptSrc->y : 0;

   /*
    * Re-create bitmap surface if it does not exist yet (first creation)
    * or is outdated (mismatching with the current requested dimensions).
    */
   if (!LrdProp->hdcSurface || LrdProp->Size.cx != cx || LrdProp->Size.cy != cy)
   {
      /* Restore background, to properly repaint the window */
      IntLayeredRestoreBackground(pWnd);

      /* Free previous surface */
      IntLayeredFreeSurface(LrdProp);

      /* Create new surface */
      hdc = NtGdiCreateCompatibleDC(info->hdcSrc);
      hBitmap = NtGdiCreateCompatibleBitmap(info->hdcSrc, cx, cy);
      if (!hBitmap || !hdc)
      {
         ERR("Failed to create bitmap/dc\n");

         /* Failed, cleanup */
         if (hBitmap)
            GreDeleteObject(hBitmap);
         if (hdc)
            IntGdiDeleteDC(hdc, FALSE);
         return FALSE;
      }
      GreSetDCOwner(hdc, GDI_OBJ_HMGR_PUBLIC);

      LrdProp->hbmOld = NtGdiSelectBitmap(hdc, hBitmap);
      LrdProp->hbmSurface = hBitmap;
      LrdProp->hdcSurface = hdc;

      /* Create background DC */
      LrdProp->hdcBackground = IntLayeredMakeMemoryDC(info->hdcSrc, cx, cy, &LrdProp->hbmBackground,  &LrdProp->hbmBackgroundOld);
      if (!LrdProp->hdcBackground)
      {
         ERR("Failed to create background dc\n");

         /* Failed, cleanup */
         IntLayeredFreeSurface(LrdProp);
         return FALSE;
      }

      /* Create composition DC */
      LrdProp->hdcComp = IntLayeredMakeMemoryDC(info->hdcSrc, cx, cy, &LrdProp->hbmComp, &LrdProp->hbmCompOld);
      if (!LrdProp->hdcComp)
      {
         ERR("Failed to create composition dc\n");

         /* Failed, cleanup */
         IntLayeredFreeSurface(LrdProp);
         return FALSE;
      }

      LrdProp->Size.cx = cx;
      LrdProp->Size.cy = cy;
   }

   /* Copy the initial source buffer to the surface */
   if (!NtGdiBitBlt(LrdProp->hdcSurface,
                    0,
                    0,
                    cx,
                    cy,
                    info->hdcSrc,
                    sx,
                    sy,
                    SRCCOPY,
                    CLR_INVALID,
                    0))
   {
      ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());

      /* Failed, cleanup */
      IntLayeredFreeSurface(LrdProp);
      return FALSE;
   }

   LrdProp->dwFlags = info->dwFlags;
   LrdProp->crKey = info->crKey;

   if (info->dwFlags & ULW_OPAQUE)
   {
      /* No alpha blending */
      LrdProp->Blend.BlendOp = AC_SRC_OVER;
      LrdProp->Blend.BlendFlags = 0;
      LrdProp->Blend.SourceConstantAlpha = 255;
      LrdProp->Blend.AlphaFormat = 0;
   }
   else
      LrdProp->Blend = *info->pblend;

   return TRUE;
}

/**
 * @brief
 *
 * Performs an offscreen or simple compositing of the layered window surface
 * in the specified rectangle.
 **/
static
VOID
IntLayeredComposite(
   _In_ PWND pWnd,
   _In_ const RECTL *pRect)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   HRGN hrgn;
   HDC hdc;

   /* Validate parameters */
   if (!LrdProp ||
       !LrdProp->hdcSurface ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Check whether the window rectangle is still valid (not empty) at this point */
   if (RECTL_bIsEmptyRect(pRect)) return;

   if (!LrdProp->BackgroundValid)
   {
      /* Create window region */
      hrgn = GreCreateRectRgnIndirect(pRect);
      if (!hrgn) return;

      /* Perform an initial capture */
      IntLayeredCaptureBackground(pWnd, hrgn);
      GreDeleteObject(hrgn);
   }

   /* Get the DC of the layered window */
   hdc = UserGetDCEx(pWnd, NULL,
                     DCX_WINDOW | DCX_USESTYLE | DCX_LAYEREDWIN);
   if (hdc)
   {
      /* Get the current window coordinates */
      INT Left = pRect->left - pWnd->rcWindow.left;
      INT Top = pRect->top - pWnd->rcWindow.top;
      INT Width = pRect->right - pRect->left;
      INT Height = pRect->bottom - pRect->top;

      /* Do we need to perform an offscreen backbuffer composition? */
      BOOL Offscreen = LrdProp->hdcBackground && LrdProp->hdcComp;

      /* Perform the compositing operation */
      if (Offscreen)
      {
         if (!NtGdiBitBlt(LrdProp->hdcComp,
                          Left,
                          Top,
                          Width,
                          Height,
                          LrdProp->hdcBackground,
                          Left,
                          Top,
                          SRCCOPY,
                          CLR_INVALID,
                          0))
         {
            ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
            UserReleaseDC(pWnd, hdc, FALSE);
            return;
         }

         if (LrdProp->dwFlags & ULW_ALPHA)
         {
            if (!NtGdiAlphaBlend(LrdProp->hdcComp,
                                 Left,
                                 Top,
                                 Width,
                                 Height,
                                 LrdProp->hdcSurface,
                                 Left,
                                 Top,
                                 Width,
                                 Height,
                                 LrdProp->Blend,
                                 0))
            {
               ERR("NtGdiAlphaBlend failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }
         else if (LrdProp->dwFlags & ULW_COLORKEY)
         {
            if (!NtGdiTransparentBlt(LrdProp->hdcComp,
                                     Left,
                                     Top,
                                     Width,
                                     Height,
                                     LrdProp->hdcSurface,
                                     Left,
                                     Top,
                                     Width,
                                     Height,
                                     LrdProp->crKey))
            {
               ERR("NtGdiTransparentBlt failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }
         else /* ULW_OPAQUE and other cases */
         {
            if (!NtGdiBitBlt(LrdProp->hdcComp,
                             Left,
                             Top,
                             Width,
                             Height,
                             LrdProp->hdcSurface,
                             Left,
                             Top,
                             SRCCOPY,
                             CLR_INVALID,
                             0))
            {
               ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }

         if (!NtGdiBitBlt(hdc,
                          Left,
                          Top,
                          Width,
                          Height,
                          LrdProp->hdcComp,
                          Left,
                          Top,
                          SRCCOPY,
                          CLR_INVALID,
                          0))
         {
            ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
            UserReleaseDC(pWnd, hdc, FALSE);
            return;
         }
      }
      else /* Simple composition, no backbuffer */
      {
         if (LrdProp->dwFlags & ULW_ALPHA)
         {
            if (!NtGdiAlphaBlend(hdc,
                                 Left,
                                 Top,
                                 Width,
                                 Height,
                                 LrdProp->hdcSurface,
                                 Left,
                                 Top,
                                 Width,
                                 Height,
                                 LrdProp->Blend,
                                 0))
            {
               ERR("NtGdiAlphaBlend failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }
         else if (LrdProp->dwFlags & ULW_COLORKEY)
         {
            if (!NtGdiTransparentBlt(hdc,
                                     Left,
                                     Top,
                                     Width,
                                     Height,
                                     LrdProp->hdcSurface,
                                     Left,
                                     Top,
                                     Width,
                                     Height,
                                     LrdProp->crKey))
            {
               ERR("NtGdiTransparentBlt failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }
         else /* ULW_OPAQUE and other cases */
         {
            if (!NtGdiBitBlt(hdc,
                             Left,
                             Top,
                             Width,
                             Height,
                             LrdProp->hdcSurface,
                             Left,
                             Top,
                             SRCCOPY,
                             CLR_INVALID,
                             0))
            {
               ERR("NtGdiBitBlt failed, last error %d\n", EngGetLastError());
               UserReleaseDC(pWnd, hdc, FALSE);
               return;
            }
         }
      }

      UserReleaseDC(pWnd, hdc, FALSE);
   }
}

/**
 * @brief
 *
 * Performs a pre-repaint restoring of the background bits back to the screen,
 * to wipe the background after the previous repaint operation.
 **/
VOID
FASTCALL
IntLayeredPreUpdate(
   _In_ PWND pWnd)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);

   /* Validate parameters */
   if (!LrdProp ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Restore background */
   IntLayeredRestoreBackground(pWnd);
}

/**
 * @brief
 *
 * Performs a post-repaint capturing of the background bits from the screen,
 * to get the new background bits from the screen before the next repaint operation.
 **/
VOID
FASTCALL
IntLayeredPostUpdate(
   _In_ PWND pWnd)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   HRGN hrgn;

   /* Validate parameters */
   if (!LrdProp ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Create window region */
   hrgn = GreCreateRectRgnIndirect(&pWnd->rcWindow);
   if (!hrgn) return;

   /* Capture the current background to the back buffer */
   IntLayeredCaptureBackground(pWnd, hrgn);
   GreDeleteObject(hrgn);
}

/**
 * @brief
 *
 * Performs a general repaint of the layered window for the specifed rectangle, if any.
 * If prcDirty parameter is not specified, the whole layered window is repainted.
 **/
VOID
FASTCALL
IntLayeredRefresh(
   _In_ PWND pWnd,
   _In_opt_ const RECTL *prcDirty)
{
   PLRD_PROP LrdProp = IntLayeredGetProp(pWnd, FALSE);
   RECTL Rect;

   /* Validate parameters */
   if (!LrdProp ||
       !LrdProp->hdcSurface ||
       !IntIsLayeredWindow(pWnd) ||
       (pWnd->style & WS_MINIMIZE) ||
       !(pWnd->style & WS_VISIBLE))
   {
      /* Nothing to do */
      return;
   }

   /* Intersect with a dirty rectangle, if any */
   Rect = pWnd->rcWindow;
   if (prcDirty && !RECTL_bIntersectRect(&Rect, &Rect, prcDirty))
   {
      return;
   }

   /* Composite layered window */
   IntLayeredComposite(pWnd, &Rect);
}

BOOL
FASTCALL
IntSetLayeredWindowAttributes(
   _In_ PWND pWnd,
   _In_ COLORREF crKey,
   _In_ BYTE bAlpha,
   _In_ DWORD dwFlags)
{
   BLENDFUNCTION blend =  { AC_SRC_OVER, 0, bAlpha, 0 };
   UPDATELAYEREDWINDOWINFO info;
   SIZE size;
   BOOL ret;
   HDC hdc;

   /* Get size of the window rectangle */
   size.cx = pWnd->rcWindow.right - pWnd->rcWindow.left;
   size.cy = pWnd->rcWindow.bottom - pWnd->rcWindow.top;

   /* Get window DC */
   hdc = UserGetWindowDC(pWnd);
   if (!hdc)
   {
      return FALSE;
   }

   info.cbSize   = sizeof(info);
   info.hdcDst   = NULL;
   info.pptDst   = NULL;
   info.psize    = &size;
   info.hdcSrc   = hdc;
   info.pptSrc   = NULL;
   info.crKey    = crKey;
   info.pblend   = &blend;
   info.dwFlags  = dwFlags;
   info.prcDirty = NULL;

   /* Repaint layered window */
   ret = IntUpdateLayeredWindowI(pWnd, &info);
   UserReleaseDC(pWnd, hdc, FALSE);

   /* Initialize the same window only once */
   IntLayeredSetState(pWnd, TRUE);
   return ret;
}

BOOL
FASTCALL
IntUpdateLayeredWindowI(
   _In_ PWND pWnd,
   _Inout_ UPDATELAYEREDWINDOWINFO *info)
{
   PWND Parent;
   RECT Window, Client;
   SIZE Offset;
   DWORD flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
   INT cx, cy;
   RECTL rcDirty;
   BOOL PosChanged;
   PLRD_PROP LrdProp;

   /* Get client and window rectangles */
   Window = pWnd->rcWindow;
   Client = pWnd->rcClient;

   /* Offset to the parent coordinates if we have parent */
   Parent = pWnd->spwndParent;
   if (pWnd->style & WS_CHILD && Parent)
   {
      RECTL_vOffsetRect(&Window, - Parent->rcClient.left, - Parent->rcClient.top);
      RECTL_vOffsetRect(&Client, - Parent->rcClient.left, - Parent->rcClient.top);
   }

   /* Offset to the custom coordinate, if any */
   if (info->pptDst)
   {
      Offset.cx = info->pptDst->x - Window.left;
      Offset.cy = info->pptDst->y - Window.top;
      RECTL_vOffsetRect(&Client, Offset.cx, Offset.cy);
      RECTL_vOffsetRect(&Window, Offset.cx, Offset.cy);
      flags &= ~SWP_NOMOVE;
   }

   /* Offset to the custom size, if any */
   if (info->psize)
   {
      Offset.cx = info->psize->cx - (Window.right  - Window.left);
      Offset.cy = info->psize->cy - (Window.bottom - Window.top);
      if (info->psize->cx <= 0 || info->psize->cy <= 0)
      {
          ERR("Update Layered Invalid Parameters\n");
          EngSetLastError( ERROR_INVALID_PARAMETER );
          return FALSE;
      }
      if ((info->dwFlags & ULW_EX_NORESIZE) && (Offset.cx || Offset.cy))
      {
          ERR("Wrong Size\n");
          EngSetLastError(ERROR_INCORRECT_SIZE);
          return FALSE;
      }
      Client.right  += Offset.cx;
      Client.bottom += Offset.cy;
      Window.right  += Offset.cx;
      Window.bottom += Offset.cy;
      flags &= ~SWP_NOSIZE;
   }

   /* Get window dimensions */
   cx = Window.right - Window.left;
   cy = Window.bottom - Window.top;

   /*
    * Check whether the window position or size has been changed.
    * Pre/Post callbacks depend on this.
    */
   PosChanged = !IntEqualRect(&Window, &pWnd->rcWindow);

   /* Get layered window property */
   LrdProp = IntLayeredGetProp(pWnd, TRUE);
   if (!LrdProp)
   {
      /* No property */
      ERR("No layered property\n");
      return FALSE;
   }

   /* Set the current surface */
   if (info->hdcSrc && !IntLayeredSetSurface(pWnd, LrdProp, info, cx, cy))
   {
      /* Failed */
      ERR("IntLayeredSetSurface() failed\n");
      return FALSE;
   }

   if (PosChanged)
   {
      /* Update window position */
      co_WinPosSetWindowPos(pWnd, NULL, Window.left, Window.top, cx, cy, flags);
   }

   /* Repaint layered window */
   if (info->prcDirty)
   {
      /* We got a dirty rectangle */
      rcDirty = *info->prcDirty;
      RECTL_vOffsetRect(&rcDirty, pWnd->rcWindow.left, pWnd->rcWindow.top);
      IntLayeredRefresh(pWnd, &rcDirty);
   }
   else
   {
      /* No dirty */
      IntLayeredRefresh(pWnd, NULL);
   }

   return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetLayeredWindowAttributes(
   _In_ HWND hwnd,
   _Out_opt_ COLORREF *pcrKey,
   _Out_opt_ BYTE *pbAlpha,
   _Out_opt_ DWORD *pdwFlags)
{
   PLRD_PROP pLrdProp;
   PWND pWnd;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserGetLayeredWindowAttributes\n");
   UserEnterExclusive();

   if (!(pWnd = UserGetWindowObject(hwnd)) ||
       !(pWnd->ExStyle & WS_EX_LAYERED))
   {
      ERR("Not a Layered Window!\n");
      goto Exit;
   }

   if (!IntLayeredGetState(pWnd))
   {
      ERR("Not initialized\n");
      goto Exit;
   }

   pLrdProp = IntLayeredGetProp(pWnd, FALSE);
   if (!pLrdProp)
   {
      ERR("No layered prop\n");
      goto Exit;
   }

   _SEH2_TRY
   {
      if (pcrKey)
      {
          ProbeForWrite(pcrKey, sizeof(*pcrKey), 1);
          *pcrKey = pLrdProp->crKey;
      }
      if (pbAlpha)
      {
          ProbeForWrite(pbAlpha, sizeof(*pbAlpha), 1);
          *pbAlpha = pLrdProp->Blend.SourceConstantAlpha;
      }
      if (pdwFlags)
      {
          ProbeForWrite(pdwFlags, sizeof(*pdwFlags), 1);
          *pdwFlags = pLrdProp->dwFlags;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(goto Exit);
   }
   _SEH2_END;

   Ret = TRUE;

Exit:
   TRACE("Leave NtUserGetLayeredWindowAttributes, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserSetLayeredWindowAttributes(
   _In_ HWND hwnd,
   _In_ COLORREF crKey,
   _In_ BYTE bAlpha,
   _In_ DWORD dwFlags)
{
   PWND pWnd;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserSetLayeredWindowAttributes\n");
   UserEnterExclusive();

   if (!(pWnd = UserGetWindowObject(hwnd)) ||
       !(pWnd->ExStyle & WS_EX_LAYERED) )
   {
      ERR("Not a Layered Window!\n");
      goto Exit;
   }

   Ret = IntSetLayeredWindowAttributes(pWnd, crKey, bAlpha, dwFlags);
Exit:
   TRACE("Leave NtUserSetLayeredWindowAttributes, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserUpdateLayeredWindow(
   _In_ HWND hwnd,
   _In_opt_ HDC hdcDst,
   _In_opt_ POINT *pptDst,
   _In_opt_ SIZE *psize,
   _In_opt_ HDC hdcSrc,
   _In_opt_ POINT *pptSrc,
   _In_ COLORREF crKey,
   _In_opt_ BLENDFUNCTION *pblend,
   _In_ DWORD dwFlags,
   _In_opt_ RECT *prcDirty)
{
   UPDATELAYEREDWINDOWINFO info;
   POINT Dst, Src; 
   SIZE Size;
   RECT Dirty;
   BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };
   PWND pWnd;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserUpdateLayeredWindow\n");
   UserEnterExclusive();

   if (!(pWnd = UserGetWindowObject(hwnd)))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit;
   }

   _SEH2_TRY
   {
      if (pptDst)
      {
         ProbeForRead(pptDst, sizeof(POINT), 1);
         Dst = *pptDst;
      }
      if (pptSrc)
      {
         ProbeForRead(pptSrc, sizeof(POINT), 1);
         Src = *pptSrc;
      }
      if (psize)
      {
         ProbeForRead(psize, sizeof(SIZE), 1);
         Size = *psize;
      }
      if (pblend)
      {
         ProbeForRead(pblend, sizeof(BLENDFUNCTION), 1);
         blend = *pblend;
      }
      if (prcDirty)
      {
         ProbeForRead(prcDirty, sizeof(RECT), 1);
         Dirty = *prcDirty;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(goto Exit);
   }
   _SEH2_END;

   if (IntLayeredGetState(pWnd) ||
       dwFlags & ~(ULW_COLORKEY | ULW_ALPHA | ULW_OPAQUE | ULW_EX_NORESIZE) ||
       !(pWnd->ExStyle & WS_EX_LAYERED))
   {
      ERR("Layered Window Invalid Parameters\n");
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit;
   }

   info.cbSize   = sizeof(info);
   info.hdcDst   = hdcDst;
   info.pptDst   = pptDst ? &Dst : NULL;
   info.psize    = psize ? &Size : NULL;
   info.hdcSrc   = hdcSrc;
   info.pptSrc   = pptSrc ? &Src : NULL;
   info.crKey    = crKey;
   info.pblend   = &blend;
   info.dwFlags  = dwFlags;
   info.prcDirty = prcDirty ? &Dirty : NULL;
   Ret = IntUpdateLayeredWindowI(pWnd, &info);
Exit:
   TRACE("Leave NtUserUpdateLayeredWindow, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/* EOF */
