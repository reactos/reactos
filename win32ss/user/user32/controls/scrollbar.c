/*
 * ReactOS User32 Library
 * - ScrollBar control
 *
 * Copyright 2001 Casper S. Hornstrup
 * Copyright 2003 Thomas Weidenmueller
 * Copyright 2003 Filip Navara
 *
 * Based on Wine code.
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994, 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* INCLUDES *******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(scrollbar);

/* GLOBAL VARIABLES ***********************************************************/

/* Definitions for scrollbar hit testing [See SCROLLBARINFO in MSDN] */
#define SCROLL_NOWHERE		0x00    /* Outside the scroll bar */
#define SCROLL_TOP_ARROW	0x01    /* Top or left arrow */
#define SCROLL_TOP_RECT		0x02    /* Rectangle between the top arrow and the thumb */
#define SCROLL_THUMB		0x03    /* Thumb rectangle */
#define SCROLL_BOTTOM_RECT	0x04    /* Rectangle between the thumb and the bottom arrow */
#define SCROLL_BOTTOM_ARROW	0x05    /* Bottom or right arrow */

#define SCROLL_FIRST_DELAY   200        /* Delay (in ms) before first repetition when
                                           holding the button down */
#define SCROLL_REPEAT_DELAY  50         /* Delay (in ms) between scroll repetitions */

#define SCROLL_TIMER   0                /* Scroll timer id */

  /* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4

  /* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 6

  /* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 0

 /* Thumb-tracking info */
static HWND ScrollTrackingWin = 0;
static INT  ScrollTrackingBar = 0;
static INT  ScrollTrackingPos = 0;
static INT  ScrollTrackingVal = 0;
 /* Hit test code of the last button-down event */
static DWORD ScrollTrackHitTest = SCROLL_NOWHERE;
static BOOL ScrollTrackVertical;

 /* Is the moving thumb being displayed? */
static BOOL ScrollMovingThumb = FALSE;

HBRUSH DefWndControlColor(HDC hDC, UINT ctlType);

UINT_PTR WINAPI SetSystemTimer(HWND,UINT_PTR,UINT,TIMERPROC);
BOOL WINAPI KillSystemTimer(HWND,UINT_PTR);

/*********************************************************************
 * scrollbar class descriptor
 */
const struct builtin_class_descr SCROLL_builtin_class =
{
    L"ScrollBar",           /* name */
    CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style */
    ScrollBarWndProcA,      /* procA */
    ScrollBarWndProcW,      /* procW */
    sizeof(SBWND)-sizeof(WND), /* extra */
    IDC_ARROW,              /* cursor */
    0                       /* brush */
};

/* PRIVATE FUNCTIONS **********************************************************/

