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


/* Build and manage clipspace/ndc/window vertices.
 *
 * Another new mechanism designed and crying out for codegen.  Before
 * that, it would be very interesting to investigate the merger of
 * these vertices and those built in t_vtx_*.
 */




#define GET_VERTEX_STATE(ctx)  &(TNL_CONTEXT(ctx)->clipspace)

static void insert_4f_viewport_4( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
   out[3] = in[3];
}

static void insert_4f_viewport_3( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
   out[3] = 1;
}

static void insert_4f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[14];
   out[3] = 1;
}

static void insert_4f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
   out[2] = vp[14];
   out[3] = 1;
}

static void insert_3f_viewport_3( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
}

static void insert_3f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
}

static void insert_3f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
   out[2] = vp[14];
}

static void insert_2f_viewport_2( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
}

static void insert_2f_viewport_1( const struct tnl_clipspace_attr *a, GLubyte *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[13];
}


static void insert_4f_4( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void insert_4f_3( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}

static void insert_4f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static void insert_4f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static void insert_3f_xyw_4( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
}

static void insert_3f_xyw_err( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   abort();
}

static void insert_3f_3( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}

static void insert_3f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
}

static void insert_3f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
}


static void insert_2f_2( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
}

static void insert_2f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = 0;
}

static void insert_1f_1( const struct tnl_clipspace_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
}

static void insert_4chan_4f_rgba_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[3], in[3]);
}

static void insert_4chan_4f_rgba_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   c[3] = CHAN_MAX;
}

static void insert_4chan_4f_rgba_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static void insert_4chan_4f_rgba_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				  const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   c[1] = 0;
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static void insert_4ub_4f_rgba_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void insert_4ub_4f_rgba_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   v[3] = 0xff;
}

static void insert_4ub_4f_rgba_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
   v[3] = 0xff;
}

static void insert_4ub_4f_rgba_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
   v[3] = 0xff;
}

static void insert_4ub_4f_bgra_4( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void insert_4ub_4f_bgra_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   v[3] = 0xff;
}

static void insert_4ub_4f_bgra_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
   v[3] = 0xff;
}

static void insert_4ub_4f_bgra_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
   v[3] = 0xff;
}

static void insert_3ub_3f_rgb_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
}

static void insert_3ub_3f_rgb_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
}

static void insert_3ub_3f_rgb_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
			       const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
}

static void insert_3ub_3f_bgr_3( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
}

static void insert_3ub_3f_bgr_2( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
}

static void insert_3ub_3f_bgr_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
				 const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
}


static void insert_1ub_1f_1( const struct tnl_clipspace_attr *a, GLubyte *v, 
			   const GLfloat *in )
{
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
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void extract_3f_xyw( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = in[2];
}


static void extract_3f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}


static void extract_2f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static void extract_1f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static void extract_4chan_4f_rgba( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   GLchan *c = (GLchan *)v;

   out[0] = CHAN_TO_FLOAT(c[0]);
   out[1] = CHAN_TO_FLOAT(c[1]);
   out[2] = CHAN_TO_FLOAT(c[2]);
   out[3] = CHAN_TO_FLOAT(c[3]);
}

