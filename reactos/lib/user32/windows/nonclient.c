/*
 * ReactOS User32 Library
 * - Window non-client area management
 *
 * Copyright (C) 2003 ReactOS Team
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

/*
 * Define this to get the code working on ReactOS. It should be removed later.
 */
/*
Already defined in makefile now.
#define __REACTOS__
*/

/*
 * Use w32api headers
 */
/*
#define __USE_W32API
#define _WIN32_WINNT 0x0501
*/

#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <menu.h>
#include <winpos.h>
#include <user32/wininternal.h>
#include <user32.h>

#define NDEBUG
#include <debug.h>

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & (WS_THICKFRAME | WS_MINIMIZE)))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && !((Style) & WS_MINIMIZE) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & (WS_BORDER | WS_MINIMIZE)) || (!((Style) & (WS_CHILD | WS_POPUP))))

/*
 * FIXME: This should be moved to a header
 */
VOID
IntDrawScrollBar(HWND hWnd, HDC hDC, INT nBar);
DWORD
IntScrollHitTest(HWND hWnd, INT nBar, POINT pt, BOOL bDragging);
HPEN STDCALL
GetSysColorPen(int nIndex);

extern ATOM AtomInternalPos;

/* PRIVATE FUNCTIONS **********************************************************/

BOOL
UserHasWindowEdge(DWORD Style, DWORD ExStyle)
{
   if (Style & WS_MINIMIZE)
      return TRUE;
   if (ExStyle & WS_EX_DLGMODALFRAME)
      return TRUE;
   if (ExStyle & WS_EX_STATICEDGE)
      return FALSE;
   if (Style & WS_THICKFRAME)
      return TRUE;
   Style &= WS_CAPTION;
   if (Style == WS_DLGFRAME || Style == WS_CAPTION)
      return TRUE;      
   return FALSE;
}

VOID
UserGetWindowBorders(DWORD Style, DWORD ExStyle, SIZE *Size, BOOL WithClient)
{
   DWORD Border = 0;

   if (UserHasWindowEdge(Style, ExStyle))
      Border += 2;
   else if (ExStyle & WS_EX_STATICEDGE)
      Border += 1;
   if ((ExStyle & WS_EX_CLIENTEDGE) && WithClient)
      Border += 2;
   if (Style & WS_CAPTION || ExStyle & WS_EX_DLGMODALFRAME)
      Border ++;
   Size->cx = Size->cy = Border;
   if ((Style & WS_THICKFRAME) && !(Style & WS_MINIMIZE))
   {
      Size->cx += GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME);
      Size->cy += GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME);
   }
   Size->cx *= GetSystemMetrics(SM_CXBORDER);
   Size->cy *= GetSystemMetrics(SM_CYBORDER);
}

BOOL
UserHasMenu(HWND hWnd, ULONG Style)
{
   return (!(Style & WS_CHILD) && GetMenu(hWnd) != 0);
}

HICON 
UserGetWindowIcon(HWND hwnd)
{
  HICON Ret = 0;
  
  SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&Ret);
  if (!Ret)
    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&Ret);
  if (!Ret)
    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&Ret);
  if (!Ret)
    Ret = (HICON)GetClassLongW(hwnd, GCL_HICONSM);
  if (!Ret)
    Ret = (HICON)GetClassLongW(hwnd, GCL_HICON);
  if (!Ret)
    SendMessageTimeoutW(hwnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (LPDWORD)&Ret);
  if (!Ret)
    Ret = LoadIconW(0, IDI_APPLICATION);
  
  return Ret;
}

BOOL
UserDrawSysMenuButton(HWND hWnd, HDC hDC, LPRECT Rect)
{
  HICON WindowIcon;
  
  if((WindowIcon = UserGetWindowIcon(hWnd)))
  {
    return DrawIconEx(hDC, Rect->left + 2, Rect->top + 2, WindowIcon,
                      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                      0, NULL, DI_NORMAL);
  }
  return FALSE;
}

/*
 * FIXME:
 * - Cache bitmaps, then just bitblt instead of calling DFC() (and
 *   wasting precious CPU cycles) every time
 * - Center the buttons verticaly in the rect
 */
