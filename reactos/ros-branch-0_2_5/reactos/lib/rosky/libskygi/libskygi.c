/*
 * ROSky - SkyOS Application Layer
 * Copyright (C) 2004 ReactOS Team
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id: libskygi.c,v 1.13 2004/10/20 19:19:12 weiden Exp $
 *
 * PROJECT:         SkyOS GI library
 * FILE:            lib/libskygi/libskygi.c
 * PURPOSE:         SkyOS GI library
 *
 * UPDATE HISTORY:
 *      08/12/2004  Created
 */
#include <windows.h>
#include <stdio.h>
#include <rosky/rosky.h>
#include "libskygi.h"
#include "resource.h"

typedef struct
{
  s_window Window;
  HWND hWnd;
  BOOL MouseInput;
} SKY_WINDOW, *PSKY_WINDOW;

typedef struct
{
  widget_menu Menu;
  HMENU hMenu;
} SKY_MENU, *PSKY_MENU;

typedef struct
{
  widget_menu_item MenuItem;
  MENUITEMINFOW MenuItemInfo;
} SKY_MENUITEM, *PSKY_MENUITEM;

typedef struct
{
  GC GraphicsContext;
  HDC hDC;
} SKY_GC, *PSKY_GC;

typedef struct
{
  DIB Dib;
  HBITMAP hBitmap;
  HDC hAssociateDC;
} SKY_DIB, *PSKY_DIB;

static ATOM SkyClassAtom;
static BOOL SkyClassRegistered = FALSE;

/**
 * Map a SkyOS window style to Windows one.
 *
 * @param SkyStyle SkyOS window style (WF_* flags).
 * @param ExStyle Contains Windows extended window style on exit.
 *
 * @return Windows window style (WS_* flags).
 *
 * @todo Handle
 *  WF_MODAL, WF_HAS_MENU, WF_HAS_STATUSBAR, WF_FREEFORM, WF_FOCUSABLE,
 *  WF_USER, WF_DESKTOP, WF_NOT_MOVEABLE, WF_NO_BUTTONS, WF_TRANSPARENT,
 *  WF_NO_INITIAL_DRAW, WF_USE_BACKGROUND, WF_DONT_EREASE_BACKGROUND,
 *  WF_NO_FRAME.
 */
ULONG
IntMapWindowStyle(ULONG SkyStyle, ULONG *ExStyle)
{
   ULONG Style;

   Style = (SkyStyle & WF_HIDE) ? 0 : WS_VISIBLE;
   Style |= (SkyStyle & WF_NO_TITLE) ? 0 : WS_CAPTION;
   Style |= (SkyStyle & WF_NOT_SIZEABLE) ? WS_THICKFRAME : 0;
   Style |= (SkyStyle & WF_POPUP) ? WS_POPUP : 0;
   Style |= (SkyStyle & WF_NO_BUTTONS) ? 0 :
            ((SkyStyle & WF_NOT_SIZEABLE) ? 0 : WS_MAXIMIZEBOX) |
            WS_MINIMIZEBOX | WS_SYSMENU;
   *ExStyle = (SkyStyle & WF_SMALL_TITLE) ? WS_EX_TOOLWINDOW : 0;

   return Style;
}


/**
 * Dispatch a Sky Message to the appropriate window callback
 *
 * @param win Specifies the destination window
 * @param type The type of the message (see MSG_ constants)
 * @param para1 Additional parameter 1
 * @param para2 Additional parameter 2
 *
 * @return Returns the return value of the window callback function
 */
unsigned long
IntDispatchMsg(s_window *win, unsigned int type, unsigned int para1, unsigned int para2)
{
  s_gi_msg msg;
  unsigned long Ret;
  
  /* fill the members of the struct */
  msg.win = win;
  msg.type = type;
  msg.para1 = para1;
  msg.para2 = para2;
  msg.next = NULL; /* ??? */
  msg.prev = NULL; /* ??? */
  /* FIXME */
  msg.timestamp = (unsigned long long)GetTickCount() * 1000LL;
  
  DBG("Dispatching window (0x%x) message type %d\n", win, type);
  Ret = win->win_func(win, &msg);
  DBG("Dispatched window (0x%x) message type %d, returned 0x%x\n", win, type, Ret);
  return Ret;
}


