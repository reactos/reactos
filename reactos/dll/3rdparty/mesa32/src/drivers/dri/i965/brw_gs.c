/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
      
#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_gs.h"



static void compile_gs_prog( struct brw_context *brw,
			     struct brw_gs_prog_key *key )
{
   struct brw_gs_compile c;
   const GLuint *program;
   GLuint program_size;

   memset(&c, 0, sizeof(c));
   
   c.key = *key;

   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.nr_attrs = brw_count_bits(c.key.attrs);
   c.nr_regs = (c.nr_attrs + 1) / 2 + 1;  /* are vertices packed, or reg-aligned? */
   c.nr_bytes = c.nr_regs * REG_SIZE;

   
   /* Begin the compilation:
    */
   brw_init_compile(&c.func);

   c.func.single_program_flow = 1;

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.  
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);


   /* Note that primitives which don't require a GS program have
    * already been weeded out by this stage:
    */
   switch (key->primitive) {
   case GL_QUADS:
      brw_gs_quads( &c ); 
      break;
   case GL_QUAD_STRIP:
      brw_gs_quad_strip( &c );
      break;
   case GL_LINE_LOOP:
      brw_gs_lines( &c );
      break;
   case GL_LINES:
      if (key->hint_gs_always)
	 brw_gs_lines( &c );
      else {
	 return;
      }
      break;
   case GL_TRIANGLES:
      if (key->hint_gs_always)
	 brw_gs_tris( &c );
      else {
	 return;
      }
      break;
   case GL_POINTS:
      if (key->hint_gs_always)
	 brw_gs_points( &c );
      else {
	 return;
      }
      break;      
   default:
      return;
   }

   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   /* Upload
    */
   brw->gs.prog_gs_offset = brw_upload_cache( &brw->cache[BRW_GS_PROG],
					      &c.key,
					      sizeof(c.key),
					      program,
					      program_size,
					      &c.prog_data,
					      &brw->gs.prog_data );
}


static GLboolean search_cache( struct brw_context *brw, 
			       struct brw_gs_prog_key *key )
{
   return brw_search_cache(&brw->cache[BRW_GS_PROG], 
			   key, sizeof(*key),
			   &brw->gs.prog_data,
			   &brw->gs.prog_gs_offset);
}


static const GLenum gs_prim[GL_POLYGON+1] = {  
   GL_POINTS,
   GL_LINES,
   GL_LINE_LOOP,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_QUADS,
   GL_QUAD_STRIP,
   GL_TRIANGLES
};

static void populate_key( struct brw_context *brw,
			  struct brw_gs_prog_key *key )
{
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_VS_PROG */
   key->attrs = brw->vs.prog_data->outputs_written;

   /* BRW_NEW_PRIMITIVE */
   key->primitive = gs_prim[brw->primitive];

   key->hint_gs_always = 0;	/* debug code? */

   key->need_gs_prog = (key->hint_gs_always ||
			brw->primitive == GL_QUADS ||
			brw->primitive == GL_QUAD_STRIP ||
			brw->primitive == GL_LINE_LOOP);
}

/* Calculate interpolants for triangle and line rasterization.
 */
static void upload_gs_prog( struct brw_context *brw )
{
   struct brw_gs_prog_key key;

   /* Populate the key:
    */
   populate_key(brw, &key);

   if (brw->gs.prog_active != key.need_gs_prog) {
      brw->state.dirty.cache |= CACHE_NEW_GS_PROG;
      brw->gs.prog_active = key.need_gs_prog;
   }

   if (brw->gs.prog_active) {
      if (!search_cache(brw, &key))
	 compile_gs_prog( brw, &key );
   }
}


const struct brw_tracked_state brw_gs_prog = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_PRIMITIVE,
      .cache = CACHE_NEW_VS_PROG
   },
   .update = upload_gs_prog
};