VOID
UserDrawCaptionButton(LPRECT Rect, DWORD Style, DWORD ExStyle, HDC hDC, BOOL bDown, ULONG Type)
{
   RECT TempRect;

   if (!(Style & WS_SYSMENU))
   {
      return;
   }

   TempRect = *Rect;

   switch (Type)
   {
      case DFCS_CAPTIONMIN:
      {
         if (ExStyle & WS_EX_TOOLWINDOW)
            return; /* ToolWindows don't have min/max buttons */

         if (Style & WS_SYSMENU)
             TempRect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
         if (Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
             TempRect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
         TempRect.left = TempRect.right - GetSystemMetrics(SM_CXSIZE) + 1;
         TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYSIZE) - 2;
         TempRect.top += 2;
         TempRect.right -= 1;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          ((Style & WS_MINIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMIN) | 
                          (bDown ? DFCS_PUSHED : 0) |
                          ((Style & WS_MINIMIZEBOX) ? 0 : DFCS_INACTIVE));
         break;
      }
      case DFCS_CAPTIONMAX:
      {
         if (ExStyle & WS_EX_TOOLWINDOW)
             return; /* ToolWindows don't have min/max buttons */

         if (Style & WS_SYSMENU)
             TempRect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
         TempRect.left = TempRect.right - GetSystemMetrics(SM_CXSIZE) + 1;
         TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYSIZE) - 2;
         TempRect.top += 2;
         TempRect.right -= 1;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          ((Style & WS_MAXIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX) |
                          (bDown ? DFCS_PUSHED : 0) |
                          ((Style & WS_MAXIMIZEBOX) ? 0 : DFCS_INACTIVE));
         break;
      }
      case DFCS_CAPTIONCLOSE:
      {
         /* FIXME: A tool window has a smaller Close button */

         if (ExStyle & WS_EX_TOOLWINDOW)
         {
            TempRect.left = TempRect.right - GetSystemMetrics(SM_CXSMSIZE);
            TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYSMSIZE) - 2;                  
         }
         else
         {
            TempRect.left = TempRect.right - GetSystemMetrics(SM_CXSIZE);
            TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYSIZE) - 2;
         }
         TempRect.top += 2;
         TempRect.right -= 2;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          (DFCS_CAPTIONCLOSE | (bDown ? DFCS_PUSHED : 0) |
                          ((Style & WS_SYSMENU) ? 0 : DFCS_INACTIVE)));
         break;
      }
   }
}

VOID
UserDrawCaptionButtonWnd(HWND hWnd, HDC hDC, BOOL bDown, ULONG Type)
{
   RECT WindowRect;
   SIZE WindowBorder;
   DWORD Style, ExStyle;

   GetWindowRect(hWnd, &WindowRect);
   WindowRect.right -= WindowRect.left;
   WindowRect.bottom -= WindowRect.top;
   WindowRect.left = WindowRect.top = 0;
   Style = GetWindowLongW(hWnd, GWL_STYLE);
   ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
   UserGetWindowBorders(Style, ExStyle, &WindowBorder, FALSE);
   InflateRect(&WindowRect, -WindowBorder.cx, -WindowBorder.cy);
   UserDrawCaptionButton(&WindowRect, Style, ExStyle, hDC, bDown, Type);
}

/*
 * FIXME:
 * - Drawing of WS_BORDER after scrollbars
 * - Correct drawing of size-box
 */
