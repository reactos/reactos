/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

/**
 * @file
 * GDI software rasterizer support.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include <windows.h>

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "state_tracker/sw_winsys.h"
#include "gdi_sw_winsys.h"


struct gdi_sw_displaytarget
{
   enum pipe_format format;
   unsigned width;
   unsigned height;
   unsigned stride;

   unsigned size;

   void *data;

   BITMAPINFO bmi;
};


/** Cast wrapper */
static INLINE struct gdi_sw_displaytarget *
gdi_sw_displaytarget( struct sw_displaytarget *buf )
{
   return (struct gdi_sw_displaytarget *)buf;
}


static boolean
gdi_sw_is_displaytarget_format_supported( struct sw_winsys *ws,
                                                unsigned tex_usage,
                                                enum pipe_format format )
{
   switch(format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return TRUE;

   /* TODO: Support other formats possible with BMPs, as described in 
    * http://msdn.microsoft.com/en-us/library/dd183376(VS.85).aspx */
      
   default:
      return FALSE;
   }
}


static void *
gdi_sw_displaytarget_map(struct sw_winsys *ws,
                               struct sw_displaytarget *dt,
                               unsigned flags )
{
   struct gdi_sw_displaytarget *gdt = gdi_sw_displaytarget(dt);

   return gdt->data;
}


static void
gdi_sw_displaytarget_unmap(struct sw_winsys *ws,
                                 struct sw_displaytarget *dt )
{

}


static void
gdi_sw_displaytarget_destroy(struct sw_winsys *winsys,
                                   struct sw_displaytarget *dt)
{
   struct gdi_sw_displaytarget *gdt = gdi_sw_displaytarget(dt);

   align_free(gdt->data);
   FREE(gdt);
}


static struct sw_displaytarget *
gdi_sw_displaytarget_create(struct sw_winsys *winsys,
                                  unsigned tex_usage,
                                  enum pipe_format format,
                                  unsigned width, unsigned height,
                                  unsigned alignment,
                                  unsigned *stride)
{
   struct gdi_sw_displaytarget *gdt;
   unsigned cpp;
   unsigned bpp;
   
   gdt = CALLOC_STRUCT(gdi_sw_displaytarget);
   if(!gdt)
      goto no_gdt;

   gdt->format = format;
   gdt->width = width;
   gdt->height = height;

   bpp = util_format_get_blocksizebits(format);
   cpp = util_format_get_blocksize(format);
   
   gdt->stride = align(width * cpp, alignment);
   gdt->size = gdt->stride * height;
   
   gdt->data = align_malloc(gdt->size, alignment);
   if(!gdt->data)
      goto no_data;

   gdt->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   gdt->bmi.bmiHeader.biWidth = gdt->stride / cpp;
   gdt->bmi.bmiHeader.biHeight= -(long)height;
   gdt->bmi.bmiHeader.biPlanes = 1;
   gdt->bmi.bmiHeader.biBitCount = bpp;
   gdt->bmi.bmiHeader.biCompression = BI_RGB;
   gdt->bmi.bmiHeader.biSizeImage = 0;
   gdt->bmi.bmiHeader.biXPelsPerMeter = 0;
   gdt->bmi.bmiHeader.biYPelsPerMeter = 0;
   gdt->bmi.bmiHeader.biClrUsed = 0;
   gdt->bmi.bmiHeader.biClrImportant = 0;

   *stride = gdt->stride;
   return (struct sw_displaytarget *)gdt;

no_data:
   FREE(gdt);
no_gdt:
   return NULL;
}


static struct sw_displaytarget *
gdi_sw_displaytarget_from_handle(struct sw_winsys *winsys,
                                 const struct pipe_resource *templet,
                                 struct winsys_handle *whandle,
                                 unsigned *stride)
{
   assert(0);
   return NULL;
}


static boolean
gdi_sw_displaytarget_get_handle(struct sw_winsys *winsys,
                                struct sw_displaytarget *dt,
                                struct winsys_handle *whandle)
{
   assert(0);
   return FALSE;
}


void
gdi_sw_display( struct sw_winsys *winsys,
                struct sw_displaytarget *dt,
                HDC hDC )
{
    struct gdi_sw_displaytarget *gdt = gdi_sw_displaytarget(dt);

    StretchDIBits(hDC,
                  0, 0, gdt->width, gdt->height,
                  0, 0, gdt->width, gdt->height,
                  gdt->data, &gdt->bmi, 0, SRCCOPY);
}

static void
gdi_sw_displaytarget_display(struct sw_winsys *winsys, 
                             struct sw_displaytarget *dt,
                             void *context_private)
{
    /* nasty:
     */
    HDC hDC = (HDC)context_private;

    gdi_sw_display(winsys, dt, hDC);
}


static void
gdi_sw_destroy(struct sw_winsys *winsys)
{
   FREE(winsys);
}

struct sw_winsys *
gdi_create_sw_winsys(void)
{
   static struct sw_winsys *winsys;

   winsys = CALLOC_STRUCT(sw_winsys);
   if(!winsys)
      return NULL;

   winsys->destroy = gdi_sw_destroy;
   winsys->is_displaytarget_format_supported = gdi_sw_is_displaytarget_format_supported;
   winsys->displaytarget_create = gdi_sw_displaytarget_create;
   winsys->displaytarget_from_handle = gdi_sw_displaytarget_from_handle;
   winsys->displaytarget_get_handle = gdi_sw_displaytarget_get_handle;
   winsys->displaytarget_map = gdi_sw_displaytarget_map;
   winsys->displaytarget_unmap = gdi_sw_displaytarget_unmap;
   winsys->displaytarget_display = gdi_sw_displaytarget_display;
   winsys->displaytarget_destroy = gdi_sw_displaytarget_destroy;

   return winsys;
}

