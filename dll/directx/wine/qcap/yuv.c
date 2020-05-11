/* DirectShow capture services (QCAP.DLL)
 *
 * Copyright 2005 Maarten Lankhorst
 *
 * This file contains the part of the vfw capture interface that
 * does the actual Video4Linux(1/2) stuff required for capturing
 * and setting/getting media format..
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "wingdi.h"
#include "objbase.h"
#include "strmif.h"
#include "qcap_main.h"
#include "wine/debug.h"

/* This is not used if V4L support is missing */
#if defined(HAVE_LINUX_VIDEODEV_H) || defined(HAVE_LIBV4L1_H)

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

static int yuv_xy[256]; /* Gray value */
static int yuv_gu[256]; /* Green U */
static int yuv_bu[256]; /* Blue  U */
static int yuv_rv[256]; /* Red   V */
static int yuv_gv[256]; /* Green V */
static BOOL initialised = FALSE;

static inline int ValidRange(int in) {
   if (in > 255) in = 255;
   if (in < 0) in = 0;
   return in;
}

typedef struct RGB {
#if 0 /* For some reason I have to revert R and B, not sure why */
  unsigned char r, g, b;
#else
  unsigned char b, g, r;
#endif
} RGB;

static inline void YUV2RGB(const unsigned char y_, const unsigned char cb, const unsigned char cr, RGB* retval) {
   retval->r = ValidRange(yuv_xy[y_] + yuv_rv[cr]);
   retval->g = ValidRange(yuv_xy[y_] + yuv_gu[cb] + yuv_gv[cr]);
   retval->b = ValidRange(yuv_xy[y_] + yuv_bu[cb]);
}

void YUV_Init(void) {
   float y, u, v;
   int y_, cb, cr;

   if (initialised) return;
   initialised = TRUE;

   for (y_ = 0; y_ <= 255; y_++)
   {
      y = ((float) 255 / 219) * (y_ - 16);
      yuv_xy[y_] = y;
   }

   for (cb = 0; cb <= 255; cb++)
   {
      u = ((float) 255 / 224) * (cb - 128);
      yuv_gu[cb] = -0.344 * u;
      yuv_bu[cb] =  1.772 * u;
   }

   for (cr = 0; cr <= 255; cr++)
   {
      v = ((float) 255 / 224) * (cr - 128);
      yuv_rv[cr] =  1.402 * v;
      yuv_gv[cr] = -0.714 * v;
   }
   TRACE("Filled hash table\n");
}

static void Parse_YUYV(unsigned char *destbuffer, const unsigned char *input, int width, int height)
{
   const unsigned char *pY, *pCb, *pCr;
   int togo = width * height / 2;
   pY = input;
   pCb = input+1;
   pCr = input+3;
   while (--togo) {
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; destbuffer += 3;
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; pCb += 4; pCr += 4; destbuffer += 3;
   }
}

static void Parse_UYVY(unsigned char *destbuffer, const unsigned char *input, int width, int height)
{
   const unsigned char *pY, *pCb, *pCr;
   int togo = width * height / 2;
   pY = input+1;
   pCb = input;
   pCr = input+2;
   while (--togo) {
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; destbuffer += 3;
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; pCb += 4; pCr += 4; destbuffer += 3;
   }
}

static void Parse_UYYVYY(unsigned char *destbuffer, const unsigned char *input, int width, int height)
{
   const unsigned char *pY, *pCb, *pCr;
   int togo = width * height / 4;
   pY = input+1;
   pCb = input;
   pCr = input+4;
   while (--togo) {
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      destbuffer += 3; pY++;
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; destbuffer += 3;
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      destbuffer += 3; pY++;
      YUV2RGB(*pY, *pCb, *pCr, (RGB *)destbuffer);
      pY += 2; pCb += 6; pCr += 6; destbuffer += 3;
   }
}

static void Parse_PYUV(unsigned char *destbuffer, const unsigned char *input, int width, int height, int wstep, int hstep)
{
   /* We have 3 pointers, One to Y, one to Cb and 1 to Cr */

/* C19 *89* declaration block (Grr julliard for not allowing C99) */
   int ysize, uvsize;
   const unsigned char *pY, *pCb, *pCr;
   int swstep = 0, shstep = 0;
   int ypos = 0, xpos = 0;
   int indexUV = 0, cUv;
/* End of Grr */

   ysize = width * height;
   uvsize = (width / wstep) * (height / hstep);
   pY = input;
   pCb = pY + ysize;
   pCr = pCb + uvsize;
   /* Bottom down DIB */
   do {
      swstep = 0;
      cUv = indexUV;
      for (xpos = 0; xpos < width; xpos++) {
         YUV2RGB(*(pY++), pCb[cUv], pCr[cUv], (RGB *)destbuffer);
         destbuffer += 3;
         if (++swstep == wstep) {
            cUv++;
            swstep = 0;
         }
      }
      if (++shstep == hstep) {
         shstep = 0;
         indexUV = cUv;
      }
   } while (++ypos < height);
}

void YUV_To_RGB24(enum YUV_Format format, unsigned char *target, const unsigned char *source, int width, int height) {
   int wstep, hstep;
   if (format < ENDPLANAR) {
      switch (format) {
         case YUVP_421: wstep = 2; hstep = 1; break;
         case YUVP_422: wstep = 2; hstep = 2; break;
         case YUVP_441: wstep = 4; hstep = 1; break;
         case YUVP_444: wstep = 4; hstep = 4; break;
         default: ERR("Unhandled format \"%d\"\n", format); return;
      }
      Parse_PYUV(target, source, width, height, wstep, hstep);
   } else {
      switch (format) {
         case YUYV: Parse_YUYV(target, source, width, height); return;
         case UYVY: Parse_UYVY(target, source, width, height); return;
         case UYYVYY: Parse_UYYVYY(target, source, width, height); return;
         default: ERR("Unhandled format \"%d\"\n", format); return;
      }
   }
}
#endif