static PSBDATA
IntGetSBData(PWND pwnd, INT Bar)
{
   PSBWND pSBWnd;
   PSBINFO pSBInfo;

   pSBInfo = DesktopPtrToUser(pwnd->pSBInfo);
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

static void
IntDrawScrollInterior(HWND hWnd, HDC hDC, INT nBar, BOOL Vertical,
   PSCROLLBARINFO ScrollBarInfo)
{
   INT ThumbSize = ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
   INT ThumbTop = ScrollBarInfo->xyThumbTop;
   RECT Rect;
   HBRUSH hSaveBrush, hBrush;
   BOOL TopSelected = FALSE, BottomSelected = FALSE;

   if (ScrollBarInfo->rgstate[SCROLL_TOP_RECT] & STATE_SYSTEM_PRESSED)
      TopSelected = TRUE;
   if (ScrollBarInfo->rgstate[SCROLL_BOTTOM_RECT] & STATE_SYSTEM_PRESSED)
      BottomSelected = TRUE;

   /*
    * Only scrollbar controls send WM_CTLCOLORSCROLLBAR.
    * The window-owned scrollbars need to call DefWndControlColor
    * to correctly setup default scrollbar colors
    */
   if (nBar == SB_CTL)
   {
      hBrush = GetControlBrush( hWnd, hDC, WM_CTLCOLORSCROLLBAR);
      if (!hBrush)
         hBrush = GetSysColorBrush(COLOR_SCROLLBAR);
   }
   else
   {
      hBrush = DefWndControlColor(hDC, CTLCOLOR_SCROLLBAR);
   }

   hSaveBrush = SelectObject(hDC, hBrush);

   /* Calculate the scroll rectangle */
   if (Vertical)
   {
      Rect.top = ScrollBarInfo->rcScrollBar.top + ScrollBarInfo->dxyLineButton;
      Rect.bottom = ScrollBarInfo->rcScrollBar.bottom - ScrollBarInfo->dxyLineButton;
      Rect.left = ScrollBarInfo->rcScrollBar.left;
      Rect.right = ScrollBarInfo->rcScrollBar.right;
   }
   else
   {
      Rect.top = ScrollBarInfo->rcScrollBar.top;
      Rect.bottom = ScrollBarInfo->rcScrollBar.bottom;
      Rect.left = ScrollBarInfo->rcScrollBar.left + ScrollBarInfo->dxyLineButton;
      Rect.right = ScrollBarInfo->rcScrollBar.right - ScrollBarInfo->dxyLineButton;
   }

   /* Draw the scroll rectangles and thumb */
   if (!ScrollBarInfo->xyThumbBottom)
   {
      PatBlt(hDC, Rect.left, Rect.top, Rect.right - Rect.left,
         Rect.bottom - Rect.top, PATCOPY);

      /* Cleanup and return */
      SelectObject(hDC, hSaveBrush);
      return;
   }

   ThumbTop -= ScrollBarInfo->dxyLineButton;

   if (ScrollBarInfo->dxyLineButton)
   {
      if (Vertical)
      {
         if (ThumbSize)
         {
            PatBlt(hDC, Rect.left, Rect.top, Rect.right - Rect.left,
                   ThumbTop, TopSelected ? BLACKNESS : PATCOPY);
            Rect.top += ThumbTop;
            PatBlt(hDC, Rect.left, Rect.top + ThumbSize, Rect.right - Rect.left,
               Rect.bottom - Rect.top - ThumbSize, BottomSelected ? BLACKNESS : PATCOPY);
            Rect.bottom = Rect.top + ThumbSize;
         }
         else
         {
            if (ThumbTop)
            {
               PatBlt(hDC, Rect.left, ScrollBarInfo->dxyLineButton,
                  Rect.right - Rect.left, Rect.bottom - Rect.top, PATCOPY);
            }
         }
      }
      else
      {
         if (ThumbSize)
         {
            PatBlt(hDC, Rect.left, Rect.top, ThumbTop,
               Rect.bottom - Rect.top, TopSelected ? BLACKNESS : PATCOPY);
            Rect.left += ThumbTop;
            PatBlt(hDC, Rect.left + ThumbSize, Rect.top,
               Rect.right - Rect.left - ThumbSize, Rect.bottom - Rect.top,
               BottomSelected ? BLACKNESS : PATCOPY);
            Rect.right = Rect.left + ThumbSize;
         }
         else
         {
            if (ThumbTop)
            {
               PatBlt(hDC, ScrollBarInfo->dxyLineButton, Rect.top,
                  Rect.right - Rect.left, Rect.bottom - Rect.top, PATCOPY);
            }
         }
      }
   }

   /* Draw the thumb */
   if (ThumbSize)
      DrawEdge(hDC, &Rect, EDGE_RAISED, BF_RECT | BF_MIDDLE);

   /* Cleanup */
   SelectObject(hDC, hSaveBrush);
}

static VOID FASTCALL
IntDrawScrollArrows(HDC hDC, PSCROLLBARINFO ScrollBarInfo, BOOL Vertical)
{
  RECT RectLT, RectRB;
  INT ScrollDirFlagLT, ScrollDirFlagRB;

  RectLT = RectRB = ScrollBarInfo->rcScrollBar;
  if (Vertical)
    {
      ScrollDirFlagLT = DFCS_SCROLLUP;
      ScrollDirFlagRB = DFCS_SCROLLDOWN;
      RectLT.bottom = RectLT.top + ScrollBarInfo->dxyLineButton;
      RectRB.top = RectRB.bottom - ScrollBarInfo->dxyLineButton;
    }
   else
    {
      ScrollDirFlagLT = DFCS_SCROLLLEFT;
      ScrollDirFlagRB = DFCS_SCROLLRIGHT;
      RectLT.right = RectLT.left + ScrollBarInfo->dxyLineButton;
      RectRB.left = RectRB.right - ScrollBarInfo->dxyLineButton;
    }

   if (ScrollBarInfo->rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_PRESSED)
     {
       ScrollDirFlagLT |= DFCS_PUSHED | DFCS_FLAT;
     }
   if (ScrollBarInfo->rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_UNAVAILABLE)
     {
       ScrollDirFlagLT |= DFCS_INACTIVE;
     }
   if (ScrollBarInfo->rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_PRESSED)
     {
       ScrollDirFlagRB |= DFCS_PUSHED | DFCS_FLAT;
     }
   if (ScrollBarInfo->rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_UNAVAILABLE)
     {
       ScrollDirFlagRB |= DFCS_INACTIVE;
     }

   DrawFrameControl(hDC, &RectLT, DFC_SCROLL, ScrollDirFlagLT);
   DrawFrameControl(hDC, &RectRB, DFC_SCROLL, ScrollDirFlagRB);
}

static VOID FASTCALL
IntScrollDrawMovingThumb(HDC Dc, PSCROLLBARINFO ScrollBarInfo, BOOL Vertical)
{
  INT Pos = ScrollTrackingPos;
  INT MaxSize;
  INT OldTop;

  if (Vertical)
      MaxSize = ScrollBarInfo->rcScrollBar.bottom - ScrollBarInfo->rcScrollBar.top;
  else
      MaxSize = ScrollBarInfo->rcScrollBar.right - ScrollBarInfo->rcScrollBar.left;

  MaxSize -= ScrollBarInfo->dxyLineButton + ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;

  if (Pos < ScrollBarInfo->dxyLineButton)
      Pos = ScrollBarInfo->dxyLineButton;
  else if (MaxSize < Pos)
      Pos = MaxSize;

  OldTop = ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbBottom = Pos + ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbTop = Pos;
  IntDrawScrollInterior(ScrollTrackingWin, Dc, ScrollTrackingBar, Vertical, ScrollBarInfo);
  ScrollBarInfo->xyThumbBottom = OldTop + ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbTop = OldTop;

  ScrollMovingThumb = !ScrollMovingThumb;
}

static LONG FASTCALL
IntScrollGetObjectId(INT SBType)
{
  if (SBType == SB_VERT)
      return OBJID_VSCROLL;
  if (SBType == SB_HORZ)
      return OBJID_HSCROLL;
  return OBJID_CLIENT;
}

static BOOL FASTCALL
IntGetScrollBarInfo(HWND Wnd, INT Bar, PSCROLLBARINFO ScrollBarInfo)
{
  ScrollBarInfo->cbSize = sizeof(SCROLLBARINFO);

  return NtUserGetScrollBarInfo(Wnd, IntScrollGetObjectId(Bar), ScrollBarInfo);
}

void
IntDrawScrollBar(HWND Wnd, HDC DC, INT Bar)
{
  //PSBWND pSBWnd;
  //INT ThumbSize;
  SCROLLBARINFO Info;
  BOOL Vertical;

  /*
   * Get scroll bar info.
   */
  switch (Bar)
    {
      case SB_HORZ:
        Vertical = FALSE;
        break;

      case SB_VERT:
        Vertical = TRUE;
        break;

      case SB_CTL:
        Vertical = (GetWindowLongPtrW(Wnd, GWL_STYLE) & SBS_VERT) != 0;
        break;

      default:
        return;
    }
  if (!IntGetScrollBarInfo(Wnd, Bar, &Info))
  {
      return;
  }

  if (IsRectEmpty(&Info.rcScrollBar))
  {
      return;
  }

  //ThumbSize = pSBWnd->pSBCalc->pxThumbBottom - pSBWnd->pSBCalc->pxThumbTop;

  /*
   * Draw the arrows.
   */
  if (Info.dxyLineButton)
  {
      IntDrawScrollArrows(DC, &Info, Vertical);
  }

  /*
   * Draw the interior.
   */
  IntDrawScrollInterior(Wnd, DC, Bar, Vertical, &Info);

  /*
   * If scroll bar has focus, reposition the caret.
   */
  if (Wnd == GetFocus() && SB_CTL == Bar)
  {
      if (Vertical)
      {
          SetCaretPos(Info.rcScrollBar.top + 1, Info.dxyLineButton + 1);
      }
      else
      {
           SetCaretPos(Info.dxyLineButton + 1, Info.rcScrollBar.top + 1);
      }
  }
}

static BOOL FASTCALL
IntScrollPtInRectEx(LPRECT Rect, POINT Pt, BOOL Vertical)
{
  RECT TempRect = *Rect;
  int scrollbarWidth;

  /* Pad hit rect to allow mouse to be dragged outside of scrollbar and
   * still be considered in the scrollbar. */
  if (Vertical)
  {
      scrollbarWidth = Rect->right - Rect->left;
      TempRect.left -= scrollbarWidth*8;
      TempRect.right += scrollbarWidth*8;
      TempRect.top -= scrollbarWidth*2;
      TempRect.bottom += scrollbarWidth*2;
  }
  else
  {
      scrollbarWidth = Rect->bottom - Rect->top;
      TempRect.left -= scrollbarWidth*2;
      TempRect.right += scrollbarWidth*2;
      TempRect.top -= scrollbarWidth*8;
      TempRect.bottom += scrollbarWidth*8;
  }

  return PtInRect(&TempRect, Pt);
}

static DWORD FASTCALL
IntScrollHitTest(PSCROLLBARINFO ScrollBarInfo, BOOL Vertical, POINT Pt, BOOL Dragging)
{
  INT ArrowSize, ThumbSize, ThumbPos;

  if ((Dragging && ! IntScrollPtInRectEx(&ScrollBarInfo->rcScrollBar, Pt, Vertical)) ||
      ! PtInRect(&ScrollBarInfo->rcScrollBar, Pt)) return SCROLL_NOWHERE;

  ThumbPos = ScrollBarInfo->xyThumbTop;
  ThumbSize = ScrollBarInfo->xyThumbBottom - ThumbPos;
  ArrowSize = ScrollBarInfo->dxyLineButton;

  if (Vertical)
  {
      if (Pt.y < ScrollBarInfo->rcScrollBar.top + ArrowSize) return SCROLL_TOP_ARROW;
      if (Pt.y >= ScrollBarInfo->rcScrollBar.bottom - ArrowSize) return SCROLL_BOTTOM_ARROW;
      if (!ThumbPos) return SCROLL_TOP_RECT;
      Pt.y -= ScrollBarInfo->rcScrollBar.top;
      if (Pt.y < ThumbPos) return SCROLL_TOP_RECT;
      if (Pt.y >= ThumbPos + ThumbSize) return SCROLL_BOTTOM_RECT;
  }
  else
  {
      if (Pt.x < ScrollBarInfo->rcScrollBar.left + ArrowSize) return SCROLL_TOP_ARROW;
      if (Pt.x >= ScrollBarInfo->rcScrollBar.right - ArrowSize) return SCROLL_BOTTOM_ARROW;
      if (!ThumbPos) return SCROLL_TOP_RECT;
      Pt.x -= ScrollBarInfo->rcScrollBar.left;
      if (Pt.x < ThumbPos) return SCROLL_TOP_RECT;
      if (Pt.x >= ThumbPos + ThumbSize) return SCROLL_BOTTOM_RECT;
  }

  return SCROLL_THUMB;
}


/***********************************************************************
 *           IntScrollGetScrollBarRect
 *
 * Compute the scroll bar rectangle, in drawing coordinates (i.e. client
 * coords for SB_CTL, window coords for SB_VERT and SB_HORZ).
 * 'arrowSize' returns the width or height of an arrow (depending on
 * the orientation of the scrollbar), 'thumbSize' returns the size of
 * the thumb, and 'thumbPos' returns the position of the thumb
 * relative to the left or to the top.
 * Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL FASTCALL
IntScrollGetScrollBarRect(HWND Wnd, INT Bar, RECT *Rect,
                          INT *ArrowSize, INT *ThumbSize,
                          INT *ThumbPos)
{
  INT Pixels;
  BOOL Vertical;
  PWND pWnd;
  PSBINFO pSBInfo;
  PSBDATA pSBData;
  PSBWND pSBWnd;

  pWnd = ValidateHwnd( Wnd );
  if (!pWnd) return FALSE;
  pSBInfo = DesktopPtrToUser(pWnd->pSBInfo);

  *Rect = pWnd->rcClient;
  OffsetRect( Rect, -pWnd->rcWindow.left, -pWnd->rcWindow.top );
  if (pWnd->ExStyle & WS_EX_LAYOUTRTL)
     mirror_rect( &pWnd->rcWindow, Rect );

  switch (Bar)
    {
      case SB_HORZ:
//        WIN_GetRectangles( Wnd, COORDS_WINDOW, NULL, Rect );
        Rect->top = Rect->bottom;
        Rect->bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (pWnd->style & WS_BORDER)
        {
            Rect->left--;
            Rect->right++;
	}
        else if (pWnd->style & WS_VSCROLL)
        {
            Rect->right++;
        }
        Vertical = FALSE;
        pSBData = &pSBInfo->Horz;
	break;

      case SB_VERT:
//        WIN_GetRectangles( Wnd, COORDS_WINDOW, NULL, Rect );
        if (pWnd->ExStyle & WS_EX_LEFTSCROLLBAR)
        {
            Rect->right = Rect->left;
            Rect->left -= GetSystemMetrics(SM_CXVSCROLL);
        }
        else
        {
            Rect->left = Rect->right;
            Rect->right += GetSystemMetrics(SM_CXVSCROLL);
        }
	if (pWnd->style & WS_BORDER)
        {
            Rect->top--;
            Rect->bottom++;
        }
        else if (pWnd->style & WS_HSCROLL)
        {
            Rect->bottom++;
        }
        Vertical = TRUE;
        pSBData = &pSBInfo->Vert;
	break;

      case SB_CTL:
        GetClientRect( Wnd, Rect );
        Vertical = (pWnd->style & SBS_VERT);
        pSBWnd = (PSBWND)pWnd;
        pSBData = (PSBDATA)&pSBWnd->SBCalc;
	break;

      default:
        return FALSE;
    }

  if (Vertical) Pixels = Rect->bottom - Rect->top;
  else Pixels = Rect->right - Rect->left;

  if (Pixels <= 2 * GetSystemMetrics(SM_CXVSCROLL) + SCROLL_MIN_RECT)
  {
      if (SCROLL_MIN_RECT < Pixels)
          *ArrowSize = (Pixels - SCROLL_MIN_RECT) / 2;
      else
          *ArrowSize = 0;
      *ThumbPos = *ThumbSize = 0;
  }
  else
  {
      *ArrowSize = GetSystemMetrics(SM_CXVSCROLL);
      Pixels -= (2 * (GetSystemMetrics(SM_CXVSCROLL) - SCROLL_ARROW_THUMB_OVERLAP));
      if (pSBData->page)
      {
          *ThumbSize = MulDiv(Pixels, pSBData->page, (pSBData->posMax - pSBData->posMin + 1));
          if (*ThumbSize < SCROLL_MIN_THUMB) *ThumbSize = SCROLL_MIN_THUMB;
      }
      else *ThumbSize = GetSystemMetrics(SM_CXVSCROLL);

      if (((Pixels -= *ThumbSize ) < 0) ||
          (( pSBInfo->WSBflags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH))
      {
          /* Rectangle too small or scrollbar disabled -> no thumb */
          *ThumbPos = *ThumbSize = 0;
      }
      else
      {
          INT Max = pSBData->posMax - max(pSBData->page - 1, 0);
          if (pSBData->posMin >= Max)
              *ThumbPos = *ArrowSize - SCROLL_ARROW_THUMB_OVERLAP;
          else
              *ThumbPos = *ArrowSize - SCROLL_ARROW_THUMB_OVERLAP
              + MulDiv(Pixels, (pSBData->pos - pSBData->posMin),(Max - pSBData->posMin));
      }
  }
  return Vertical;
}

