
/*
 * Copyright 2003 Tungsten Graphics, inc.
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
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "t_context.h"
#include "t_vertex.h"
#include "simple_list.h"



/*
 * These functions take the NDC coordinates pointed to by 'in', apply the
 * NDC->Viewport mapping and store the results at 'v'.
 */

static INLINE void insert_4f_viewport_4( const struct tnl_clipspace_attr *a, GLubyte *v,
                      const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
   out[3] = in[3];
}

static INLINE void insert_4f_viewport_3( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
   out[3] = 1;
}

static INLINE void insert_4f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[14];
   out[3] = 1;
}

static INLINE void insert_4f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
   out[2] = vp[14];
   out[3] = 1;
}

static INLINE void insert_3f_viewport_3( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
}

static INLINE void insert_3f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
}

static INLINE void insert_3f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
   out[2] = vp[14];
}

static INLINE void insert_2f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
}

static INLINE void insert_2f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
}


/*
 * These functions do the same as above, except for the viewport mapping.
 */

static INLINE void insert_4f_4( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static INLINE void insert_4f_3( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}

static INLINE void insert_4f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_4f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_3f_xyw_4( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
}

static INLINE void insert_3f_xyw_err( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   (void) a; (void) v; (void) in;
   _mesa_exit(1);
}

static INLINE void insert_3f_3( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}

static INLINE void insert_3f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
}

static INLINE void insert_3f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
}


static INLINE void insert_2f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
}

static INLINE void insert_2f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
}

static INLINE void insert_1f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;

   out[0] = in[0];
}

static INLINE void insert_null( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   (void) a; (void) v; (void) in;
}

static INLINE void insert_4chan_4f_rgba_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[3], in[3]);
}

static INLINE void insert_4chan_4f_rgba_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   c[3] = CHAN_MAX;
}

static INLINE void insert_4chan_4f_rgba_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static INLINE void insert_4chan_4f_rgba_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   c[1] = 0;
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static INLINE void insert_4ub_4f_rgba_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_rgba_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_bgra_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_argb_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_argb_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   v[2] = 0x00;
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_abgr_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   v[2] = 0x00;
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_3ub_3f_rgb_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
}

static INLINE void insert_3ub_3f_rgb_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
}

static INLINE void insert_3ub_3f_rgb_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
}

static INLINE void insert_3ub_3f_bgr_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
}

static INLINE void insert_3ub_3f_bgr_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
}

static INLINE void insert_3ub_3f_bgr_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
}


static INLINE void insert_1ub_1f_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
			   const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
}


/***********************************************************************
 * Functions to perform the reverse operations to the above, for
 * swrast translation and clip-interpolation.
 * 
 * Currently always extracts a full 4 floats.
 */

static void extract_4f_viewport( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   /* Although included for completeness, the position coordinate is
    * usually handled differently during clipping.
    */
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = (in[2] - vp[14]) / vp[10];
   out[3] = in[3];
}

static void extract_3f_viewport( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = (in[2] - vp[14]) / vp[10];
   out[3] = 1;
}


static void extract_2f_viewport( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = 0;
   out[3] = 1;
}


static void extract_4f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v  )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void extract_3f_xyw( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = in[2];
}


static void extract_3f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}


static void extract_2f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static void extract_1f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static void extract_4chan_4f_rgba( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   GLchan *c = (GLchan *)v;
   (void) a;

   out[0] = CHAN_TO_FLOAT(c[0]);
   out[1] = CHAN_TO_FLOAT(c[1]);
   out[2] = CHAN_TO_FLOAT(c[2]);
   out[3] = CHAN_TO_FLOAT(c[3]);
}

