/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: libskygi.c,v 1.3 2004/08/12 23:38:17 weiden Exp $
 *
 * PROJECT:         SkyOS GI library
 * FILE:            lib/libskygi/libskygi.c
 * PURPOSE:         SkyOS GI library
 *
 * UPDATE HISTORY:
 *      08/12/2004  Created
 */
#include <windows.h>
#include <rosky/rosky.h>
#include "libskygi.h"
#include "resource.h"

typedef struct
{
  s_window Window;
  MSG LastMsg;
  HWND hWnd;
  s_gi_msg DispatchMsg;
} SKY_WINDOW, *PSKY_WINDOW;

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
   Style |= (SkyStyle & WF_NO_BUTTONS) ? 0 : WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;
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
      
      if(GetUpdateRect(skw->hWnd, &rc, FALSE))
      {
        smsg->type = MSG_GUI_REDRAW;
        smsg->para1 = 0;
        smsg->para2 = 0;

        smsg->rect.x1 = rc.left;
        smsg->rect.y1 = rc.top;
        smsg->rect.x2 = rc.right;
        smsg->rect.y2 = rc.bottom;

        return TRUE;
      }
    }

    case WM_QUIT:
      smsg->type = MSG_QUIT;
      smsg->para1 = 0;
      smsg->para2 = 0;
      smsg->win = (s_window*)Msg->wParam;
      return TRUE;
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
  PSKY_WINDOW skw = (PSKY_WINDOW)GetWindowLongW(hWnd, GWL_USERDATA);
  
  if(skw != NULL)
  {
    switch(msg)
    {
      case WM_ERASEBKGND:
        return 1; /* don't handle this message */
      
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
      
      case WM_CLOSE:
        IntDispatchMsg(&skw->Window, MSG_DESTROY, 0, 0);
        return 0;

      case WM_DESTROY:
        SetWindowLongW(hWnd, GWL_USERDATA, 0);
        /* free the SKY_WINDOW structure */
        HeapFree(GetProcessHeap(), 0, skw);
        return 0;
    }
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
  
  /* create the Win32 window */
  skw->hWnd = CreateWindowExW(ExStyle,
                              L"ROSkyWindow",
                              WindowName,
                              WS_OVERLAPPEDWINDOW,
                              p->ulX,
                              p->ulY,
                              p->ulWidth,
                              p->ulHeight,
                              NULL,
                              NULL,
                              GetModuleHandleW(NULL),
                              NULL);

  if(skw->hWnd == NULL)
  {
    DBG("CreateWindow() failed!\n");
    HeapFree(GetProcessHeap(), 0, skw);
    return NULL;
  }

  skw->Window.win_func = p->win_func;
  /* FIXME - fill the window structure */
  
  /* save the pointer to the structure so we can access it later when dispatching
     the win32 messages so we know which sky window it is and dispatch the right
     messages */
  SetWindowLongW(skw->hWnd, GWL_USERDATA, (LONG)skw);
  
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
  
  return (int)DestroyWindow(skw->hWnd);
}


/*
 * @implemented
 */
unsigned int __cdecl
GI_wait_message(s_gi_msg *m,
                s_window* w)
{
  MSG Msg;
  BOOL Ret;
  HWND hwndFilter;
  PSKY_WINDOW msgwnd, filterwnd;
  
  DBG("GI_wait_message(0x%x, 0x%x)\n", m, w);
  
  filterwnd = (w != NULL ? (PSKY_WINDOW)w : NULL);
  
  hwndFilter = (w != NULL ? filterwnd->hWnd : NULL);
  for(;;)
  {
    /* loop until we found a message that a sky app would handle, too */
    RtlZeroMemory(m, sizeof(s_gi_msg));
  
    Ret = GetMessage(&Msg, hwndFilter, 0, 0);
    if(Ret)
    {
      if(Msg.hwnd != NULL && (msgwnd = (PSKY_WINDOW)GetWindowLongW(Msg.hwnd, GWL_USERDATA)))
      {
        msgwnd->LastMsg = Msg;
        if(!IntIsSkyMessage(msgwnd, &Msg, m))
        {
          /* We're not interested in dispatching a sky message, try again */
          TranslateMessage(&Msg);
          DispatchMessage(&Msg);
        }
      }
      else
      {
        /* We're not interested in dispatching a sky message, try again */
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
      }
    }
    else
    {
      /* break the loop, the sky app is supposed to shut down */
      m->type = MSG_QUIT;
      break;
    }
  }
  
  if(m->win == NULL)
  {
    /* only set the win field if it's not set yet by IntIsSkyMessage() */
    m->win = (msgwnd != NULL ? &msgwnd->Window : NULL);
  }
  /* FIXME */
  
  return (m->type != MSG_QUIT);
}


/*
 * @implemented
 */
int __cdecl
GI_dispatch_message(s_window *win,
                    s_gi_msg *m)
{
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  DBG("GI_dispatch_message(0x%x, 0x%x)\n", win, m);
  
  /* FIXME - why is win==1?! */
  if(win == (s_window*)0x1) return 0;
  
  if(win != NULL)
  {
    /* save the dispatched message */
    skywnd->DispatchMsg = *m;
    /* dispatch the last win32 message to the win32 window procedure */
    DispatchMessage(&skywnd->LastMsg);
  }
  
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
  DBG("GI_redraw_window(0x%x)!\n", win);
  PSKY_WINDOW skywnd = (PSKY_WINDOW)win;
  if(skywnd != NULL)
  {
    RedrawWindow(skywnd->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
  }
  return 1;
}


/*
 * @unimplemented
 */
void __cdecl
GI_post_quit(s_window *win)
{
  DBG("GI_post_quit(0x%x)\n", win);
  PostQuitMessage((int)win);
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

/* EOF */
