/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef VIDEO_H_included
#define VIDEO_H_included

typedef int fixed;

#define VL_GET_CARD_NAME   0x0100
#define VL_GET_VRAM        0x0101
#define VL_GET_CI_PREC     0x0200
#define VL_GET_HPIXELS     0x0201
#define VL_GET_SCREEN_SIZE 0x0202
#define VL_GET_VIDEO_MODES 0x0300

extern void (*vl_flip) (void);

void vl_setCI (int index, float red, float green, float blue);

int vl_sync_buffer (void **buffer, int x, int y, int width, int height);
int vl_get (int pname, int *params);

void vl_video_exit (void);
int vl_video_init (int width, int height, int bpp, int rgb, int refresh, int fbbits);

#endif