LRESULT
DefWndNCPaint(HWND hWnd, HRGN hRgn)
{
   HDC hDC;
   BOOL Active;
   DWORD Style, ExStyle;
   RECT ClientRect, WindowRect, CurrentRect, TempRect;

   if (!IsWindowVisible(hWnd))
      return 0;

   Style = GetWindowLongW(hWnd, GWL_STYLE);

   hDC = GetDCEx(hWnd, hRgn, DCX_WINDOW | DCX_INTERSECTRGN | 0x10000);
   if (hDC == 0)
   {
      return 0;
   }

   ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
   if (ExStyle & WS_EX_MDICHILD)
   {
      Active = IsChild(GetForegroundWindow(), hWnd);
      if (Active)
         Active = (hWnd == (HWND)SendMessageW(GetParent(hWnd), WM_MDIGETACTIVE, 0, 0));
   }
   else
   {
      Active = (GetForegroundWindow() == hWnd);
   }
   GetWindowRect(hWnd, &WindowRect);
   GetClientRect(hWnd, &ClientRect);

   CurrentRect.top = CurrentRect.left = 0;
   CurrentRect.right = WindowRect.right - WindowRect.left;
   CurrentRect.bottom = WindowRect.bottom - WindowRect.top;

   /* Draw outer edge */
   if (UserHasWindowEdge(Style, ExStyle))
   {
      DrawEdge(hDC, &CurrentRect, EDGE_RAISED, BF_RECT | BF_ADJUST);
   } else
   if (ExStyle & WS_EX_STATICEDGE)
   {
      DrawEdge(hDC, &CurrentRect, BDR_SUNKENINNER, BF_RECT | BF_ADJUST | BF_FLAT);
   }
    
   /* Firstly the "thick" frame */
   if ((Style & WS_THICKFRAME) && !(Style & WS_MINIMIZE))
   {
      DWORD Width =
         (GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME)) *
         GetSystemMetrics(SM_CXBORDER);
      DWORD Height =
         (GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME)) *
         GetSystemMetrics(SM_CYBORDER);

      SelectObject(hDC, GetSysColorBrush(Active ? COLOR_ACTIVEBORDER :
         COLOR_INACTIVEBORDER));

      /* Draw frame */
      PatBlt(hDC, CurrentRect.left, CurrentRect.top, CurrentRect.right - CurrentRect.left, Height, PATCOPY);
      PatBlt(hDC, CurrentRect.left, CurrentRect.top, Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#ifdef __REACTOS__
      PatBlt(hDC, CurrentRect.left, CurrentRect.bottom - 1, CurrentRect.right - CurrentRect.left, -Height, PATCOPY);
      PatBlt(hDC, CurrentRect.right - 1, CurrentRect.top, -Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#else
      PatBlt(hDC, CurrentRect.left, CurrentRect.bottom, CurrentRect.right - CurrentRect.left, -Height, PATCOPY);
      PatBlt(hDC, CurrentRect.right, CurrentRect.top, -Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#endif

      InflateRect(&CurrentRect, -Width, -Height);
   }

   /* Now the other bit of the frame */
   if (Style & WS_CAPTION || ExStyle & WS_EX_DLGMODALFRAME)
   {
      DWORD Width = GetSystemMetrics(SM_CXBORDER);
      DWORD Height = GetSystemMetrics(SM_CYBORDER);
  
      SelectObject(hDC, GetSysColorBrush(
         (ExStyle & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE)) ? COLOR_3DFACE :
         (ExStyle & WS_EX_STATICEDGE) ? COLOR_WINDOWFRAME :
         (Style & (WS_DLGFRAME | WS_THICKFRAME)) ? COLOR_3DFACE :
         COLOR_WINDOWFRAME));

      /* Draw frame */
      PatBlt(hDC, CurrentRect.left, CurrentRect.top, CurrentRect.right - CurrentRect.left, Height, PATCOPY);
      PatBlt(hDC, CurrentRect.left, CurrentRect.top, Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#ifdef __REACTOS__
      PatBlt(hDC, CurrentRect.left, CurrentRect.bottom - 1, CurrentRect.right - CurrentRect.left, -Height, PATCOPY);
      PatBlt(hDC, CurrentRect.right - 1, CurrentRect.top, -Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#else
      PatBlt(hDC, CurrentRect.left, CurrentRect.bottom, CurrentRect.right - CurrentRect.left, -Height, PATCOPY);
      PatBlt(hDC, CurrentRect.right, CurrentRect.top, -Width, CurrentRect.bottom - CurrentRect.top, PATCOPY);
#endif

      InflateRect(&CurrentRect, -Width, -Height);
   }

   /* Draw caption */
   if ((Style & WS_CAPTION) == WS_CAPTION)
   {
      DWORD CaptionFlags = DC_ICON | DC_TEXT | DC_BUTTONS;
      HPEN PreviousPen;
      BOOL Gradient = FALSE;
      
      if(SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &Gradient, 0) && Gradient)
      {
        CaptionFlags |= DC_GRADIENT;
      }

      TempRect = CurrentRect;

      if (Active)
      {
         CaptionFlags |= DC_ACTIVE;
      }

      if (ExStyle & WS_EX_TOOLWINDOW)
      {
         CaptionFlags |= DC_SMALLCAP;
         TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYSMCAPTION) - 1;
         CurrentRect.top += GetSystemMetrics(SM_CYSMCAPTION);
      }
      else
      {
         TempRect.bottom = TempRect.top + GetSystemMetrics(SM_CYCAPTION) - 1;
         CurrentRect.top += GetSystemMetrics(SM_CYCAPTION);
      }
        
      DrawCaption(hWnd, hDC, &TempRect, CaptionFlags);

      /* Draw buttons */
      if (Style & WS_SYSMENU)
      {
         UserDrawCaptionButton(&TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONCLOSE);
         if ((Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX)) && !(ExStyle & WS_EX_TOOLWINDOW))
         {
            UserDrawCaptionButton(&TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMIN);
            UserDrawCaptionButton(&TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMAX);
         }
      }
      if(!(Style & WS_MINIMIZE))
      {
        /* Line under caption */
        PreviousPen = SelectObject(hDC, GetSysColorPen(
           ((ExStyle & (WS_EX_STATICEDGE | WS_EX_CLIENTEDGE |
                        WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
           COLOR_WINDOWFRAME : COLOR_3DFACE));
        MoveToEx(hDC, TempRect.left, TempRect.bottom, NULL);
        LineTo(hDC, TempRect.right, TempRect.bottom);
        SelectObject(hDC, PreviousPen);
      }
   }
   
   if(!(Style & WS_MINIMIZE))
   {
     HMENU menu = GetMenu(hWnd);
     /* Draw menu bar */
     if (menu && !(Style & WS_CHILD))
     {
        TempRect = CurrentRect;
        TempRect.bottom = TempRect.top + (UINT)NtUserSetMenuBarHeight(menu, 0);
        CurrentRect.top += MenuDrawMenuBar(hDC, &TempRect, hWnd, TRUE);
     }
     
     if (ExStyle & WS_EX_CLIENTEDGE)
     {
        DrawEdge(hDC, &CurrentRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
     }
     
     /* Draw the scrollbars */
     if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL) &&
         (CurrentRect.bottom - CurrentRect.top) > GetSystemMetrics(SM_CYHSCROLL))
     {
        TempRect = CurrentRect;
        if (ExStyle & WS_EX_LEFTSCROLLBAR)
           TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
        TempRect.top = TempRect.bottom - GetSystemMetrics(SM_CYHSCROLL);
        FillRect(hDC, &TempRect, GetSysColorBrush(COLOR_SCROLLBAR));
        /* FIXME: Correct drawing of size-box with WS_EX_LEFTSCROLLBAR */
        if (!(Style & WS_CHILD) || (ExStyle & WS_EX_MDICHILD))
        {
           TempRect.top--;
           TempRect.bottom++;
           TempRect.left--;
           TempRect.right++;
           DrawFrameControl(hDC, &TempRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
        }
        IntDrawScrollBar(hWnd, hDC, SB_VERT);
        IntDrawScrollBar(hWnd, hDC, SB_HORZ);
     }
     else
     {
        if (Style & WS_VSCROLL)
           IntDrawScrollBar(hWnd, hDC, SB_VERT);
        else if (Style & WS_HSCROLL)
           IntDrawScrollBar(hWnd, hDC, SB_HORZ);
     }
   }

   ReleaseDC(hWnd, hDC);

   return 0;
}

LRESULT
DefWndNCCalcSize(HWND hWnd, BOOL CalcSizeStruct, RECT *Rect)
{
   LRESULT Result = 0;
   DWORD Style = GetClassLongW(hWnd, GCL_STYLE);
   DWORD ExStyle;
   SIZE WindowBorders;
   RECT OrigRect = *Rect;

   if (CalcSizeStruct)
   {
      if (Style & CS_VREDRAW)
      {
         Result |= WVR_VREDRAW;
      }
      if (Style & CS_HREDRAW)
      {
         Result |= WVR_HREDRAW;
      }
      Result |= WVR_VALIDRECTS;
   }
    
   Style = GetWindowLongW(hWnd, GWL_STYLE);
   ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
    
   if (!(Style & WS_MINIMIZE))
   {
      ULONG menuheight;
      HMENU menu = GetMenu(hWnd);
      
      UserGetWindowBorders(Style, ExStyle, &WindowBorders, FALSE);
      InflateRect(Rect, -WindowBorders.cx, -WindowBorders.cy);
      if ((Style & WS_CAPTION) == WS_CAPTION)
      {
         if (ExStyle & WS_EX_TOOLWINDOW)
            Rect->top += GetSystemMetrics(SM_CYSMCAPTION);
         else
            Rect->top += GetSystemMetrics(SM_CYCAPTION);
      }

      if (menu && !(Style & WS_CHILD))
      {
         HDC hDC = GetWindowDC(hWnd);
         if(hDC)
         {
           RECT CliRect = *Rect;
           CliRect.bottom -= OrigRect.top;
           CliRect.right -= OrigRect.left;
           CliRect.left -= OrigRect.left;
           CliRect.top -= OrigRect.top;
           menuheight = (ULONG)MenuDrawMenuBar(hDC, &CliRect, hWnd, FALSE);
           ReleaseDC(hWnd, hDC);
           Rect->top += max(menuheight, GetSystemMetrics(SM_CYMENU));
         }
      }

      if (ExStyle & WS_EX_CLIENTEDGE)
      {
         InflateRect(Rect, -2 * GetSystemMetrics(SM_CXBORDER),
            -2 * GetSystemMetrics(SM_CYBORDER));
      }

      if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL) &&
          (Rect->bottom - Rect->top) > GetSystemMetrics(SM_CYHSCROLL))
      {
         if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
            Rect->left += GetSystemMetrics(SM_CXVSCROLL);
         else
            Rect->right -= GetSystemMetrics(SM_CXVSCROLL);
         Rect->bottom -= GetSystemMetrics(SM_CYHSCROLL);
      }
      else
      {
         if (Style & WS_VSCROLL)
         {
            if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
               Rect->left += GetSystemMetrics(SM_CXVSCROLL);
            else
               Rect->right -= GetSystemMetrics(SM_CXVSCROLL);
         }
         else if (Style & WS_HSCROLL)
            Rect->bottom -= GetSystemMetrics(SM_CYHSCROLL);
      }
      if (Rect->top > Rect->bottom)
         Rect->bottom = Rect->top;
      if (Rect->left > Rect->right)
         Rect->right = Rect->left;
   }
   else
   {
      Rect->right = Rect->left;
      Rect->bottom = Rect->top;
   }
    
   return Result;
}

