/* $Id $
 *
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
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <draw.h>
#include <stdlib.h>
#include <string.h>
#include <oleacc.h>
#include <user32/regcontrol.h>
#include <rosrtl/minmax.h>

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

 /* Thumb-tracking info */
static HWND ScrollTrackingWin = 0;
static INT  ScrollTrackingBar = 0;
static INT  ScrollTrackingPos = 0;
static INT  ScrollTrackingVal = 0;
static BOOL ScrollMovingThumb = FALSE;

static DWORD ScrollTrackHitTest = SCROLL_NOWHERE;
static BOOL ScrollTrackVertical;

HBRUSH DefWndControlColor(HDC hDC, UINT ctlType);

static LRESULT WINAPI ScrollBarWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

UINT STDCALL SetSystemTimer(HWND,UINT_PTR,UINT,TIMERPROC);
WINBOOL STDCALL KillSystemTimer(HWND,UINT_PTR);

/*********************************************************************
 * scrollbar class descriptor
 */
const struct builtin_class_descr SCROLL_builtin_class =
{
    L"ScrollBar",           /* name */
    CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style */
    ScrollBarWndProc,       /* procW */
    NULL,                   /* procA (winproc is Unicode only) */
    0,                      /* extra */
    IDC_ARROW,              /* cursor */
    0                       /* brush */
};

/* PRIVATE FUNCTIONS **********************************************************/


static void
IntDrawScrollInterior(HWND hWnd, HDC hDC, INT nBar, BOOL Vertical,
   PSCROLLBARINFO ScrollBarInfo)
{
   INT ThumbSize = ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
   INT ThumbTop = ScrollBarInfo->xyThumbTop;
   RECT Rect;
   HPEN hSavePen;
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
      hBrush = (HBRUSH)SendMessageW(GetParent(hWnd), WM_CTLCOLORSCROLLBAR, (WPARAM)hDC, (LPARAM)hWnd);
      if (!hBrush)
         hBrush = GetSysColorBrush(COLOR_SCROLLBAR);
   }
   else
   {
      hBrush = DefWndControlColor(hDC, CTLCOLOR_SCROLLBAR);
   }

   hSavePen = SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
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
      SelectObject(hDC, hSavePen);
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
   SelectObject(hDC, hSavePen);
   SelectObject(hDC, hSaveBrush);
}

STATIC VOID FASTCALL
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

STATIC VOID FASTCALL
IntScrollDrawMovingThumb(HDC Dc, PSCROLLBARINFO ScrollBarInfo, BOOL Vertical)
{
  INT Pos = ScrollTrackingPos;
  INT MaxSize;
  INT OldTop;

  if (Vertical)
    {
      MaxSize = ScrollBarInfo->rcScrollBar.bottom - ScrollBarInfo->rcScrollBar.top;
    }
  else
    {
      MaxSize = ScrollBarInfo->rcScrollBar.right - ScrollBarInfo->rcScrollBar.left;
    }

  MaxSize -= ScrollBarInfo->dxyLineButton + ScrollBarInfo->xyThumbBottom
             - ScrollBarInfo->xyThumbTop;

  if (Pos < ScrollBarInfo->dxyLineButton)
    {
      Pos = ScrollBarInfo->dxyLineButton;
    }
  else if (MaxSize < Pos)
    {
      Pos = MaxSize;
    }

  OldTop = ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbBottom = Pos + ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbTop = Pos;
  IntDrawScrollInterior(ScrollTrackingWin, Dc, ScrollTrackingBar, Vertical, ScrollBarInfo);
  ScrollBarInfo->xyThumbBottom = OldTop + ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
  ScrollBarInfo->xyThumbTop = OldTop;

  ScrollMovingThumb = ! ScrollMovingThumb;
}