/***********************************************************************
 *           IntScrollGetThumbVal
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT FASTCALL
IntScrollGetThumbVal(HWND Wnd, INT SBType, PSCROLLBARINFO ScrollBarInfo,
                     BOOL Vertical, INT Pos)
{
  PWND pWnd;
  PSBDATA pSBData;
  INT Pixels = Vertical ? ScrollBarInfo->rcScrollBar.bottom
                          - ScrollBarInfo->rcScrollBar.top
                        : ScrollBarInfo->rcScrollBar.right
                          - ScrollBarInfo->rcScrollBar.left;

  pWnd = ValidateHwnd( Wnd );
  if (!pWnd) return FALSE; 

  pSBData = IntGetSBData(pWnd, SBType);

  if ((Pixels -= 2 * ScrollBarInfo->dxyLineButton) <= 0)
  {
      return pSBData->posMin;
  }

  if ((Pixels -= (ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop)) <= 0)
  {
      return pSBData->posMin;
  }

  Pos = Pos - ScrollBarInfo->dxyLineButton;
  if (Pos < 0)
  {
      Pos = 0;
  }
  if (Pos > Pixels) Pos = Pixels;

  if (!pSBData->page)
      Pos *= pSBData->posMax - pSBData->posMin;
  else
      Pos *= pSBData->posMax - pSBData->posMin - pSBData->page + 1;

  return pSBData->posMin + ((Pos + Pixels / 2) / Pixels);
}

/***********************************************************************
 *           IntScrollClipPos
 */
