/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#ifndef vl_csc_h
#define vl_csc_h

#include "pipe/p_compiler.h"

struct vl_procamp
{
   float brightness;
   float contrast;
   float saturation;
   float hue;
};

enum VL_CSC_COLOR_STANDARD
{
   VL_CSC_COLOR_STANDARD_IDENTITY,
   VL_CSC_COLOR_STANDARD_BT_601,
   VL_CSC_COLOR_STANDARD_BT_709,
   VL_CSC_COLOR_STANDARD_SMPTE_240M
};

extern const struct vl_procamp vl_default_procamp;

void vl_csc_get_matrix(enum VL_CSC_COLOR_STANDARD cs,
                       struct vl_procamp *procamp,
                       bool full_range,
                       float *matrix);

#endif /* vl_csc_h */
