/* $Id: stubs.c,v 1.1 2004/08/12 02:50:35 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         SkyOS GI library
 * FILE:            lib/libskygi/stubs.c
 * PURPOSE:         libskygi.dll stubs
 * NOTES:           If you implement a function, remove it from this file
 *
 * UPDATE HISTORY:
 *      08/12/2004  Created
 */
#include <windows.h>
#include <rosky/rosky.h>
#include "libskygi.h"


/*
 * @unimplemented
 */
int __cdecl
DefaultWindowFunc(HANDLE hWin,
                  s_gi_msg *pMsg)
{
  STUB("DefaultWindowFunc(0x%x, 0x%x) returns 0!\n", hWin, pMsg);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_ResetBlit(sBlit *pBlit)
{
  STUB("GC_ResetBlit(0x%x) returns 0!\n", pBlit);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_blit(GC *gc,
        sBlit *pBlit)
{
  STUB("GC_blit(0x%x, 0x%x) returns 0!\n", gc, pBlit);
  return 0;
}


/*
 * @unimplemented
 */
GC* __cdecl
GC_create_connected(unsigned int type,
                    unsigned int width,
                    unsigned int height,
                    HANDLE win)
{
  STUB("GC_create_connected(0x%x, 0x%x, 0x%x, 0x%x) returns NULL!\n", type, width, height, win);
  return NULL;
}


/*
 * @unimplemented
 */
int __cdecl
GC_destroy(GC *gc)
{
  STUB("GC_destroy(0x%x) returns 0!\n", gc);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_draw_line(GC *gc,
             int x1,
             int y1,
             int x2,
             int y2)
{
  STUB("GC_draw_line(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) returns 0!\n", gc, x1, y1, x2, y2);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_draw_rect_fill(GC *gc,
                 int x,
                 int y,
                 int width,
                 int height)
{
  STUB("GC_draw_rect_fill(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) returns 0!\n", gc, x, y, width, height);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_bg_color(GC *gc,
                COLOR col)
{
  STUB("GC_set_bg_color(0x%x, 0x%x) returns 0!\n", gc, col);
  return 0;
}


/*
 * @unimplemented
 */
HRESULT __cdecl
GI_ShowApplicationWindow(HANDLE hWnd)
{
  STUB("GI_ShowApplicationWindow(0x%x) returns 0!\n", hWnd);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_add_menu_item(widget_menu *menu,
                 widget_menu_item *item)
{
  STUB("GI_add_menu_item(0x%x, 0x%x) returns 0!\n", menu, item);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_add_menu_sub(widget_menu *menu,
                widget_menu_item *item,
                widget_menu *sub)
{
  STUB("GI_add_menu_sub(0x%x, 0x%x, 0x%x) returns 0!\n", menu, item, sub);
  return 0;
}


/*
 * @unimplemented
 */
DDB* __cdecl
GI_create_DDB_from_DIB(DIB *dib)
{
  STUB("GI_create_DDB_from_DIB(0x%x) returns NULL!\n", dib);
  return NULL;
}


/*
 * @unimplemented
 */
HANDLE __cdecl
GI_create_app(app_para *p)
{
  STUB("GI_create_app(0x%x) returns NULL!\n", p);
  return NULL;
}


/*
 * @unimplemented
 */
widget_menu* __cdecl
GI_create_menu(HANDLE win)
{
  STUB("GI_create_menu(0x%x) returns NULL!\n", win);
  return NULL;
}


/*
 * @unimplemented
 */
widget_menu_item* __cdecl
GI_create_menu_item(unsigned char *text,
                    unsigned int ID,
                    unsigned int flags,
                    unsigned int enabled)
{
  STUB("GI_create_menu_item(0x%x, 0x%x, 0x%x, 0x%x) returns NULL!\n", text, ID, flags, enabled);
  return NULL;
}


/*
 * @unimplemented
 */
int __cdecl
GI_destroy_window(s_window *win)
{
  STUB("GI_destroy_window(0x%x) returns 0!\n", win);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_dispatch_message(s_window *win,
                    s_gi_msg *m)
{
  STUB("GI_dispatch_message(0x%x, 0x%x) returns 0!\n", win, m);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_kill_timer(unsigned int uiID)
{
  STUB("GI_kill_timer(0x%x) returns 0!\n", uiID);
  return 0;
}


/*
 * @unimplemented
 */
DIB* __cdecl
GI_load_bitmap(char *filename,
               unsigned int ImageIndex)
{
  STUB("GI_load_bitmap(0x%x, 0x%x) returns NULL!\n", filename, ImageIndex);
  return NULL;
}

/*
 * @unimplemented
 */
int __cdecl
GI_messagebox(HANDLE hWnd,
              unsigned int flags,
              char *titel,
              char *fmt,
              ...)
{
  STUB("GI_messagebox(0x%x, 0x%x, 0x%x, 0x%x, ...) returns 0!\n", hWnd, flags, titel, fmt);
  return 0;
}

/*
 * @unimplemented
 */
void __cdecl
GI_post_quit(HANDLE win)
{
  STUB("GI_post_quit(0x%x)\n", win);
}


/*
 * @unimplemented
 */
unsigned int __cdecl
GI_set_high_timer(HANDLE w,
                  unsigned int msec)
{
  STUB("GI_set_high_timer(0x%x, 0x%x) returns 0!\n", w, msec);
  return 0;
}


/*
 * @unimplemented
 */
unsigned int __cdecl
GI_wait_message(s_gi_msg *m,
                s_window* w)
{
  STUB("GI_wait_message(0x%x, 0x%x) returns 0!\n", m, w);
  return 0;
}

/* EOF */
