/**************************************************************************
 *
 * Copyright 2008-2009 Vmware, Inc.
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

#ifndef STW_WINSYS_H
#define STW_WINSYS_H

#include <windows.h> /* for HDC */

#include "pipe/p_compiler.h"

struct pipe_screen;
struct pipe_context;
struct pipe_resource;

struct stw_shared_surface;

struct stw_winsys
{
   struct pipe_screen *
   (*create_screen)( void );

   /* XXX is it actually possible to have non-zero level/layer ??? */
   /**
    * Present the color buffer to the window associated with the device context.
    */
   void
   (*present)( struct pipe_screen *screen,
               struct pipe_resource *res,
               HDC hDC );

   /**
    * Locally unique identifier (LUID) of the graphics adapter.
    *
    * @sa GLCBPRESENTBUFFERSDATA::AdapterLuid;
    */
   boolean
   (*get_adapter_luid)( struct pipe_screen *screen,
                        LUID *pAdapterLuid );

   /**
    * Open a shared surface (optional).
    *
    * @sa GLCBPRESENTBUFFERSDATA::hSharedSurface;
    */
   struct stw_shared_surface *
   (*shared_surface_open)(struct pipe_screen *screen,
                          HANDLE hSharedSurface);

   /**
    * Close a shared surface (optional).
    */
   void
   (*shared_surface_close)(struct pipe_screen *screen,
                           struct stw_shared_surface *surface);

   /**
    * Compose into a shared (optional).
    *
    * Blit the color buffer into a shared surface.
    *
    * @sa GLPRESENTBUFFERSDATA::PresentHistoryToken.
    */
   void
   (*compose)( struct pipe_screen *screen,
               struct pipe_resource *res,
               struct stw_shared_surface *dest,
               LPCRECT pRect,
               ULONGLONG PresentHistoryToken );
};

boolean
stw_init(const struct stw_winsys *stw_winsys);

boolean
stw_init_thread(void);

void
stw_cleanup_thread(void);

void
stw_cleanup(void);

#endif /* STW_WINSYS_H */
