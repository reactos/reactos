/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file spantmp_common.h
 *
 * common macros for span read / write functions to be used in the depth,
 * stencil and pixel span templates.
 */

#ifndef HW_WRITE_LOCK
#define HW_WRITE_LOCK()		HW_LOCK()
#endif

#ifndef HW_WRITE_UNLOCK
#define HW_WRITE_UNLOCK()	HW_UNLOCK()
#endif

#ifndef HW_READ_LOCK
#define HW_READ_LOCK()		HW_LOCK()
#endif

#ifndef HW_READ_UNLOCK
#define HW_READ_UNLOCK()	HW_UNLOCK()
#endif

#ifndef HW_CLIPLOOP
#define HW_CLIPLOOP()							\
   do {									\
      int _nc = dPriv->numClipRects;					\
      while ( _nc-- ) {							\
	 int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
	 int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;		\
	 int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
	 int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;
#endif

#ifndef HW_ENDCLIPLOOP
#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)
#endif

#ifndef CLIPPIXEL
#define CLIPPIXEL( _x, _y )						\
   ((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))
#endif

#ifndef CLIPSPAN
#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
   if ( _y < miny || _y >= maxy /*|| _x + n < minx || _x >=maxx*/ ) {	\
      _n1 = 0, _x1 = x;							\
   } else {								\
      _n1 = _n;								\
      _x1 = _x;								\
      if ( _x1 < minx ) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx; \
      if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + n1 - maxx);		        \
   }
#endif
