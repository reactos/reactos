/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_alloc.c,v 1.7 2001/01/08 01:07:29 martin Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_alloc.h"

#include "sis_common.h"

#include <unistd.h>

#define Z_BUFFER_HW_ALIGNMENT 16
#define Z_BUFFER_HW_PLUS (16 + 4)

/* 3D engine uses 2, and bitblt uses 4 */
#define DRAW_BUFFER_HW_ALIGNMENT 16
#define DRAW_BUFFER_HW_PLUS (16 + 4)

#define ALIGNMENT(value, align) (((value) + (align) - 1) / (align) * (align))

static int _total_video_memory_used = 0;
static int _total_video_memory_count = 0;

void *
sisAllocFB( sisContextPtr smesa, GLuint size, void **handle )
{
   drm_sis_mem_t fb;

   _total_video_memory_used += size;

   fb.context = smesa->hHWContext;
   fb.size = size;
   if (drmCommandWriteRead( smesa->driFd, DRM_SIS_FB_ALLOC, &fb, 
      sizeof(drm_sis_mem_t) ) || fb.offset == 0)
   {
      return NULL;
   }
   *handle = (void *)fb.free;

   if (SIS_VERBOSE & VERBOSE_SIS_MEMORY) {
      fprintf(stderr, "sisAllocFB: size=%d, offset=%lu, pid=%d, count=%d\n", 
              size, fb.offset, (GLint)getpid(), 
              ++_total_video_memory_count);
   }

   return (void *)(smesa->FbBase + fb.offset);
}

void
sisFreeFB( sisContextPtr smesa, void *handle )
{
   drm_sis_mem_t fb;

   if (SIS_VERBOSE & VERBOSE_SIS_MEMORY) {
      fprintf(stderr, "sisFreeFB: free=%p, pid=%d, count=%d\n", 
              handle, (GLint)getpid(), --_total_video_memory_count);
   }

   fb.context = smesa->hHWContext;
   fb.free = handle;
   drmCommandWrite( smesa->driFd, DRM_SIS_FB_FREE, &fb, sizeof(drm_sis_mem_t) );
}

void *
sisAllocAGP( sisContextPtr smesa, GLuint size, void **handle )
{
   drm_sis_mem_t agp;
   
   if (smesa->AGPSize == 0)
      return NULL;

   agp.context = smesa->hHWContext;
   agp.size = size;
   if (drmCommandWriteRead( smesa->driFd, DRM_SIS_AGP_ALLOC, &agp,
      sizeof(drm_sis_mem_t) ) || agp.offset == 0)
   {
      return NULL;
   }
   *handle = (void *)agp.free;

   if (SIS_VERBOSE & VERBOSE_SIS_MEMORY) {
      fprintf(stderr, "sisAllocAGP: size=%u, offset=%lu, pid=%d, count=%d\n", 
              size, agp.offset, (GLint)getpid(), 
              ++_total_video_memory_count);
   }

   return (void *)(smesa->AGPBase + agp.offset);
}

void
sisFreeAGP( sisContextPtr smesa, void *handle )
{
   drm_sis_mem_t agp;

   if (SIS_VERBOSE & VERBOSE_SIS_MEMORY) {
      fprintf(stderr, "sisFreeAGP: free=%p, pid=%d, count=%d\n", 
              handle, (GLint)getpid(), --_total_video_memory_count);
   }
  
   agp.context = smesa->hHWContext;
   agp.free = handle;
   drmCommandWrite( smesa->driFd, DRM_SIS_AGP_FREE, &agp,
      sizeof(drm_sis_mem_t) );
}

void
sisAllocZStencilBuffer( sisContextPtr smesa )
{
   int cpp = ( smesa->glCtx->Visual.depthBits +
               smesa->glCtx->Visual.stencilBits ) / 8;
   unsigned char *addr;

   smesa->depth.bpp = cpp * 8;
   smesa->depth.pitch = ALIGNMENT(smesa->driDrawable->w * cpp, 4);
   smesa->depth.size = smesa->depth.pitch * smesa->driDrawable->h;
   smesa->depth.size += Z_BUFFER_HW_PLUS;

   addr = sisAllocFB(smesa, smesa->depth.size, &smesa->depth.handle);
   if (addr == NULL)
      sis_fatal_error("Failure to allocate Z buffer.\n");
   addr = (char *)ALIGNMENT((unsigned long)addr, Z_BUFFER_HW_ALIGNMENT);

   smesa->depth.map = addr;
   smesa->depth.offset = addr - smesa->FbBase;

   /* stencil buffer is same as depth buffer */
   smesa->stencil.size = smesa->depth.size;
   smesa->stencil.offset = smesa->depth.offset;
   smesa->stencil.handle = smesa->depth.handle;
   smesa->stencil.pitch = smesa->depth.pitch;
   smesa->stencil.bpp = smesa->depth.bpp;
   smesa->stencil.map = smesa->depth.map;
}

void
sisFreeZStencilBuffer( sisContextPtr smesa )
{
   sisFreeFB(smesa, smesa->depth.handle);
   smesa->depth.map = NULL; 
   smesa->depth.offset = 0; 
}

void
sisAllocBackbuffer( sisContextPtr smesa )
{
   int cpp = smesa->bytesPerPixel;
   unsigned char *addr;

   smesa->back.bpp = smesa->bytesPerPixel * 8;
   smesa->back.pitch = ALIGNMENT(smesa->driDrawable->w * cpp, 4);
   smesa->back.size = smesa->back.pitch * smesa->driDrawable->h;
   smesa->back.size += DRAW_BUFFER_HW_PLUS;

   addr = sisAllocFB(smesa, smesa->back.size, &smesa->back.handle);
   if (addr == NULL)
      sis_fatal_error("Failure to allocate back buffer.\n");
   addr = (char *)ALIGNMENT((unsigned long)addr, DRAW_BUFFER_HW_ALIGNMENT);

   smesa->back.map = addr;
   smesa->back.offset = addr - smesa->FbBase;
}

void
sisFreeBackbuffer( sisContextPtr smesa )
{
   sisFreeFB(smesa, smesa->back.handle);
   smesa->back.map = NULL; 
   smesa->back.offset = 0; 
}