/**
 * Dispatch a Sky Message with a update rect to the appropriate window callback
 *
 * @param win Specifies the destination window
 * @param type The type of the message (see MSG_ constants)
 * @param para1 Additional parameter 1
 * @param para2 Additional parameter 2
 * @param rect Rectangle of the window to be repainted
 *
 * @return Returns the return value of the window callback function
 */
unsigned long
IntDispatchMsgRect(s_window *win, unsigned int type, unsigned int para1, unsigned int para2, s_region *rect)
{
  s_gi_msg msg;
  unsigned long Ret;

  /* fill the members of the struct */
  msg.win = win;
  msg.type = type;
  msg.para1 = para1;
  msg.para2 = para2;
  msg.next = NULL; /* ??? */
  msg.prev = NULL; /* ??? */
  msg.rect = *rect;
  /* FIXME */
  msg.timestamp = (unsigned long long)GetTickCount() * 1000LL;

  DBG("Dispatching window (0x%x) message type %d\n", win, type);
  Ret = win->win_func(win, &msg);
  DBG("Dispatched window (0x%x) message type %d, returned 0x%x\n", win, type, Ret);
  return Ret;
}


/**
 * Determines whether a win32 message should cause a Sky message to be dispatched
 *
 * @param skw Specifies the destination window
 * @param Msg Contains the win32 message
 * @param smsg Address to the sky message structure that will be filled in with
 *             appropriate information in case a sky message should be dispatched
 *
 * @return Returns TRUE if a Sky message should be dispatched
 */
BOOL
IntIsSkyMessage(PSKY_WINDOW skw, MSG *Msg, s_gi_msg *smsg)
{
  smsg->win = skw;

  switch(Msg->message)
  {
    case WM_DESTROY:
      smsg->type = MSG_DESTROY;
      smsg->para1 = 0;
      smsg->para2 = 0;
      return TRUE;

    case WM_PAINT:
    {
      RECT rc;
      PAINTSTRUCT ps;
      
      if(GetUpdateRect(skw->hWnd, &rc, FALSE))
      {
        BeginPaint(skw->hWnd, &ps);
        EndPaint(skw->hWnd, &ps);

        smsg->type = MSG_GUI_REDRAW;
        smsg->para1 = 0;
        smsg->para2 = 0;

        smsg->rect.x1 = rc.left;
        smsg->rect.y1 = rc.top;
        smsg->rect.x2 = rc.right;
        smsg->rect.y2 = rc.bottom;

        return TRUE;
      }
      break;
    }

    case WM_QUIT:
      smsg->type = MSG_QUIT;
      smsg->para1 = 0;
      smsg->para2 = 0;
      return TRUE;

    case WM_COMMAND:
      smsg->type = MSG_COMMAND;
      smsg->para1 = LOWORD(Msg->wParam);
      return TRUE;

    case WM_MOUSEMOVE:
      if(skw->MouseInput)
      {
        smsg->type = MSG_MOUSE_MOVED;
        goto DoMouseInputMessage;
      }
      break;

    case WM_LBUTTONDOWN:
      smsg->type = MSG_MOUSE_BUT1_PRESSED;
      goto DoMouseInputMessage;

    case WM_LBUTTONUP:
      smsg->type = MSG_MOUSE_BUT1_RELEASED;
      goto DoMouseInputMessage;

    case WM_RBUTTONDOWN:
      smsg->type = MSG_MOUSE_BUT2_PRESSED;
      goto DoMouseInputMessage;

    case WM_RBUTTONUP:
    {
      POINT pt;

      smsg->type = MSG_MOUSE_BUT2_RELEASED;

DoMouseInputMessage:
#if 0
      pt.x = LOWORD(Msg->lParam);
      pt.y = HIWORD(Msg->lParam);
#else
      pt = Msg->pt;
      MapWindowPoints(NULL, skw->hWnd, &pt, 1);
#endif
      smsg->para1 = pt.x;
      smsg->para2 = pt.y;
      return TRUE;
    }
  }
  
  return FALSE;
}


