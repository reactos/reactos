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
 * DOS/DJGPP device driver v1.5 for Mesa
 *
 *  Copyright (C) 2002 - Borca Daniel
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



#if HUGE_LOOKUP
/* These lookup tables are used to extract RGB values in [0,255]
 * from 15/16-bit pixel values.
 */
static unsigned char pix15r[0x8000];
static unsigned char pix15g[0x8000];
static unsigned char pix15b[0x8000];
static unsigned char pix16r[0x10000];
static unsigned char pix16g[0x10000];
static unsigned char pix16b[0x10000];
#else
/* lookup table for scaling 5 bit colors up to 8 bits */
static int _rgb_scale_5[32] = {
   0,   8,   16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   197, 206, 214, 222, 230, 239, 247, 255
};
#endif

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

/* FakeColor data */
#define R_CNT 6
#define G_CNT 6
#define B_CNT 6

#define R_BIAS 7
#define G_BIAS 7
#define B_BIAS 7

static word32 VGAPalette[256];
static word8 array_r[256];
static word8 array_g[256];
static word8 array_b[256];



int (*vl_mixfix) (fixed r, fixed g, fixed b);
int (*vl_mixrgb) (const unsigned char rgb[]);
int (*vl_mixrgba) (const unsigned char rgba[]);
void (*vl_getrgba) (unsigned int offset, unsigned char rgba[4]);
int (*vl_getpixel) (unsigned int offset);
void (*vl_clear) (int color);
void (*vl_rect) (int x, int y, int width, int height, int color);
void (*vl_flip) (void);
void (*vl_putpixel) (unsigned int offset, int color);



/* Desc: color composition (w/o ALPHA)
 *
 * In  : R, G, B
 * Out : color
 *
 * Note: -
 */
static int vl_mixfix8fake (fixed r, fixed g, fixed b)
{
 return array_b[b>>FIXED_SHIFT]*G_CNT*R_CNT
      + array_g[g>>FIXED_SHIFT]*R_CNT
      + array_r[r>>FIXED_SHIFT];
}
#define vl_mixfix8 vl_mixfix8fake
static int vl_mixfix15 (fixed r, fixed g, fixed b)
{
 return ((r>>(3+FIXED_SHIFT))<<10)
       |((g>>(3+FIXED_SHIFT))<<5)
       |(b>>(3+FIXED_SHIFT));
}
static int vl_mixfix16 (fixed r, fixed g, fixed b)
{
 return ((r>>(3+FIXED_SHIFT))<<11)
       |((g>>(2+FIXED_SHIFT))<<5)
       |(b>>(3+FIXED_SHIFT));
}
#define vl_mixfix24 vl_mixfix32
static int vl_mixfix32 (fixed r, fixed g, fixed b)
{
 return ((r>>FIXED_SHIFT)<<16)
       |((g>>FIXED_SHIFT)<<8)
       |(b>>FIXED_SHIFT);
}



/* Desc: color composition (w/ ALPHA)
 *
 * In  : array of integers (R, G, B, A)
 * Out : color
 *
 * Note: -
 */
#define vl_mixrgba8 vl_mixrgb8fake
#define vl_mixrgba15 vl_mixrgb15
#define vl_mixrgba16 vl_mixrgb16
#define vl_mixrgba24 vl_mixrgb24
static int vl_mixrgba32 (const unsigned char rgba[])
{
 /* Hack alert:
  * currently, DMesa uses Mesa's alpha buffer;
  * so we don't really care about alpha value here...
  */
 return /*(rgba[3]<<24)|*/(rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}



/* Desc: color composition (w/o ALPHA)
 *
 * In  : array of integers (R, G, B)
 * Out : color
 *
 * Note: -
 */
static int vl_mixrgb8fake (const unsigned char rgb[])
{
 return array_b[rgb[2]]*G_CNT*R_CNT
      + array_g[rgb[1]]*R_CNT
      + array_r[rgb[0]];
}
#define vl_mixrgb8 vl_mixrgb8fake
static int vl_mixrgb15 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<10)|((rgb[1]>>3)<<5)|(rgb[2]>>3);
}
static int vl_mixrgb16 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<11)|((rgb[1]>>2)<<5)|(rgb[2]>>3);
}
#define vl_mixrgb24 vl_mixrgb32
static int vl_mixrgb32 (const unsigned char rgb[])
{
 return (rgb[0]<<16)|(rgb[1]<<8)|(rgb[2]);
}