STATIC LONG FASTCALL
IntScrollGetObjectId(INT SBType)
{
  if (SB_VERT == SBType)
    {
      return OBJID_VSCROLL;
    }
  if (SB_HORZ == SBType)
    {
      return OBJID_HSCROLL;
    }

  return OBJID_CLIENT;
}

STATIC BOOL FASTCALL
IntGetScrollBarInfo(HWND Wnd, INT Bar, PSCROLLBARINFO ScrollBarInfo)
{  
  ScrollBarInfo->cbSize = sizeof(SCROLLBARINFO);

  return NtUserGetScrollBarInfo(Wnd, IntScrollGetObjectId(Bar), ScrollBarInfo);
}

void
IntDrawScrollBar(HWND Wnd, HDC DC, INT Bar)
{
  INT ThumbSize;
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
        Vertical = (GetWindowLongW(Wnd, GWL_STYLE) & SBS_VERT) != 0;
        break;

      default:
        return;
    }
  if (! IntGetScrollBarInfo(Wnd, Bar, &Info))
    {
      return;
    }
  
  if (IsRectEmpty(&Info.rcScrollBar))
    {
      return;
    }

  ThumbSize = Info.xyThumbBottom - Info.xyThumbTop;

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

STATIC BOOL FASTCALL
IntScrollPtInRectEx(LPRECT Rect, POINT Pt, BOOL Vertical)
{
  RECT TempRect = *Rect;
  if (Vertical)
    {
      TempRect.left -= Rect->right - Rect->left;
      TempRect.right += Rect->right - Rect->left;
    }
   else
    {
      TempRect.top -= Rect->bottom - Rect->top;
      TempRect.bottom += Rect->bottom - Rect->top;
    }

   return PtInRect(&TempRect, Pt);
}