/**
 * The standard win32 window procedure that handles win32 messages delivered from ReactOS
 *
 * @param hWnd Handle of the window
 * @param msg Specifies the type of the message
 * @param wParam Additional data to the message
 * @param lParam Additional data to the message
 *
 * @return Depends on the message type
 */
LRESULT CALLBACK
IntDefaultWin32Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PSKY_WINDOW skw;

  if (msg == WM_NCCREATE)
  {
    /*
     * Save the pointer to the structure so we can access it later when
     * dispatching the Win32 messages so we know which sky window it is
     * and dispatch the right messages.
     */
    skw = (PSKY_WINDOW)((LPCREATESTRUCTW)lParam)->lpCreateParams;
    SetWindowLongPtr(hWnd, GWL_USERDATA, (ULONG_PTR)skw);
  }
  else
  {
    skw = (PSKY_WINDOW)GetWindowLongPtr(hWnd, GWL_USERDATA);
    if (skw == NULL)
      return DefWindowProcW(hWnd, msg, wParam, lParam);
  }

  switch(msg)
  {
    case WM_CLOSE:
      IntDispatchMsg(&skw->Window, MSG_DESTROY, 0, 0);
      return 0;

    case WM_CREATE:
      return 1;
    
    /* FIXME: Find a more general solution! */
    /* We can get there for message sent by SendMessage. */
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      s_region srect;
      
      BeginPaint(hWnd, &ps);
      srect.x1 = ps.rcPaint.left;
      srect.y1 = ps.rcPaint.top;
      srect.x2 = ps.rcPaint.right;
      srect.y2 = ps.rcPaint.bottom;
      IntDispatchMsgRect(&skw->Window, MSG_GUI_REDRAW, 0, 0, &srect);
      EndPaint(hWnd, &ps);
      
      return 0;
    }

    case WM_COMMAND:
      IntDispatchMsg(&skw->Window, MSG_COMMAND, LOWORD(wParam), 0);
      return 0;

    case WM_ERASEBKGND:
      return 1; /* don't handle this message */
  }

  return DefWindowProcW(hWnd, msg, wParam, lParam);
}


/**
 * Registers a Win32 window class for all Sky windows
 *
 * @return Returns the atom of the class registered.
 */
ATOM
IntRegisterClass(void)
{
  WNDCLASSW wc;
  
  wc.lpszClassName = L"ROSkyWindow";
  wc.lpfnWndProc = IntDefaultWin32Proc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  
  return RegisterClassW(&wc);
}


/*
 * @implemented
 */
s_window* __cdecl
GI_create_app(app_para *p)
{
  PSKY_WINDOW skw;
  ULONG Style, ExStyle;
  WCHAR WindowName[sizeof(p->cpName) / sizeof(p->cpName[0])];
  RECT ClientRect;
  
  DBG("GI_create_app(0x%x)\n", p);

  /* FIXME - lock */
  if(!SkyClassRegistered)
  {
    SkyClassAtom = IntRegisterClass();
    SkyClassRegistered = SkyClassAtom != 0;

    if(!SkyClassRegistered)
    {
      DBG("Unable to register the ROSkyWindow class\n");
      return NULL;
    }
  }
  /* FIXME - unlock */

  skw = (PSKY_WINDOW)HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(SKY_WINDOW));
  if(skw == NULL)
  {
    DBG("Not enough memory to allocate a SKY_WINDOW structure!\n");
    return NULL;
  }
  
  /* Convert the Sky window style to a Win32 window style */
  Style = IntMapWindowStyle(p->ulStyle, &ExStyle);
  
  /* convert the window caption to unicode */
  MultiByteToWideChar(CP_UTF8, 0, p->cpName, -1, WindowName,
                      sizeof(WindowName) / sizeof(WindowName[0]));
  
  skw->Window.win_func = p->win_func;
  /* FIXME - fill the window structure */

  /*
   * We must convert the client rect passed in to the window rect expected
   * by CreateWindowExW.
   */
  ClientRect.left = 0;
  ClientRect.top = 0;
  ClientRect.right = 0 + p->ulWidth;
  ClientRect.bottom = 0 + p->ulHeight;
  AdjustWindowRectEx(&ClientRect, Style, p->ulStyle & WF_HAS_MENU, ExStyle);

  DBG("Menu: %x\n", p->pMenu ? ((PSKY_MENU)p->pMenu)->hMenu : NULL);

  /* create the Win32 window */
  skw->hWnd = CreateWindowExW(ExStyle,
                              L"ROSkyWindow",
                              WindowName,
                              WS_OVERLAPPEDWINDOW,
                              p->ulX,
                              p->ulY,
                              ClientRect.right - ClientRect.left,
                              ClientRect.bottom - ClientRect.top,
                              NULL,
                              p->pMenu ? ((PSKY_MENU)p->pMenu)->hMenu : NULL,
                              GetModuleHandleW(NULL),
                              skw);

  if(skw->hWnd == NULL)
  {
    DBG("CreateWindow() failed!\n");
    HeapFree(GetProcessHeap(), 0, skw);
    return NULL;
  }
    
  DBG("Created Win32 window: 0x%x\n", skw->hWnd);
  
  return &skw->Window;
}