LRESULT
DefWndNCActivate(HWND hWnd, WPARAM wParam)
{
   DefWndNCPaint(hWnd, (HRGN)1);
   return TRUE;
}

/*
 * FIXME:
 * - Check the scrollbar handling
 */
LRESULT
DefWndNCHitTest(HWND hWnd, POINT Point)
{
   RECT WindowRect, ClientRect;
   POINT ClientPoint;
   SIZE WindowBorders;
   ULONG Style = GetWindowLongW(hWnd, GWL_STYLE);
   ULONG ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);

   GetWindowRect(hWnd, &WindowRect);
   if (!PtInRect(&WindowRect, Point))
   {      
      return HTNOWHERE;
   }

   if (UserHasWindowEdge(Style, ExStyle))
   {
      DWORD XSize, YSize; 

      UserGetWindowBorders(Style, ExStyle, &WindowBorders, FALSE);
      InflateRect(&WindowRect, -WindowBorders.cx, -WindowBorders.cy);
      XSize = GetSystemMetrics(SM_CXSIZE) * GetSystemMetrics(SM_CXBORDER);
      YSize = GetSystemMetrics(SM_CYSIZE) * GetSystemMetrics(SM_CYBORDER);
      if (!PtInRect(&WindowRect, Point))
      {
         BOOL ThickFrame;
         
         ThickFrame = (Style & WS_THICKFRAME);
         if (Point.y < WindowRect.top)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.x < (WindowRect.left + XSize))
               return HTTOPLEFT;
            if (Point.x >= (WindowRect.right - XSize))
               return HTTOPRIGHT;
            return HTTOP;
         }
         if (Point.y >= WindowRect.bottom)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.x < (WindowRect.left + XSize))
               return HTBOTTOMLEFT;
            if (Point.x >= (WindowRect.right - XSize))
               return HTBOTTOMRIGHT;
            return HTBOTTOM;
         }
         if (Point.x < WindowRect.left)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.y < (WindowRect.top + YSize))
               return HTTOPLEFT;
            if (Point.y >= (WindowRect.bottom - YSize))
               return HTBOTTOMLEFT;
            return HTLEFT;
         }
         if (Point.x >= WindowRect.right)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.y < (WindowRect.top + YSize))
               return HTTOPRIGHT;
            if (Point.y >= (WindowRect.bottom - YSize))
               return HTBOTTOMRIGHT;
            return HTRIGHT;
         }
      }
   }
   else
   {
      if (ExStyle & WS_EX_STATICEDGE)
         InflateRect(&WindowRect,
            -GetSystemMetrics(SM_CXBORDER),
            -GetSystemMetrics(SM_CYBORDER));
      if (!PtInRect(&WindowRect, Point))
         return HTBORDER;
   }

   if ((Style & WS_CAPTION) == WS_CAPTION)
   {
      if (ExStyle & WS_EX_TOOLWINDOW)
         WindowRect.top += GetSystemMetrics(SM_CYSMCAPTION);
      else
         WindowRect.top += GetSystemMetrics(SM_CYCAPTION);
      if (!PtInRect(&WindowRect, Point))
      {
         if (Style & WS_SYSMENU)
         {
            if (ExStyle & WS_EX_TOOLWINDOW)
            {
               WindowRect.right -= GetSystemMetrics(SM_CXSMSIZE);
            }
            else
            {
               WindowRect.left += GetSystemMetrics(SM_CXSIZE);
               WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
            }
         }
         if (Point.x <= WindowRect.left)
            return HTSYSMENU;
         if (WindowRect.right <= Point.x)
            return HTCLOSE;
         if (Style & WS_MAXIMIZEBOX || Style & WS_MINIMIZEBOX)
            WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
         if (Point.x >= WindowRect.right)
            return HTMAXBUTTON;
         if (Style & WS_MINIMIZEBOX)
            WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
         if (Point.x >= WindowRect.right)
            return HTMINBUTTON;
         return HTCAPTION;
      }
   }
   
   if(!(Style & WS_MINIMIZE))
   {
     HMENU menu;
     
     ClientPoint = Point;
     ScreenToClient(hWnd, &ClientPoint);
     GetClientRect(hWnd, &ClientRect);
   
     if (PtInRect(&ClientRect, ClientPoint))
     {
        return HTCLIENT;
     }
     
     if ((menu = GetMenu(hWnd)) && !(Style & WS_CHILD))
     {
        if (Point.x > 0 && Point.x < WindowRect.right && ClientPoint.y < 0)
           return HTMENU;
     }

     if (ExStyle & WS_EX_CLIENTEDGE)
     {
        InflateRect(&WindowRect, -2 * GetSystemMetrics(SM_CXBORDER),
           -2 * GetSystemMetrics(SM_CYBORDER));
     }

     if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL) &&
         (WindowRect.bottom - WindowRect.top) > GetSystemMetrics(SM_CYHSCROLL))
     {
        RECT TempRect = WindowRect, TempRect2 = WindowRect;

        TempRect.bottom -= GetSystemMetrics(SM_CYHSCROLL);
        if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
           TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
        if (PtInRect(&TempRect, Point))
           return HTVSCROLL;

        TempRect2.top = TempRect2.bottom - GetSystemMetrics(SM_CYHSCROLL);
        if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
           TempRect2.left += GetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect2.right -= GetSystemMetrics(SM_CXVSCROLL);
        if (PtInRect(&TempRect2, Point))
           return HTHSCROLL;

        TempRect.top = TempRect2.top;
        TempRect.bottom = TempRect2.bottom;
        if (PtInRect(&TempRect, Point) && 
            (!(Style & WS_CHILD) || (ExStyle & WS_EX_MDICHILD)))
        {
           if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
              return HTBOTTOMLEFT;
           else
              return HTBOTTOMRIGHT;
        }
     }
     else
     {
        if (Style & WS_VSCROLL)
        {
           RECT TempRect = WindowRect;

           if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
              TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
           else
              TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
           if (PtInRect(&TempRect, Point))
              return HTVSCROLL;
        } else
        if (Style & WS_HSCROLL)
        {
           RECT TempRect = WindowRect;
           TempRect.top = TempRect.bottom - GetSystemMetrics(SM_CYHSCROLL);
           if (PtInRect(&TempRect, Point))
              return HTHSCROLL;
        }
     }
   }
  
   return HTNOWHERE;
}