STATIC DWORD FASTCALL
IntScrollHitTest(PSCROLLBARINFO ScrollBarInfo, BOOL Vertical, POINT Pt, BOOL Dragging)
{
  INT ArrowSize, ThumbSize, ThumbPos;

  if ((Dragging && ! IntScrollPtInRectEx(&ScrollBarInfo->rcScrollBar, Pt, Vertical)) ||
      ! PtInRect(&ScrollBarInfo->rcScrollBar, Pt))
    {
      return SCROLL_NOWHERE;
    }

  ThumbPos = ScrollBarInfo->xyThumbTop;
  ThumbSize = ScrollBarInfo->xyThumbBottom - ThumbPos;
  ArrowSize = ScrollBarInfo->dxyLineButton;

  if (Vertical) 
    {
      if (Pt.y < ScrollBarInfo->rcScrollBar.top + ArrowSize)
        {
          return SCROLL_TOP_ARROW;
        }
      if (ScrollBarInfo->rcScrollBar.bottom - ArrowSize <= Pt.y)
        {
          return SCROLL_BOTTOM_ARROW;
        }
      if (0 == ThumbPos)
        {
          return SCROLL_TOP_RECT;
        }
      Pt.y -= ScrollBarInfo->rcScrollBar.top;
      if (Pt.y < ThumbPos)
        {
          return SCROLL_TOP_RECT;
        }
      if (ThumbPos + ThumbSize <= Pt.y)
        {
          return SCROLL_BOTTOM_RECT;
        }
    }
  else
    {
      if (Pt.x < ScrollBarInfo->rcScrollBar.left + ArrowSize)
        {
          return SCROLL_TOP_ARROW;
        }
      if (ScrollBarInfo->rcScrollBar.right - ArrowSize <= Pt.x)
        {
          return SCROLL_BOTTOM_ARROW;
        }
      if (0 == ThumbPos)
        {
          return SCROLL_TOP_RECT;
        }
      Pt.x -= ScrollBarInfo->rcScrollBar.left;
      if (Pt.x < ThumbPos)
        {
          return SCROLL_TOP_RECT;
        }
      if (ThumbPos + ThumbSize <= Pt.x)
        {
          return SCROLL_BOTTOM_RECT;
        }
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
STATIC BOOL FASTCALL
IntScrollGetScrollBarRect(HWND Wnd, INT Bar, RECT *Rect,
                          INT *ArrowSize, INT *ThumbSize,
                          INT *ThumbPos)
{
  INT Pixels;
  BOOL Vertical;
  RECT ClientRect;
  RECT WindowRect;
  DWORD Style, ExStyle;

  GetClientRect(Wnd, &ClientRect);
  if (SB_HORZ == Bar || SB_VERT == Bar)
    {
      ClientToScreen(Wnd, (LPPOINT) &ClientRect.left);
      ClientToScreen(Wnd, (LPPOINT) &ClientRect.right);
      GetWindowRect(Wnd, &WindowRect);
    }
  Style = GetWindowLongW(Wnd, GWL_STYLE);

  switch (Bar)
    {
      case SB_HORZ:
        Rect->left   = ClientRect.left - WindowRect.left;
        Rect->top    = ClientRect.bottom - WindowRect.top;
        Rect->right  = ClientRect.right - WindowRect.left;
        Rect->bottom = Rect->top + GetSystemMetrics(SM_CYHSCROLL);
	if (0 != (Style & WS_BORDER))
          {
            Rect->left--;
            Rect->right++;
	  }
        else if (0 != (Style & WS_VSCROLL))
          {
            Rect->right++;
          }
        Vertical = FALSE;
	break;

      case SB_VERT:
        ExStyle = GetWindowLongW(Wnd, GWL_EXSTYLE);
        if (0 != (ExStyle & WS_EX_LEFTSCROLLBAR))
          {
            Rect->left   = ClientRect.left - WindowRect.left - GetSystemMetrics(SM_CXVSCROLL);
          }
        else
          {
            Rect->left   = ClientRect.right - WindowRect.left;
          }
        Rect->top    = ClientRect.top - WindowRect.top;
        Rect->right  = Rect->left + GetSystemMetrics(SM_CXVSCROLL);
        Rect->bottom = ClientRect.bottom - WindowRect.top;
	if (0 != (Style & WS_BORDER))
          {
            Rect->top--;
            Rect->bottom++;
          }
        else if (0 != (Style & WS_HSCROLL))
          {
            Rect->bottom++;
          }
        Vertical = TRUE;
	break;

      case SB_CTL:
        *Rect = ClientRect;
        Vertical = (0 != (Style & SBS_VERT));
	break;

      default:
        return FALSE;
    }

  if (Vertical)
    {
      Pixels = Rect->bottom - Rect->top;
    }
  else
    {
      Pixels = Rect->right - Rect->left;
    }

  if (Pixels <= 2 * GetSystemMetrics(SM_CXVSCROLL) + SCROLL_MIN_RECT)
    {
      if (SCROLL_MIN_RECT < Pixels)
        {
          *ArrowSize = (Pixels - SCROLL_MIN_RECT) / 2;
        }
      else
        {
          *ArrowSize = 0;
        }
      *ThumbPos = *ThumbSize = 0;
    }
  else
    {
      SCROLLINFO Info;

      NtUserGetScrollInfo(Wnd, Bar, &Info);
      *ArrowSize = GetSystemMetrics(SM_CXVSCROLL);
      Pixels -= (2 * GetSystemMetrics(SM_CXVSCROLL));

      if (0 != Info.nPage)
        {
          *ThumbSize = MulDiv(Pixels, Info.nPage, (Info.nMax - Info.nMin + 1));
          if (*ThumbSize < SCROLL_MIN_THUMB)
            {
              *ThumbSize = SCROLL_MIN_THUMB;
            }
        }
      else
        {
          *ThumbSize = GetSystemMetrics(SM_CXVSCROLL);
        }

#if 0 /* FIXME */
      if (((pixels -= *ThumbSize ) < 0) ||
          ((info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH))
#else
      if ((Pixels -= *ThumbSize ) < 0)
#endif
        {
          /* Rectangle too small or scrollbar disabled -> no thumb */
          *ThumbPos = *ThumbSize = 0;
        }
      else
        {
          INT Max = Info.nMax - max(Info.nPage - 1, 0);
          if (Max <= Info.nMin)
            {
              *ThumbPos = *ArrowSize;
            }
          else
            {
              *ThumbPos = *ArrowSize
                          + MulDiv(Pixels, (Info.nPos - Info.nMin),
                                   (Max - Info.nMin));
            }
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
STATIC UINT FASTCALL
IntScrollGetThumbVal(HWND Wnd, INT SBType, PSCROLLBARINFO ScrollBarInfo,
                     BOOL Vertical, INT Pos)
{
  SCROLLINFO si;
  INT Pixels = Vertical ? ScrollBarInfo->rcScrollBar.bottom
                          - ScrollBarInfo->rcScrollBar.top
                        : ScrollBarInfo->rcScrollBar.right
                          - ScrollBarInfo->rcScrollBar.left;

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE | SIF_PAGE;
  NtUserGetScrollInfo(Wnd, SBType, &si);
  if ((Pixels -= 2 * ScrollBarInfo->dxyLineButton) <= 0)
    {
      return si.nMin;
    }

  if ((Pixels -= (ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop)) <= 0)
    {
      return si.nMin;
    }

  Pos = RtlRosMax(0, Pos - ScrollBarInfo->dxyLineButton);
  if (Pixels < Pos)
    {
      Pos = Pixels;
    }

  if (0 == si.nPage)
    {
      Pos *= si.nMax - si.nMin;
    }
  else
    {
      Pos *= si.nMax - si.nMin - si.nPage + 1;
    }
  return si.nMin + ((Pos + Pixels / 2) / Pixels);
}

/***********************************************************************
 *           IntScrollClipPos
 */
STATIC POINT IntScrollClipPos(PRECT Rect, POINT Pt)
{
  if (Pt.x < Rect->left)
    {
      Pt.x = Rect->left;
    }
  else if (Rect->right < Pt.x)
    {
      Pt.x = Rect->right;
    }

  if (Pt.y < Rect->top)
    {
      Pt.y = Rect->top;
    }
  else if (Rect->bottom < Pt.y)
    {
      Pt.y = Rect->bottom;
    }

  return Pt;
}

/***********************************************************************
 *           IntScrollDrawSizeGrip
 *
 *  Draw the size grip.
 */
STATIC void FASTCALL
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
 *           IntScrollHandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars with focus).
 */
STATIC void FASTCALL
IntScrollHandleKbdEvent(
  HWND Wnd /* [in] Handle of window with scrollbar(s) */,
  WPARAM wParam /* [in] Variable input including enable state */,
  LPARAM lParam /* [in] Variable input including input point */)
{
  DPRINT("Wnd=%p wParam=%d lParam=%ld\n", Wnd, wParam, lParam);

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
               (0 != (GetWindowLongW(Wnd, GWL_STYLE ) & SBS_VERT) ?
                WM_VSCROLL : WM_HSCROLL), wParam, (LPARAM) Wnd);
}

/***********************************************************************
 *           IntScrollHandleScrollEvent
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'Pt' is the location of the mouse event in drawing coordinates
 */
STATIC VOID FASTCALL
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
  if (SCROLL_NOWHERE == ScrollTrackHitTest && WM_LBUTTONDOWN != Msg)
    {
      return;
    }

  NewInfo.nTrackPos = ScrollTrackingVal;
  NewInfo.reserved = ScrollBarInfo.reserved;
  memcpy(NewInfo.rgstate, ScrollBarInfo.rgstate, (CCHILDREN_SCROLLBAR + 1) * sizeof(DWORD));

  if (SB_CTL == SBType
      && 0 != (GetWindowLongW(Wnd, GWL_STYLE) & (SBS_SIZEGRIP | SBS_SIZEBOX)))
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
            if (Wnd == GetFocus())
              {
                ShowCaret(Wnd);
              }
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
      Vertical = (0 != (GetWindowLongW(Wnd, GWL_STYLE) & SBS_VERT));
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
        if (SB_CTL == SBType && 0 != (GetWindowLongW(Wnd, GWL_STYLE) & WS_TABSTOP))
          {
            SetFocus(Wnd);
          }
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
        if (Wnd == GetFocus())
          {
            ShowCaret(Wnd);
          }
        ScrollBarInfo.rgstate[ScrollTrackHitTest] &= ~STATE_SYSTEM_PRESSED;
        NewInfo.rgstate[ScrollTrackHitTest] = ScrollBarInfo.rgstate[ScrollTrackHitTest];
        NtUserSetScrollBarInfo(Wnd, IntScrollGetObjectId(SBType), &NewInfo);
        break;

      case WM_SYSTIMER:
        Pt = PrevPt;
        HitTest = IntScrollHitTest(&ScrollBarInfo, Vertical, Pt, FALSE);
        break;

      default:
          return;  /* Should never happen */
    }

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
        if (WM_LBUTTONDOWN == Msg)
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
        else if (WM_LBUTTONUP == Msg)
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
            if ((WM_LBUTTONDOWN == Msg) || (WM_SYSTIMER == Msg))
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
            if ((WM_LBUTTONDOWN == Msg) || (WM_SYSTIMER == Msg))
              {
                SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                             SB_LINEDOWN, (LPARAM) WndCtl);
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
    }

  if (WM_LBUTTONDOWN == Msg)
    {
      if (SCROLL_THUMB == HitTest)
        {
          UINT Val = IntScrollGetThumbVal(Wnd, SBType, &ScrollBarInfo, Vertical,
                                          TrackThumbPos + LastMousePos - LastClickPos);
          SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                       MAKEWPARAM(SB_THUMBTRACK, Val), (LPARAM) WndCtl);
        }
    }

  if (WM_LBUTTONUP == Msg)
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
      SendMessageW(WndOwner, Vertical ? WM_VSCROLL : WM_HSCROLL,
                   SB_ENDSCROLL, (LPARAM) WndCtl);
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

  DPRINT("hwnd=%p lpCreate=%p\n", Wnd, lpCreate);

#if 0 /* FIXME */
  if (lpCreate->style & WS_DISABLED)
    {
      info->flags = ESB_DISABLE_BOTH;
      TRACE("Created WS_DISABLED scrollbar\n");
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

STATIC INT FASTCALL
IntScrollGetScrollPos(HWND Wnd, INT Bar)
{
  SCROLLINFO ScrollInfo;
  
  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_POS;
  if (! NtUserGetScrollInfo(Wnd, Bar, &ScrollInfo))
    {
      return 0;
    }

  return ScrollInfo.nPos;
}

STATIC BOOL FASTCALL
IntScrollGetScrollRange(HWND Wnd, int Bar, LPINT MinPos, LPINT MaxPos)
{
  BOOL Result;
  SCROLLINFO ScrollInfo;
  
  if (NULL == MinPos || NULL == MaxPos)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  
  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_RANGE;
  Result = NtUserGetScrollInfo(Wnd, Bar, &ScrollInfo);
  if (Result)
    {
      *MinPos = ScrollInfo.nMin;
      *MaxPos = ScrollInfo.nMax;
    }
  
  return Result;
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
  RECT WindowRect;
  UINT XOffset, YOffset;
  POINT TopLeft;

  if (SB_CTL != SBType)
    {
      NtUserGetWindowRect(Wnd, &WindowRect);
  
      Pt.x -= WindowRect.left;
      Pt.y -= WindowRect.top;

      TopLeft.x = WindowRect.left;
      TopLeft.y = WindowRect.top;
      ScreenToClient(Wnd, &TopLeft);
      XOffset = - TopLeft.x;
      YOffset = - TopLeft.y;
    }
  else
    {
      XOffset = 0;
      YOffset = 0;
    }

  IntScrollHandleScrollEvent(Wnd, SBType, WM_LBUTTONDOWN, Pt);

  do
    {
      if (! GetMessageW(&Msg, 0, 0, 0))
        {
          break;
        }
      if (CallMsgFilterW(&Msg, MSGF_SCROLLBAR))
        {
          continue;
        }

      switch(Msg.message)
        {
          case WM_SYSTIMER:
          case WM_LBUTTONUP:
          case WM_MOUSEMOVE:
            Pt.x = LOWORD(Msg.lParam) + XOffset;
            Pt.y = HIWORD(Msg.lParam) + YOffset;
            IntScrollHandleScrollEvent(Wnd, SBType, Msg.message, Pt);
            break;
          default:
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
            break;
        }

      if (! IsWindow(Wnd))
        {
          ReleaseCapture();
          break;
        }
    }
  while (WM_LBUTTONUP != Msg.message);
}


/***********************************************************************
 *           ScrollBarWndProc
 */
static LRESULT WINAPI
ScrollBarWndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (! IsWindow(Wnd))
    {
      return 0;
    }

  switch (Msg)
    {
      case WM_CREATE:
        IntScrollCreateScrollBar(Wnd, (LPCREATESTRUCTW) lParam);
        break;

#if 0 /* FIXME */
      case WM_ENABLE:
        {
          SCROLLBAR_INFO *infoPtr;
          if ((infoPtr = SCROLL_GetScrollBarInfo( hwnd, SB_CTL )))
            {
              infoPtr->flags = wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;
              SCROLL_RefreshScrollBar(hwnd, SB_CTL, TRUE, TRUE);
            }
	}
	return 0;
#endif

      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONDOWN:
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

          if (GetWindowLongW(Wnd, GWL_STYLE) & SBS_SIZEGRIP)
            {
              IntScrollDrawSizeGrip(Wnd, Dc);
            }
          else if (0 != (GetWindowLongW(Wnd, GWL_STYLE) & SBS_SIZEBOX))
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

      case SBM_SETPOS:
        return SetScrollPos(Wnd, SB_CTL, wParam, (BOOL) lParam);

      case SBM_GETPOS:
        return IntScrollGetScrollPos(Wnd, SB_CTL);

      case SBM_SETRANGE:
        {
          INT OldPos = IntScrollGetScrollPos(Wnd, SB_CTL);
          SetScrollRange(Wnd, SB_CTL, wParam, lParam, FALSE);
          if (OldPos != IntScrollGetScrollPos(Wnd, SB_CTL))
            {
              return OldPos;
            }
        }
        return 0;

      case SBM_GETRANGE:
        return IntScrollGetScrollRange(Wnd, SB_CTL, (LPINT) wParam, (LPINT) lParam);

      case SBM_ENABLE_ARROWS:
        return EnableScrollBar(Wnd, SB_CTL, wParam);

      case SBM_SETRANGEREDRAW:
        {
          INT OldPos = IntScrollGetScrollPos(Wnd, SB_CTL);
          SetScrollRange(Wnd, SB_CTL, wParam, lParam, TRUE);
          if (OldPos != IntScrollGetScrollPos(Wnd, SB_CTL))
            {
              return OldPos;
            }
        }
        return 0;

      case SBM_SETSCROLLINFO:
        return NtUserSetScrollInfo(Wnd, SB_CTL, (SCROLLINFO *) lParam, wParam);

      case SBM_GETSCROLLINFO:
        return NtUserGetScrollInfo(Wnd, SB_CTL, (SCROLLINFO *) lParam);

      case 0x00e5:
      case 0x00e7:
      case 0x00e8:
      case 0x00eb:
      case 0x00ec:
      case 0x00ed:
      case 0x00ee:
      case 0x00ef:
        DPRINT("unknown Win32 msg %04x wp=%08x lp=%08lx\n",
		Msg, wParam, lParam );
        break;

      default:
        if (WM_USER <= Msg)
          {
            DPRINT("unknown msg %04x wp=%04x lp=%08lx\n", Msg, wParam, lParam);
          }
        return DefWindowProcW(Wnd, Msg, wParam, lParam );
    }

  return 0;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL STDCALL
EnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows)
{
  return NtUserEnableScrollBar(hWnd, wSBflags, wArrows);
}

/*
 * @implemented
 */
BOOL STDCALL
GetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
  return NtUserGetScrollBarInfo(hWnd, idObject, psbi);
}