static void extract_4ub_4f_rgba( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_bgra( const struct tnl_clipspace_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_3ub_3f_rgb( const struct tnl_clipspace_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_3ub_3f_bgr( const struct tnl_clipspace_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_1ub_1f( const struct tnl_clipspace_attr *a, GLfloat *out, const GLubyte *v )
{
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}


struct {
   const char *name;
   extract_func extract;
   insert_func insert[4];
   GLuint attrsize;
} format_info[EMIT_MAX] = {

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

   { "4chan_4f_rgba",
     extract_4chan_4f_rgba,
     { insert_4chan_4f_rgba_1, insert_4chan_4f_rgba_2, insert_4chan_4f_rgba_3,
       insert_4chan_4f_rgba_4 },
     4 * sizeof(GLchan) }

};
     

/***********************************************************************
 * Generic (non-codegen) functions for whole vertices or groups of
 * vertices
 */

static void generic_emit( GLcontext *ctx,
			  GLuint start, GLuint end,
			  void *dest )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   GLubyte *v = (GLubyte *)dest;
   GLuint i, j;
   GLuint count = vtx->attr_count;
   GLuint stride;

   for (j = 0; j < count; j++) {
      GLvector4f *vptr = VB->AttribPtr[a[j].attrib];
      a[j].inputstride = vptr->stride;
      a[j].inputptr = ((GLubyte *)vptr->data) + start * vptr->stride;
      a[j].emit = a[j].insert[vptr->size - 1];
   }

   end -= start;
   stride = vtx->vertex_size;

   for (i = 0 ; i < end ; i++, v += stride) {
      for (j = 0; j < count; j++) {
	 GLfloat *in = (GLfloat *)a[j].inputptr;
	 a[j].inputptr += a[j].inputstride;
	 a[j].emit( &a[j], v + a[j].vertoffset, in );
      }
   }
}


static void generic_interp( GLcontext *ctx,
			    GLfloat t,
			    GLuint edst, GLuint eout, GLuint ein,
			    GLboolean force_boundary )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   GLubyte *vin  = vtx->vertex_buf + ein  * vtx->vertex_size;
   GLubyte *vout = vtx->vertex_buf + eout * vtx->vertex_size;
   GLubyte *vdst = vtx->vertex_buf + edst * vtx->vertex_size;
   const struct tnl_clipspace_attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;

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
static void generic_copy_pv( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   GLubyte *vsrc = vtx->vertex_buf + esrc * vtx->vertex_size;
   GLubyte *vdst = vtx->vertex_buf + edst * vtx->vertex_size;
   const struct tnl_clipspace_attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;

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
static void generic_interp_extras( GLcontext *ctx,
				   GLfloat t,
				   GLuint dst, GLuint out, GLuint in,
				   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      assert(VB->ColorPtr[1]->stride == 4 * sizeof(GLfloat));

      INTERP_4F( t,
		 VB->ColorPtr[1]->data[dst],
		 VB->ColorPtr[1]->data[out],
		 VB->ColorPtr[1]->data[in] );

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3F( t,
		    VB->SecondaryColorPtr[1]->data[dst],
		    VB->SecondaryColorPtr[1]->data[out],
		    VB->SecondaryColorPtr[1]->data[in] );
      }
   }
   else if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst][0] = LINTERP( t,
					       VB->IndexPtr[1]->data[out][0],
					       VB->IndexPtr[1]->data[in][0] );
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   generic_interp(ctx, t, dst, out, in, force_boundary);
}

static void generic_copy_pv_extras( GLcontext *ctx, 
				    GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      COPY_4FV( VB->ColorPtr[1]->data[dst], 
		VB->ColorPtr[1]->data[src] );

      if (VB->SecondaryColorPtr[1]) {
	 COPY_4FV( VB->SecondaryColorPtr[1]->data[dst], 
		   VB->SecondaryColorPtr[1]->data[src] );
      }
   }
   else if (VB->IndexPtr[1]) {
      VB->IndexPtr[1]->data[dst][0] = VB->IndexPtr[1]->data[src][0];
   }

   generic_copy_pv(ctx, dst, src);
}






/***********************************************************************
 * Build codegen functions or return generic ones:
 */


static void choose_emit_func( GLcontext *ctx,
			      GLuint start, GLuint end,
			      void *dest )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->emit = generic_emit;
   vtx->emit( ctx, start, end, dest );
}


static void choose_interp_func( GLcontext *ctx,
				GLfloat t,
				GLuint edst, GLuint eout, GLuint ein,
				GLboolean force_boundary )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   if (vtx->need_extras && 
       (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      vtx->interp = generic_interp_extras;
   } else {
      vtx->interp = generic_interp;
   }

   vtx->interp( ctx, t, edst, eout, ein, force_boundary );
}


static void choose_copy_pv_func(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   if (vtx->need_extras && 
       (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      vtx->copy_pv = generic_copy_pv_extras;
   } else {
      vtx->copy_pv = generic_copy_pv;
   }

   vtx->copy_pv( ctx, edst, esrc );
}


/***********************************************************************
 * Public entrypoints, mostly dispatch to the above:
 */


/* Interpolate between two vertices to produce a third:
 */
void _tnl_interp( GLcontext *ctx,
		  GLfloat t,
		  GLuint edst, GLuint eout, GLuint ein,
		  GLboolean force_boundary )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->interp( ctx, t, edst, eout, ein, force_boundary );
}

