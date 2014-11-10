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

 /* What to do after SetScrollInfo() */
 #define SA_SSI_HIDE             0x0001
 #define SA_SSI_SHOW             0x0002
 #define SA_SSI_REFRESH          0x0004
 #define SA_SSI_REPAINT_ARROWS   0x0008

#define SBRG_SCROLLBAR     0 /* The scrollbar itself */
#define SBRG_TOPRIGHTBTN   1 /* The top or right button */
#define SBRG_PAGEUPRIGHT   2 /* The page up or page right region */
#define SBRG_SCROLLBOX     3 /* The scroll box */
#define SBRG_PAGEDOWNLEFT  4 /* The page down or page left region */
#define SBRG_BOTTOMLEFTBTN 5 /* The bottom or left button */

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
static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

PSBDATA FASTCALL
IntGetSBData(PWND pwnd, INT Bar)
{
   PSBWND pSBWnd;
   PSBINFO pSBInfo;

   pSBInfo = pwnd->pSBInfo;
   switch (Bar)
   {
       case SB_HORZ:
         return &pSBInfo->Horz;
       case SB_VERT:
         return &pSBInfo->Vert;
       case SB_CTL:
         if ( pwnd->cbwndExtra != (sizeof(SBWND)-sizeof(WND)) )
         {
            ERR("IntGetSBData Wrong Extra bytes for CTL Scrollbar!\n");
            return 0;
         }
         pSBWnd = (PSBWND)pwnd;
         return (PSBDATA)&pSBWnd->SBCalc;
       default:
            ERR("IntGetSBData Bad Bar!\n");
   }
   return NULL;
}

BOOL FASTCALL
IntGetScrollBarRect (PWND Wnd, INT nBar, RECTL *lprect)
{
   BOOL vertical;
   *lprect = Wnd->rcClient;

   RECTL_vOffsetRect( lprect, -Wnd->rcWindow.left, -Wnd->rcWindow.top );
   if (Wnd->ExStyle & WS_EX_LAYOUTRTL)
      mirror_rect( &Wnd->rcWindow, lprect );

   switch (nBar)
   {
      case SB_HORZ:
         lprect->top = lprect->bottom;
         lprect->bottom += UserGetSystemMetrics (SM_CYHSCROLL);
         if (Wnd->style & WS_BORDER)
         {
            lprect->left--;
            lprect->right++;
         }
         else if (Wnd->style & WS_VSCROLL)
         {
            lprect->right++;
         }
         vertical = FALSE;
         break;

      case SB_VERT:
         if(Wnd->ExStyle & WS_EX_LEFTSCROLLBAR)
         {
            lprect->right = lprect->left;
            lprect->left -= UserGetSystemMetrics(SM_CXVSCROLL);
         }
         else
         {
            lprect->left = lprect->right;
            lprect->right += UserGetSystemMetrics(SM_CXVSCROLL);
         }
         if (Wnd->style & WS_BORDER)
         {
            lprect->top--;
            lprect->bottom++;
         }
         else if (Wnd->style & WS_HSCROLL)
         {
            lprect->bottom++;
         }
         vertical = TRUE;
         break;

      case SB_CTL:
         IntGetClientRect (Wnd, lprect);
         vertical = !!(Wnd->style & SBS_VERT);
         break;

      default:
         return FALSE;
   }

   return vertical;
}