/* Desc: color decomposition
 *
 * In  : pixel offset, array of integers to hold color components (R, G, B, A)
 * Out : -
 *
 * Note: uses current read buffer
 */
static void v_getrgba8fake6 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
 rgba[0] = _rgb_scale_6[(c >> 16) & 0x3F];
 rgba[1] = _rgb_scale_6[(c >> 8) & 0x3F];
 rgba[2] = _rgb_scale_6[c & 0x3F];
 /*rgba[3] = c >> 24;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}
static void v_getrgba8fake8 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 /*rgba[3] = c >> 24;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}
#define v_getrgba8 v_getrgba8fake6
static void v_getrgba15 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word16 *)vl_current_read_buffer)[offset];
#if HUGE_LOOKUP
 c &= 0x7fff;
 rgba[0] = pix15r[c];
 rgba[1] = pix15g[c];
 rgba[2] = pix15b[c];
#else
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
#endif
 /*rgba[3] = 255;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}
static void v_getrgba16 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word16 *)vl_current_read_buffer)[offset];
#if HUGE_LOOKUP
 rgba[0] = pix16r[c];
 rgba[1] = pix16g[c];
 rgba[2] = pix16b[c];
#else
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
#endif
 /*rgba[3] = 255;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}
static void v_getrgba24 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = *(word32 *)((long)vl_current_read_buffer+offset*3);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 /*rgba[3] = 255;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}
static void v_getrgba32 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word32 *)vl_current_read_buffer)[offset];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c; 
 /*rgba[3] = c >> 24;*/ /* dummy alpha; we have separate SW alpha, so ignore */
}



/* Desc: pixel retrieval
 *
 * In  : pixel offset
 * Out : pixel value
 *
 * Note: uses current read buffer
 */
static int v_getpixel8 (unsigned int offset)
{
 return ((word8 *)vl_current_read_buffer)[offset];
}
#define v_getpixel15 v_getpixel16
static int v_getpixel16 (unsigned int offset)
{
 return ((word16 *)vl_current_read_buffer)[offset];
}
static int v_getpixel24 (unsigned int offset)
{
 return *(word32 *)((long)vl_current_read_buffer+offset*3);
}
static int v_getpixel32 (unsigned int offset)
{
 return ((word32 *)vl_current_read_buffer)[offset];
}



/* Desc: set one palette entry
 *
 * In  : index, R, G, B
 * Out : -
 *
 * Note: color components are in range [0.0 .. 1.0]
 */
void vl_setCI (int index, float red, float green, float blue)
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
static void fake_setcolor (int c, int r, int g, int b)
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
static void fake_buildpalette (int bits)
{
 double c_r, c_g, c_b;
 int r, g, b, color = 0;

 double max = (1 << bits) - 1;

 for (b=0; b<B_CNT; ++b) {
     for (g=0; g<G_CNT; ++g) {
         for (r=0; r<R_CNT; ++r) {
             c_r = 0.5 + (double)r*(max-R_BIAS)/(R_CNT-1.) + R_BIAS;
             c_g = 0.5 + (double)g*(max-G_BIAS)/(G_CNT-1.) + G_BIAS;
             c_b = 0.5 + (double)b*(max-B_BIAS)/(B_CNT-1.) + B_BIAS;
             fake_setcolor(color++, (int)c_r, (int)c_g, (int)c_b);
         }
     }
 }

 for (color=0; color<256; color++) {
     c_r = (double)color*R_CNT/256.;
     c_g = (double)color*G_CNT/256.;
     c_b = (double)color*B_CNT/256.;
     array_r[color] = (int)c_r;
     array_g[color] = (int)c_g;
     array_b[color] = (int)c_b;
 }
}



