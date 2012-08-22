/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef STW_FRAMEBUFFER_H
#define STW_FRAMEBUFFER_H

#include <windows.h>

#include "os/os_thread.h"

struct pipe_resource;
struct st_framebuffer_iface;
struct stw_pixelformat_info;

/**
 * Windows framebuffer, derived from gl_framebuffer.
 */
struct stw_framebuffer
{
   /**
    * This mutex has two purposes:
    * - protect the access to the mutable data members below
    * - prevent the framebuffer from being deleted while being accessed.
    * 
    * It is OK to lock this mutex while holding the stw_device::fb_mutex lock, 
    * but the opposite must never happen.
    */
   pipe_mutex mutex;
   
   /*
    * Immutable members.
    * 
    * Note that even access to immutable members implies acquiring the mutex 
    * above, to prevent the framebuffer from being destroyed.
    */
   
   HDC hDC;
   HWND hWnd;

   int iPixelFormat;
   const struct stw_pixelformat_info *pfi;

   struct st_framebuffer_iface *stfb;

   /*
    * Mutable members. 
    */

   unsigned refcnt;

   
   /* FIXME: Make this work for multiple contexts bound to the same framebuffer */
   boolean must_resize;

   unsigned width;
   unsigned height;
   
   /**
    * Client area rectangle, relative to the window upper-left corner.
    *
    * @sa GLCBPRESENTBUFFERSDATA::rect.
    */
   RECT client_rect;

   HANDLE hSharedSurface;
   struct stw_shared_surface *shared_surface;

   /** 
    * This is protected by stw_device::fb_mutex, not the mutex above.
    * 
    * Deletions must be done by first acquiring stw_device::fb_mutex, and then
    * acquiring the stw_framebuffer::mutex of the framebuffer to be deleted. 
    * This ensures that nobody else is reading/writing to the.
    * 
    * It is not necessary to aquire the mutex above to navigate the linked list
    * given that deletions are done with stw_device::fb_mutex held, so no other
    * thread can delete.
    */
   struct stw_framebuffer *next;
};


/**
 * Create a new framebuffer object which will correspond to the given HDC.
 * 
 * This function will acquire stw_framebuffer::mutex. stw_framebuffer_release
 * must be called when done 
 */
struct stw_framebuffer *
stw_framebuffer_create(
   HDC hdc,
   int iPixelFormat );

void
stw_framebuffer_reference(
   struct stw_framebuffer **ptr,
   struct stw_framebuffer *fb);

/**
 * Search a framebuffer with a matching HWND.
 * 
 * This function will acquire stw_framebuffer::mutex. stw_framebuffer_release
 * must be called when done 
 */
struct stw_framebuffer *
stw_framebuffer_from_hwnd(
   HWND hwnd );

/**
 * Search a framebuffer with a matching HDC.
 * 
 * This function will acquire stw_framebuffer::mutex. stw_framebuffer_release
 * must be called when done 
 */
struct stw_framebuffer *
stw_framebuffer_from_hdc(
   HDC hdc );

BOOL
stw_framebuffer_present_locked(HDC hdc,
                               struct stw_framebuffer *fb,
                               struct pipe_resource *res);

void
stw_framebuffer_update(
   struct stw_framebuffer *fb);

/**
 * Release stw_framebuffer::mutex lock. This framebuffer must not be accessed
 * after calling this function, as it may have been deleted by another thread
 * in the meanwhile.
 */
void
stw_framebuffer_release(
   struct stw_framebuffer *fb);

/**
 * Cleanup any existing framebuffers when exiting application.
 */
void
stw_framebuffer_cleanup(void);

#endif /* STW_FRAMEBUFFER_H */