VOID
DefWndDoButton(HWND hWnd, WPARAM wParam)
{
  MSG Msg;
  BOOL InBtn, HasBtn = FALSE;
  ULONG Btn, Style;
  WPARAM SCMsg, CurBtn = wParam, OrigBtn = wParam;
  HDC WindowDC;
  
  Style = GetWindowLongW(hWnd, GWL_STYLE);
  switch(wParam)
  {
    case HTCLOSE:
      Btn = DFCS_CAPTIONCLOSE;
      SCMsg = SC_CLOSE;
      HasBtn = (Style & WS_SYSMENU);
      break;
    case HTMINBUTTON:
      Btn = DFCS_CAPTIONMIN;
      SCMsg = ((Style & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE);
      HasBtn = (Style & WS_MINIMIZEBOX);
      break;
    case HTMAXBUTTON:
      Btn = DFCS_CAPTIONMAX;
      SCMsg = ((Style & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE);
      HasBtn = (Style & WS_MAXIMIZEBOX);
      break;
    default:
      return;
  }
  
  InBtn = HasBtn;
  
  SetCapture(hWnd);
  
  if(HasBtn)
  {
    WindowDC = GetWindowDC(hWnd);
    UserDrawCaptionButtonWnd(hWnd, WindowDC, HasBtn , Btn);
  }
  
 for(;;)
  {
    GetMessageW(&Msg, 0, 0, 0);
    switch(Msg.message)
    {
      case WM_NCLBUTTONUP:
      case WM_LBUTTONUP:
        if(InBtn)
          goto done;
        else
        {
          ReleaseCapture();
          if (HasBtn)
            ReleaseDC(hWnd, WindowDC);
          return;
        }
      case WM_NCMOUSEMOVE:
      case WM_MOUSEMOVE:
        if(HasBtn)
        {
          CurBtn = DefWndNCHitTest(hWnd, Msg.pt);
          if(InBtn != (CurBtn == OrigBtn))
          {
            UserDrawCaptionButtonWnd( hWnd, WindowDC, (CurBtn == OrigBtn) , Btn);
          }
          InBtn = CurBtn == OrigBtn;
        }
        break;
    }
  }
  
done:
  UserDrawCaptionButtonWnd( hWnd, WindowDC, FALSE , Btn);
  ReleaseDC(hWnd, WindowDC);
  ReleaseCapture();
  SendMessageW(hWnd, WM_SYSCOMMAND, SCMsg, 0);
  return;
}


LRESULT
DefWndNCLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case HTCAPTION:
        {
	        HWND hTopWnd = GetAncestor(hWnd, GA_ROOT);
	        if (SetActiveWindow(hTopWnd) || GetActiveWindow() == hTopWnd)
	        {
	            SendMessageW(hWnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam);
	        }
	        break;
        }
        case HTSYSMENU:
        {
	  if (GetWindowLongW(hWnd, GWL_STYLE) & WS_SYSMENU)
            {
	      SendMessageW(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU,
			   lParam);
	    }
	  break;
        }
        case HTMENU:
        {
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTMENU, lParam);
            break;
        }
        case HTHSCROLL:
        {
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam);
            break;
        }
        case HTVSCROLL:
        {
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam);
            break;
        }
        case HTMINBUTTON:
        case HTMAXBUTTON:
        case HTCLOSE:
        {
          DefWndDoButton(hWnd, wParam);
          break;
        }
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTBOTTOM:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        {
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_SIZE + wParam - 2, lParam);
            break;
        }
    }
    return(0);
}