static POINT IntScrollClipPos(PRECT lpRect, POINT pt)
{
    if( pt.x < lpRect->left )
        pt.x = lpRect->left;
    else
    if( pt.x > lpRect->right )
        pt.x = lpRect->right;

    if( pt.y < lpRect->top )
        pt.y = lpRect->top;
    else
    if( pt.y > lpRect->bottom )
        pt.y = lpRect->bottom;

    return pt;
}

/***********************************************************************
 *           IntScrollDrawSizeGrip
 *
 *  Draw the size grip.
 */
static void FASTCALL
IntScrollDrawSizeGrip(HWND Wnd, HDC Dc)
{
  RECT Rect;

  GetClientRect(Wnd, &Rect);
  FillRect(Dc, &Rect, GetSysColorBrush(COLOR_SCROLLBAR));
  Rect.left = max(Rect.left, Rect.right - GetSystemMetrics(SM_CXVSCROLL) - 1);
  Rect.top  = max(Rect.top, Rect.bottom - GetSystemMetrics(SM_CYHSCROLL) - 1);
  DrawFrameControl(Dc, &Rect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
}

/***********************************************************************
 *           SCROLL_RefreshScrollBar
 *
 * Repaint the scroll bar interior after a SetScrollRange() or
 * SetScrollPos() call.
 */
static void SCROLL_RefreshScrollBar( HWND hwnd, INT nBar,
				     BOOL arrows, BOOL interior )
{
    HDC hdc = GetDCEx( hwnd, 0,
                           DCX_CACHE | ((nBar == SB_CTL) ? 0 : DCX_WINDOW) );
    if (!hdc) return;

    IntDrawScrollBar( hwnd, hdc, nBar);//, arrows, interior );
    ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           IntScrollHandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars with focus).
 */
static void FASTCALL
IntScrollHandleKbdEvent(
  HWND Wnd /* [in] Handle of window with scrollbar(s) */,
  WPARAM wParam /* [in] Variable input including enable state */,
  LPARAM lParam /* [in] Variable input including input point */)
{
  TRACE("Wnd=%p wParam=%ld lParam=%ld\n", Wnd, wParam, lParam);

  /* hide caret on first KEYDOWN to prevent flicker */
  if (0 == (lParam & PFD_DOUBLEBUFFER_DONTCARE))
    {
      HideCaret(Wnd);
    }

  switch(wParam)
    {
      case VK_PRIOR:
        wParam = SB_PAGEUP;
        break;

      case VK_NEXT:
        wParam = SB_PAGEDOWN;
        break;

      case VK_HOME:
        wParam = SB_TOP;
        break;

      case VK_END:
        wParam = SB_BOTTOM;
        break;

      case VK_UP:
        wParam = SB_LINEUP;
        break;

      case VK_DOWN:
        wParam = SB_LINEDOWN;
        break;

      default:
        return;
    }

  SendMessageW(GetParent(Wnd),
               (0 != (GetWindowLongPtrW(Wnd, GWL_STYLE ) & SBS_VERT) ?
                WM_VSCROLL : WM_HSCROLL), wParam, (LPARAM) Wnd);
}

/***********************************************************************
 *           IntScrollHandleScrollEvent
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'Pt' is the location of the mouse event in drawing coordinates
 */
static VOID FASTCALL
IntScrollHandleScrollEvent(HWND Wnd, INT SBType, UINT Msg, POINT Pt)
{
  static POINT PrevPt;           /* Previous mouse position for timer events */
  static UINT TrackThumbPos;     /* Thumb position when tracking started. */
  static INT LastClickPos;       /* Position in the scroll-bar of the last
                                    button-down event. */
  static INT LastMousePos;       /* Position in the scroll-bar of the last
                                    mouse event. */

  DWORD HitTest;
  HWND WndOwner, WndCtl;
  BOOL Vertical;
  HDC Dc;
  SCROLLBARINFO ScrollBarInfo;
  SETSCROLLBARINFO NewInfo;

  if (! IntGetScrollBarInfo(Wnd, SBType, &ScrollBarInfo))
  {
     return;
  }
  if ((ScrollTrackHitTest == SCROLL_NOWHERE) && (Msg != WM_LBUTTONDOWN))
  {
     //// ReactOS : Justin Case something goes wrong.
     if (Wnd == GetCapture())
     {
        ReleaseCapture();
     }
     ////
     return;
  }

  NewInfo.nTrackPos = ScrollTrackingVal;
  NewInfo.reserved = ScrollBarInfo.reserved;
  memcpy(NewInfo.rgstate, ScrollBarInfo.rgstate, (CCHILDREN_SCROLLBAR + 1) * sizeof(DWORD));

  if (SBType == SB_CTL && (GetWindowLongPtrW(Wnd, GWL_STYLE) & (SBS_SIZEGRIP | SBS_SIZEBOX)))
  {
      switch(Msg)
      {
          case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
            HideCaret(Wnd);  /* hide caret while holding down LBUTTON */
            SetCapture(Wnd);
            PrevPt = Pt;
            ScrollTrackHitTest = HitTest = SCROLL_THUMB;
            break;
          case WM_MOUSEMOVE:
            GetClientRect(GetParent(GetParent(Wnd)), &ScrollBarInfo.rcScrollBar);
            PrevPt = Pt;
            break;
          case WM_LBUTTONUP:
            ReleaseCapture();
            ScrollTrackHitTest = HitTest = SCROLL_NOWHERE;
            if (Wnd == GetFocus()) ShowCaret(Wnd);
            break;
          case WM_SYSTIMER:
            Pt = PrevPt;
            break;
      }
      return;
  }

  Dc = GetDCEx(Wnd, 0, DCX_CACHE | ((SB_CTL == SBType) ? 0 : DCX_WINDOW));
  if (SB_VERT == SBType)
  {
      Vertical = TRUE;
  }
  else if (SB_HORZ == SBType)
  {
      Vertical = FALSE;
  }
  else
  {
      Vertical = (0 != (GetWindowLongPtrW(Wnd, GWL_STYLE) & SBS_VERT));
  }
  WndOwner = (SB_CTL == SBType) ? GetParent(Wnd) : Wnd;
  WndCtl   = (SB_CTL == SBType) ? Wnd : NULL;

  switch (Msg)
  {
      case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
        HideCaret(Wnd);     /* hide caret while holding down LBUTTON */
        ScrollTrackVertical = Vertical;
        ScrollTrackHitTest  = HitTest = IntScrollHitTest(&ScrollBarInfo, Vertical, Pt, FALSE );
        LastClickPos  = Vertical ? (Pt.y - ScrollBarInfo.rcScrollBar.top)
                        : (Pt.x - ScrollBarInfo.rcScrollBar.left);
        LastMousePos  = LastClickPos;
        TrackThumbPos = ScrollBarInfo.xyThumbTop;
        PrevPt = Pt;
        if (SBType == SB_CTL && (GetWindowLongPtrW(Wnd, GWL_STYLE) & WS_TABSTOP)) SetFocus(Wnd);
        SetCapture(Wnd);
        ScrollBarInfo.rgstate[ScrollTrackHitTest] |= STATE_SYSTEM_PRESSED;
        NewInfo.rgstate[ScrollTrackHitTest] = ScrollBarInfo.rgstate[ScrollTrackHitTest];
        NtUserSetScrollBarInfo(Wnd, IntScrollGetObjectId(SBType), &NewInfo);
        break;

      case WM_MOUSEMOVE:
        HitTest = IntScrollHitTest(&ScrollBarInfo, Vertical, Pt, TRUE);
        PrevPt = Pt;
        break;

      case WM_LBUTTONUP:
        HitTest = SCROLL_NOWHERE;
        ReleaseCapture();
        /* if scrollbar has focus, show back caret */
        if (Wnd == GetFocus()) ShowCaret(Wnd);
        ScrollBarInfo.rgstate[ScrollTrackHitTest] &= ~STATE_SYSTEM_PRESSED;
        NewInfo.rgstate[ScrollTrackHitTest] = ScrollBarInfo.rgstate[ScrollTrackHitTest];
        NtUserSetScrollBarInfo(Wnd, IntScrollGetObjectId(SBType), &NewInfo);

        IntDrawScrollArrows(Dc, &ScrollBarInfo, Vertical);

        break;

      case WM_SYSTIMER:
        Pt = PrevPt;
        HitTest = IntScrollHitTest(&ScrollBarInfo, Vertical, Pt, FALSE);
        break;

      default:
          return;  /* Should never happen */
  }

  TRACE("Event: hwnd=%p bar=%d msg=%s pt=%d,%d hit=%d\n",
            Wnd, SBType, SPY_GetMsgName(Msg,Wnd), Pt.x, Pt.y, HitTest );

  switch (ScrollTrackHitTest)
    {
      case SCROLL_NOWHERE:  /* No tracking in progress */
        break;

      case SCROLL_TOP_ARROW:
        if (HitTest == ScrollTrackHitTest)
          {
            if ((WM_LBUTTONDOWN == Msg) || (WM_SYSTIMER == Msg))
              {
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEUP, (LPARAM) WndCtl);
              }
	    SetSystemTimer(Wnd, SCROLL_TIMER, (WM_LBUTTONDOWN == Msg) ?
                           SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                           (TIMERPROC) NULL);
          }
        else
          {
            KillSystemTimer(Wnd, SCROLL_TIMER);
          }
        break;

      case SCROLL_TOP_RECT:
        if (HitTest == ScrollTrackHitTest)
          {
            if ((WM_LBUTTONDOWN == Msg) || (WM_SYSTIMER == Msg))
              {
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEUP, (LPARAM) WndCtl);
              }
            SetSystemTimer(Wnd, SCROLL_TIMER, (WM_LBUTTONDOWN == Msg) ?
                           SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                           (TIMERPROC) NULL);
          }
        else
          {
            KillSystemTimer(Wnd, SCROLL_TIMER);
          }
        break;

      case SCROLL_THUMB:
        if (Msg == WM_LBUTTONDOWN)
          {
            ScrollTrackingWin = Wnd;
            ScrollTrackingBar = SBType;
            ScrollTrackingPos = TrackThumbPos + LastMousePos - LastClickPos;
            ScrollTrackingVal = IntScrollGetThumbVal(Wnd, SBType, &ScrollBarInfo,
                                                     Vertical, ScrollTrackingPos);
            NewInfo.nTrackPos = ScrollTrackingVal;
            NtUserSetScrollBarInfo(Wnd, IntScrollGetObjectId(SBType), &NewInfo);
            IntScrollDrawMovingThumb(Dc, &ScrollBarInfo, Vertical);
          }
        else if (Msg == WM_LBUTTONUP)
          {
            ScrollTrackingWin = 0;
            ScrollTrackingVal = 0;
            IntDrawScrollInterior(Wnd, Dc, SBType, Vertical, &ScrollBarInfo);
          }
        else  /* WM_MOUSEMOVE */
          {
            UINT Pos;

            if (! IntScrollPtInRectEx(&ScrollBarInfo.rcScrollBar, Pt, Vertical))
              {
                Pos = LastClickPos;
              }
            else
              {
                Pt = IntScrollClipPos(&ScrollBarInfo.rcScrollBar, Pt);
		Pos = Vertical ? (Pt.y - ScrollBarInfo.rcScrollBar.top)
                               : (Pt.x - ScrollBarInfo.rcScrollBar.left);
              }
            if (Pos != LastMousePos || ! ScrollMovingThumb)
              {
                LastMousePos = Pos;
                ScrollTrackingPos = TrackThumbPos + Pos - LastClickPos;
                ScrollTrackingVal = IntScrollGetThumbVal(Wnd, SBType, &ScrollBarInfo,
                                                         Vertical, ScrollTrackingPos);
                NewInfo.nTrackPos = ScrollTrackingVal;
                NtUserSetScrollBarInfo(Wnd, IntScrollGetObjectId(SBType), &NewInfo);
                IntScrollDrawMovingThumb(Dc, &ScrollBarInfo, Vertical);
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             MAKEWPARAM(SB_THUMBTRACK, ScrollTrackingVal),
                             (LPARAM) WndCtl);
             }
        }
        break;

      case SCROLL_BOTTOM_RECT:
        if (HitTest == ScrollTrackHitTest)
          {
            if ((Msg == WM_LBUTTONDOWN) || (Msg == WM_SYSTIMER))
              {
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_PAGEDOWN, (LPARAM) WndCtl);
              }
            SetSystemTimer(Wnd, SCROLL_TIMER, (WM_LBUTTONDOWN == Msg) ?
                           SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                           (TIMERPROC) NULL);
          }
        else
          {
            KillSystemTimer(Wnd, SCROLL_TIMER);
          }
        break;

      case SCROLL_BOTTOM_ARROW:
        if (HitTest == ScrollTrackHitTest)
          {
            if ((Msg == WM_LBUTTONDOWN) || (Msg == WM_SYSTIMER))
              {
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEDOWN, (LPARAM) WndCtl);
              }
	    SetSystemTimer(Wnd, SCROLL_TIMER, (WM_LBUTTONDOWN == Msg) ?
                           SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                           (TIMERPROC) NULL);
          }
        else KillSystemTimer(Wnd, SCROLL_TIMER);
        break;
    }

  if (Msg == WM_LBUTTONDOWN)
    {
      if (SCROLL_THUMB == HitTest)
        {
          UINT Val = IntScrollGetThumbVal(Wnd, SBType, &ScrollBarInfo, Vertical,
                                          TrackThumbPos + LastMousePos - LastClickPos);
          SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                       MAKEWPARAM(SB_THUMBTRACK, Val), (LPARAM) WndCtl);
        }
    }

  if (Msg == WM_LBUTTONUP)
    {
      HitTest = ScrollTrackHitTest;
      ScrollTrackHitTest = SCROLL_NOWHERE;  /* Terminate tracking */

      if (SCROLL_THUMB == HitTest)
        {
          UINT Val = IntScrollGetThumbVal(Wnd, SBType, &ScrollBarInfo, Vertical,
                                          TrackThumbPos + LastMousePos - LastClickPos);
          SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                       MAKEWPARAM(SB_THUMBPOSITION, Val), (LPARAM) WndCtl);
        }
      /* SB_ENDSCROLL doesn't report thumb position */
      SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                   SB_ENDSCROLL, (LPARAM) WndCtl);

      /* Terminate tracking */
      ScrollTrackingWin = 0;
    }

  ReleaseDC(Wnd, Dc);
}


