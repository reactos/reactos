/* $Id$
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
DefaultWindowFunc(s_window *win,
                  s_gi_msg *pMsg)
{
  STUB("DefaultWindowFunc(0x%x, 0x%x) returns 0!\n", win, pMsg);
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
DDB* __cdecl
GI_create_DDB_from_DIB(DIB *dib)
{
  STUB("GI_create_DDB_from_DIB(0x%x) returns NULL!\n", dib);
  return NULL;
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
int __cdecl
GC_draw_text(GC *gc,
             s_region *rect,
             unsigned char *text)
{
  STUB("GC_draw_text(0x%x, 0x%x, 0x%x) returns 0!\n", gc, rect, text);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_font(GC *gc,
            unsigned int fontIndex)
{
  STUB("GC_set_font(0x%x, 0x%x) returns 0!\n", gc, fontIndex);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_font_size(GC *gc,
                 unsigned int fontSize)
{
  STUB("GC_set_font_size(0x%x, 0x%x) returns 0!\n", gc, fontSize);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_font_flags(GC *gc,
                  unsigned int flags)
{
  STUB("GC_set_font_flags(0x%x, 0x%x) returns 0!\n", gc, flags);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_font_param(GC *gc,
                  unsigned int font,
                  unsigned int fontsize,
                  unsigned int flags,
                  unsigned int trans)
{
  STUB("GC_set_font_param(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) returns 0!\n", gc, font, fontsize, flags, trans);
  return 0;
}


/*
 * @unimplemented
 */
HANDLE __cdecl
GI_CreateApplication(sCreateApplication *application)
{
  STUB("GI_CreateApplication(0x%x) returns NULL!\n", application);
  return NULL;
}


/*
 * @unimplemented
 */
int __cdecl
GI_create_font(unsigned char *family,
               unsigned char *style,
               unsigned char *filename)
{
  STUB("GI_create_font(0x%x, 0x%x, 0x%x) returns 0!\n", family, style, filename);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_textheight(unsigned int index,
              unsigned int size,
              unsigned char *text)
{
  STUB("GI_textheight(0x%x, 0x%x, 0x%x) returns 0!\n", index, size, text);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_textlength(unsigned int index,
              unsigned int size,
              unsigned char *text)
{
  STUB("GI_textlength(0x%x, 0x%x, 0x%x) returns 0!\n", index, size, text);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_init(void)
{
  STUB("GI_init() returns 0!\n");
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_set_dimension(s_window *win,
                 int notify,
                 int x1,
                 int y1,
                 unsigned int width,
                 unsigned int height)
{
  STUB("GI_set_dimension(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) returns 0!\n", win, notify, x1, y1, width, height);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GC_set_clip(GC *gc,
            s_region *clip)
{
  STUB("GC_set_clip(0x%x, 0x%x) returns 0!\n", gc, clip);
  return 0;
}


/*
 * @unimplemented
 */
DIB* __cdecl
GI_ScaleDIB(DIB *srcbitmap,
            int iWidth,
            int iHeight,
            int filtertype,
            float filterwidth)
{
  STUB("GI_ScaleDIB(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) returns 0!\n", srcbitmap, iWidth, iHeight, filtertype, filterwidth);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_get_resolution(s_resolution *res)
{
  STUB("GI_get_resolution(0x%x) returns 0!\n", res);
  return 0;
}


/*
 * @unimplemented
 */
int __cdecl
GI_widget_status_set(s_window *win,
                     unsigned char *text)
{
  STUB("GI_widget_status_set(0x%x, 0x%x) returns 0!\n", win, text);
  return 0;
}


/*
 * @unimplemented
 */
DIB* __cdecl
GI_load_bitmap(char *filename,
               unsigned int ImageIndex)
{
  STUB("GI_load_bitmap(%s, 0x%x) returns NULL!\n", filename, ImageIndex);
  return NULL;
}

/* EOF */
