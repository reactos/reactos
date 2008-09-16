/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef _I810_INIT_H_
#define _I810_INIT_H_

#include <sys/time.h>
#include "dri_util.h"

typedef struct {
   drm_handle_t handle;
   drmSize size;
   char *map;
} i810Region, *i810RegionPtr;

typedef struct {
   i810Region front;
   i810Region back;
   i810Region depth;
   i810Region tex;

   int deviceID;
   int width;
   int height;
   int mem;

   int cpp;			/* for front and back buffers */
   int bitsPerPixel;

   int fbFormat;
   int fbOffset;
   int fbStride;

   int backOffset;
   int depthOffset;

   int backPitch;
   int backPitchBits;

   int textureOffset;
   int textureSize;
   int logTextureGranularity;

   __DRIscreenPrivate *driScrnPriv;
   drmBufMapPtr  bufs;
   unsigned int sarea_priv_offset;
} i810ScreenPrivate;


extern GLboolean
i810CreateContext( const __GLcontextModes *mesaVis,
                   __DRIcontextPrivate *driContextPriv,
                   void *sharedContextPrivate );

extern void
i810DestroyContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
i810UnbindContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
i810MakeCurrent(__DRIcontextPrivate *driContextPriv,
                __DRIdrawablePrivate *driDrawPriv,
                __DRIdrawablePrivate *driReadPriv);

extern void
i810SwapBuffers(__DRIdrawablePrivate *driDrawPriv);

#endif
