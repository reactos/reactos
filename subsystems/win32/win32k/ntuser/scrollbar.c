/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Scrollbars
 * FILE:             subsys/win32k/ntuser/scrollbar.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                   Jason Filby (jasonfilby@yahoo.com)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserScrollbar);

#define MINTRACKTHUMB    8               /* Minimum size of the rectangle between the arrows */

#define SBRG_SCROLLBAR     0 /* the scrollbar itself */
#define SBRG_TOPRIGHTBTN   1 /* the top or right button */
#define SBRG_PAGEUPRIGHT   2 /* the page up or page right region */
#define SBRG_SCROLLBOX     3 /* the scroll box */
#define SBRG_PAGEDOWNLEFT  4 /* the page down or page left region */
#define SBRG_BOTTOMLEFTBTN 5 /* the bottom or left button */

#define CHANGERGSTATE(item, status) \
  if(Info->rgstate[(item)] != (status)) \
    Chg = TRUE; \
  Info->rgstate[(item)] = (status);

/* FUNCTIONS *****************************************************************/

/* Ported from WINE20020904 */
/* Compute the scroll bar rectangle, in drawing coordinates (i.e. client coords for SB_CTL, window coords for SB_VERT and
 * SB_HORZ). 'arrowSize' returns the width or height of an arrow (depending on * the orientation of the scrollbar),
 * 'thumbSize' returns the size of the thumb, and 'thumbPos' returns the position of the thumb relative to the left or to
 * the top. Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
BOOL FASTCALL
IntGetScrollBarRect (PWND Wnd, INT nBar, RECTL *lprect)
{
   BOOL vertical;
   RECTL ClientRect = Wnd->rcClient;
   RECTL WindowRect = Wnd->rcWindow;

   switch (nBar)
   {
      case SB_HORZ:
         lprect->left = ClientRect.left - WindowRect.left;
         lprect->top = ClientRect.bottom - WindowRect.top;
         lprect->right = ClientRect.right - WindowRect.left;
         lprect->bottom = lprect->top + UserGetSystemMetrics (SM_CYHSCROLL);
         vertical = FALSE;
         break;

      case SB_VERT:
         if(Wnd->ExStyle & WS_EX_LEFTSCROLLBAR)
         {
            lprect->right = ClientRect.left - WindowRect.left;
            lprect->left = lprect->right - UserGetSystemMetrics(SM_CXVSCROLL);
         }
         else
         {
            lprect->left = ClientRect.right - WindowRect.left;
            lprect->right = lprect->left + UserGetSystemMetrics(SM_CXVSCROLL);
         }
         lprect->top = ClientRect.top - WindowRect.top;
         lprect->bottom = ClientRect.bottom - WindowRect.top;
         vertical = TRUE;
         break;

      case SB_CTL:
         IntGetClientRect (Wnd, lprect);
         vertical = ((Wnd->style & SBS_VERT) != 0);
         break;

      default:
         return FALSE;
   }

   return vertical;
}

BOOL FASTCALL
IntCalculateThumb(PWND Wnd, LONG idObject, PSCROLLBARINFO psbi, LPSCROLLINFO psi)
{
   INT Thumb, ThumbBox, ThumbPos, cxy, mx;
   RECTL ClientRect;

   switch(idObject)
   {
      case SB_HORZ:
         Thumb = UserGetSystemMetrics(SM_CXHSCROLL);
         cxy = psbi->rcScrollBar.right - psbi->rcScrollBar.left;
         break;
      case SB_VERT:
         Thumb = UserGetSystemMetrics(SM_CYVSCROLL);
         cxy = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
         break;
      case SB_CTL:
         IntGetClientRect(Wnd, &ClientRect);
         if(Wnd->style & SBS_VERT)
         {
            Thumb = UserGetSystemMetrics(SM_CYVSCROLL);
            cxy = ClientRect.bottom - ClientRect.top;
         }
         else
         {
            Thumb = UserGetSystemMetrics(SM_CXHSCROLL);
            cxy = ClientRect.right - ClientRect.left;
         }
         break;
      default:
         return FALSE;
   }

   ThumbPos = Thumb;
   /* calculate Thumb */
   if(cxy <= (2 * Thumb))
   {
      Thumb = cxy / 2;
      psbi->xyThumbTop = 0;
      psbi->xyThumbBottom = 0;
      ThumbPos = Thumb;
   }
   else
   {
      ThumbBox = psi->nPage ? MINTRACKTHUMB : UserGetSystemMetrics(SM_CXHTHUMB);
      cxy -= (2 * Thumb);
      if(cxy >= ThumbBox)
      {
         if(psi->nPage)
         {
            ThumbBox = max(EngMulDiv(cxy, psi->nPage, psi->nMax - psi->nMin + 1), ThumbBox);
         }

         if(cxy > ThumbBox)
         {
            mx = psi->nMax - max(psi->nPage - 1, 0);
            if(psi->nMin < mx)
               ThumbPos = Thumb + EngMulDiv(cxy - ThumbBox, psi->nPos - psi->nMin, mx - psi->nMin);
            else
               ThumbPos = Thumb + ThumbBox;
         }

         psbi->xyThumbTop = ThumbPos;
         psbi->xyThumbBottom = ThumbPos + ThumbBox;
      }
      else
      {
         psbi->xyThumbTop = 0;
         psbi->xyThumbBottom = 0;
      }
   }
   psbi->dxyLineButton = Thumb;

   return TRUE;
}