/***********************************************************************
 *           IntScrollCreateScrollBar
 *
 *  Create a scroll bar
 */
static void IntScrollCreateScrollBar(
  HWND Wnd /* [in] Handle of window with scrollbar(s) */,
  LPCREATESTRUCTW lpCreate /* [in] The style and place of the scroll bar */)
{
  SCROLLINFO Info;

  Info.cbSize = sizeof(SCROLLINFO);
  Info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
  Info.nMin = 0;
  Info.nMax = 100;
  Info.nPage = 0;
  Info.nPos = 0;
  Info.nTrackPos = 0;
  NtUserSetScrollInfo(Wnd, SB_CTL, &Info, FALSE);

  TRACE("hwnd=%p lpCreate=%p\n", Wnd, lpCreate);

#if 0 /* FIXME */
  if (lpCreate->style & WS_DISABLED)
  {
  //    info->flags = ESB_DISABLE_BOTH;
      //NtUserEnableScrollBar(Wnd,SB_CTL,(wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH));
      NtUserMessageCall( Wnd, WM_ENABLE, FALSE, 0, 0, FNID_SCROLLBAR, FALSE);
      ERR("Created WS_DISABLED scrollbar\n");
  }
#endif
  if (0 != (lpCreate->style & (SBS_SIZEGRIP | SBS_SIZEBOX)))
    {
      if (0 != (lpCreate->style & SBS_SIZEBOXTOPLEFTALIGN))
        {
          MoveWindow(Wnd, lpCreate->x, lpCreate->y, GetSystemMetrics(SM_CXVSCROLL) + 1,
                     GetSystemMetrics(SM_CYHSCROLL) + 1, FALSE);
        }
      else if (0 != (lpCreate->style & SBS_SIZEBOXBOTTOMRIGHTALIGN))
        {
          MoveWindow(Wnd, lpCreate->x + lpCreate->cx - GetSystemMetrics(SM_CXVSCROLL) - 1,
                     lpCreate->y + lpCreate->cy - GetSystemMetrics(SM_CYHSCROLL) - 1,
                     GetSystemMetrics(SM_CXVSCROLL) + 1,
                     GetSystemMetrics(SM_CYHSCROLL) + 1, FALSE);
        }
    }
  else if (0 != (lpCreate->style & SBS_VERT))
    {
      if (0 != (lpCreate->style & SBS_LEFTALIGN))
        {
          MoveWindow(Wnd, lpCreate->x, lpCreate->y,
                     GetSystemMetrics(SM_CXVSCROLL) + 1, lpCreate->cy, FALSE);
        }
      else if (0 != (lpCreate->style & SBS_RIGHTALIGN))
        {
          MoveWindow(Wnd,
                     lpCreate->x + lpCreate->cx - GetSystemMetrics(SM_CXVSCROLL) - 1,
                     lpCreate->y,
                      GetSystemMetrics(SM_CXVSCROLL) + 1, lpCreate->cy, FALSE);
        }
    }
  else  /* SBS_HORZ */
    {
      if (0 != (lpCreate->style & SBS_TOPALIGN))
        {
          MoveWindow(Wnd, lpCreate->x, lpCreate->y,
                     lpCreate->cx, GetSystemMetrics(SM_CYHSCROLL) + 1, FALSE);
        }
      else if (0 != (lpCreate->style & SBS_BOTTOMALIGN))
        {
          MoveWindow(Wnd,
                     lpCreate->x,
                     lpCreate->y + lpCreate->cy - GetSystemMetrics(SM_CYHSCROLL) - 1,
                     lpCreate->cx, GetSystemMetrics(SM_CYHSCROLL) + 1, FALSE);
        }
    }
}

