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
