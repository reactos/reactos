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
/* $Id: libskygi.c,v 1.2 2004/08/12 19:27:12 weiden Exp $
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

LRESULT CALLBACK
SkyWndDefaultWin32Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PSKY_WINDOW skw = (PSKY_WINDOW)GetWindowLong(hWnd, GWL_USERDATA);
  
  if(skw != NULL)
  {
    switch(msg)
    {
      case WM_CLOSE:
        PostQuitMessage(0);
        break;
      case WM_NCDESTROY:
        /* free the SKY_WINDOW structure */
        HeapFree(GetProcessHeap(), 0, skw);
        break;
    }
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

ATOM
SkyWndRegisterClass(void)
{
  WNDCLASS wc;
  
  wc.lpszClassName = "ROSkyWindow";
  wc.lpfnWndProc = SkyWndDefaultWin32Proc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  
  return RegisterClass(&wc);
}


/*
 * @implemented
 */
s_window* __cdecl
GI_create_app(app_para *p)
{
  PSKY_WINDOW skw;
  
  DBG("GI_create_app(0x%x)\n", p);

  /* FIXME - lock */
  if(!SkyClassRegistered)
  {
    SkyClassAtom = SkyWndRegisterClass();
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
  
  skw->hWnd = CreateWindow("ROSkyWindow",
                           (p->cpName[0] != '\0' ? (char*)p->cpName : ""),
                           WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                           p->ulX,
                           p->ulY,
                           p->ulWidth,
                           p->ulHeight,
                           NULL,
                           NULL,
                           GetModuleHandle(NULL),
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
  SetWindowLong(skw->hWnd, GWL_USERDATA, (LONG)skw);
  
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
  Ret = GetMessage(&Msg, hwndFilter, 0, 0);
  if(Ret)
  {
    if(Msg.hwnd != NULL)
    {
      msgwnd = (PSKY_WINDOW)GetWindowLong(Msg.hwnd, GWL_USERDATA);
      msgwnd->LastMsg = Msg;
    }
    else
    {
      msgwnd = NULL;
    }
  }
  
  RtlZeroMemory(m, sizeof(s_gi_msg));
  m->win = (msgwnd != NULL ? &msgwnd->Window : NULL);
  /* FIXME - fill in the other messags */
  
  return (int)Ret;
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
  
  skywnd->DispatchMsg = *m;
  DispatchMessage(&skywnd->LastMsg);
  
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
  
  DBG("->0x%x\n", skywnd->hWnd);
  ShowWindow(skywnd->hWnd, SW_SHOW);
  return 1;
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
