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
   GLuint z_depth;
   GLuint totalBytes;
   int width2;

   GLubyte *addr;

   z_depth = ( smesa->glCtx->Visual.depthBits +
               smesa->glCtx->Visual.stencilBits ) / 8;

   width2 = ALIGNMENT( smesa->width * z_depth, 4 );

   totalBytes = smesa->height * width2 + Z_BUFFER_HW_PLUS;

   addr = sisAllocFB( smesa, totalBytes, &smesa->zbFree );
   if (addr == NULL)
      sis_fatal_error("Failure to allocate Z buffer.\n");

   if (SIS_VERBOSE & VERBOSE_SIS_BUFFER) {
      fprintf(stderr, "sis_alloc_z_stencil_buffer: addr=%p\n", addr);
   }

   addr = (GLubyte *)ALIGNMENT( (unsigned long)addr, Z_BUFFER_HW_ALIGNMENT );

   smesa->depthbuffer = (void *) addr;
   smesa->depthPitch = width2;
   smesa->depthOffset = (unsigned long)addr - (unsigned long)smesa->FbBase;

   /* set pZClearPacket */
   memset( &smesa->zClearPacket, 0, sizeof(ENGPACKET) );

   smesa->zClearPacket.dwSrcPitch = (z_depth == 2) ? 0x80000000 : 0xf0000000;
   smesa->zClearPacket.dwDestBaseAddr = (unsigned long)(addr -
      (unsigned long)smesa->FbBase);
   smesa->zClearPacket.wDestPitch = width2;
   smesa->zClearPacket.stdwDestPos.wY = 0;
   smesa->zClearPacket.stdwDestPos.wX = 0;

   smesa->zClearPacket.wDestHeight = smesa->virtualY;
   smesa->zClearPacket.stdwDim.wWidth = (GLshort)width2 / z_depth;
   smesa->zClearPacket.stdwDim.wHeight = (GLshort)smesa->height;
   smesa->zClearPacket.stdwCmd.cRop = 0xf0;

   if (smesa->blockWrite)
      smesa->zClearPacket.stdwCmd.cCmd0 = CMD0_PAT_FG_COLOR;
   else
      smesa->zClearPacket.stdwCmd.cCmd0 = 0;
   smesa->zClearPacket.stdwCmd.cCmd1 = CMD1_DIR_X_INC | CMD1_DIR_Y_INC;
}

void
sisFreeZStencilBuffer( sisContextPtr smesa )
{
   sisFreeFB( smesa, smesa->zbFree );
   smesa->zbFree = NULL;
   smesa->depthbuffer = NULL;
}

void
sisAllocBackbuffer( sisContextPtr smesa )
{
   GLuint depth = smesa->bytesPerPixel;
   GLuint size, width2;

   char *addr;

   width2 = (depth == 2) ? ALIGNMENT (smesa->width, 2) : smesa->width;
   size = width2 * smesa->height * depth + DRAW_BUFFER_HW_PLUS;

   /* Fixme: unique context alloc/free back-buffer? */
   addr = sisAllocFB( smesa, size, &smesa->bbFree );
   if (addr == NULL)
      sis_fatal_error("Failure to allocate back buffer.\n");

   addr = (char *)ALIGNMENT( (unsigned long)addr, DRAW_BUFFER_HW_ALIGNMENT );

   smesa->backbuffer = addr;
   smesa->backOffset = (unsigned long)(addr - (unsigned long)smesa->FbBase);
   smesa->backPitch = width2 * depth;

   memset ( &smesa->cbClearPacket, 0, sizeof(ENGPACKET) );

   smesa->cbClearPacket.dwSrcPitch = (depth == 2) ? 0x80000000 : 0xf0000000;
   smesa->cbClearPacket.dwDestBaseAddr = smesa->backOffset;
   smesa->cbClearPacket.wDestPitch = smesa->backPitch;
   smesa->cbClearPacket.stdwDestPos.wY = 0;
   smesa->cbClearPacket.stdwDestPos.wX = 0;

   smesa->cbClearPacket.wDestHeight = smesa->virtualY;
   smesa->cbClearPacket.stdwDim.wWidth = (GLshort) width2;
   smesa->cbClearPacket.stdwDim.wHeight = (GLshort) smesa->height;
   smesa->cbClearPacket.stdwCmd.cRop = 0xf0;

   if (smesa->blockWrite)
      smesa->cbClearPacket.stdwCmd.cCmd0 = (GLbyte)(CMD0_PAT_FG_COLOR);
   else
      smesa->cbClearPacket.stdwCmd.cCmd0 = 0;
   smesa->cbClearPacket.stdwCmd.cCmd1 = CMD1_DIR_X_INC | CMD1_DIR_Y_INC;
}

void
sisFreeBackbuffer( sisContextPtr smesa )
{
   sisFreeFB( smesa, smesa->bbFree );
   smesa->backbuffer = NULL; 
}
