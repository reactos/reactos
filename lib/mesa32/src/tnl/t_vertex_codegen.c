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

/* Another codegen scheme, hopefully portable to a few different
 * architectures without too much work.
 */




static GLboolean emit_4f_viewport_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13)) &&
      p->emit_mad(p, out(2), vp(10), in(2), vp(14)) &&
      p->emit_mov(p, out(3), in(3));
}

static GLboolean emit_4f_viewport_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13)) &&
      p->emit_mad(p, out(2), vp(10), in(2), vp(14)) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_4f_viewport_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13)) &&
      p->emit_mov(p, out(2), vp(14)) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_4f_viewport_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mov(p, out(1), vp(13)) &&
      p->emit_mov(p, out(2), vp(14)) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_3f_viewport_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13)) &&
      p->emit_mad(p, out(2), vp(10), in(2), vp(14));
}

static GLboolean emit_3f_viewport_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13)) &&
      p->emit_mov(p, out(2), vp(14));
}

static GLboolean emit_3f_viewport_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mov(p, out(1), vp(13)) &&
      p->emit_mov(p, out(2), vp(14));
}

static GLboolean emit_2f_viewport_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12)) &&
      p->emit_mad(p, out(1), vp(5), in(1), vp(13));
}

static GLboolean emit_2f_viewport_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mad(p, out(0), vp(0), in(0), vp(12));
}


static GLboolean emit_4f_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0), in(0)) &&
      p->emit_mov(p, out(1), in(1)) &&
      p->emit_mov(p, out(2), in(2)) &&
      p->emit_mov(p, out(3), in(3));
}

static GLboolean emit_4f_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_mov(p, out(1),  in(1)) &&
      p->emit_mov(p, out(2),  in(2)) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_4f_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_mov(p, out(1),  in(1)) &&
      p->emit_const(p, out(2), 0.0) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_4f_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_const(p, out(1), 0.0) &&
      p->emit_const(p, out(2), 0.0) &&
      p->emit_const(p, out(3), 1.0);
}

static GLboolean emit_3f_xyw_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0), in(0)) &&
      p->emit_mov(p, out(1), in(1)) &&
      p->emit_mov(p, out(2), in(3));
}

static GLboolean emit_3f_xyw_err( struct tnl_clipspace_codegen *p )
{
   (void) p;
   assert(0);
   return GL_FALSE;
}

static GLboolean emit_3f_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0), in(0)) &&
      p->emit_mov(p, out(1), in(1)) &&
      p->emit_mov(p, out(2), in(2));
}

static GLboolean emit_3f_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),   in(0)) &&
      p->emit_mov(p, out(1),   in(1)) &&
      p->emit_const(p, out(2), 0.0);
}

static GLboolean emit_3f_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_const(p, out(1),  0.0) &&
      p->emit_const(p, out(2),  0.0);
}


static GLboolean emit_2f_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_mov(p, out(1),  in(1));
}

static GLboolean emit_2f_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0)) &&
      p->emit_const(p, out(1),  0.0);
}

static GLboolean emit_1f_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_mov(p, out(0),  in(0));
}

static GLboolean emit_4chan_4f_rgba_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_chan(p, out(0), in(0)) && 
      p->emit_float_to_chan(p, out(1), in(1)) && 
      p->emit_float_to_chan(p, out(2), in(2)) && 
      p->emit_float_to_chan(p, out(3), in(3));
}

static GLboolean emit_4chan_4f_rgba_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_chan(p, out(0), in(0)) && 
      p->emit_float_to_chan(p, out(1), in(1)) && 
      p->emit_float_to_chan(p, out(2), in(2)) && 
      p->emit_const_chan(p, out(3), CHAN_MAX);
}

static GLboolean emit_4chan_4f_rgba_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_chan(p, out(0), in(0)) && 
      p->emit_float_to_chan(p, out(1), in(1)) && 
      p->emit_const_chan(p, out(2), 0) &&
      p->emit_const_chan(p, out(3), CHAN_MAX);
}

static GLboolean emit_4chan_4f_rgba_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_chan(p, out(0), in(0)) && 
      p->emit_const_chan(p, out(1), 0) &&
      p->emit_const_chan(p, out(2), 0) &&  
      p->emit_const_chan(p, out(3), CHAN_MAX);
}

static GLboolean emit_4ub_4f_rgba_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(2), in(2)) &&
      p->emit_float_to_ubyte(p, out(3), in(3));
}

static GLboolean emit_4ub_4f_rgba_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(2), in(2)) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_4ub_4f_rgba_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_const_ubyte(p, out(2), 0) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_4ub_4f_rgba_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_const_ubyte(p, out(1), 0) &&
      p->emit_const_ubyte(p, out(2), 0) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_4ub_4f_bgra_4( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(0), in(2)) &&
      p->emit_float_to_ubyte(p, out(3), in(3));
}

static GLboolean emit_4ub_4f_bgra_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(0), in(2)) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_4ub_4f_bgra_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_const_ubyte(p, out(0), 0) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_4ub_4f_bgra_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_const_ubyte(p, out(1), 0) &&
      p->emit_const_ubyte(p, out(0), 0) &&
      p->emit_const_ubyte(p, out(3), 0xff);
}

static GLboolean emit_3ub_3f_rgb_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(2), in(2));
}

static GLboolean emit_3ub_3f_rgb_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_const_ubyte(p, out(2), 0);
}

static GLboolean emit_3ub_3f_rgb_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0)) &&
      p->emit_const_ubyte(p, out(1), 0) &&
      p->emit_const_ubyte(p, out(2), 0);
}

static GLboolean emit_3ub_3f_bgr_3( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_float_to_ubyte(p, out(0), in(2));
}