LRESULT
DefWndNCLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  ULONG Style;
  
  Style = GetWindowLongW(hWnd, GWL_STYLE);
  switch(wParam)
  {
    case HTCAPTION:
    {
      /* Maximize/Restore the window */
      if((Style & WS_CAPTION) && (Style & WS_MAXIMIZEBOX))
      {
        SendMessageW(hWnd, WM_SYSCOMMAND, ((Style & (WS_MINIMIZE | WS_MAXIMIZE)) ? SC_RESTORE : SC_MAXIMIZE), 0);
      }
      break;
    }
    case HTSYSMENU:
    {
      SendMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
      break;
    }
    default:
      return DefWndNCLButtonDown(hWnd, wParam, lParam);
  }
  return(0);
}

VOID
DefWndTrackScrollBar(HWND hWnd, WPARAM wParam, POINT Point)
{
   INT ScrollBar;

   if ((wParam & 0xfff0) == SC_HSCROLL)
   {
      if ((wParam & 0x0f) != HTHSCROLL)
         return;
      ScrollBar = SB_HORZ;
   }
   else
   {
      if ((wParam & 0x0f) != HTVSCROLL)
         return;
      ScrollBar = SB_VERT;
   }

   /* FIXME */
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL STDCALL
AdjustWindowRectEx(LPRECT lpRect, 
		   DWORD dwStyle, 
		   BOOL bMenu, 
		   DWORD dwExStyle)
{
   SIZE BorderSize;
   
   if (bMenu)
   {
      lpRect->top -= GetSystemMetrics(SM_CYMENU);
   }
   if ((dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      if (dwExStyle & WS_EX_TOOLWINDOW)
         lpRect->top -= GetSystemMetrics(SM_CYSMCAPTION);
      else
         lpRect->top -= GetSystemMetrics(SM_CYCAPTION);
   }
   UserGetWindowBorders(dwStyle, dwExStyle, &BorderSize, TRUE);
   InflateRect(
      lpRect,
      BorderSize.cx,
      BorderSize.cy);

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
AdjustWindowRect(LPRECT lpRect,
		 DWORD dwStyle,
		 BOOL bMenu)
{
   return AdjustWindowRectEx(lpRect, dwStyle, bMenu, 0);
}

// Enabling this will cause captions to draw smoother, but slower:
#define DOUBLE_BUFFER_CAPTION

/*
 * @implemented
 */
BOOL WINAPI
DrawCaption(HWND hWnd, HDC hDC, LPCRECT lprc, UINT uFlags)
{
    NONCLIENTMETRICSW nclm;
    BOOL result = FALSE;
    RECT r = *lprc;
    UINT VCenter = 0, Padding = 0, Height;
    ULONG Style;
    WCHAR buffer[256];
    HFONT hFont = NULL;
    HFONT hOldFont = NULL;
    HBRUSH OldBrush = NULL;
    HDC MemDC = NULL;
    int ButtonWidth;
    COLORREF OldTextColor;

#ifdef DOUBLE_BUFFER_CAPTION
    HBITMAP MemBMP = NULL, OldBMP = NULL;

    MemDC = CreateCompatibleDC(hDC);
    if (! MemDC) goto cleanup;
    MemBMP = CreateCompatibleBitmap(hDC, lprc->right - lprc->left, lprc->bottom - lprc->top);
    if (! MemBMP) goto cleanup;
    OldBMP = SelectObject(MemDC, MemBMP);
    if (! OldBMP) goto cleanup;
#else
    MemDC = hDC;

    OffsetViewportOrgEx(MemDC, lprc->left, lprc->top, NULL);
#endif

    Style = GetWindowLongW(hWnd, GWL_STYLE);
    
    /* Windows behaves like this */
    Height = GetSystemMetrics(SM_CYCAPTION) - 1;

    VCenter = (lprc->bottom - lprc->top) / 2;
    Padding = VCenter - (Height / 2);

    r.left = Padding;
    r.right = r.left + (lprc->right - lprc->left);
    r.top = Padding;
    r.bottom = r.top + (Height / 2);

    // Draw the caption background
    if (uFlags & DC_INBUTTON)
    {
        OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_BTNFACE : COLOR_BTNSHADOW) );
        if (! OldBrush) goto cleanup;
        if (! PatBlt(MemDC, 0, 0, lprc->right - lprc->left, lprc->bottom - lprc->top, PATCOPY )) goto cleanup;
    }
    else
    {
        if (uFlags & DC_GRADIENT)
        {
          COLORREF Colors[2];
          BYTE cr[3], cg[3], cb[3];
          LONG xx, wd, wdh;
          POINT OldPoint;
          HPEN Pen;
          HGDIOBJ OldObj;
          
          r.right = (lprc->right - lprc->left);
          if (uFlags & DC_SMALLCAP)
            ButtonWidth = GetSystemMetrics(SM_CXSMSIZE) - 2;
          else
            ButtonWidth = GetSystemMetrics(SM_CXSIZE) - 2;
          
          if (Style & WS_SYSMENU)
          {
            r.right -= 3 + ButtonWidth;
            if (! (uFlags & DC_SMALLCAP))
            {
              if(Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
                r.right -= 2 + 2 * ButtonWidth;
              else
                r.right -= 2;
              r.right -= 2;
            }
          }
          
          Colors[0] = GetSysColor((uFlags & DC_ACTIVE) ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
          Colors[1] = GetSysColor((uFlags & DC_ACTIVE) ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);
          
          if ((uFlags & DC_ICON) && (Style & WS_SYSMENU) && !(uFlags & DC_SMALLCAP))
          {
            OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
            if (!OldBrush) goto cleanup;
            xx = GetSystemMetrics(SM_CXSIZE) + Padding;
            /* draw icon background */
            PatBlt(MemDC, 0, 0, xx, lprc->bottom - lprc->top, PATCOPY);
            // For some reason the icon isn't centered correctly...
            r.top --;
            UserDrawSysMenuButton(hWnd, MemDC, &r);
            r.top ++;
            r.left += xx;
          }          
          
          cr[1] = GetRValue(Colors[0]);
          cg[1] = GetGValue(Colors[0]);
          cb[1] = GetBValue(Colors[0]);
          cr[2] = GetRValue(Colors[1]) - cr[1];
          cg[2] = GetGValue(Colors[1]) - cg[1];
          cb[2] = GetBValue(Colors[1]) - cb[1];
          
          wd = r.right - r.left;
          wdh = wd / 2;
          
          if(wd > 0)
          {
            MoveToEx(MemDC, 0, 0, &OldPoint);
            
            for(xx = 0; xx < wd; xx++)
            {
              cr[0] = (cr[1] + ((cr[2] * xx) + wdh) / wd);
              cg[0] = (cg[1] + ((cg[2] * xx) + wdh) / wd);
              cb[0] = (cb[1] + ((cb[2] * xx) + wdh) / wd);
              Pen = CreatePen(PS_SOLID, 0, RGB(cr[0], cg[0], cb[0]));
              if(!Pen)
              {
                break;
              }
              OldObj = SelectObject(MemDC, Pen);
              MoveToEx(MemDC, r.left + xx, 0, NULL);
              LineTo(MemDC, r.left + xx, lprc->bottom - lprc->top);
              if(OldObj)
                SelectObject(MemDC, OldObj);
              DeleteObject(Pen);
            }
            MoveToEx(MemDC, OldPoint.x, OldPoint.y, NULL);
          }
          
          if(OldBrush)
          {
            SelectObject(MemDC, OldBrush);
            OldBrush = NULL;
          }
          xx = lprc->right - lprc->left - r.right;
          if(xx > 0)
          {
            OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION));
            if (!OldBrush) goto cleanup;
            PatBlt(MemDC, r.right, 0, xx, lprc->bottom - lprc->top, PATCOPY);
          }
        }
        else
        {
            OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION) );
            if (! OldBrush) goto cleanup;
            if (! PatBlt(MemDC, 0, 0, lprc->right - lprc->left, lprc->bottom - lprc->top, PATCOPY )) goto cleanup;
        }
    }
    
    if ((uFlags & DC_ICON) && !(uFlags & DC_GRADIENT) && (Style & WS_SYSMENU) && !(uFlags & DC_SMALLCAP))
    {
        // For some reason the icon isn't centered correctly...
        r.top --;
        UserDrawSysMenuButton(hWnd, MemDC, &r);
        r.top ++;
    }
    r.top ++;
    r.left += 2;

    r.bottom = r.top + Height;

  if ((uFlags & DC_TEXT) && (GetWindowTextW( hWnd, buffer, sizeof(buffer)/sizeof(buffer[0]) )))
  {
    if(!(uFlags & DC_GRADIENT))
    {
    if (!(uFlags & DC_SMALLCAP) && ((uFlags & DC_ICON) || (uFlags & DC_INBUTTON)))
        r.left += GetSystemMetrics(SM_CXSIZE) + Padding;

    r.right = (lprc->right - lprc->left);
    if (uFlags & DC_SMALLCAP)
      ButtonWidth = GetSystemMetrics(SM_CXSMSIZE) - 2;
    else
      ButtonWidth = GetSystemMetrics(SM_CXSIZE) - 2;

    if (Style & WS_SYSMENU)
    {
      r.right -= 3 + ButtonWidth;
      if (! (uFlags & DC_SMALLCAP))
      {
        if(Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
	      r.right -= 2 + 2 * ButtonWidth;
        else
	      r.right -= 2;
        r.right -= 2;
      }
    }
    }

    nclm.cbSize = sizeof(nclm);
    if (! SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nclm, 0)) goto cleanup;

    SetBkMode( MemDC, TRANSPARENT );
    if (uFlags & DC_SMALLCAP)
        hFont = CreateFontIndirectW(&nclm.lfSmCaptionFont);
    else
        hFont = CreateFontIndirectW(&nclm.lfCaptionFont);

    if (! hFont) goto cleanup;

    hOldFont = SelectObject(MemDC, hFont);
    if (! hOldFont) goto cleanup;
    
    if (uFlags & DC_INBUTTON)
        OldTextColor = SetTextColor(MemDC, GetSysColor(uFlags & DC_ACTIVE ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    else
        OldTextColor = SetTextColor(MemDC, GetSysColor(uFlags & DC_ACTIVE ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

    DrawTextW(MemDC, buffer, wcslen(buffer), &r, DT_VCENTER | DT_END_ELLIPSIS);
    
    SetTextColor(MemDC, OldTextColor);
  }

#if 0
    if (uFlags & DC_BUTTONS)
    {
        // Windows XP draws the caption buttons with DC_BUTTONS
//        r.left += GetSystemMetrics(SM_CXSIZE) + 1;
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONCLOSE);
//        r.right -= GetSystemMetrics(SM_CXSMSIZE) + 1;
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMIN);
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMAX);
    }
#endif

#ifdef DOUBLE_BUFFER_CAPTION
    if (! BitBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
            MemDC, 0, 0, SRCCOPY)) goto cleanup;
#endif

    result = TRUE;

    cleanup :
        if (MemDC)
        {
            if (OldBrush) SelectObject(MemDC, OldBrush);
            if (hOldFont) SelectObject(MemDC, hOldFont);
            if (hFont) DeleteObject(hFont);
#ifdef DOUBLE_BUFFER_CAPTION
            if (OldBMP) SelectObject(MemDC, OldBMP);
            if (MemBMP) DeleteObject(MemBMP);
            DeleteDC(MemDC);
#else
            OffsetViewportOrgEx(MemDC, -lprc->left, -lprc->top, NULL);
#endif
        }

        return result;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DrawCaptionTempW(
		 HWND        hwnd,
		 HDC         hdc,
		 const RECT *rect,
		 HFONT       hFont,
		 HICON       hIcon,
		 LPCWSTR     str,
		 UINT        uFlags
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DrawCaptionTempA(
		 HWND        hwnd,
		 HDC         hdc,
		 const RECT *rect,
		 HFONT       hFont,
		 HICON       hIcon,
		 LPCSTR      str,
		 UINT        uFlags
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}
