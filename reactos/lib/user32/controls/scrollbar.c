/*
 * ReactOS User32 Library
 * - ScrollBar control
 *
 * Copyright 2001 Casper S. Hornstrup
 * Copyright 2003 Thomas Weidenmueller
 * Copyright 2003 Filip Navara
 *
 * Portions based on Wine code.
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

/* GLOBAL VARIABLES ***********************************************************/

/* Definitions for scrollbar hit testing [See SCROLLBARINFO in MSDN] */
#define SCROLL_NOWHERE		0x00    /* Outside the scroll bar */
#define SCROLL_TOP_ARROW	0x01    /* Top or left arrow */
#define SCROLL_TOP_RECT		0x02    /* Rectangle between the top arrow and the thumb */
#define SCROLL_THUMB		0x03    /* Thumb rectangle */
#define SCROLL_BOTTOM_RECT	0x04    /* Rectangle between the thumb and the bottom arrow */
#define SCROLL_BOTTOM_ARROW	0x05    /* Bottom or right arrow */

HBRUSH DefWndControlColor(HDC hDC, UINT ctlType);

/* PRIVATE FUNCTIONS **********************************************************/

static void
IntDrawScrollInterior(HWND hWnd, HDC hDC, INT nBar, BOOL Vertical,
   INT ArrowSize, PSCROLLBARINFO ScrollBarInfo)
{
   INT ThumbSize = ScrollBarInfo->xyThumbBottom - ScrollBarInfo->xyThumbTop;
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
      Rect.top = ScrollBarInfo->rcScrollBar.top + ArrowSize;
      Rect.bottom = ScrollBarInfo->rcScrollBar.bottom - ArrowSize;
      Rect.left = ScrollBarInfo->rcScrollBar.left;
      Rect.right = ScrollBarInfo->rcScrollBar.right;
   }
   else
   {
      Rect.top = ScrollBarInfo->rcScrollBar.top;
      Rect.bottom = ScrollBarInfo->rcScrollBar.bottom;
      Rect.left = ScrollBarInfo->rcScrollBar.left + ArrowSize;
      Rect.right = ScrollBarInfo->rcScrollBar.right - ArrowSize;
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
  
   ScrollBarInfo->xyThumbTop -= ArrowSize;

   if (ScrollBarInfo->dxyLineButton)
   {
      if (Vertical)
      {
         if (ThumbSize)
         {
            PatBlt(hDC, Rect.left, Rect.top, Rect.right - Rect.left,
               ScrollBarInfo->xyThumbTop, TopSelected ? BLACKNESS : PATCOPY);
            Rect.top += ScrollBarInfo->xyThumbTop;
            PatBlt(hDC, Rect.left, Rect.top + ThumbSize, Rect.right - Rect.left,
               Rect.bottom - Rect.top - ThumbSize, BottomSelected ? BLACKNESS : PATCOPY);
            Rect.bottom = Rect.top + ThumbSize;
         }
         else
         {
            if (ScrollBarInfo->xyThumbTop)
            {
               PatBlt(hDC, Rect.left, ScrollBarInfo->dxyLineButton,
                  Rect.right - Rect.left, Rect.bottom - Rect.bottom, PATCOPY);
            }
         }
      }
      else
      {
         if (ThumbSize)
         {
            PatBlt(hDC, Rect.left, Rect.top, ScrollBarInfo->xyThumbTop,
               Rect.bottom - Rect.top, TopSelected ? BLACKNESS : PATCOPY);
            Rect.left += ScrollBarInfo->xyThumbTop;
            PatBlt(hDC, Rect.left + ThumbSize, Rect.top,
               Rect.right - Rect.left - ThumbSize, Rect.bottom - Rect.top,
               BottomSelected ? BLACKNESS : PATCOPY);
            Rect.right = Rect.left + ThumbSize;
         }
         else
         {
            if (ScrollBarInfo->xyThumbTop)
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

static void
IntDrawScrollArrows(HDC hDC, PSCROLLBARINFO ScrollBarInfo, LPRECT Rect,
   INT ArrowSize, BOOL Vertical)
{
   RECT RectLT, RectRB;
   INT ScrollDirFlagLT, ScrollDirFlagRB;

   RectLT = RectRB = *Rect;
   if (Vertical)
   {
      ScrollDirFlagLT = DFCS_SCROLLUP;
      ScrollDirFlagRB = DFCS_SCROLLDOWN;
      RectLT.bottom = RectLT.top + ArrowSize;
      RectRB.top = RectRB.bottom - ArrowSize;
   }
   else
   {
      ScrollDirFlagLT = DFCS_SCROLLLEFT;
      ScrollDirFlagRB = DFCS_SCROLLRIGHT;
      RectLT.right = RectLT.left + ArrowSize;
      RectRB.left = RectRB.right - ArrowSize;
   }

   if (ScrollBarInfo->rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_PRESSED)
      ScrollDirFlagLT |= DFCS_PUSHED | DFCS_FLAT;
   if (ScrollBarInfo->rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_UNAVAILABLE)
      ScrollDirFlagLT |= DFCS_INACTIVE;
   if (ScrollBarInfo->rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_PRESSED)
      ScrollDirFlagRB |= DFCS_PUSHED | DFCS_FLAT;
   if (ScrollBarInfo->rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_UNAVAILABLE)
      ScrollDirFlagRB |= DFCS_INACTIVE;

   DrawFrameControl(hDC, &RectLT, DFC_SCROLL, ScrollDirFlagLT);
   DrawFrameControl(hDC, &RectRB, DFC_SCROLL, ScrollDirFlagRB);
}

void
IntDrawScrollBar(HWND hWnd, HDC hDC, INT nBar)
{
   INT ArrowSize = 0;
   INT ThumbSize;
   SCROLLBARINFO Info;
   BOOL Vertical;

   /*
    * Get scroll bar info.
    */

   Info.cbSize = sizeof(SCROLLBARINFO);
   switch (nBar)
   {
      case SB_HORZ:
         Vertical = FALSE;
         NtUserGetScrollBarInfo(hWnd, OBJID_HSCROLL, &Info);
         break;

      case SB_VERT:
         Vertical = TRUE;
         NtUserGetScrollBarInfo(hWnd, OBJID_VSCROLL, &Info);
         break;

      case SB_CTL:
         Vertical = (GetWindowLongW(hWnd, GWL_STYLE) & SBS_VERT) != 0;
         NtUserGetScrollBarInfo(hWnd, OBJID_CLIENT, &Info);
         break;
   }
  
   if (IsRectEmpty(&Info.rcScrollBar))
   {
      return;
   }

   ArrowSize = Info.dxyLineButton;
   ThumbSize = Info.xyThumbBottom - Info.xyThumbTop;

   /*
    * Draw the arrows.
    */

   if (ArrowSize)
      IntDrawScrollArrows(hDC, &Info, &Info.rcScrollBar, ArrowSize, Vertical);

   /*
    * Draw the interior.
    */

   IntDrawScrollInterior(hWnd, hDC, nBar, Vertical, ArrowSize, &Info);

   /*
    * If scroll bar has focus, reposition the caret.
    */

   if (hWnd == GetFocus() && nBar == SB_CTL)
   {
      if (nBar == SB_HORZ)
         SetCaretPos(Info.dxyLineButton + 1, Info.rcScrollBar.top + 1);
      else if (nBar == SB_VERT)
         SetCaretPos(Info.rcScrollBar.top + 1, Info.dxyLineButton + 1);
   }
}

static BOOL 
IntScrollPtInRectEx(LPRECT lpRect, POINT pt, BOOL Vertical)
{
   RECT Rect = *lpRect;
   if (Vertical)
   {
      Rect.left -= lpRect->right - lpRect->left;
      Rect.right += lpRect->right - lpRect->left;
   }
   else
   {
      Rect.top -= lpRect->bottom - lpRect->top;
      Rect.bottom += lpRect->bottom - lpRect->top;
   }
   return PtInRect(&Rect, pt);
}

DWORD
IntScrollHitTest(HWND hWnd, INT nBar, POINT pt, BOOL bDragging)
{
   SCROLLBARINFO ScrollButtonInfo;
   RECT WindowRect;
   INT ArrowSize, ThumbSize, ThumbPos;
   BOOL Vertical = (nBar == SB_VERT); /* FIXME - ((Window->Style & SBS_VERT) != 0) */

   NtUserGetWindowRect(hWnd, &WindowRect);
  
   ScrollButtonInfo.cbSize = sizeof(SCROLLBARINFO);
   NtUserGetScrollBarInfo(hWnd, Vertical ? OBJID_VSCROLL : OBJID_HSCROLL,
      &ScrollButtonInfo);
  
   OffsetRect(&ScrollButtonInfo.rcScrollBar, WindowRect.left, WindowRect.top);

   if ((bDragging && !IntScrollPtInRectEx(&ScrollButtonInfo.rcScrollBar, pt, Vertical)) ||
       !PtInRect(&ScrollButtonInfo.rcScrollBar, pt))
   {
      return SCROLL_NOWHERE;
   }

   ThumbPos = ScrollButtonInfo.xyThumbTop;
   ThumbSize = ScrollButtonInfo.xyThumbBottom - ThumbPos;
   ArrowSize = ScrollButtonInfo.dxyLineButton;

   if (Vertical) 
   {
      if (pt.y < ScrollButtonInfo.rcScrollBar.top + ArrowSize)
         return SCROLL_TOP_ARROW;
      if (pt.y >= ScrollButtonInfo.rcScrollBar.bottom - ArrowSize)
         return SCROLL_BOTTOM_ARROW;
      if (!ThumbPos)
         return SCROLL_TOP_RECT;
      pt.y -= ScrollButtonInfo.rcScrollBar.top;
      if (pt.y < ThumbPos)
         return SCROLL_TOP_RECT;
      if (pt.y >= ThumbPos + ThumbSize)
         return SCROLL_BOTTOM_RECT;
   }
   else
   {
      if (pt.x < ScrollButtonInfo.rcScrollBar.left + ArrowSize)
         return SCROLL_TOP_ARROW;
      if (pt.x >= ScrollButtonInfo.rcScrollBar.right - ArrowSize)
         return SCROLL_BOTTOM_ARROW;
      if (!ThumbPos)
         return SCROLL_TOP_RECT;
      pt.x -= ScrollButtonInfo.rcScrollBar.left;
      if (pt.x < ThumbPos)
         return SCROLL_TOP_RECT;
      if (pt.x >= ThumbPos + ThumbSize)
         return SCROLL_BOTTOM_RECT;
   }

   return SCROLL_THUMB;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
WINBOOL STDCALL
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
GetScrollInfo(HWND hWnd, INT nBar, LPSCROLLINFO lpsi)
{
   return NtUserGetScrollInfo(hWnd, nBar, lpsi);
}

/*
 * @implemented
 */
INT STDCALL
GetScrollPos(HWND hWnd, INT nBar)
{
   SCROLLINFO ScrollInfo;
  
   ScrollInfo.cbSize = sizeof(SCROLLINFO);
   ScrollInfo.fMask = SIF_POS;
   if (NtUserGetScrollInfo(hWnd, nBar, &ScrollInfo))
      return ScrollInfo.nPos;
   return 0;
}

/*
 * @implemented
 */
BOOL STDCALL
GetScrollRange(HWND hWnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos)
{
   BOOL Result;
   SCROLLINFO ScrollInfo;
  
   if (!lpMinPos || !lpMaxPos)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
  
   ScrollInfo.cbSize = sizeof(SCROLLINFO);
   ScrollInfo.fMask = SIF_RANGE;
   Result = NtUserGetScrollInfo(hWnd, nBar, &ScrollInfo);
   if (Result)
   {
      *lpMinPos = ScrollInfo.nMin;
      *lpMaxPos = ScrollInfo.nMax;
   }
  
   return Result;
}

/*
 * @implemented
 */
INT STDCALL
SetScrollInfo(HWND hWnd, int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw)
{
   return NtUserSetScrollInfo(hWnd, nBar, lpsi, bRedraw);
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