static VOID FASTCALL
IntUpdateSBInfo(PWND Window, int wBar)
{
   PSCROLLBARINFO sbi;
   LPSCROLLINFO psi;

   ASSERT(Window);
   ASSERT(Window->pSBInfoex);

   sbi = IntGetScrollbarInfoFromWindow(Window, wBar);
   psi = IntGetScrollInfoFromWindow(Window, wBar);
   IntGetScrollBarRect(Window, wBar, &(sbi->rcScrollBar));
   IntCalculateThumb(Window, wBar, sbi, psi);
}

static BOOL FASTCALL
co_IntGetScrollInfo(PWND Window, INT nBar, LPSCROLLINFO lpsi)
{
   UINT Mask;
   LPSCROLLINFO psi;

   ASSERT_REFS_CO(Window);

   if(!SBID_IS_VALID(nBar))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to get scrollinfo for unknown scrollbar type %d\n", nBar);
      return FALSE;
   }

   if(!co_IntCreateScrollBars(Window))
   {
      return FALSE;
   }

   psi = IntGetScrollInfoFromWindow(Window, nBar);

   if (lpsi->fMask == SIF_ALL)
   {
      Mask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
   }
   else
   {
      Mask = lpsi->fMask;
   }

   if (0 != (Mask & SIF_PAGE))
   {
      lpsi->nPage = psi->nPage;
   }

   if (0 != (Mask & SIF_POS))
   {
      lpsi->nPos = psi->nPos;
   }

   if (0 != (Mask & SIF_RANGE))
   {
      lpsi->nMin = psi->nMin;
      lpsi->nMax = psi->nMax;
   }

   if (0 != (Mask & SIF_TRACKPOS))
   {
      lpsi->nTrackPos = psi->nTrackPos;
   }

   return TRUE;
}

BOOL FASTCALL
NEWco_IntGetScrollInfo(
  PWND pWnd,
  INT nBar,
  PSBDATA pSBData,
  LPSCROLLINFO lpsi)
{
  UINT Mask;
  PSBTRACK pSBTrack = pWnd->head.pti->pSBTrack;

  if (!SBID_IS_VALID(nBar))
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     ERR("Trying to get scrollinfo for unknown scrollbar type %d\n", nBar);
     return FALSE;
  }

  Mask = lpsi->fMask;

  if (0 != (Mask & SIF_PAGE))
  {
     lpsi->nPage = pSBData->page;
  }

  if (0 != (Mask & SIF_POS))
  {
     lpsi->nPos = pSBData->pos;
  }

  if (0 != (Mask & SIF_RANGE))
  {
     lpsi->nMin = pSBData->posMin;
     lpsi->nMax = pSBData->posMax;
  }

  if (0 != (Mask & SIF_TRACKPOS))
  {
     if ( pSBTrack &&
          pSBTrack->nBar == nBar &&
          pSBTrack->spwndTrack == pWnd )
        lpsi->nTrackPos = pSBTrack->posNew;
     else
        lpsi->nTrackPos = pSBData->pos;
  }
  return (Mask & SIF_ALL) !=0;
}

