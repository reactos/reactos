/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef SP_LIMITS_H
#define SP_LIMITS_H



#define SP_MAX_TEXTURE_2D_LEVELS 15  /* 16K x 16K */
#define SP_MAX_TEXTURE_3D_LEVELS 9   /* 512 x 512 x 512 */


/** Max surface size */
#define MAX_WIDTH (1 << (SP_MAX_TEXTURE_2D_LEVELS - 1))
#define MAX_HEIGHT (1 << (SP_MAX_TEXTURE_2D_LEVELS - 1))


#endif /* SP_LIMITS_H */