/* USER32 INTERNAL FUNCTIONS **************************************************/

/***********************************************************************
 *           ScrollTrackScrollBar
 *
 * Track a mouse button press on a scroll-bar.
 * pt is in screen-coordinates for non-client scroll bars.
 */
VOID FASTCALL
ScrollTrackScrollBar(HWND Wnd, INT SBType, POINT Pt)
{
  MSG Msg;
  UINT XOffset = 0, YOffset = 0;

  if (SBType != SB_CTL)
  { // Used with CMD mouse tracking.
      PWND pwnd = ValidateHwnd(Wnd);
      if (!pwnd) return;
      XOffset = pwnd->rcClient.left - pwnd->rcWindow.left;
      YOffset = pwnd->rcClient.top - pwnd->rcWindow.top;
//      RECT rect;
//      WIN_GetRectangles( Wnd, COORDS_CLIENT, &rect, NULL );
      ScreenToClient(Wnd, &Pt);
//      Pt.x -= rect.left;
//      Pt.y -= rect.top;
      Pt.x += XOffset;
      Pt.y += YOffset;
  }

  IntScrollHandleScrollEvent(Wnd, SBType, WM_LBUTTONDOWN, Pt);

  do
  {
      if (!GetMessageW(&Msg, 0, 0, 0)) break;
      if (CallMsgFilterW(&Msg, MSGF_SCROLLBAR)) continue;
      if ( Msg.message == WM_LBUTTONUP ||
           Msg.message == WM_MOUSEMOVE ||
          (Msg.message == WM_SYSTIMER && Msg.wParam == SCROLL_TIMER))
      {
          Pt.x = LOWORD(Msg.lParam) + XOffset;
          Pt.y = HIWORD(Msg.lParam) + YOffset;
          IntScrollHandleScrollEvent(Wnd, SBType, Msg.message, Pt);
      }
      else
      {
          TranslateMessage(&Msg);
          DispatchMessageW(&Msg);
      }
      if (!IsWindow(Wnd))
      {
          ReleaseCapture();
          break;
      }
   } while (Msg.message != WM_LBUTTONUP && GetCapture() == Wnd);
}


static DWORD FASTCALL
IntSetScrollInfo(HWND Wnd, LPCSCROLLINFO Info, BOOL bRedraw)
{
   DWORD Ret = NtUserSetScrollInfo(Wnd, SB_CTL, Info, bRedraw);
   if (Ret) IntNotifyWinEvent(EVENT_OBJECT_VALUECHANGE, Wnd, OBJID_CLIENT, CHILDID_SELF, WEF_SETBYWNDPTI);
   return Ret;
}