static DWORD FASTCALL
co_IntSetScrollInfo(PWND Window, INT nBar, LPCSCROLLINFO lpsi, BOOL bRedraw)
{
   /*
    * Update the scrollbar state and set action flags according to
    * what has to be done graphics wise.
    */

   LPSCROLLINFO Info;
   PSCROLLBARINFO psbi;
   /*   UINT new_flags;*/
   BOOL bChangeParams = FALSE; /* don't show/hide scrollbar if params don't change */

   ASSERT_REFS_CO(Window);

   if(!SBID_IS_VALID(nBar))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to set scrollinfo for unknown scrollbar type %d", nBar);
      return FALSE;
   }

   if(!co_IntCreateScrollBars(Window))
   {
      return FALSE;
   }

   if (lpsi->cbSize != sizeof(SCROLLINFO) &&
         lpsi->cbSize != (sizeof(SCROLLINFO) - sizeof(lpsi->nTrackPos)))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }
   if (lpsi->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   psbi = IntGetScrollbarInfoFromWindow(Window, nBar);
   Info = IntGetScrollInfoFromWindow(Window, nBar);

   /* Set the page size */
   if (0 != (lpsi->fMask & SIF_PAGE))
   {
      if (Info->nPage != lpsi->nPage)
      {
         Info->nPage = lpsi->nPage;
         bChangeParams = TRUE;
      }
   }

   /* Set the scroll pos */
   if (0 != (lpsi->fMask & SIF_POS))
   {
      if (Info->nPos != lpsi->nPos)
      {
         Info->nPos = lpsi->nPos;
      }
   }

   /* Set the scroll range */
   if (0 != (lpsi->fMask & SIF_RANGE))
   {
      /* Invalid range -> range is set to (0,0) */
      if (lpsi->nMin > lpsi->nMax ||
            0x80000000 <= (UINT)(lpsi->nMax - lpsi->nMin))
      {
         Info->nMin = 0;
         Info->nMax = 0;
         bChangeParams = TRUE;
      }
      else if (Info->nMin != lpsi->nMin || Info->nMax != lpsi->nMax)
      {
         Info->nMin = lpsi->nMin;
         Info->nMax = lpsi->nMax;
         bChangeParams = TRUE;
      }
   }

   /* Make sure the page size is valid */
   if (Info->nMax - Info->nMin + 1 < Info->nPage)
   {
      Info->nPage = Info->nMax - Info->nMin + 1;
   }

   /* Make sure the pos is inside the range */
   if (Info->nPos < Info->nMin)
   {
      Info->nPos = Info->nMin;
   }
   else if (Info->nPos > Info->nMax - max(Info->nPage - 1, 0))
   {
      Info->nPos = Info->nMax - max(Info->nPage - 1, 0);
   }

   /*
    * Don't change the scrollbar state if SetScrollInfo is just called
    * with SIF_DISABLENOSCROLL
    */
   if (0 == (lpsi->fMask & SIF_ALL))
   {
      return Info->nPos;
   }

   /* Check if the scrollbar should be hidden or disabled */
   if (0 != (lpsi->fMask & (SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL)))
   {
      if (Info->nMin >= (int)(Info->nMax - max(Info->nPage - 1, 0)))
      {
         /* Hide or disable scroll-bar */
         if (0 != (lpsi->fMask & SIF_DISABLENOSCROLL))
         {
            /*            new_flags = ESB_DISABLE_BOTH;*/
         }
         else if ((nBar != SB_CTL) && bChangeParams)
         {
            co_UserShowScrollBar(Window, nBar, FALSE);
            return Info->nPos;
         }
      }
      else  /* Show and enable scroll-bar */
      {
         /*         new_flags = 0;*/
         if ((nBar != SB_CTL) && bChangeParams)
         {
            co_UserShowScrollBar(Window, nBar, TRUE);
         }
      }

#if 0
      if (infoPtr->flags != new_flags) /* check arrow flags */
      {
         infoPtr->flags = new_flags;
         *Action |= SA_SSI_REPAINT_ARROWS;
      }
#endif

   }

   if (bRedraw)
   {
      RECTL UpdateRect = psbi->rcScrollBar;
      UpdateRect.left -= Window->rcClient.left - Window->rcWindow.left;
      UpdateRect.right -= Window->rcClient.left - Window->rcWindow.left;
      UpdateRect.top -= Window->rcClient.top - Window->rcWindow.top;
      UpdateRect.bottom -= Window->rcClient.top - Window->rcWindow.top;
      co_UserRedrawWindow(Window, &UpdateRect, 0, RDW_INVALIDATE | RDW_FRAME);
   }

   /* Return current position */
   return Info->nPos;
}

