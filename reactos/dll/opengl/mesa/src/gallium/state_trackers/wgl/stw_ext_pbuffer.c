/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <windows.h>

#define WGL_WGLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/wglext.h>

#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "stw_device.h"
#include "stw_pixelformat.h"
#include "stw_framebuffer.h"


#define LARGE_WINDOW_SIZE 60000


static LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MINMAXINFO *pMMI;
    switch (uMsg) {
    case WM_GETMINMAXINFO:
        // Allow to create a window bigger than the desktop
        pMMI = (MINMAXINFO *)lParam;
        pMMI->ptMaxSize.x = LARGE_WINDOW_SIZE;
        pMMI->ptMaxSize.y = LARGE_WINDOW_SIZE;
        pMMI->ptMaxTrackSize.x = LARGE_WINDOW_SIZE;
        pMMI->ptMaxTrackSize.y = LARGE_WINDOW_SIZE;
        break;
    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


HPBUFFERARB WINAPI
wglCreatePbufferARB(HDC _hDC,
                    int iPixelFormat,
                    int iWidth,
                    int iHeight,
                    const int *piAttribList)
{
   static boolean first = TRUE;
   const int *piAttrib;
   int useLargest = 0;
   const struct stw_pixelformat_info *info;
   struct stw_framebuffer *fb;
   DWORD dwExStyle;
   DWORD dwStyle;
   RECT rect;
   HWND hWnd;
   HDC hDC;

   info = stw_pixelformat_get_info(iPixelFormat);
   if (!info) {
      SetLastError(ERROR_INVALID_PIXEL_FORMAT);
      return 0;
   }

   if (iWidth <= 0 || iHeight <= 0) {
      SetLastError(ERROR_INVALID_DATA);
      return 0;
   }

   for (piAttrib = piAttribList; *piAttrib; piAttrib++) {
      switch (*piAttrib) {
      case WGL_PBUFFER_LARGEST_ARB:
         piAttrib++;
         useLargest = *piAttrib;
         break;
      default:
         SetLastError(ERROR_INVALID_DATA);
         return 0;
      }
   }

   if (iWidth > stw_dev->max_2d_length) {
      if (useLargest) {
         iWidth = stw_dev->max_2d_length;
      } else {
         SetLastError(ERROR_NO_SYSTEM_RESOURCES);
         return 0;
      }
   }

   if (iHeight > stw_dev->max_2d_length) {
      if (useLargest) {
         iHeight = stw_dev->max_2d_length;
      } else {
         SetLastError(ERROR_NO_SYSTEM_RESOURCES);
         return 0;
      }
   }

   /*
    * Implement pbuffers through invisible windows
    */

   if (first) {
      WNDCLASS wc;
      memset(&wc, 0, sizeof wc);
      wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wc.lpfnWndProc = WndProc;
      wc.lpszClassName = "wglpbuffer";
      wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
      RegisterClass(&wc);
      first = FALSE;
   }

   dwExStyle = 0;
   dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

   if (0) {
      /*
       * Don't hide the window -- useful for debugging what the application is
       * drawing
       */

      dwStyle |= WS_VISIBLE | WS_OVERLAPPEDWINDOW;
   } else {
      dwStyle |= WS_POPUPWINDOW;
   }

   rect.left = 0;
   rect.top = 0;
   rect.right = rect.left + iWidth;
   rect.bottom = rect.top + iHeight;

   /*
    * The CreateWindowEx parameters are the total (outside) dimensions of the
    * window, which can vary with Windows version and user settings.  Use
    * AdjustWindowRect to get the required total area for the given client area.
    *
    * AdjustWindowRectEx does not accept WS_OVERLAPPED style (which is defined
    * as 0), which means we need to use some other style instead, e.g.,
    * WS_OVERLAPPEDWINDOW or WS_POPUPWINDOW as above.
    */

   AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

   hWnd = CreateWindowEx(dwExStyle,
                         "wglpbuffer", /* wc.lpszClassName */
                         NULL,
                         dwStyle,
                         CW_USEDEFAULT, /* x */
                         CW_USEDEFAULT, /* y */
                         rect.right - rect.left, /* width */
                         rect.bottom - rect.top, /* height */
                         NULL,
                         NULL,
                         NULL,
                         NULL);
   if (!hWnd) {
      return 0;
   }

#ifdef DEBUG
   /*
    * Verify the client area size matches the specified size.
    */

   GetClientRect(hWnd, &rect);
   assert(rect.left == 0);
   assert(rect.top == 0);
   assert(rect.right - rect.left == iWidth);
   assert(rect.bottom - rect.top == iHeight);
#endif

   hDC = GetDC(hWnd);
   if (!hDC) {
      return 0;
   }

   SetPixelFormat(hDC, iPixelFormat, &info->pfd);

   fb = stw_framebuffer_create(hDC, iPixelFormat);
   if (!fb) {
      SetLastError(ERROR_NO_SYSTEM_RESOURCES);
   } else {
      stw_framebuffer_release(fb);
   }

   return (HPBUFFERARB)fb;
}


HDC WINAPI
wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
{
   struct stw_framebuffer *fb;
   HDC hDC;

   fb = (struct stw_framebuffer *)hPbuffer;

   hDC = GetDC(fb->hWnd);
   SetPixelFormat(hDC, fb->iPixelFormat, &fb->pfi->pfd);

   return hDC;
}


int WINAPI
wglReleasePbufferDCARB(HPBUFFERARB hPbuffer,
                       HDC hDC)
{
   struct stw_framebuffer *fb;

   fb = (struct stw_framebuffer *)hPbuffer;

   return ReleaseDC(fb->hWnd, hDC);
}


BOOL WINAPI
wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
   struct stw_framebuffer *fb;

   fb = (struct stw_framebuffer *)hPbuffer;

   /* This will destroy all our data */
   return DestroyWindow(fb->hWnd);
}


BOOL WINAPI
wglQueryPbufferARB(HPBUFFERARB hPbuffer,
                   int iAttribute,
                   int *piValue)
{
   struct stw_framebuffer *fb;

   fb = (struct stw_framebuffer *)hPbuffer;

   switch (iAttribute) {
   case WGL_PBUFFER_WIDTH_ARB:
      *piValue = fb->width;
      return TRUE;
   case WGL_PBUFFER_HEIGHT_ARB:
      *piValue = fb->height;
      return TRUE;
   case WGL_PBUFFER_LOST_ARB:
      /* We assume that no content is ever lost due to display mode change */
      *piValue = FALSE;
      return TRUE;
   default:
      SetLastError(ERROR_INVALID_DATA);
      return FALSE;
   }
}