/*
 * @implemented
 */
BOOL STDCALL
GetScrollInfo(HWND Wnd, INT SBType, LPSCROLLINFO Info)
{
  if (SB_CTL == SBType)
    {
      return SendMessageW(Wnd, SBM_GETSCROLLINFO, 0, (LPARAM) Info);
    }
  else
    {
      return NtUserGetScrollInfo(Wnd, SBType, Info);
    }
}

/*
 * @implemented
 */
INT STDCALL
GetScrollPos(HWND Wnd, INT Bar)
{
  DPRINT("Wnd=%p Bar=%d\n", Wnd, Bar);

  /* Refer SB_CTL requests to the window */
  if (SB_CTL == Bar)
    {
      return SendMessageW(Wnd, SBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
    }
  else
    {
      return IntScrollGetScrollPos(Wnd, Bar);
    }
}

/*
 * @implemented
 */
BOOL STDCALL
GetScrollRange(HWND Wnd, int Bar, LPINT MinPos, LPINT MaxPos)
{
  DPRINT("Wnd=%x Bar=%d Min=%p Max=%p\n", Wnd, Bar, MinPos, MaxPos);

  /* Refer SB_CTL requests to the window */
  if (SB_CTL == Bar)
    {
      return SendMessageW(Wnd, SBM_GETRANGE, (WPARAM) MinPos, (LPARAM) MaxPos);
    }
  else
    {
      return IntScrollGetScrollRange(Wnd, Bar, MinPos, MaxPos);
    }
}

/*
 * @implemented
 */
INT STDCALL
SetScrollInfo(HWND Wnd, int SBType, LPCSCROLLINFO Info, BOOL bRedraw)
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
INT STDCALL
SetScrollPos(HWND hWnd, INT nBar, INT nPos, BOOL bRedraw)
{
  INT Result = 0;
  SCROLLINFO ScrollInfo;
  
  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_POS;
  
  /*
   * Call NtUserGetScrollInfo() to get the previous position that
   * we will later return.
   */
  if (NtUserGetScrollInfo(hWnd, nBar, &ScrollInfo))
    {
      Result = ScrollInfo.nPos;
      if (Result != nPos)
        {
          ScrollInfo.nPos = nPos;
          /* Finally set the new position */
          NtUserSetScrollInfo(hWnd, nBar, &ScrollInfo, bRedraw);
        }
    }
  
  return Result;
}

/*
 * @implemented
 */
BOOL STDCALL
SetScrollRange(HWND hWnd, INT nBar, INT nMinPos, INT nMaxPos, BOOL bRedraw)
{
  SCROLLINFO ScrollInfo;

  ScrollInfo.cbSize = sizeof(SCROLLINFO);
  ScrollInfo.fMask = SIF_RANGE;
  ScrollInfo.nMin = nMinPos;
  ScrollInfo.nMax = nMaxPos;
  NtUserSetScrollInfo(hWnd, nBar, &ScrollInfo, bRedraw);

  return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
ShowScrollBar(HWND hWnd, INT wBar, BOOL bShow)
{
  return NtUserShowScrollBar(hWnd, wBar, bShow);
}