BOOL FASTCALL
co_IntGetScrollBarInfo(PWND Window, LONG idObject, PSCROLLBARINFO psbi)
{
   INT Bar;
   PSCROLLBARINFO sbi;
   LPSCROLLINFO psi;

   ASSERT_REFS_CO(Window);

   Bar = SBOBJ_TO_SBID(idObject);

   if(!SBID_IS_VALID(Bar))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to get scrollinfo for unknown scrollbar type %d\n", Bar);
      return FALSE;
   }

   if(!co_IntCreateScrollBars(Window))
   {
      return FALSE;
   }

   sbi = IntGetScrollbarInfoFromWindow(Window, Bar);
   psi = IntGetScrollInfoFromWindow(Window, Bar);

   IntGetScrollBarRect(Window, Bar, &(sbi->rcScrollBar));
   IntCalculateThumb(Window, Bar, sbi, psi);

   RtlCopyMemory(psbi, sbi, sizeof(SCROLLBARINFO));

   return TRUE;
}

BOOL FASTCALL
co_IntCreateScrollBars(PWND Window)
{
   PSCROLLBARINFO psbi;
   LPSCROLLINFO psi;
   ULONG Size, s;
   INT i;

   ASSERT_REFS_CO(Window);

   if(Window->pSBInfoex)
   {
      /* no need to create it anymore */
      return TRUE;
   }

   /* allocate memory for all scrollbars (HORZ, VERT, CONTROL) */
   Size = 3 * (sizeof(SBINFOEX));
   if(!(Window->pSBInfoex = ExAllocatePoolWithTag(PagedPool, Size, TAG_SBARINFO)))
   {
      ERR("Unable to allocate memory for scrollbar information for window 0x%x\n", Window->head.h);
      return FALSE;
   }

   RtlZeroMemory(Window->pSBInfoex, Size);

   co_WinPosGetNonClientSize(Window,
                             &Window->rcWindow,
                             &Window->rcClient);

   for(s = SB_HORZ; s <= SB_VERT; s++)
   {
      psbi = IntGetScrollbarInfoFromWindow(Window, s);
      psbi->cbSize = sizeof(SCROLLBARINFO);
      for (i = 0; i < CCHILDREN_SCROLLBAR + 1; i++)
         psbi->rgstate[i] = 0;

      psi = IntGetScrollInfoFromWindow(Window, s);
      psi->cbSize = sizeof(LPSCROLLINFO);
      psi->nMax = 100;

      IntGetScrollBarRect(Window, s, &(psbi->rcScrollBar));
      IntCalculateThumb(Window, s, psbi, psi);
   }

   return TRUE;
}

BOOL FASTCALL
IntDestroyScrollBars(PWND Window)
{
   if(Window->pSBInfoex)
   {
      ExFreePool(Window->pSBInfoex);
      Window->pSBInfoex = NULL;
      return TRUE;
   }
   return FALSE;
}