static void extract_4ub_4f_rgba( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_bgra( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_argb( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[3] = UBYTE_TO_FLOAT(v[0]);
   out[0] = UBYTE_TO_FLOAT(v[1]);
   out[1] = UBYTE_TO_FLOAT(v[2]);
   out[2] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_abgr( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[3] = UBYTE_TO_FLOAT(v[0]);
   out[2] = UBYTE_TO_FLOAT(v[1]);
   out[1] = UBYTE_TO_FLOAT(v[2]);
   out[0] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_3ub_3f_rgb( const struct tnl_clipspace_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_3ub_3f_bgr( const struct tnl_clipspace_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   (void) a;
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_1ub_1f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}


const struct tnl_format_info _tnl_format_info[EMIT_MAX] = 
{
   { "1f",
     extract_1f,
     { insert_1f_1, insert_1f_1, insert_1f_1, insert_1f_1 },
     sizeof(GLfloat) },

   { "2f",
     extract_2f,
     { insert_2f_1, insert_2f_2, insert_2f_2, insert_2f_2 },
     2 * sizeof(GLfloat) },

   { "3f",
     extract_3f,
     { insert_3f_1, insert_3f_2, insert_3f_3, insert_3f_3 },
     3 * sizeof(GLfloat) },

   { "4f",
     extract_4f,
     { insert_4f_1, insert_4f_2, insert_4f_3, insert_4f_4 },
     4 * sizeof(GLfloat) },

   { "2f_viewport",
     extract_2f_viewport,
     { insert_2f_viewport_1, insert_2f_viewport_2, insert_2f_viewport_2,
       insert_2f_viewport_2 },
     2 * sizeof(GLfloat) },

   { "3f_viewport",
     extract_3f_viewport,
     { insert_3f_viewport_1, insert_3f_viewport_2, insert_3f_viewport_3,
       insert_3f_viewport_3 },
     3 * sizeof(GLfloat) },

   { "4f_viewport",
     extract_4f_viewport,
     { insert_4f_viewport_1, insert_4f_viewport_2, insert_4f_viewport_3,
       insert_4f_viewport_4 }, 
     4 * sizeof(GLfloat) },

   { "3f_xyw",
     extract_3f_xyw,
     { insert_3f_xyw_err, insert_3f_xyw_err, insert_3f_xyw_err, 
       insert_3f_xyw_4 },
     3 * sizeof(GLfloat) },

   { "1ub_1f",
     extract_1ub_1f,
     { insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1 },
     sizeof(GLubyte) },

   { "3ub_3f_rgb",
     extract_3ub_3f_rgb,
     { insert_3ub_3f_rgb_1, insert_3ub_3f_rgb_2, insert_3ub_3f_rgb_3,
       insert_3ub_3f_rgb_3 },
     3 * sizeof(GLubyte) },

   { "3ub_3f_bgr",
     extract_3ub_3f_bgr,
     { insert_3ub_3f_bgr_1, insert_3ub_3f_bgr_2, insert_3ub_3f_bgr_3,
       insert_3ub_3f_bgr_3 },
     3 * sizeof(GLubyte) },

   { "4ub_4f_rgba",
     extract_4ub_4f_rgba,
     { insert_4ub_4f_rgba_1, insert_4ub_4f_rgba_2, insert_4ub_4f_rgba_3, 
       insert_4ub_4f_rgba_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_bgra",
     extract_4ub_4f_bgra,
     { insert_4ub_4f_bgra_1, insert_4ub_4f_bgra_2, insert_4ub_4f_bgra_3,
       insert_4ub_4f_bgra_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_argb",
     extract_4ub_4f_argb,
     { insert_4ub_4f_argb_1, insert_4ub_4f_argb_2, insert_4ub_4f_argb_3,
       insert_4ub_4f_argb_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_abgr",
     extract_4ub_4f_abgr,
     { insert_4ub_4f_abgr_1, insert_4ub_4f_abgr_2, insert_4ub_4f_abgr_3,
       insert_4ub_4f_abgr_4 },
     4 * sizeof(GLubyte) },

   { "4chan_4f_rgba",
     extract_4chan_4f_rgba,
     { insert_4chan_4f_rgba_1, insert_4chan_4f_rgba_2, insert_4chan_4f_rgba_3,
       insert_4chan_4f_rgba_4 },
     4 * sizeof(GLchan) },

   { "pad",
     NULL,
     { NULL, NULL, NULL, NULL },
     0 }

};



    
/***********************************************************************
 * Hardwired fastpaths for emitting whole vertices or groups of
 * vertices
 */
#define EMIT5(NR, F0, F1, F2, F3, F4, NAME)				\
static void NAME( GLcontext *ctx,					\
		  GLuint count,						\
		  GLubyte *v )						\
{									\
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);			\
   struct tnl_clipspace_attr *a = vtx->attr;				\
   GLuint i;								\
									\
   for (i = 0 ; i < count ; i++, v += vtx->vertex_size) {		\
      if (NR > 0) {							\
	 F0( &a[0], v + a[0].vertoffset, (GLfloat *)a[0].inputptr );	\
	 a[0].inputptr += a[0].inputstride;				\
      }									\
      									\
      if (NR > 1) {							\
	 F1( &a[1], v + a[1].vertoffset, (GLfloat *)a[1].inputptr );	\
	 a[1].inputptr += a[1].inputstride;				\
      }									\
      									\
      if (NR > 2) {							\
	 F2( &a[2], v + a[2].vertoffset, (GLfloat *)a[2].inputptr );	\
	 a[2].inputptr += a[2].inputstride;				\
      }									\
      									\
      if (NR > 3) {							\
	 F3( &a[3], v + a[3].vertoffset, (GLfloat *)a[3].inputptr );	\
	 a[3].inputptr += a[3].inputstride;				\
      }									\
									\
      if (NR > 4) {							\
	 F4( &a[4], v + a[4].vertoffset, (GLfloat *)a[4].inputptr );	\
	 a[4].inputptr += a[4].inputstride;				\
      }									\
   }									\
}

   
#define EMIT2(F0, F1, NAME) EMIT5(2, F0, F1, insert_null, \
				  insert_null, insert_null, NAME)

#define EMIT3(F0, F1, F2, NAME) EMIT5(3, F0, F1, F2, insert_null, \
				      insert_null, NAME)
   
#define EMIT4(F0, F1, F2, F3, NAME) EMIT5(4, F0, F1, F2, F3, \
				          insert_null, NAME)
   

EMIT2(insert_3f_viewport_3, insert_4ub_4f_rgba_4, emit_viewport3_rgba4)
EMIT2(insert_3f_viewport_3, insert_4ub_4f_bgra_4, emit_viewport3_bgra4)
EMIT2(insert_3f_3, insert_4ub_4f_rgba_4, emit_xyz3_rgba4)

EMIT3(insert_4f_viewport_4, insert_4ub_4f_rgba_4, insert_2f_2, emit_viewport4_rgba4_st2)
EMIT3(insert_4f_viewport_4, insert_4ub_4f_bgra_4, insert_2f_2,  emit_viewport4_bgra4_st2)
EMIT3(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, emit_xyzw4_rgba4_st2)

EMIT4(insert_4f_viewport_4, insert_4ub_4f_rgba_4, insert_2f_2, insert_2f_2, emit_viewport4_rgba4_st2_st2)
EMIT4(insert_4f_viewport_4, insert_4ub_4f_bgra_4, insert_2f_2, insert_2f_2,  emit_viewport4_bgra4_st2_st2)
EMIT4(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, insert_2f_2, emit_xyzw4_rgba4_st2_st2)


/* Use the codegen paths to select one of a number of hardwired
 * fastpaths.
 */
void _tnl_generate_hardwired_emit( GLcontext *ctx )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   tnl_emit_func func = NULL;

   /* Does it fit a hardwired fastpath?  Help! this is growing out of
    * control!
    */
   switch (vtx->attr_count) {
   case 2:
      if (vtx->attr[0].emit == insert_3f_viewport_3) {
	 if (vtx->attr[1].emit == insert_4ub_4f_bgra_4) 
	    func = emit_viewport3_bgra4;
	 else if (vtx->attr[1].emit == insert_4ub_4f_rgba_4) 
	    func = emit_viewport3_rgba4;
      }
      else if (vtx->attr[0].emit == insert_3f_3 &&
	       vtx->attr[1].emit == insert_4ub_4f_rgba_4) {
 	 func = emit_xyz3_rgba4; 
      }
      break;
   case 3:
      if (vtx->attr[2].emit == insert_2f_2) {
	 if (vtx->attr[1].emit == insert_4ub_4f_rgba_4) {
	    if (vtx->attr[0].emit == insert_4f_viewport_4)
	       func = emit_viewport4_rgba4_st2;
	    else if (vtx->attr[0].emit == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2;
	 }
	 else if (vtx->attr[1].emit == insert_4ub_4f_bgra_4 &&
		  vtx->attr[0].emit == insert_4f_viewport_4)
	    func = emit_viewport4_bgra4_st2;
      }
      break;
   case 4:
      if (vtx->attr[2].emit == insert_2f_2 &&
	  vtx->attr[3].emit == insert_2f_2) {
	 if (vtx->attr[1].emit == insert_4ub_4f_rgba_4) {
	    if (vtx->attr[0].emit == insert_4f_viewport_4)
	       func = emit_viewport4_rgba4_st2_st2;
	    else if (vtx->attr[0].emit == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2_st2;
	 }
	 else if (vtx->attr[1].emit == insert_4ub_4f_bgra_4 &&
		  vtx->attr[0].emit == insert_4f_viewport_4)
	    func = emit_viewport4_bgra4_st2_st2;
      }
      break;
   }

   vtx->emit = func;
}

/***********************************************************************
 * Generic (non-codegen) functions for whole vertices or groups of
 * vertices
 */

void _tnl_generic_emit( GLcontext *ctx,
			GLuint count,
			GLubyte *v )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   const GLuint stride = vtx->vertex_size;
   GLuint i, j;

   for (i = 0 ; i < count ; i++, v += stride) {
      for (j = 0; j < attr_count; j++) {
	 GLfloat *in = (GLfloat *)a[j].inputptr;
	 a[j].inputptr += a[j].inputstride;
	 a[j].emit( &a[j], v + a[j].vertoffset, in );
      }
   }
}


void _tnl_generic_interp( GLcontext *ctx,
			    GLfloat t,
			    GLuint edst, GLuint eout, GLuint ein,
			    GLboolean force_boundary )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   const GLubyte *vin  = vtx->vertex_buf + ein  * vtx->vertex_size;
   const GLubyte *vout = vtx->vertex_buf + eout * vtx->vertex_size;
   GLubyte *vdst = vtx->vertex_buf + edst * vtx->vertex_size;
   const struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   GLuint j;
   (void) force_boundary;

   if (tnl->NeedNdcCoords) {
      const GLfloat *dstclip = VB->ClipPtr->data[edst];
      if (dstclip[3] != 0.0) {
	 const GLfloat w = 1.0f / dstclip[3];
	 GLfloat pos[4];

	 pos[0] = dstclip[0] * w;
	 pos[1] = dstclip[1] * w;
	 pos[2] = dstclip[2] * w;
	 pos[3] = w;

	 a[0].insert[4-1]( &a[0], vdst, pos );
      }
   }
   else {
      a[0].insert[4-1]( &a[0], vdst, VB->ClipPtr->data[edst] );
   }


   for (j = 1; j < attr_count; j++) {
      GLfloat fin[4], fout[4], fdst[4];
	 
      a[j].extract( &a[j], fin, vin + a[j].vertoffset );
      a[j].extract( &a[j], fout, vout + a[j].vertoffset );

      INTERP_F( t, fdst[3], fout[3], fin[3] );
      INTERP_F( t, fdst[2], fout[2], fin[2] );
      INTERP_F( t, fdst[1], fout[1], fin[1] );
      INTERP_F( t, fdst[0], fout[0], fin[0] );

      a[j].insert[4-1]( &a[j], vdst + a[j].vertoffset, fdst );
   }
}


/* Extract color attributes from one vertex and insert them into
 * another.  (Shortcircuit extract/insert with memcpy).
 */
void _tnl_generic_copy_pv( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   GLubyte *vsrc = vtx->vertex_buf + esrc * vtx->vertex_size;
   GLubyte *vdst = vtx->vertex_buf + edst * vtx->vertex_size;
   const struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   GLuint j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attrib == VERT_ATTRIB_COLOR0 ||
	  a[j].attrib == VERT_ATTRIB_COLOR1) {

	 _mesa_memcpy( vdst + a[j].vertoffset,
                       vsrc + a[j].vertoffset,
                       a[j].vertattrsize );
      }
   }
}


/* Helper functions for hardware which doesn't put back colors and/or
 * edgeflags into vertices.
 */
void _tnl_generic_interp_extras( GLcontext *ctx,
				   GLfloat t,
				   GLuint dst, GLuint out, GLuint in,
				   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /* If stride is zero, ColorPtr[1] is constant across the VB, so
    * there is no point interpolating between two values as they will
    * be identical.  In all other cases, this value is generated by
    * t_vb_lighttmp.h and has a stride of 4 dwords.
    */
   if (VB->ColorPtr[1] && VB->ColorPtr[1]->stride) {
      assert(VB->ColorPtr[1]->stride == 4 * sizeof(GLfloat));

      INTERP_4F( t,
		 VB->ColorPtr[1]->data[dst],
		 VB->ColorPtr[1]->data[out],
		 VB->ColorPtr[1]->data[in] );
   }

   if (VB->SecondaryColorPtr[1]) {
      assert(VB->SecondaryColorPtr[1]->stride == 4 * sizeof(GLfloat));
      
      INTERP_3F( t,
		 VB->SecondaryColorPtr[1]->data[dst],
		 VB->SecondaryColorPtr[1]->data[out],
		 VB->SecondaryColorPtr[1]->data[in] );
   }
   
   if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst][0] = LINTERP( t,
					       VB->IndexPtr[1]->data[out][0],
					       VB->IndexPtr[1]->data[in][0] );
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   _tnl_generic_interp(ctx, t, dst, out, in, force_boundary);
}

void _tnl_generic_copy_pv_extras( GLcontext *ctx, 
				  GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /* See above comment:
    */
   if (VB->ColorPtr[1] && VB->ColorPtr[1]->stride) {
      COPY_4FV( VB->ColorPtr[1]->data[dst], 
		VB->ColorPtr[1]->data[src] );
   }

   if (VB->SecondaryColorPtr[1]) {
      COPY_4FV( VB->SecondaryColorPtr[1]->data[dst], 
		VB->SecondaryColorPtr[1]->data[src] );
   }

   if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst][0] = VB->IndexPtr[1]->data[src][0];
   }

   _tnl_generic_copy_pv(ctx, dst, src);
}