/*
 * @implemented
 */
int __cdecl
GI_destroy_window(s_window *win)
{
  PSKY_WINDOW skw = (PSKY_WINDOW)win;

  DBG("GI_destroy_window(0x%x)\n", win);
  DestroyWindow(skw->hWnd);
  HeapFree(GetProcessHeap(), 0, skw);

  return 0;
}


/*
 * @implemented
 */
unsigned int __cdecl
GI_wait_message(s_gi_msg *m,
                s_window* w)
{
  MSG Msg;
  BOOL Ret, SkyMessage;
  HWND hwndFilter;
  PSKY_WINDOW msgwnd;
  
  DBG("GI_wait_message(0x%x, 0x%x)\n", m, w);
  
  hwndFilter = (w != NULL ? ((PSKY_WINDOW)w)->hWnd : NULL);
  do
  {
    Ret = GetMessage(&Msg, hwndFilter, 0, 0);

    /* loop until we found a message that a sky app would handle, too */
    RtlZeroMemory(m, sizeof(s_gi_msg));

    if(Msg.hwnd != NULL && (msgwnd = (PSKY_WINDOW)GetWindowLongPtrW(Msg.hwnd, GWL_USERDATA)))
      {
        SkyMessage = IntIsSkyMessage(msgwnd, &Msg, m);
      }
    else
      {
        SkyMessage = FALSE;
      }

    if (!SkyMessage)
    {
      /* We're not interested in dispatching a sky message, try again */
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
  }
  while (!SkyMessage);
  
  return Ret;
}


/*
 * @implemented
 */
int __cdecl
GI_dispatch_message(s_window *win,
                    s_gi_msg *m)
{
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  DBG("GI_dispatch_message(0x%x, 0x%x - %d)\n", win, m, m->type);
  /* dispatch the SkyOS message to the SkyOS window procedure */
  if (skywnd != 0)
    return skywnd->Window.win_func(win, m);
  return 1;
}


/*
 * @implemented
 */
HRESULT __cdecl
GI_ShowApplicationWindow(s_window *win)
{
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  DBG("GI_ShowApplicationWindow(0x%x)\n", win);
  ShowWindow(skywnd->hWnd, SW_SHOW);
  return 1;
}


/*
 * @implemented
 */
int __cdecl
GI_redraw_window(s_window *win)
{
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  DBG("GI_redraw_window(0x%x)!\n", win);
  if(skywnd != NULL)
  {
    RedrawWindow(skywnd->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
  }
  return 1;
}


/*
 * @implemented
 */
void __cdecl
GI_post_quit(s_window *win)
{
  DBG("GI_post_quit(0x%x)\n", win);
  PostMessage(((PSKY_WINDOW)win)->hWnd, WM_QUIT, 0, 0);
}


/*
 * @implemented
 */
sCreateApplication* __cdecl
GI_CreateApplicationStruct(void)
{
  sCreateApplication *app;
  
  app = (sCreateApplication*)HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(sCreateApplication));
  STUB("GI_CreateApplicationStruct() returns 0x%x (allocated structure on the heap)!\n", app);

  return app;
}


/*
 * @implemented
 */
int __cdecl
GI_GetWindowX(s_window *win)
{
  RECT rc;
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  if((skywnd != NULL) && GetWindowRect(skywnd->hWnd, &rc))
  {
    MapWindowPoints(HWND_DESKTOP, GetParent(skywnd->hWnd), (LPPOINT)&rc, 2);
    DBG("GI_GetWindowS(0x%x) returns %d!\n", win, rc.left);
    return rc.left;
  }
  #if DEBUG
  else
  {
    DBG("GI_GetWindowS(0x%x) failed!\n", win);
  }
  #endif
  return 0;
}


/*
 * @implemented
 */
int __cdecl
GI_GetWindowY(s_window *win)
{
  RECT rc;
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  if((skywnd != NULL) && GetWindowRect(skywnd->hWnd, &rc))
  {
    MapWindowPoints(HWND_DESKTOP, GetParent(skywnd->hWnd), (LPPOINT)&rc, 2);
    DBG("GI_GetWindowY(0x%x) returns %d!\n", win, rc.top);
    return rc.left;
  }
  #if DEBUG
  else
  {
    DBG("GI_GetWindowY(0x%x) failed!\n", win);
  }
  #endif
  return 0;
}


/*
 * @implemented
 */
int __cdecl
GI_GetWindowWidth(s_window *win)
{
  RECT rc;
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  if((skywnd != NULL) && GetWindowRect(skywnd->hWnd, &rc))
  {
    DBG("GI_GetWindowWidth(0x%x) returns %d!\n", win, (rc.right - rc.left));
    return (rc.right - rc.left);
  }
  #if DEBUG
  else
  {
    DBG("GI_GetWindowWidth(0x%x) failed!\n", win);
  }
  #endif
  return 0;
}


/*
 * @implemented
 */
int __cdecl
GI_GetWindowHeight(s_window *win)
{
  RECT rc;
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  if((skywnd != NULL) && GetWindowRect(skywnd->hWnd, &rc))
  {
    DBG("GI_GetWindowHeight(0x%x) returns %d!\n", win, (rc.bottom - rc.top));
    return (rc.bottom - rc.top);
  }
  #if DEBUG
  else
  {
    DBG("GI_GetWindowHeight(0x%x) failed!\n", win);
  }
  #endif
  return 0;
}


/*
 * @unimplemented
 */
s_window* __cdecl
GI_GetTopLevelWindow(s_window *win)
{
  STUB("GI_GetTopLevelWindow(0x%x) returns 0x%x!\n", win, win);
  return win;
}


/*
 * @implemented
 */
DIB* __cdecl
GI_create_DIB(void *Data,
              unsigned int Width,
              unsigned int Height,
              unsigned int Bpp,
              void *Palette,
              unsigned int PaletteSize)
{
   SKY_DIB *Dib;
   BITMAPINFO *BitmapInfo;

   DBG("GI_create_DIB(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", 
       Data, Width, Height, Bpp, Palette, PaletteSize);

   Dib = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SKY_DIB));
   if (Dib == NULL)
   {
      return NULL;
   }

   BitmapInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER) +
                          PaletteSize * sizeof(RGBQUAD));
   if (BitmapInfo == NULL)
   {
      HeapFree(GetProcessHeap(), 0, Dib);
      return NULL;
   }

   Dib->Dib.color = Bpp;
   Dib->Dib.width = Width;
   Dib->Dib.height = Height;
   Dib->Dib.data = Data;
   Dib->Dib.palette_size = PaletteSize;
   Dib->Dib.palette = Palette;

   BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   BitmapInfo->bmiHeader.biWidth = Width;
   BitmapInfo->bmiHeader.biHeight = Height;
   BitmapInfo->bmiHeader.biPlanes = 1;
   BitmapInfo->bmiHeader.biBitCount = Bpp;
   BitmapInfo->bmiHeader.biCompression = BI_RGB;
   BitmapInfo->bmiHeader.biSizeImage = 0;
   BitmapInfo->bmiHeader.biXPelsPerMeter = 0;
   BitmapInfo->bmiHeader.biYPelsPerMeter = 0;
   BitmapInfo->bmiHeader.biClrUsed = PaletteSize;
   BitmapInfo->bmiHeader.biClrImportant = 0;
   RtlCopyMemory(BitmapInfo->bmiColors, Palette, PaletteSize * sizeof(RGBQUAD));

   Dib->hBitmap = CreateDIBSection(NULL,
                                   BitmapInfo,
                                   DIB_RGB_COLORS,
                                   Data,
                                   NULL,
                                   0);
   HeapFree(GetProcessHeap(), 0, BitmapInfo);
   if (Dib->hBitmap == NULL)
   {
      HeapFree(GetProcessHeap(), 0, Dib);
      return NULL;
   }

   return (DIB*)Dib;
}


