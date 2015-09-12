/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Layered window support
 * FILE:             win32ss/user/ntuser/layered.c
 * PROGRAMER:
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);


typedef struct _LRD_PROP
{ 
   COLORREF Key;
   BYTE     Alpha;
   BYTE     is_Layered;
   BYTE     NoUsed[2];
   DWORD    Flags;
} LRD_PROP, *PLRD_PROP;

BOOL FASTCALL
GetLayeredStatus(PWND pWnd)
{
   PLRD_PROP pLrdProp = UserGetProp(pWnd, AtomLayer);
   if (pLrdProp)
   {
      return pLrdProp->is_Layered;
   }
   return FALSE;
}

BOOL FASTCALL
SetLayeredStatus(PWND pWnd, BYTE set)
{
   PLRD_PROP pLrdProp = UserGetProp(pWnd, AtomLayer);
   if (pLrdProp)
   {
      pLrdProp->is_Layered = set;
      return TRUE;
   }
   return FALSE;
}

BOOL FASTCALL
IntSetLayeredWindowAttributes(PWND pWnd,
                              COLORREF crKey,
                              BYTE bAlpha,
                              DWORD dwFlags)
{
   PLRD_PROP pLrdProp;
   INT was_Layered;

   if (!(pWnd->ExStyle & WS_EX_LAYERED) )
   {
      ERR("Not a Layered Window!\n");
      return FALSE;
   }

   pLrdProp = UserGetProp(pWnd, AtomLayer);

   if (!pLrdProp)
   {
      pLrdProp = ExAllocatePoolWithTag(PagedPool, sizeof(LRD_PROP), USERTAG_REDIRECT);
      if (pLrdProp == NULL)
      {
         ERR("failed to allocate LRD_PROP\n");
         return FALSE;
      }
      RtlZeroMemory(pLrdProp, sizeof(LRD_PROP));
      IntSetProp(pWnd, AtomLayer, (HANDLE)pLrdProp);
   }

   if (pLrdProp)
   {
      was_Layered = pLrdProp->is_Layered;

      pLrdProp->Key = crKey;

      if (dwFlags & LWA_ALPHA)
      {
         pLrdProp->Alpha = bAlpha;
      }
      else
      {
         if (!pLrdProp->is_Layered) pLrdProp->Alpha = 0;
      }

      pLrdProp->Flags = dwFlags;

      pLrdProp->is_Layered = 1;
  
      if (!was_Layered)
         co_UserRedrawWindow(pWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
   }
   // FIXME: Now set some bits to the Window DC!!!!
   return TRUE;
}

BOOL FASTCALL
IntUpdateLayeredWindowI( PWND pWnd,
                         UPDATELAYEREDWINDOWINFO *info )
{
   PWND Parent;
   RECT Window, Client;
   SIZE Offset;
   BOOL ret = FALSE;
   DWORD flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;

   Window = pWnd->rcWindow;
   Client = pWnd->rcClient;

   Parent = pWnd->spwndParent;
   if (pWnd->style & WS_CHILD && Parent)
   {
      RECTL_vOffsetRect(&Window, - Parent->rcClient.left, - Parent->rcClient.top);
      RECTL_vOffsetRect(&Client, - Parent->rcClient.left, - Parent->rcClient.top);
   }

   if (info->pptDst)
   {
      Offset.cx = info->pptDst->x - Window.left;
      Offset.cy = info->pptDst->y - Window.top;
      RECTL_vOffsetRect( &Client, Offset.cx, Offset.cy );
      RECTL_vOffsetRect( &Window, Offset.cx, Offset.cy );
      flags &= ~SWP_NOMOVE;
   }
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
          EngSetLastError( ERROR_INCORRECT_SIZE );
          return FALSE;
      }
      Client.right  += Offset.cx;
      Client.bottom += Offset.cy;
      Window.right  += Offset.cx;
      Window.bottom += Offset.cy;
      flags &= ~SWP_NOSIZE;
   }

   if (info->hdcSrc)
   {
      HDC hdc, hdcBuffer;
      RECT Rect;
      BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };
      COLORREF color_key = (info->dwFlags & ULW_COLORKEY) ? info->crKey : CLR_INVALID;
      HBITMAP hBitmapBuffer, hOldBitmap;

      Rect = Window;

      RECTL_vOffsetRect( &Rect, -Window.left, -Window.top );

      TRACE("H %d W %d\n",Rect.bottom - Rect.top,Rect.right - Rect.left);

      if (!info->hdcDst) hdc = UserGetDCEx(pWnd, NULL, DCX_USESTYLE);
      else hdc = info->hdcDst;

      hdcBuffer = NtGdiCreateCompatibleDC(hdc);
      hBitmapBuffer = NtGdiCreateCompatibleBitmap(hdc, Rect.right - Rect.left, Rect.bottom - Rect.top);
      hOldBitmap = (HBITMAP)NtGdiSelectBitmap(hdcBuffer, hBitmapBuffer);

      NtGdiStretchBlt( hdcBuffer,
                       Rect.left,
                       Rect.top,
                       Rect.right - Rect.left,
                       Rect.bottom - Rect.top,
                       info->hdcSrc,
                       Rect.left + (info->pptSrc ? info->pptSrc->x : 0),
                       Rect.top  + (info->pptSrc ? info->pptSrc->y : 0),
                       Rect.right - Rect.left,
                       Rect.bottom - Rect.top,
                       SRCCOPY,
                       color_key );

      // Need to test this, Dirty before or after StretchBlt?
      if (info->prcDirty)
      {
         ERR("prcDirty\n");
         RECTL_bIntersectRect( &Rect, &Rect, info->prcDirty );
         NtGdiPatBlt( hdc, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, BLACKNESS );
      }

      if (info->dwFlags & ULW_ALPHA)
      {
         blend = *info->pblend;
         TRACE("ULW_ALPHA bop %d Alpha %d aF %d\n", blend.BlendOp, blend.SourceConstantAlpha, blend.AlphaFormat);
      }

      ret = NtGdiAlphaBlend( hdc,
                             Rect.left,
                             Rect.top,
                             Rect.right - Rect.left,
                             Rect.bottom - Rect.top,
                             hdcBuffer,
                             Rect.left + (info->pptSrc ? info->pptSrc->x : 0),
                             Rect.top  + (info->pptSrc ? info->pptSrc->y : 0),
                             Rect.right - Rect.left,
                             Rect.bottom - Rect.top,
                             blend,
                             0);

      NtGdiSelectBitmap(hdcBuffer, hOldBitmap);
      if (hBitmapBuffer) GreDeleteObject(hBitmapBuffer);
      if (hdcBuffer) IntGdiDeleteDC(hdcBuffer, FALSE);
      if (!info->hdcDst) UserReleaseDC(pWnd, hdc, FALSE);
   }
   else
      ret = TRUE;

   co_WinPosSetWindowPos(pWnd, 0, Window.left, Window.top, Window.right - Window.left, Window.bottom - Window.top, flags);
   return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
   PLRD_PROP pLrdProp;
   PWND pWnd;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserGetLayeredWindowAttributes\n");
   UserEnterExclusive();

   if (!(pWnd = UserGetWindowObject(hwnd)) ||
       !(pWnd->ExStyle & WS_EX_LAYERED) )
   {
      ERR("Not a Layered Window!\n");
      goto Exit;
   }

   pLrdProp = UserGetProp(pWnd, AtomLayer);

   if (!pLrdProp)
   {
      TRACE("No Prop!\n");
      goto Exit;
   }

   if (pLrdProp->is_Layered == 0)
   {
      goto Exit;
   }

   _SEH2_TRY
   {
      if (pcrKey)
      {
          ProbeForWrite(pcrKey, sizeof(*pcrKey), 1);
          *pcrKey = pLrdProp->Key;
      }
      if (pbAlpha)
      {
          ProbeForWrite(pbAlpha, sizeof(*pbAlpha), 1);
          *pbAlpha = pLrdProp->Alpha;
      }
      if (pdwFlags)
      {
          ProbeForWrite(pdwFlags, sizeof(*pdwFlags), 1);
          *pdwFlags = pLrdProp->Flags;
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
BOOL APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
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
   HWND hwnd,
   HDC hdcDst,
   POINT *pptDst,
   SIZE *psize,
   HDC hdcSrc,
   POINT *pptSrc,
   COLORREF crKey,
   BLENDFUNCTION *pblend,
   DWORD dwFlags,
   RECT *prcDirty)
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
      ProbeForRead(psize, sizeof(SIZE), 1);
      Size = *psize;
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
      EngSetLastError( ERROR_INVALID_PARAMETER );
      _SEH2_YIELD(goto Exit);
   }
   _SEH2_END;

   if ( GetLayeredStatus(pWnd) ||
        dwFlags & ~(ULW_COLORKEY | ULW_ALPHA | ULW_OPAQUE | ULW_EX_NORESIZE) ||
       !(pWnd->ExStyle & WS_EX_LAYERED) )
   {
      ERR("Layered Window Invalid Parameters\n");
      EngSetLastError( ERROR_INVALID_PARAMETER );
      goto Exit;
   }

   info.cbSize   = sizeof(info);
   info.hdcDst   = hdcDst;
   info.pptDst   = pptDst? &Dst : NULL;
   info.psize    = &Size;
   info.hdcSrc   = hdcSrc;
   info.pptSrc   = pptSrc ? &Src : NULL;
   info.crKey    = crKey;
   info.pblend   = &blend;
   info.dwFlags  = dwFlags;
   info.prcDirty = prcDirty ? &Dirty : NULL;
   Ret = IntUpdateLayeredWindowI( pWnd, &info );
Exit:
   TRACE("Leave NtUserUpdateLayeredWindow, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/* EOF */