#if HUGE_LOOKUP
/* Desc: initialize lookup arrays
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
void v_init_pixeltables (void)
{
 unsigned int pixel;

 for (pixel = 0; pixel <= 0xffff; pixel++) {
     unsigned int r, g, b;

     if (pixel <= 0x7fff) {
        /* 15bit */
        r = (pixel & 0x7c00) >> 8;
        g = (pixel & 0x03E0) >> 3;
        b = (pixel & 0x001F) << 2;

        r = (unsigned int)(((double)r * 255. / 0x7c) + 0.5);
        g = (unsigned int)(((double)g * 255. / 0x7c) + 0.5);
        b = (unsigned int)(((double)b * 255. / 0x7c) + 0.5);

        pix15r[pixel] = r;
        pix15g[pixel] = g;
        pix15b[pixel] = b;
     }

     /* 16bit */
     r = (pixel & 0xF800) >> 8;
     g = (pixel & 0x07E0) >> 3;
     b = (pixel & 0x001F) << 3;

     r = (unsigned int)(((double)r * 255. / 0xF8) + 0.5);
     g = (unsigned int)(((double)g * 255. / 0xFC) + 0.5);
     b = (unsigned int)(((double)b * 255. / 0xF8) + 0.5);

     pix16r[pixel] = r;
     pix16g[pixel] = g;
     pix16b[pixel] = b;
 }
}
#endif



/* Desc: initialize hardware
 *
 * In  : -
 * Out : list of available modes
 *
 * Note: when returning non-NULL, global variable `drv' is guaranteed to be ok
 */
static vl_mode *v_init_hw (void)
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
int vl_sync_buffer (void **buffer, int x, int y, int width, int height)
{
 if ((width & 7) || (x < 0) || (y < 0) || (x+width > video_mode->xres) || (y+height > video_mode->yres)) {
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
int vl_get (int pname, int *params)
{
 switch (pname) {
        case VL_GET_SCREEN_SIZE:
             params[0] = video_mode->xres;
             params[1] = video_mode->yres;
             break;
        case VL_GET_VIDEO_MODES:
             {
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
static int vl_setup_mode (vl_mode *p)
{
 if (p == NULL) {
    return -1;
 }

#define INITPTR(bpp) \
        vl_putpixel = v_putpixel##bpp; \
        vl_getrgba = v_getrgba##bpp;   \
        vl_getpixel = v_getpixel##bpp; \
        vl_rect = v_rect##bpp;         \
        vl_mixfix = vl_mixfix##bpp;    \
        vl_mixrgb = vl_mixrgb##bpp;    \
        vl_mixrgba = vl_mixrgba##bpp;  \
        vl_clear = _can_mmx() ? v_clear##bpp##_mmx : v_clear##bpp
        
 switch (p->bpp) {
        case 8:
             INITPTR(8);
             break;
        case 15:
             INITPTR(15);
             break;
        case 16:
             INITPTR(16);
             break;
        case 24:
             INITPTR(24);
             break;
        case 32:
             INITPTR(32);
             break;
        default:
             return -1;
 }

#undef INITPTR

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
void vl_video_exit (void)
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
int vl_video_init (int width, int height, int bpp, int rgb, int refresh)
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
#if HUGE_LOOKUP
 else if (bpp < 24) {
    v_init_pixeltables();
 }
#endif

 /* initialize hardware */
 if ((q = v_init_hw()) == NULL) {
    return 0;
 }

 /* search for a mode that fits our request */
 for (min=-1, p=NULL; q->mode!=0xffff; q++) {
     if ((q->xres>=width) && (q->yres>=height) && (q->bpp==bpp)) {
        if (min>=(unsigned)(q->xres*q->yres)) {
           min = q->xres*q->yres;
           p = q;
        }
     }
 }

 /* setup and enter mode */
 if ((vl_setup_mode(p) == 0) && (drv->entermode(p, refresh) == 0)) {
    vl_flip = drv->blit;
    if (fake) {
       drv->get(VL_GET_CI_PREC, (int *)(&min));
       fake_buildpalette(min);
       if (min == 8) {
          vl_getrgba = v_getrgba8fake8;
       }
    }
    return bpp;
 }

 /* abort */
 return 0;
}
