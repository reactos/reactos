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
 *
 * Thanks to CrazyPyro (Neil Funk) for FakeColor
 */


#include <stdlib.h>

#include "internal.h"
#include "vesa.h"
#include "vga.h"
#include "null.h"
#include "video.h"


static vl_driver *drv;
/* based upon mode specific data: valid entire session */
int vl_video_selector;
static vl_mode *video_mode;
static int video_scanlen, video_bypp;
/* valid until next buffer */
void *vl_current_draw_buffer, *vl_current_read_buffer;
int vl_current_stride, vl_current_width, vl_current_height, vl_current_bytes;
int vl_current_offset, vl_current_delta;


void (*vl_flip) (void);


/* FakeColor data */
#define R_CNT 6
#define G_CNT 6
#define B_CNT 6

#define R_BIAS 7
#define G_BIAS 7
#define B_BIAS 7

static word32 VGAPalette[256];
word8 array_r[256];
word8 array_g[256];
word8 array_b[256];
word8 tab_16_8[0x10000];


/* lookup table for scaling 5 bit colors up to 8 bits */
static int _rgb_scale_5[32] = {
   0,   8,   16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   197, 206, 214, 222, 230, 239, 247, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static int _rgb_scale_6[64] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  45,  49,  53,  57,  61,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


/* Desc: color composition (w/o ALPHA)
 *
 * In  : array of integers (R, G, B)
 * Out : color
 *
 * Note: -
 */
static int
v_mixrgb8fake (const unsigned char rgb[])
{
   return array_b[rgb[2]]*G_CNT*R_CNT
        + array_g[rgb[1]]*R_CNT
        + array_r[rgb[0]];
}


/* Desc: color decomposition
 *
 * In  : pixel offset, array of integers to hold color components (R, G, B, A)
 * Out : -
 *
 * Note: uses current read buffer
 */
static void
v_getrgb8fake6 (unsigned int offset, unsigned char rgb[])
{
   word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
   rgb[0] = _rgb_scale_6[(c >> 16) & 0x3F];
   rgb[1] = _rgb_scale_6[(c >> 8)  & 0x3F];
   rgb[2] = _rgb_scale_6[ c        & 0x3F];
}
static void
v_getrgb8fake8 (unsigned int offset, unsigned char rgb[])
{
   word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
   rgb[0] = c >> 16;
   rgb[1] = c >> 8;
   rgb[2] = c;
}


/* Desc: create R5G6B5 to FakeColor table lookup
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
init_tab_16_8 (void)
{
    int i;
    for (i = 0; i < 0x10000; i++) {
	unsigned char rgb[3];
	rgb[0] = _rgb_scale_5[(i >> 11) & 0x1F];
	rgb[1] = _rgb_scale_6[(i >>  5) & 0x3F];
	rgb[2] = _rgb_scale_5[ i        & 0x1F];
	tab_16_8[i] = v_mixrgb8fake(rgb);
    }
    (void)v_getrgb8fake6;
    (void)v_getrgb8fake8;
}


/* Desc: set one palette entry
 *
 * In  : index, R, G, B
 * Out : -
 *
 * Note: color components are in range [0.0 .. 1.0]
 */
void
vl_setCI (int index, float red, float green, float blue)
{
   drv->setCI_f(index, red, green, blue);
}


/* Desc: set one palette entry
 *
 * In  : color, R, G, B
 * Out : -
 *
 * Note: -
 */
static void
fake_setcolor (int c, int r, int g, int b)
{
   VGAPalette[c] = 0xff000000 | (r<<16) | (g<<8) | b;

   drv->setCI_i(c, r, g, b);
}


/* Desc: build FakeColor palette
 *
 * In  : CI precision in bits
 * Out : -
 *
 * Note: -
 */
static void
fake_buildpalette (int bits)
{
   double c_r, c_g, c_b;
   int r, g, b, color = 0;

   double max = (1 << bits) - 1;

   for (b = 0; b < B_CNT; ++b) {
      for (g = 0; g < G_CNT; ++g) {
         for (r = 0; r < R_CNT; ++r) {
            c_r = 0.5 + (double)r * (max-R_BIAS) / (R_CNT-1.) + R_BIAS;
            c_g = 0.5 + (double)g * (max-G_BIAS) / (G_CNT-1.) + G_BIAS;
            c_b = 0.5 + (double)b * (max-B_BIAS) / (B_CNT-1.) + B_BIAS;
            fake_setcolor(color++, (int)c_r, (int)c_g, (int)c_b);
         }
      }
   }

   for (color = 0; color < 256; color++) {
      c_r = (double)color * R_CNT / 256.;
      c_g = (double)color * G_CNT / 256.;
      c_b = (double)color * B_CNT / 256.;
      array_r[color] = (int)c_r;
      array_g[color] = (int)c_g;
      array_b[color] = (int)c_b;
   }
}


/* Desc: initialize hardware
 *
 * In  : -
 * Out : list of available modes
 *
 * Note: when returning non-NULL, global variable `drv' is guaranteed to be ok
 */
static vl_mode *
v_init_hw (void)
{
   static vl_mode *q = NULL;

   if (q == NULL) {
      /* are we forced to NUL driver? */
      if (getenv("DMESA_NULDRV")) {
         if ((q = NUL.init()) != NULL) {
            drv = &NUL;
         }
         return q;
      }
      /* initialize hardware */
      if ((q = VESA.init()) != NULL) {
         drv = &VESA;
      } else if ((q = VGA.init()) != NULL) {
         drv = &VGA;
      } else {
         drv = NULL;
      }
   }

   return q;
}


/* Desc: sync buffer with video hardware
 *
 * In  : ptr to old buffer, position, size
 * Out : 0 if success
 *
 * Note: -
 */
int
vl_sync_buffer (void **buffer, int x, int y, int width, int height)
{
   if ((/*XXX*/width & 7) || (x < 0) || (y < 0) || (x+width > video_mode->xres) || (y+height > video_mode->yres)) {
      return -1;
   } else {
      void *newbuf = *buffer;

      if ((newbuf == NULL) || (vl_current_width != width) || (vl_current_height != height)) {
         newbuf = realloc(newbuf, width * height * video_bypp);
      }

      if (newbuf == NULL) {
         return -2;
      }

      vl_current_width = width;
      vl_current_height = height;
      vl_current_stride = vl_current_width * video_bypp;
      vl_current_bytes = vl_current_stride * height;

      vl_current_offset = video_scanlen * y + video_bypp * x;
      vl_current_delta = video_scanlen - vl_current_stride;

      vl_current_draw_buffer = vl_current_read_buffer = *buffer = newbuf;
      return 0;
   }
}


/* Desc: state retrieval
 *
 * In  : name, storage
 * Out : -1 for an error
 *
 * Note: -
 */
int
vl_get (int pname, int *params)
{
   switch (pname) {
      case VL_GET_SCREEN_SIZE:
         params[0] = video_mode->xres;
         params[1] = video_mode->yres;
         break;
      case VL_GET_VIDEO_MODES: {
         int n;
         vl_mode *q;
         if ((q = v_init_hw()) == NULL) {
            return -1;
         }
         /* count available visuals */
         for (n = 0; q->mode != 0xffff; q++) {
            if ((q + 1)->mode == (q->mode | 0x4000)) {
               /* same mode, but linear */
               q++;
            }
            if (params) {
               params[n] = (int)q;
            }
            n++;
         }
         return n;
      }
      default:
         return (drv != NULL) ? drv->get(pname, params) : -1;
   }
   return 0;
}


/* Desc: setup mode
 *
 * In  : ptr to mode definition
 * Out : 0 if success
 *
 * Note: -
 */
static int
vl_setup_mode (vl_mode *p)
{
   if (p == NULL) {
      return -1;
   }

   switch (p->bpp) {
      case 8:
         break;
      case 15:
         break;
      case 16:
         break;
      case 24:
         break;
      case 32:
         break;
      default:
         return -1;
   }

   video_mode = p;
   video_bypp = (p->bpp+7)/8;
   video_scanlen = p->scanlen;
   vl_video_selector = p->sel;

   return 0;
}


/* Desc: restore to the mode prior to first call to `vl_video_init'.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
void
vl_video_exit (void)
{
   drv->restore();
   drv->fini();
   video_mode = NULL;
}


/* Desc: enter mode
 *
 * In  : xres, yres, bits/pixel, RGB, refresh rate
 * Out : pixel width in bits if success
 *
 * Note: -
 */
int
vl_video_init (int width, int height, int bpp, int rgb, int refresh, int fbbits)
{
   int fake;
   vl_mode *p, *q;
   unsigned int min;

   fake = 0;
   if (!rgb) {
      bpp = 8;
   } else if (bpp == 8) {
      fake = 1;
   }

   /* initialize hardware */
   if ((q = v_init_hw()) == NULL) {
      return 0;
   }

   /* search for a mode that fits our request */
   for (min = -1, p = NULL; q->mode != 0xffff; q++) {
      if ((q->xres >= width) && (q->yres >= height) && (q->bpp == bpp)) {
         if (min >= (unsigned)(q->xres * q->yres)) {
            min = q->xres * q->yres;
            p = q;
         }
      }
   }

    /* setup and enter mode */
    if ((vl_setup_mode(p) == 0) && (drv->entermode(p, refresh, fbbits) == 0)) {
	vl_flip = drv->blit;
	if (fake) {
	    drv->get(VL_GET_CI_PREC, (int *)(&min));
	    fake_buildpalette(min);
	    init_tab_16_8();
	}
	return bpp;
    }

   /* abort */
   return 0;
}
