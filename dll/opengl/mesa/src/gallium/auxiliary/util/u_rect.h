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


#ifndef U_RECT_H
#define U_RECT_H

#include "pipe/p_compiler.h"

struct u_rect {
   int x0, x1;
   int y0, y1;
};

/* Do two rectangles intersect?
 */
static INLINE boolean
u_rect_test_intersection(const struct u_rect *a,
                         const struct u_rect *b)
{
   return (!(a->x1 < b->x0 ||
             b->x1 < a->x0 ||
             a->y1 < b->y0 ||
             b->y1 < a->y0));
}

/* Find the intersection of two rectangles known to intersect.
 */
static INLINE void
u_rect_find_intersection(const struct u_rect *a,
                         struct u_rect *b)
{
   /* Caller should verify intersection exists before calling.
    */
   if (b->x0 < a->x0) b->x0 = a->x0;
   if (b->x1 > a->x1) b->x1 = a->x1;
   if (b->y0 < a->y0) b->y0 = a->y0;
   if (b->y1 > a->y1) b->y1 = a->y1;
}


static INLINE void
u_rect_possible_intersection(const struct u_rect *a,
                             struct u_rect *b)
{
   if (u_rect_test_intersection(a,b)) {
      u_rect_find_intersection(a,b);
   }
   else {
      b->x0 = b->x1 = b->y0 = b->y1 = 0;
   }
}

#include "pipe/p_format.h"
#include "util/u_pack_color.h"



/**********************************************************************
 * Pipe copy/fill rect helpers.
 */

/* These really should move to a different file:
 */
#include "pipe/p_format.h"

extern void
util_copy_rect(ubyte * dst, enum pipe_format format,
               unsigned dst_stride, unsigned dst_x, unsigned dst_y,
               unsigned width, unsigned height, const ubyte * src,
               int src_stride, unsigned src_x, unsigned src_y);

extern void
util_fill_rect(ubyte * dst, enum pipe_format format,
               unsigned dst_stride, unsigned dst_x, unsigned dst_y,
               unsigned width, unsigned height, union util_color *uc);


#endif /* U_RECT_H */
