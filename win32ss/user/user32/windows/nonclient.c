/*
 * ReactOS User32 Library
 * - Window non-client area management
 *
 * Copyright (C) 2003 ReactOS Team
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
WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & (WS_THICKFRAME | WS_MINIMIZE)))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && !((Style) & WS_MINIMIZE) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & (WS_BORDER | WS_MINIMIZE)) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define HASSIZEGRIP(Style, ExStyle, ParentStyle, WindowRect, ParentClientRect) \
            ((!(Style & WS_CHILD) && (Style & WS_THICKFRAME) && !(Style & WS_MAXIMIZE))  || \
             ((Style & WS_CHILD) && (ParentStyle & WS_THICKFRAME) && !(ParentStyle & WS_MAXIMIZE) && \
             (WindowRect.right - WindowRect.left == ParentClientRect.right) && \
             (WindowRect.bottom - WindowRect.top == ParentClientRect.bottom)))


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

/*
 * FIXME:
 * - Check the scrollbar handling
 */
LRESULT
DefWndNCHitTest(HWND hWnd, POINT Point)
{
   RECT WindowRect, ClientRect, OrigWndRect;
   POINT ClientPoint;
   SIZE WindowBorders;
   DWORD Style = GetWindowLongPtrW(hWnd, GWL_STYLE);
   DWORD ExStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);

   GetWindowRect(hWnd, &WindowRect);
   if (!PtInRect(&WindowRect, Point))
   {
      return HTNOWHERE;
   }
   OrigWndRect = WindowRect;

   if (UserHasWindowEdge(Style, ExStyle))
   {
      LONG XSize, YSize;

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
               if(!(ExStyle & WS_EX_DLGMODALFRAME))
                  WindowRect.left += GetSystemMetrics(SM_CXSIZE);
               WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
            }
         }
         if (Point.x < WindowRect.left)
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
     ClientPoint = Point;
     ScreenToClient(hWnd, &ClientPoint);
     GetClientRect(hWnd, &ClientRect);

     if (PtInRect(&ClientRect, ClientPoint))
     {
        return HTCLIENT;
     }

     if (GetMenu(hWnd) && !(Style & WS_CHILD))
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
        RECT ParentRect, TempRect = WindowRect, TempRect2 = WindowRect;
        HWND Parent = GetParent(hWnd);

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
        if(Parent)
          GetClientRect(Parent, &ParentRect);
        if (PtInRect(&TempRect, Point) && HASSIZEGRIP(Style, ExStyle,
            GetWindowLongPtrW(Parent, GWL_STYLE), OrigWndRect, ParentRect))
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

/*
 RealUserDrawCaption: This function is passed through RegisterUserApiHook to uxtheme
                      to call it when the classic caption is needed to be drawn.
 */
LRESULT WINAPI
RealUserDrawCaption(HWND hWnd, HDC hDC, LPCRECT lpRc, UINT uFlags)
{
    ERR("Real DC flags %08x\n",uFlags);
    return NtUserDrawCaption(hWnd, hDC, lpRc, uFlags);
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOL WINAPI
RealAdjustWindowRectEx(LPRECT lpRect,
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
BOOL
WINAPI
DECLSPEC_HOTPATCH
AdjustWindowRectEx(LPRECT lpRect,
		   DWORD dwStyle,
		   BOOL bMenu,
		   DWORD dwExStyle)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

     /* Bypass SEH and go direct. */
   if (!Hook) return RealAdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);

   _SEH2_TRY
   {
      Ret = guah.AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
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
BOOL
WINAPI
DECLSPEC_HOTPATCH
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
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return NtUserDrawCaption(hWnd, hDC, lprc, uFlags);

   _SEH2_TRY
   {
      Ret = guah.DrawCaption(hWnd, hDC, lprc, uFlags);
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
BOOL
WINAPI
DrawCaptionTempW(
		 HWND        hWnd,
		 HDC         hDC,
		 const RECT *rect,
		 HFONT       hFont,
		 HICON       hIcon,
		 LPCWSTR     str,
		 UINT        uFlags
		 )
{
   UNICODE_STRING Text = {0};
   RtlInitUnicodeString(&Text, str);
   return NtUserDrawCaptionTemp(hWnd, hDC, rect, hFont, hIcon, &Text, uFlags);
}

/*
 * @implemented
 */
BOOL
WINAPI
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
  LPWSTR strW;
  INT len;
  BOOL ret = FALSE;

  if (!(uFlags & DC_TEXT) || !str)
    return DrawCaptionTempW(hwnd, hdc, rect, hFont, hIcon, NULL, uFlags);

  len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
  if ((strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
  {
    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len );
    ret = DrawCaptionTempW(hwnd, hdc, rect, hFont, hIcon, strW, uFlags);
    HeapFree(GetProcessHeap(), 0, strW);
  }
  return ret;
}