static GLboolean emit_3ub_3f_bgr_2( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_float_to_ubyte(p, out(1), in(1)) &&
      p->emit_const_ubyte(p, out(0), 0);
}

static GLboolean emit_3ub_3f_bgr_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(2), in(0)) &&
      p->emit_const_ubyte(p, out(1), 0) &&
      p->emit_const_ubyte(p, out(0), 0);
}


static GLboolean emit_1ub_1f_1( struct tnl_clipspace_codegen *p )
{
   return 
      p->emit_float_to_ubyte(p, out(0), in(0));
}




static struct {
   const char *name;
   GLenum out_type;
   GLboolean need_vp;
   GLboolean (*emit[4])( struct tnl_clipspace_codegen * );
} emit_info[EMIT_MAX] = {

   { "1f", GL_FLOAT, GL_FALSE,
     { emit_1f_1, emit_1f_1, emit_1f_1, emit_1f_1 } },

   { "2f", GL_FLOAT, GL_FALSE,
     { emit_2f_1, emit_2f_2, emit_2f_2, emit_2f_2 } },

   { "3f", GL_FLOAT, GL_FALSE,
     { emit_3f_1, emit_3f_2, emit_3f_3, emit_3f_3 } },

   { "4f", GL_FLOAT, GL_FALSE,
     { emit_4f_1, emit_4f_2, emit_4f_3, emit_4f_4 } },

   { "2f_viewport", GL_FLOAT, GL_TRUE,
     { emit_2f_viewport_1, emit_2f_viewport_2, emit_2f_viewport_2,
       emit_2f_viewport_2 } },

   { "3f_viewport", GL_FLOAT, GL_TRUE,
     { emit_3f_viewport_1, emit_3f_viewport_2, emit_3f_viewport_3,
       emit_3f_viewport_3 } },

   { "4f_viewport", GL_FLOAT, GL_TRUE,
     { emit_4f_viewport_1, emit_4f_viewport_2, emit_4f_viewport_3,
       emit_4f_viewport_4 } },

   { "3f_xyw", GL_FLOAT, GL_FALSE,
     { emit_3f_xyw_err, emit_3f_xyw_err, emit_3f_xyw_err, 
       emit_3f_xyw_4 } },

   { "1ub_1f", GL_UNSIGNED_BYTE, GL_FALSE,
     { emit_1ub_1f_1, emit_1ub_1f_1, emit_1ub_1f_1, emit_1ub_1f_1 } },

   { "3ub_3f_rgb", GL_UNSIGNED_BYTE, GL_FALSE,
     { emit_3ub_3f_rgb_1, emit_3ub_3f_rgb_2, emit_3ub_3f_rgb_3,
       emit_3ub_3f_rgb_3 } },

   { "3ub_3f_bgr", GL_UNSIGNED_BYTE, GL_FALSE,
     { emit_3ub_3f_bgr_1, emit_3ub_3f_bgr_2, emit_3ub_3f_bgr_3,
       emit_3ub_3f_bgr_3 } },

   { "4ub_4f_rgba", GL_UNSIGNED_BYTE, GL_FALSE,
     { emit_4ub_4f_rgba_1, emit_4ub_4f_rgba_2, emit_4ub_4f_rgba_3, 
       emit_4ub_4f_rgba_4 } },

   { "4ub_4f_bgra", GL_UNSIGNED_BYTE, GL_FALSE,
     { emit_4ub_4f_bgra_1, emit_4ub_4f_bgra_2, emit_4ub_4f_bgra_3,
       emit_4ub_4f_bgra_4 } },

   { "4chan_4f_rgba", CHAN_TYPE, GL_FALSE,
     { emit_4chan_4f_rgba_1, emit_4chan_4f_rgba_2, emit_4chan_4f_rgba_3,
       emit_4chan_4f_rgba_4 } },

   { "pad", 0, 0,
     { 0, 0, 0, 0 } }

};
     

/***********************************************************************
 * list(attrib, size) --> function
 *
 * Because of the dependence of size, this all has to take place after
 * the pipeline has been run.
 */

tnl_emit_func _tnl_codegen_emit( GLcontext *ctx )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   struct tnl_clipspace_codegen *p = &vtx->codegen;
   const GLuint count = vtx->attr_count;
   GLuint j;

   /* Need a faster lookup, or is this linear scan of an MRU list good
    * enough?  MRU chosen based on the guess that consecutive VB's are
    * likely to be of the same format.  A hash of attributes and sizes
    * might be a better technique.
    *
    * With the vtx code now in place, it should be possible to track
    * changes to the sizes of input arrays (and state, of course) and
    * only invalidate this function when those sizes have changed.
    */
#if 0
   foreach (l, p->codegen_list) {
      if (l->attr_count != count) 
	 continue;

      /* Assumptions:  
       *    a[j].vp will not change for a given attrib
       *    a[j].vertex_offset will not change nothing else has changed.
       */
      for (j = 0; j < count; j++) 
	 if (a[j].attrib != l->a[j].attrib ||
	     a[j].sz != l->a[j].sz) 
	    break;
	 
      if (j == count) {
	 move_to_head(l, p->codegen_list);
	 return l->func;
      }
   }
#endif

   p->emit_header( p, vtx );

   for (j = 0; j < count; j++) {
      GLuint sz = VB->AttribPtr[a[j].attrib]->size - 1;
      p->emit_attr_header( p, a, j, 
			   emit_info[a[j].format].out_type,
			   emit_info[a[j].format].need_vp );

      if (!emit_info[a[j].format].emit[sz]( p )) {
	 fprintf(stderr, "codegen failed\n");
	 return 0;
      }

      p->emit_attr_footer( p );
   }
   
   p->emit_footer( p );
   
   return p->emit_store_func( p );
}

