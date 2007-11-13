/**************************************************************************

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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef __SIS_SCREEN_H
#define __SIS_SCREEN_H

typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress map;			/* Mapping of the DRM region */
} sisRegionRec2, *sisRegionPtr2;

typedef struct {
   sisRegionRec2 mmio;
   sisRegionRec2 agp;
   unsigned long agpBaseOffset;

   unsigned int AGPCmdBufOffset;
   unsigned int AGPCmdBufSize;

   int deviceID;

   int cpp;
   unsigned int screenX, screenY;

   __DRIscreenPrivate *driScreen;
   unsigned int sarea_priv_offset;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;

} sisScreenRec, *sisScreenPtr;

#endif