BOOL APIENTRY
IntEnableScrollBar(BOOL Horz, PSCROLLBARINFO Info, UINT wArrows)
{
   BOOL Chg = FALSE;
   switch(wArrows)
   {
      case ESB_DISABLE_BOTH:
         CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
         CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
         break;
      case ESB_DISABLE_RTDN:
         if(Horz)
         {
            CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
         }
         else
         {
            CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
         }
         break;
      case ESB_DISABLE_LTUP:
         if(Horz)
         {
            CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
         }
         else
         {
            CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
         }
         break;
      case ESB_ENABLE_BOTH:
         CHANGERGSTATE(SBRG_TOPRIGHTBTN, 0);
         CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, 0);
         break;
   }
   return Chg;
}


BOOL
APIENTRY
NtUserGetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
   NTSTATUS Status;
   SCROLLBARINFO sbi;
   PWND Window;
   BOOL Ret;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserGetScrollBarInfo\n");
   UserEnterExclusive();

   Status = MmCopyFromCaller(&sbi, psbi, sizeof(SCROLLBARINFO));
   if(!NT_SUCCESS(Status) || (sbi.cbSize != sizeof(SCROLLBARINFO)))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   Ret = co_IntGetScrollBarInfo(Window, idObject, &sbi);
   UserDerefObjectCo(Window);

   Status = MmCopyToCaller(psbi, &sbi, sizeof(SCROLLBARINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      Ret = FALSE;
   }

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserGetScrollBarInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}