/***********************************************************************
 *           ScrollBarWndProc
 */
LRESULT WINAPI
ScrollBarWndProc_common(WNDPROC DefWindowProc, HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
#ifdef __REACTOS__ // Do this now, remove after Server side is fixed.
  PWND pWnd;
  PSBWND pSBWnd;
  SCROLLINFO ScrollInfo;

  pWnd = ValidateHwnd(Wnd);
  if (pWnd)
  {
     if (!pWnd->fnid)
     {
        ERR("ScrollBar CTL size %d\n",(sizeof(SBWND)-sizeof(WND)));
        if ( pWnd->cbwndExtra != (sizeof(SBWND)-sizeof(WND)) )
        {
           ERR("Wrong Extra bytes for Scrollbar!\n");
           return 0;
        }
     
        if (Msg != WM_CREATE)
        {
           return DefWindowProc(Wnd, Msg, wParam, lParam);
        }
        NtUserSetWindowFNID(Wnd, FNID_SCROLLBAR);
     }
     else
     {
        if (pWnd->fnid != FNID_SCROLLBAR)
        {
           ERR("Wrong window class for Scrollbar!\n");
           return 0;
        }
     }
  }    
#endif    

  if (! IsWindow(Wnd))
    {
      return 0;
    }

  // Must be a scroll bar control!
  pSBWnd = (PSBWND)pWnd;

  switch (Msg)
    {
      case WM_CREATE:
        IntScrollCreateScrollBar(Wnd, (LPCREATESTRUCTW) lParam);
        break;

      case WM_ENABLE:
        {
          return SendMessageW( Wnd, SBM_ENABLE_ARROWS, wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH, 0);
        }

      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONDOWN:
        if (GetWindowLongW( Wnd, GWL_STYLE ) & SBS_SIZEGRIP)
        {
           SendMessageW( GetParent(Wnd), WM_SYSCOMMAND,
                         SC_SIZE + ((GetWindowLongW( Wnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) ?
                                   WMSZ_BOTTOMLEFT : WMSZ_BOTTOMRIGHT), lParam );
        }
        else
        {
          POINT Pt;
          Pt.x = (short)LOWORD(lParam);
          Pt.y = (short)HIWORD(lParam);
          ScrollTrackScrollBar(Wnd, SB_CTL, Pt);
	}
        break;

      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
      case WM_SYSTIMER:
        {
          POINT Pt;
          Pt.x = (short)LOWORD(lParam);
          Pt.y = (short)HIWORD(lParam);
          IntScrollHandleScrollEvent(Wnd, SB_CTL, Msg, Pt);
        }
        break;

      case WM_KEYDOWN:
        IntScrollHandleKbdEvent(Wnd, wParam, lParam);
        break;

      case WM_KEYUP:
        ShowCaret(Wnd);
        break;

      case WM_SETFOCUS:
        {
          /* Create a caret when a ScrollBar get focus */
          RECT Rect;
          int ArrowSize, ThumbSize, ThumbPos, Vertical;

          Vertical = IntScrollGetScrollBarRect(Wnd, SB_CTL, &Rect,
                                               &ArrowSize, &ThumbSize, &ThumbPos);
          if (! Vertical)
            {
              CreateCaret(Wnd, (HBITMAP) 1, ThumbSize - 2, Rect.bottom - Rect.top - 2);
              SetCaretPos(ThumbPos + 1, Rect.top + 1);
            }
          else
            {
              CreateCaret(Wnd, (HBITMAP) 1, Rect.right - Rect.left - 2, ThumbSize - 2);
              SetCaretPos(Rect.top + 1, ThumbPos + 1);
            }
          ShowCaret(Wnd);
        }
        break;

      case WM_KILLFOCUS:
        {
          RECT Rect;
          int ArrowSize, ThumbSize, ThumbPos, Vertical;

          Vertical = IntScrollGetScrollBarRect(Wnd, SB_CTL, &Rect,
                                               &ArrowSize, &ThumbSize, &ThumbPos);
          if (! Vertical)
            {
              Rect.left = ThumbPos + 1;
              Rect.right = Rect.left + ThumbSize;
            }
          else
            {
              Rect.top = ThumbPos + 1;
              Rect.bottom = Rect.top + ThumbSize;
            }
          HideCaret(Wnd);
          InvalidateRect(Wnd, &Rect, FALSE);
          DestroyCaret();
        }
        break;

      case WM_ERASEBKGND:
         return 1;

      case WM_GETDLGCODE:
         return DLGC_WANTARROWS; /* Windows returns this value */

      case WM_PAINT:
        {
          PAINTSTRUCT Ps;
          HDC Dc;

          Dc = (0 != wParam ? (HDC) wParam : BeginPaint(Wnd, &Ps));

          if (GetWindowLongPtrW(Wnd, GWL_STYLE) & SBS_SIZEGRIP)
            {
              IntScrollDrawSizeGrip(Wnd, Dc);
            }
          else if (0 != (GetWindowLongPtrW(Wnd, GWL_STYLE) & SBS_SIZEBOX))
            {
              RECT Rect;
              GetClientRect(Wnd, &Rect);
              FillRect(Dc, &Rect, GetSysColorBrush(COLOR_SCROLLBAR));
            }
          else
            {
              IntDrawScrollBar(Wnd, Dc, SB_CTL/*, TRUE, TRUE*/);
            }

          if (0 == wParam)
            {
              EndPaint(Wnd, &Ps);
            }
        }
        break;

      case SBM_GETPOS:
        return pSBWnd->SBCalc.pos;

      case SBM_GETRANGE:
        *(LPINT)wParam = pSBWnd->SBCalc.posMin;
        *(LPINT)lParam = pSBWnd->SBCalc.posMax;
        // This message does not return a value.
        return 0;
 
      case SBM_ENABLE_ARROWS:
        return EnableScrollBar( Wnd, SB_CTL, wParam );

      case SBM_SETPOS:
        {
           ScrollInfo.cbSize = sizeof(SCROLLINFO);
           ScrollInfo.fMask = SIF_POS|SIF_PREVIOUSPOS;
           ScrollInfo.nPos = wParam;
           return IntSetScrollInfo(Wnd, &ScrollInfo, lParam);
        }

      case SBM_SETRANGEREDRAW:
      case SBM_SETRANGE:
        {
           ScrollInfo.cbSize = sizeof(SCROLLINFO);
           ScrollInfo.fMask = SIF_RANGE|SIF_PREVIOUSPOS;
           ScrollInfo.nMin = wParam;
           ScrollInfo.nMax = lParam;
           return IntSetScrollInfo(Wnd, &ScrollInfo, Msg == SBM_SETRANGEREDRAW ? TRUE : FALSE);
        }

      case SBM_SETSCROLLINFO:
        return IntSetScrollInfo(Wnd, (LPCSCROLLINFO)lParam, wParam);

      case SBM_GETSCROLLINFO:
        {
         PSBDATA pSBData = (PSBDATA)&pSBWnd->SBCalc;
         DWORD ret = NtUserSBGetParms(Wnd, SB_CTL, pSBData, (SCROLLINFO *) lParam);
         if (!ret)
         {
            ERR("SBM_GETSCROLLINFO No ScrollInfo\n");
         }
         return ret;
        }
      case SBM_GETSCROLLBARINFO:
        ((PSCROLLBARINFO)lParam)->cbSize = sizeof(SCROLLBARINFO);
        return NtUserGetScrollBarInfo(Wnd, OBJID_CLIENT, (PSCROLLBARINFO)lParam);

      case 0x00e5:
      case 0x00e7:
      case 0x00e8:
      case 0x00ec:
      case 0x00ed:
      case 0x00ee:
      case 0x00ef:
        WARN("unknown Win32 msg %04x wp=%08lx lp=%08lx\n",
		Msg, wParam, lParam );
        break;

      default:
        if (WM_USER <= Msg)
          {
            WARN("unknown msg %04x wp=%04lx lp=%08lx\n", Msg, wParam, lParam);
          }
        if (unicode)
            return DefWindowProcW( Wnd, Msg, wParam, lParam );
        else
            return DefWindowProcA( Wnd, Msg, wParam, lParam );
    }

  return 0;
}

LRESULT WINAPI
ScrollBarWndProcW(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return ScrollBarWndProc_common(DefWindowProcW, Wnd, Msg, wParam, lParam, TRUE);
}

LRESULT WINAPI
ScrollBarWndProcA(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return ScrollBarWndProc_common(DefWindowProcA, Wnd, Msg, wParam, lParam, FALSE);
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
EnableScrollBar( HWND hwnd, UINT nBar, UINT flags )
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

     /* Bypass SEH and go direct. */
   if (!Hook)
   {
      Ret = NtUserEnableScrollBar(hwnd, nBar, flags);
      if (!Ret) return Ret;
      SCROLL_RefreshScrollBar( hwnd, nBar, TRUE, TRUE );
      return Ret;
   }
   _SEH2_TRY
   {
      Ret = guah.EnableScrollBar(hwnd, nBar, flags);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

BOOL WINAPI
RealGetScrollInfo(HWND Wnd, INT SBType, LPSCROLLINFO Info)
{
  PWND pWnd;
  PSBDATA pSBData;

  if (SB_CTL == SBType)
  {
     return SendMessageW(Wnd, SBM_GETSCROLLINFO, 0, (LPARAM) Info);
  }

  pWnd = ValidateHwnd(Wnd);
  if (!pWnd) return FALSE;

  if (SBType < SB_HORZ || SBType > SB_VERT)
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return FALSE;
  }
  if (!pWnd->pSBInfo)
  {
     SetLastError(ERROR_NO_SCROLLBARS);
     return FALSE;
  }
  pSBData = IntGetSBData(pWnd, SBType);
  return NtUserSBGetParms(Wnd, SBType, pSBData, Info);
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetScrollInfo(HWND Wnd, INT SBType, LPSCROLLINFO Info)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

     /* Bypass SEH and go direct. */
   if (!Hook) return RealGetScrollInfo(Wnd, SBType, Info);

   _SEH2_TRY
   {
      Ret = guah.GetScrollInfo(Wnd, SBType, Info);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

/*
 * @implemented
 */
INT
WINAPI
DECLSPEC_HOTPATCH
GetScrollPos(HWND Wnd, INT Bar)
{
  PWND pwnd;
  PSBDATA pSBData;

  TRACE("Wnd=%p Bar=%d\n", Wnd, Bar);

  /* Refer SB_CTL requests to the window */
  if (SB_CTL == Bar)
  {
      return SendMessageW(Wnd, SBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
  }
  else if (Bar == SB_HORZ || Bar == SB_VERT )
  {
     pwnd = ValidateHwnd(Wnd);
     if (!pwnd) return 0;

     if (pwnd->pSBInfo)
     {
        pSBData = IntGetSBData(pwnd, Bar);
        return pSBData->pos;
     }

     SetLastError(ERROR_NO_SCROLLBARS);
     TRACE("GetScrollPos No Scroll Info\n");
     return 0;
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetScrollRange(HWND Wnd, int Bar, LPINT MinPos, LPINT MaxPos)
{
  PWND pwnd;
  PSBDATA pSBData;  

  TRACE("Wnd=%x Bar=%d Min=%p Max=%p\n", Wnd, Bar, MinPos, MaxPos);

  /* Refer SB_CTL requests to the window */
  if (SB_CTL == Bar)
  {
      return SendMessageW(Wnd, SBM_GETRANGE, (WPARAM) MinPos, (LPARAM) MaxPos);
  }
  else if (Bar == SB_HORZ || Bar == SB_VERT )
  {
      pwnd = ValidateHwnd(Wnd);
      if (!pwnd) return FALSE;

      if (pwnd->pSBInfo)
      {
         pSBData = IntGetSBData(pwnd, Bar);
         *MinPos = pSBData->posMin;
         *MaxPos = pSBData->posMax;
      }
      else
      {
         SetLastError(ERROR_NO_SCROLLBARS);
         *MinPos = 0;
         *MaxPos = 0;
      }
      return TRUE;
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
}

INT WINAPI
RealSetScrollInfo(HWND Wnd, int SBType, LPCSCROLLINFO Info, BOOL bRedraw)
{
  if (SB_CTL == SBType)
    {
      return SendMessageW(Wnd, SBM_SETSCROLLINFO, (WPARAM) bRedraw, (LPARAM) Info);
    }
  else
    {
      return NtUserSetScrollInfo(Wnd, SBType, Info, bRedraw);
    }
}

/*
 * @implemented
 */
INT
WINAPI
DECLSPEC_HOTPATCH
SetScrollInfo(HWND Wnd, int SBType, LPCSCROLLINFO Info, BOOL bRedraw)
{
   BOOL Hook;
   INT Ret = 0;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

     /* Bypass SEH and go direct. */
   if (!Hook) return RealSetScrollInfo(Wnd, SBType, Info, bRedraw);

   _SEH2_TRY
   {
      Ret = guah.SetScrollInfo(Wnd, SBType, Info, bRedraw);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;

}

/*
 * @implemented
 */
INT
WINAPI
DECLSPEC_HOTPATCH
SetScrollPos(HWND hWnd, INT nBar, INT nPos, BOOL bRedraw)
{
  SCROLLINFO ScrollInfo;

  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_POS|SIF_PREVIOUSPOS;
  ScrollInfo.nPos = nPos;

  return RealSetScrollInfo(hWnd, nBar, &ScrollInfo, bRedraw);
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetScrollRange(HWND hWnd, INT nBar, INT nMinPos, INT nMaxPos, BOOL bRedraw)
{
  PWND pWnd;
  SCROLLINFO ScrollInfo;

  pWnd = ValidateHwnd(hWnd);
  if ( !pWnd ) return FALSE;

  if ((nMaxPos - nMinPos) > MAXLONG)
  {
     SetLastError(ERROR_INVALID_SCROLLBAR_RANGE);
     return FALSE;
  }

  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_RANGE;
  ScrollInfo.nMin = nMinPos;
  ScrollInfo.nMax = nMaxPos;
  SetScrollInfo(hWnd, nBar, &ScrollInfo, bRedraw); // do not bypass themes.
  return TRUE;
}