BOOL FASTCALL
IntCalculateThumb(PWND Wnd, LONG idObject, PSCROLLBARINFO psbi, PSBDATA pSBData)
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
   /* Calculate Thumb */
   if(cxy <= (2 * Thumb))
   {
      Thumb = cxy / 2;
      psbi->xyThumbTop = 0;
      psbi->xyThumbBottom = 0;
      ThumbPos = Thumb;
   }
   else
   {
      ThumbBox = pSBData->page ? MINTRACKTHUMB : UserGetSystemMetrics(SM_CXHTHUMB);
      cxy -= (2 * Thumb);
      if(cxy >= ThumbBox)
      {
         if(pSBData->page)
         {
            ThumbBox = max(EngMulDiv(cxy, pSBData->page, pSBData->posMax - pSBData->posMin + 1), ThumbBox);
         }

         if(cxy > ThumbBox)
         {
            mx = pSBData->posMax - max(pSBData->page - 1, 0);
            if(pSBData->posMin < mx)
               ThumbPos = Thumb + EngMulDiv(cxy - ThumbBox, pSBData->pos - pSBData->posMin, mx - pSBData->posMin);
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
/*
static VOID FASTCALL
IntUpdateSBInfo(PWND Window, int wBar)
{
   PSCROLLBARINFO sbi;
   PSBDATA pSBData;

   ASSERT(Window);
   ASSERT(Window->pSBInfo);
   ASSERT(Window->pSBInfoex);

   sbi = IntGetScrollbarInfoFromWindow(Window, wBar);
   pSBData = IntGetSBData(Window, wBar);
   IntGetScrollBarRect(Window, wBar, &(sbi->rcScrollBar));
   IntCalculateThumb(Window, wBar, sbi, pSBData);
}
*/
static BOOL FASTCALL
co_IntGetScrollInfo(PWND Window, INT nBar, PSBDATA pSBData, LPSCROLLINFO lpsi)
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

   if (!Window->pSBInfo)
   {
      ERR("IntGetScrollInfo No window scrollbar info!\n");
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

  if (!pWnd->pSBInfo || !pSBTrack) return FALSE;

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
   UINT new_flags;
   INT action = 0;
   PSBDATA pSBData;
   DWORD OldPos = 0;
   BOOL bChangeParams = FALSE; /* Don't show/hide scrollbar if params don't change */

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
   if (lpsi->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL | SIF_PREVIOUSPOS))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   psbi = IntGetScrollbarInfoFromWindow(Window, nBar);
   Info = IntGetScrollInfoFromWindow(Window, nBar);
   pSBData = IntGetSBData(Window, nBar);

   /* Set the page size */
   if (lpsi->fMask & SIF_PAGE)
   {
      if (Info->nPage != lpsi->nPage)
      {
         Info->nPage = lpsi->nPage;
         pSBData->page = lpsi->nPage;
         bChangeParams = TRUE;
      }
   }

   /* Set the scroll pos */
   if (lpsi->fMask & SIF_POS)
   {
      if (Info->nPos != lpsi->nPos)
      {
         OldPos = Info->nPos;
         Info->nPos = lpsi->nPos;
         pSBData->pos = lpsi->nPos;
         bChangeParams = TRUE;
      }
   }

   /* Set the scroll range */
   if (lpsi->fMask & SIF_RANGE)
   {
      /* Invalid range -> range is set to (0,0) */
      if ((lpsi->nMin > lpsi->nMax) ||
                  ((UINT)(lpsi->nMax - lpsi->nMin) >= 0x80000000))
      {
         Info->nMin = 0;
         Info->nMax = 0;
         pSBData->posMin = 0;
         pSBData->posMax = 0;
         bChangeParams = TRUE;
      }
      else if (Info->nMin != lpsi->nMin || Info->nMax != lpsi->nMax)
      {
         Info->nMin = lpsi->nMin;
         Info->nMax = lpsi->nMax;
         pSBData->posMin = lpsi->nMin;
         pSBData->posMax = lpsi->nMax;
         bChangeParams = TRUE;
      }
   }

   /* Make sure the page size is valid */
   if (Info->nPage < 0)
   {
      pSBData->page = Info->nPage = 0;
   }
   else if ((Info->nMax - Info->nMin + 1UL) < Info->nPage)
   {
      pSBData->page = Info->nPage = Info->nMax - Info->nMin + 1;
   }

   /* Make sure the pos is inside the range */
   if (Info->nPos < Info->nMin)
   {
      pSBData->pos = Info->nPos = Info->nMin;
   }
   else if (Info->nPos > (Info->nMax - max((int)Info->nPage - 1, 0)))
   {
      pSBData->pos = Info->nPos = Info->nMax - max(Info->nPage - 1, 0);
   }

   /*
    * Don't change the scrollbar state if SetScrollInfo is just called
    * with SIF_DISABLENOSCROLL
    */
   if (!(lpsi->fMask & SIF_ALL))
   {
      //goto done;
      return lpsi->fMask & SIF_PREVIOUSPOS ? OldPos : pSBData->pos;
   }

   /* Check if the scrollbar should be hidden or disabled */
   if (lpsi->fMask & (SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL))
   {
      new_flags = Window->pSBInfo->WSBflags;
      if (Info->nMin >= (int)(Info->nMax - max(Info->nPage - 1, 0)))
      {
         /* Hide or disable scroll-bar */
         if (lpsi->fMask & SIF_DISABLENOSCROLL)
         {
            new_flags = ESB_DISABLE_BOTH;
            bChangeParams = TRUE;
         }
         else if ((nBar != SB_CTL) && bChangeParams)
         {
            action = SA_SSI_HIDE;
         }
      }
      else /* Show and enable scroll-bar only if no page only changed. */
      if (lpsi->fMask != SIF_PAGE)
      {
         new_flags = ESB_ENABLE_BOTH;
         if ((nBar != SB_CTL) && bChangeParams)
         {
            action |= SA_SSI_SHOW;
         }
      }

      if (Window->pSBInfo->WSBflags != new_flags) /* Check arrow flags */
      {
         Window->pSBInfo->WSBflags = new_flags;
         action |= SA_SSI_REPAINT_ARROWS;
      }
   }

//done:
   if ( action & SA_SSI_HIDE )
   {
      co_UserShowScrollBar(Window, nBar, FALSE, FALSE);
   }
   else
   {
      if ( action & SA_SSI_SHOW )
         if ( co_UserShowScrollBar(Window, nBar, TRUE, TRUE) )
            return lpsi->fMask & SIF_PREVIOUSPOS ? OldPos : pSBData->pos; /* SetWindowPos() already did the painting */
      if (bRedraw)
      { // FIXME: Arrows and interior.
         RECTL UpdateRect = psbi->rcScrollBar;
         UpdateRect.left -= Window->rcClient.left - Window->rcWindow.left;
         UpdateRect.right -= Window->rcClient.left - Window->rcWindow.left;
         UpdateRect.top -= Window->rcClient.top - Window->rcWindow.top;
         UpdateRect.bottom -= Window->rcClient.top - Window->rcWindow.top;
         co_UserRedrawWindow(Window, &UpdateRect, 0, RDW_INVALIDATE | RDW_FRAME);
      } // FIXME: Arrows
/*      else if( action & SA_SSI_REPAINT_ARROWS )
      {
         RECTL UpdateRect = psbi->rcScrollBar;
         UpdateRect.left -= Window->rcClient.left - Window->rcWindow.left;
         UpdateRect.right -= Window->rcClient.left - Window->rcWindow.left;
         UpdateRect.top -= Window->rcClient.top - Window->rcWindow.top;
         UpdateRect.bottom -= Window->rcClient.top - Window->rcWindow.top;
         co_UserRedrawWindow(Window, &UpdateRect, 0, RDW_INVALIDATE | RDW_FRAME);
      }
*/   }
   /* Return current position */
   return lpsi->fMask & SIF_PREVIOUSPOS ? OldPos : pSBData->pos;
}

BOOL FASTCALL
co_IntGetScrollBarInfo(PWND Window, LONG idObject, PSCROLLBARINFO psbi)
{
   INT Bar;
   PSCROLLBARINFO sbi;
   PSBDATA pSBData;
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
      ERR("Failed to create scrollbars for window.\n");
      return FALSE;
   }

   sbi = IntGetScrollbarInfoFromWindow(Window, Bar);
   pSBData = IntGetSBData(Window, Bar);

   IntGetScrollBarRect(Window, Bar, &(sbi->rcScrollBar));
   IntCalculateThumb(Window, Bar, sbi, pSBData);

   RtlCopyMemory(psbi, sbi, sizeof(SCROLLBARINFO));

   return TRUE;
}

BOOL FASTCALL
co_IntCreateScrollBars(PWND Window)
{
   PSCROLLBARINFO psbi;
   PSBDATA pSBData;
   ULONG Size, s;
   INT i;

   ASSERT_REFS_CO(Window);

   if (Window->pSBInfo && Window->pSBInfoex)
   {
      /* No need to create it anymore */
      return TRUE;
   }

   /* Allocate memory for all scrollbars (HORZ, VERT, CONTROL) */
   Size = 3 * (sizeof(SBINFOEX));
   if(!(Window->pSBInfoex = ExAllocatePoolWithTag(PagedPool, Size, TAG_SBARINFO)))
   {
      ERR("Unable to allocate memory for scrollbar information for window %p\n", Window->head.h);
      return FALSE;
   }

   RtlZeroMemory(Window->pSBInfoex, Size);

   if(!(Window->pSBInfo = DesktopHeapAlloc( Window->head.rpdesk, sizeof(SBINFO))))
   {
      ERR("Unable to allocate memory for scrollbar information for window %p\n", Window->head.h);
      return FALSE;
   }

   RtlZeroMemory(Window->pSBInfo, sizeof(SBINFO));
   Window->pSBInfo->Vert.posMax = 100;
   Window->pSBInfo->Horz.posMax = 100;

   co_WinPosGetNonClientSize(Window,
                             &Window->rcWindow,
                             &Window->rcClient);

   for(s = SB_HORZ; s <= SB_VERT; s++)
   {
      psbi = IntGetScrollbarInfoFromWindow(Window, s);
      psbi->cbSize = sizeof(SCROLLBARINFO);
      for (i = 0; i < CCHILDREN_SCROLLBAR + 1; i++)
         psbi->rgstate[i] = 0;

      pSBData = IntGetSBData(Window, s);

      IntGetScrollBarRect(Window, s, &(psbi->rcScrollBar));
      IntCalculateThumb(Window, s, psbi, pSBData);
   }

   return TRUE;
}

BOOL FASTCALL
IntDestroyScrollBars(PWND Window)
{
   if (Window->pSBInfo && Window->pSBInfoex)
   {
      DesktopHeapFree(Window->head.rpdesk, Window->pSBInfo);
      Window->pSBInfo = NULL;
      ExFreePoolWithTag(Window->pSBInfoex, TAG_SBARINFO);
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

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD FASTCALL
co_UserShowScrollBar(PWND Wnd, int nBar, BOOL fShowH, BOOL fShowV)
{
   ULONG old_style, set_bits = 0, clear_bits = 0;

   ASSERT_REFS_CO(Wnd);

   switch(nBar)
   {
      case SB_CTL:
         {
            //IntUpdateSBInfo(Wnd, SB_CTL); // Is this needed? Was tested w/o!

            co_WinPosShowWindow(Wnd, fShowH ? SW_SHOW : SW_HIDE);
            return TRUE;
         }
      case SB_BOTH:
      case SB_HORZ:
         if (fShowH) set_bits |= WS_HSCROLL;
         else clear_bits |= WS_HSCROLL;
         if( nBar == SB_HORZ ) break;
      /* Fall through */
      case SB_VERT:
         if (fShowV) set_bits |= WS_VSCROLL;
         else clear_bits |= WS_VSCROLL;
         break;
      default:
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return FALSE;
   }

   old_style = IntSetStyle( Wnd, set_bits, clear_bits );
   if ((old_style & clear_bits) != 0 || (old_style & set_bits) != set_bits)
   {
      /////  Is this needed? Was tested w/o!
      //if (Wnd->style & WS_HSCROLL) IntUpdateSBInfo(Wnd, SB_HORZ);
      //if (Wnd->style & WS_VSCROLL) IntUpdateSBInfo(Wnd, SB_VERT);
      /////
         /* Frame has been changed, let the window redraw itself */
      co_WinPosSetWindowPos(Wnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
      return TRUE;
   }
   return FALSE;
}

LRESULT APIENTRY
ScrollBarWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lResult = 0;
   PWND pWnd;
   pWnd = UserGetWindowObject(hWnd);
   if (!pWnd) return 0;

   switch(Msg)
   {
       case WM_ENABLE:
       {
           if (pWnd->pSBInfo)
           {
              pWnd->pSBInfo->WSBflags = wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;
           }
       }
       break;
   }
   return lResult;
}

////

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
   PWND Window;
   SCROLLINFO psi;
   BOOL Ret;
   SBDATA SBDataSafe;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserGetScrollInfo\n");
   UserEnterShared();

   _SEH2_TRY
   {
      RtlCopyMemory(&psi, lpsi, sizeof(SCROLLINFO));
      if (pSBData)
      {
         RtlCopyMemory(&SBDataSafe, pSBData, sizeof(SBDATA));
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("NtUserGetScrollInfo Failed size.\n");
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(RETURN(FALSE));
   }
   _SEH2_END

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      ERR("NtUserGetScrollInfo Bad window.\n");
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   Ret = co_IntGetScrollInfo(Window, fnBar, &SBDataSafe, &psi);
   UserDerefObjectCo(Window);

   _SEH2_TRY
   {
      RtlCopyMemory(lpsi, &psi, sizeof(SCROLLINFO));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("NtUserGetScrollInfo Failed copy to user.\n");
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(RETURN(FALSE));
   }
   _SEH2_END

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
   UINT OrigArrows;
   PWND Window = NULL;
   PSCROLLBARINFO InfoV = NULL, InfoH = NULL;
   BOOL Chg = FALSE;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserEnableScrollBar\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)) || // FIXME:
         Window == UserGetDesktopWindow() ||    // pWnd->fnid == FNID_DESKTOP
         Window == UserGetMessageWindow() )     // pWnd->fnid == FNID_MESSAGEWND
   {
      RETURN(FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   if (!co_IntCreateScrollBars(Window))
   {
      RETURN( FALSE);
   }

   OrigArrows = Window->pSBInfo->WSBflags;
   Window->pSBInfo->WSBflags = wArrows;

   if (wSBflags == SB_CTL)
   {
      if ((wArrows == ESB_DISABLE_BOTH || wArrows == ESB_ENABLE_BOTH))
         IntEnableWindow(hWnd, (wArrows == ESB_ENABLE_BOTH));

      RETURN(TRUE);
   }

   if(wSBflags != SB_BOTH && !SBID_IS_VALID(wSBflags))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ERR("Trying to set scrollinfo for unknown scrollbar type %u", wSBflags);
      RETURN(FALSE);
   }

   switch(wSBflags)
   {
      case SB_BOTH:
         InfoV = IntGetScrollbarInfoFromWindow(Window, SB_VERT);
         /* Fall through */
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

   ERR("FIXME: EnableScrollBar wSBflags %u wArrows %u Chg %d\n", wSBflags, wArrows, Chg);
// Done in user32:
//   SCROLL_RefreshScrollBar( hwnd, nBar, TRUE, TRUE );

   RETURN( Chg);
   if (OrigArrows == wArrows) RETURN( FALSE);
   RETURN( TRUE);

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserEnableScrollBar, ret=%i\n",_ret_);
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

   if(!(Window = UserGetWindowObject(hWnd)) || // FIXME:
        Window == UserGetDesktopWindow() ||    // pWnd->fnid == FNID_DESKTOP
        Window == UserGetMessageWindow() )     // pWnd->fnid == FNID_MESSAGEWND
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

   TRACE("Leave NtUserSetScrollInfo, ret=%lu\n", _ret_);
   UserLeave();
   END_CLEANUP;

}

DWORD APIENTRY
NtUserShowScrollBar(HWND hWnd, int nBar, DWORD bShow)
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
   ret = co_UserShowScrollBar(Window, nBar, (nBar == SB_VERT) ? 0 : bShow,
                                            (nBar == SB_HORZ) ? 0 : bShow);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserShowScrollBar,  ret%lu\n", _ret_);
   UserLeave();
   END_CLEANUP;

}


//// Ugly NtUser API ////

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

/* EOF */