BOOL
APIENTRY
NtUserSBGetParms(
  HWND hWnd, 
  int fnBar, 
  PSBDATA pSBData,
  LPSCROLLINFO lpsi)
{
   NTSTATUS Status;
   PWND Window;
   SCROLLINFO psi;
   DWORD sz;
   BOOL Ret;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserGetScrollInfo\n");
   UserEnterExclusive();

   Status = MmCopyFromCaller(&psi.cbSize, &(lpsi->cbSize), sizeof(UINT));
   if(!NT_SUCCESS(Status) ||
         !((psi.cbSize == sizeof(SCROLLINFO)) || (psi.cbSize == sizeof(SCROLLINFO) - sizeof(psi.nTrackPos))))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }
   sz = psi.cbSize;
   Status = MmCopyFromCaller(&psi, lpsi, sz);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   Ret = co_IntGetScrollInfo(Window, fnBar, &psi);
   UserDerefObjectCo(Window);

   Status = MmCopyToCaller(lpsi, &psi, sz);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserGetScrollInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserEnableScrollBar(
   HWND hWnd,
   UINT wSBflags,
   UINT wArrows)
{
   PWND Window = NULL;
   PSCROLLBARINFO InfoV = NULL, InfoH = NULL;
   BOOL Chg = FALSE;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserEnableScrollBar\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   if(wSBflags == SB_CTL)
   {
      /* FIXME Enable or Disable SB Ctrl*/
      ERR("Enable Scrollbar SB_CTL\n");
      InfoV = IntGetScrollbarInfoFromWindow(Window, SB_CTL);
      Chg = IntEnableScrollBar(FALSE, InfoV ,wArrows);
      /* Chg? Scrollbar is Refresh in user32/controls/scrollbar.c. */

      RETURN(TRUE);
   }

   if(wSBflags != SB_BOTH && !SBID_IS_VALID(wSBflags))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to set scrollinfo for unknown scrollbar type %d", wSBflags);
      RETURN(FALSE);
   }

   if(!co_IntCreateScrollBars(Window))
   {
      RETURN( FALSE);
   }

   switch(wSBflags)
   {
      case SB_BOTH:
         InfoV = IntGetScrollbarInfoFromWindow(Window, SB_VERT);
         /* fall through */
      case SB_HORZ:
         InfoH = IntGetScrollbarInfoFromWindow(Window, SB_HORZ);
         break;
      case SB_VERT:
         InfoV = IntGetScrollbarInfoFromWindow(Window, SB_VERT);
         break;
      default:
         RETURN(FALSE);
   }

   if(InfoV)
      Chg = IntEnableScrollBar(FALSE, InfoV, wArrows);

   if(InfoH)
      Chg = (IntEnableScrollBar(TRUE, InfoH, wArrows) || Chg);

   //if(Chg && (Window->style & WS_VISIBLE))
   /* FIXME - repaint scrollbars */

   RETURN( TRUE);

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserEnableScrollBar, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserSetScrollBarInfo(
   HWND hWnd,
   LONG idObject,
   SETSCROLLBARINFO *info)
{
   PWND Window = NULL;
   SETSCROLLBARINFO Safeinfo;
   PSCROLLBARINFO sbi;
   LPSCROLLINFO psi;
   NTSTATUS Status;
   LONG Obj;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserSetScrollBarInfo\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   Obj = SBOBJ_TO_SBID(idObject);
   if(!SBID_IS_VALID(Obj))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to set scrollinfo for unknown scrollbar type %d\n", Obj);
      RETURN( FALSE);
   }

   if(!co_IntCreateScrollBars(Window))
   {
      RETURN(FALSE);
   }

   Status = MmCopyFromCaller(&Safeinfo, info, sizeof(SETSCROLLBARINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   sbi = IntGetScrollbarInfoFromWindow(Window, Obj);
   psi = IntGetScrollInfoFromWindow(Window, Obj);

   psi->nTrackPos = Safeinfo.nTrackPos;
   sbi->reserved = Safeinfo.reserved;
   RtlCopyMemory(&sbi->rgstate, &Safeinfo.rgstate, sizeof(Safeinfo.rgstate));

   RETURN(TRUE);

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserSetScrollBarInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD
APIENTRY
NtUserSetScrollInfo(
   HWND hWnd,
   int fnBar,
   LPCSCROLLINFO lpsi,
   BOOL bRedraw)
{
   PWND Window = NULL;
   NTSTATUS Status;
   SCROLLINFO ScrollInfo;
   DECLARE_RETURN(DWORD);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserSetScrollInfo\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }
   UserRefObjectCo(Window, &Ref);

   Status = MmCopyFromCaller(&ScrollInfo, lpsi, sizeof(SCROLLINFO) - sizeof(ScrollInfo.nTrackPos));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   RETURN(co_IntSetScrollInfo(Window, fnBar, &ScrollInfo, bRedraw));

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserSetScrollInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD FASTCALL
co_UserShowScrollBar(PWND Wnd, int wBar, DWORD bShow)
{
   DWORD Style, OldStyle;

   ASSERT_REFS_CO(Wnd);

   switch(wBar)
   {
      case SB_HORZ:
         Style = WS_HSCROLL;
         break;
      case SB_VERT:
         Style = WS_VSCROLL;
         break;
      case SB_BOTH:
         Style = WS_HSCROLL | WS_VSCROLL;
         break;
      case SB_CTL:
         Style = 0;
         break;
      default:
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return( FALSE);
   }

   if(!co_IntCreateScrollBars(Wnd))
   {
      return( FALSE);
   }

   if (wBar == SB_CTL)
   {
      IntUpdateSBInfo(Wnd, SB_CTL);

      co_WinPosShowWindow(Wnd, bShow ? SW_SHOW : SW_HIDE);
      return( TRUE);
   }

   OldStyle = Wnd->style;
   if(bShow)
      Wnd->style |= Style;
   else
      Wnd->style &= ~Style;

   if(Wnd->style != OldStyle)
   {
      if(Wnd->style & WS_HSCROLL)
         IntUpdateSBInfo(Wnd, SB_HORZ);
      if(Wnd->style & WS_VSCROLL)
         IntUpdateSBInfo(Wnd, SB_VERT);

      if(Wnd->style & WS_VISIBLE)
      {
         /* Frame has been changed, let the window redraw itself */
         co_WinPosSetWindowPos(Wnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
      }
   }

   return( TRUE);
}


DWORD APIENTRY
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
   PWND Window;
   DECLARE_RETURN(DWORD);
   DWORD ret;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserShowScrollBar\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(0);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_UserShowScrollBar(Window, wBar, bShow);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserShowScrollBar,  ret%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}
/* EOF */