/*
 * @implemented
 */
GC* __cdecl
GC_create_connected(unsigned int Type,
                    unsigned int Width,
                    unsigned int Height,
                    s_window *Win)
{
   SKY_GC *Gc;

   DBG("GC_create_connected(0x%x, 0x%x, 0x%x, 0x%x)\n",
       Type, Width, Height, Win);

   if(Win == NULL)
   {
     DBG("GC_create_connected: no window specified! returned NULL!\n");
     return NULL;
   }

   Gc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SKY_GC));
   if (Gc == NULL)
   {
      return NULL;
   }

   Gc->GraphicsContext.type = Type;
   Gc->GraphicsContext.width = Width;
   Gc->GraphicsContext.height = Height;

   switch (Type)
   {
      case GC_TYPE_DIB:
         Gc->hDC = CreateCompatibleDC(0);
         if (Gc->hDC)
         {
            Gc->GraphicsContext.hDIB = (DIB*)Win;
            SelectObject(Gc->hDC, ((PSKY_DIB)Win)->hBitmap);
            ((PSKY_DIB)Win)->hAssociateDC = Gc->hDC;
         }
         break;

      case GC_TYPE_WINDOW:
         Gc->hDC = GetDC(((PSKY_WINDOW)Win)->hWnd);
         Gc->GraphicsContext.window = Win;
         break;

      default:
         DBG("Unknown GC type: %x\n", Type);
   }

   if (Gc->hDC == NULL)
   {
      HeapFree(GetProcessHeap(), 0, Gc);
      return NULL;
   }
   else
   {
      SelectObject(Gc->hDC, GetStockObject(DC_BRUSH));
      SelectObject(Gc->hDC, GetStockObject(DC_PEN));
   }

   return (GC*)Gc;
}