/* Copy colors from one vertex to another:
 */
void _tnl_copy_pv(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->copy_pv( ctx, edst, esrc );
}


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied:
 */
void _tnl_get_attr( GLcontext *ctx, const void *vin,
			      GLenum attr, GLfloat *dest )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   const struct tnl_clipspace_attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attrib == (int)attr) {
	 a[j].extract( &a[j], dest, (GLubyte *)vin + a[j].vertoffset );
	 return;
      }
   }

   /* Else return the value from ctx->Current
    */
   _mesa_memcpy( dest, ctx->Current.Attrib[attr], 4*sizeof(GLfloat));
}

void *_tnl_get_vertex( GLcontext *ctx, GLuint nr )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   return vtx->vertex_buf + nr * vtx->vertex_size;
}

void _tnl_invalidate_vertex_state( GLcontext *ctx, GLuint new_state )
{
   if (new_state & (_DD_NEW_TRI_LIGHT_TWOSIDE|_DD_NEW_TRI_UNFILLED) ) {
      struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
      vtx->new_inputs = ~0;
      vtx->interp = choose_interp_func;
      vtx->copy_pv = choose_copy_pv_func;
   }
}


GLuint _tnl_install_attrs( GLcontext *ctx, const struct tnl_attr_map *map,
			   GLuint nr, const GLfloat *vp, 
			   GLuint unpacked_size )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   int offset = 0;
   GLuint i;

   assert(nr < _TNL_ATTRIB_MAX);
   assert(nr == 0 || map[0].attrib == VERT_ATTRIB_POS);

   vtx->attr_count = nr;
   vtx->emit = choose_emit_func;
   vtx->interp = choose_interp_func;
   vtx->copy_pv = choose_copy_pv_func;
   vtx->new_inputs = ~0;

   for (i = 0; i < nr; i++) {
      GLuint format = map[i].format;
      vtx->attr[i].attrib = map[i].attrib;
      vtx->attr[i].vp = vp;
      vtx->attr[i].insert = format_info[format].insert;
      vtx->attr[i].extract = format_info[format].extract;
      vtx->attr[i].vertattrsize = format_info[format].attrsize;

      if (unpacked_size) 
	 vtx->attr[i].vertoffset = map[i].offset;
      else
	 vtx->attr[i].vertoffset = offset;

/*       fprintf(stderr, "%d: %s offset %d\n", i, */
/* 	      format_info[format].name, vtx->attr[i].vertoffset); */

      offset += format_info[format].attrsize;
   }

   if (unpacked_size)
      vtx->vertex_size = unpacked_size;
   else
      vtx->vertex_size = offset;

   assert(vtx->vertex_size <= vtx->max_vertex_size);
   
   return vtx->vertex_size;
}



void _tnl_invalidate_vertices( GLcontext *ctx, GLuint newinputs )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->new_inputs |= newinputs;
}



void _tnl_build_vertices( GLcontext *ctx,
		       GLuint start,
		       GLuint count,
		       GLuint newinputs )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   GLuint stride = vtx->vertex_size;
   GLubyte *v = ((GLubyte *)vtx->vertex_buf + (start*stride));

   newinputs |= vtx->new_inputs;
   vtx->new_inputs = 0;

   if (newinputs)
      vtx->emit( ctx, start, count, v );
}


void *_tnl_emit_vertices_to_buffer( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   void *dest )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->emit( ctx, start, count, dest );
   return (void *)((GLubyte *)dest + vtx->vertex_size * (count - start));
}


void _tnl_init_vertices( GLcontext *ctx, 
			GLuint vb_size,
			GLuint max_vertex_size )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);  

   _tnl_install_attrs( ctx, 0, 0, 0, 0 );

   vtx->need_extras = GL_TRUE;
   if (max_vertex_size > vtx->max_vertex_size) {
      _tnl_free_vertices( ctx );
      vtx->max_vertex_size = max_vertex_size;
      vtx->vertex_buf = (GLubyte *)ALIGN_CALLOC(vb_size * max_vertex_size, 32 );
   }
}


void _tnl_free_vertices( GLcontext *ctx )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   if (vtx->vertex_buf) {
      ALIGN_FREE(vtx->vertex_buf);
      vtx->vertex_buf = 0;
   }
}