/*
 * @implemented
 */
int __cdecl
GC_set_fg_color(GC *Gc,
                COLOR Color)
{
   if(Gc != NULL)
   {
     Gc->fg_color = Color;
     SetDCPenColor(((PSKY_GC)Gc)->hDC, Color);
     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_set_fg_color: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
int __cdecl
GC_set_bg_color(GC *Gc,
                COLOR Color)
{
   if(Gc != NULL)
   {
     Gc->bg_color = Color;
     SetDCBrushColor(((PSKY_GC)Gc)->hDC, Color);
     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_set_bg_color: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
int __cdecl
GC_draw_pixel(GC *Gc,
              int X,
              int Y)
{
   if(Gc != NULL)
   {
     SetPixelV(((PSKY_GC)Gc)->hDC, X, Y, Gc->fg_color);
     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_draw_pixel: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
int __cdecl
GC_blit_from_DIB(GC *Gc,
                 DIB *Dib,
                 int X,
                 int Y)
{
   int Result;
   HDC hSrcDC;
   HBITMAP hOldBitmap;

   DBG("GC_blit_from_DIB(0x%x, 0x%x, 0x%x, 0x%x)\n", Gc, Dib, X, Y);

   if (((PSKY_DIB)Dib)->hAssociateDC == NULL)
   {
      hSrcDC = CreateCompatibleDC(0);
      hOldBitmap = SelectObject(hSrcDC, ((PSKY_DIB)Dib)->hBitmap);
   }
   else
   {
      hSrcDC = ((PSKY_DIB)Dib)->hAssociateDC;
   }

   Result = BitBlt(((PSKY_GC)Gc)->hDC, X, Y, Dib->width, Dib->height,
                   hSrcDC, 0, 0, SRCCOPY);
   
   if (((PSKY_DIB)Dib)->hAssociateDC == NULL)
   {
      SelectObject(hSrcDC, hOldBitmap);
      DeleteDC(hSrcDC);
   }

   return !Result;
}


/*
 * @implemented
 */
int __cdecl
GC_draw_rect_fill(GC *Gc,
                 int X,
                 int Y,
                 int Width,
                 int Height)
{
   DBG("GC_draw_rect_fill(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
       Gc, X, Y, Width, Height);

   if(Gc != NULL)
   {
     HBRUSH hBrush;
     RECT Rect;

     Rect.left = X;
     Rect.top = Y;
     Rect.right = X + Width;
     Rect.bottom = Y + Height;

     hBrush = CreateSolidBrush(Gc->bg_color);
     FillRect(((PSKY_GC)Gc)->hDC, &Rect, hBrush);
     DeleteObject(hBrush);

     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_draw_rect_fill: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
int __cdecl
GC_draw_line(GC *Gc,
             int x1,
             int y1,
             int x2,
             int y2)
{
   DBG("GC_draw_line(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", Gc, x1, y1, x2, y2);
   if(Gc != NULL)
   {
     MoveToEx(((PSKY_GC)Gc)->hDC, x1, y1, NULL);
     LineTo(((PSKY_GC)Gc)->hDC, x2, y2);
     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_draw_line: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
int __cdecl
GC_destroy(GC *Gc)
{
   DBG("GC_destroy(0x%x)\n", Gc);
   if (Gc != NULL)
   {
      switch (Gc->type)
      {
         case GC_TYPE_DIB:
            DeleteDC(((PSKY_GC)Gc)->hDC);
            break;

         case GC_TYPE_WINDOW:
            ReleaseDC(((PSKY_WINDOW)Gc->window)->hWnd, ((PSKY_GC)Gc)->hDC);
            break;

         default:
            DBG("Unknown GC type: %x\n", Gc->type);
     }
     HeapFree(GetProcessHeap(), 0, Gc);
     return 1;
   }
   #if DEBUG
   else
   {
     DBG("GC_destroy: Gc == NULL!\n");
   }
   #endif
   return 0;
}


/*
 * @implemented
 */
widget_menu* __cdecl
GI_create_menu(s_window *win)
{
   PSKY_MENU Menu;

   DBG("GI_create_menu(0x%x)\n", win);

   Menu = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SKY_MENU));
   if (Menu == NULL)
   {
      return NULL;
   }

   /* Shouldn't we use CreatePopupMenu in some cases? */
   Menu->hMenu = CreateMenu();
   if (Menu->hMenu == NULL)
   {
      HeapFree(GetProcessHeap(), 0, Menu);
      return NULL;
   }

   if (win)
   {
      SetMenu(((PSKY_WINDOW)win)->hWnd, Menu->hMenu);
   }

   return (widget_menu *)Menu;
}


/*
 * @implemented
 */
widget_menu_item* __cdecl
GI_create_menu_item(unsigned char *Text,
                    unsigned int Id,
                    unsigned int Flags,
                    unsigned int Enabled)
{
   PSKY_MENUITEM MenuItem;
   ULONG TextLength;
   
   DBG("GI_create_menu_item(0x%x, 0x%x, 0x%x, 0x%x)\n",
       Text, Id, Flags, Enabled);

   TextLength = MultiByteToWideChar(CP_UTF8, 0, Text, -1, NULL, 0);
   MenuItem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof(SKY_MENUITEM) + TextLength * sizeof(WCHAR));
   if (MenuItem == NULL)
   {
      return NULL;
   }

   lstrcpyA(MenuItem->MenuItem.text, Text);
   MenuItem->MenuItem.ID = Id;
   MenuItem->MenuItem.flags = Flags;
   MenuItem->MenuItem.enabled = Enabled;

   MenuItem->MenuItemInfo.cbSize = sizeof(MENUITEMINFOW);
   MenuItem->MenuItemInfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
   if (Flags & MENU_SEPERATOR)
      MenuItem->MenuItemInfo.fType = MF_SEPARATOR;
   else
      MenuItem->MenuItemInfo.fType = MF_STRING;
   MenuItem->MenuItemInfo.fState = Enabled ? MFS_ENABLED : 0;
   MenuItem->MenuItemInfo.wID = Id;
   MenuItem->MenuItemInfo.dwTypeData = (LPWSTR)(MenuItem + 1);
   MenuItem->MenuItemInfo.cch = TextLength;
   MultiByteToWideChar(CP_UTF8, 0, Text, TextLength, (LPWSTR)(MenuItem + 1),
                       TextLength);

   return (widget_menu_item *)MenuItem;
}


/*
 * @implemented
 */
int __cdecl
GI_add_menu_item(widget_menu *Menu,
                 widget_menu_item *Item)
{
   DBG("GI_add_menu_item(0x%x, 0x%x)\n", Menu, Item);
   InsertMenuItemW(((PSKY_MENU)Menu)->hMenu, -1, TRUE,
                   &((PSKY_MENUITEM)Item)->MenuItemInfo);
   return 1;
}


/*
 * @implemented
 */
int __cdecl
GI_add_menu_sub(widget_menu *Menu,
                widget_menu_item *Item,
                widget_menu *Sub)
{
   PSKY_MENUITEM MenuItem = (PSKY_MENUITEM)Item;

   DBG("GI_add_menu_sub(0x%x, 0x%x, 0x%x)\n", Menu, Item, Sub);
   MenuItem->MenuItemInfo.fMask |= MIIM_SUBMENU;
   MenuItem->MenuItemInfo.hSubMenu = ((PSKY_MENU)Sub)->hMenu;
   InsertMenuItemW(((PSKY_MENU)Menu)->hMenu, -1, TRUE,
                   &MenuItem->MenuItemInfo);
   return 1;
}


/*
 * @implemented
 */
int __cdecl
GI_messagebox(s_window *Window,
              unsigned int Flags,
              char *Title,
              char *Fmt,
              ...)
{
   CHAR Buffer[4096];
   va_list ArgList;
   ULONG MbFlags, MbResult;

   DBG("GI_messagebox(0x%x, 0x%x, 0x%x, 0x%x, ...)\n",
       Window, Flags, Title, Fmt);

   va_start(ArgList, Fmt);
   _vsnprintf(Buffer, sizeof(Buffer) / sizeof(Buffer[0]), Fmt, ArgList);
   va_end(ArgList);

   if ((Flags & (WGF_MB_CANCEL | WGF_MB_YESNO)) ==
       (WGF_MB_CANCEL | WGF_MB_YESNO))
      MbFlags = MB_YESNOCANCEL;
   else if (Flags & WGF_MB_YESNO)
      MbFlags = MB_YESNO;
   else if (Flags & WGF_MB_OK)
      MbFlags = MB_OK;
   MbFlags |= (Flags & WGF_MB_ICON_INFO) ? MB_ICONASTERISK : 0;
   MbFlags |= (Flags & WGF_MB_ICON_ASK) ? MB_ICONQUESTION : 0;
   MbFlags |= (Flags & WGF_MB_ICON_STOP) ? MB_ICONERROR : 0;

   MbResult = MessageBoxA(Window ? ((PSKY_WINDOW)Window)->hWnd : NULL, 
                          Buffer, Title, MbFlags);

   switch (MbResult)
   {
      case IDOK: return ID_OK;
      case IDYES: return ID_YES;
      case IDNO: return ID_NO;
      case IDCANCEL: return ID_CANCEL;
   }

   return 0;
}


/*
 * @implemented
 */
int __cdecl
GI_EnableMouseTracking(s_window *win)
{
  DBG("GI_EnableMouseTracking(0x%x)!\n", win);
  if(win != NULL)
  {
    ((PSKY_WINDOW)win)->MouseInput = TRUE;
    return 1;
  }
   #if DEBUG
   else
   {
     DBG("GI_EnableMouseTracking: win == NULL!\n");
   }
   #endif
  return 0;
}

/* EOF */
