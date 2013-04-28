/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/glheader.h"
#include "main/context.h"

#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bitmap.h"
#include "st_program.h"
#include "st_manager.h"


/**
 * This is used to initialize st->atoms[].
 */
static const struct st_tracked_state *atoms[] =
{
   &st_update_depth_stencil_alpha,
   &st_update_clip,

   &st_finalize_textures,
   &st_update_fp,
   &st_update_gp,
   &st_update_vp,

   &st_update_rasterizer,
   &st_update_polygon_stipple,
   &st_update_viewport,
   &st_update_scissor,
   &st_update_blend,
   &st_update_sampler,
   &st_update_vertex_texture,
   &st_update_texture,
   &st_update_framebuffer,
   &st_update_msaa,
   &st_update_vs_constants,
   &st_update_gs_constants,
   &st_update_fs_constants,
   &st_update_pixel_transfer
};


void st_init_atoms( struct st_context *st )
{
   /* no-op */
}


void st_destroy_atoms( struct st_context *st )
{
   /* no-op */
}


/***********************************************************************
 */

static GLboolean check_state( const struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   return ((a->mesa & b->mesa) ||
	   (a->st & b->st));
}

static void accumulate_state( struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   a->mesa |= b->mesa;
   a->st |= b->st;
}


static void xor_states( struct st_state_flags *result,
			     const struct st_state_flags *a,
			      const struct st_state_flags *b )
{
   result->mesa = a->mesa ^ b->mesa;
   result->st = a->st ^ b->st;
}


/* Too complex to figure out, just check every time:
 */
static void check_program_state( struct st_context *st )
{
   struct gl_context *ctx = st->ctx;

   if (ctx->VertexProgram._Current != &st->vp->Base)
      st->dirty.st |= ST_NEW_VERTEX_PROGRAM;

   if (ctx->FragmentProgram._Current != &st->fp->Base)
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;

   if (ctx->GeometryProgram._Current != &st->gp->Base)
      st->dirty.st |= ST_NEW_GEOMETRY_PROGRAM;
}


/***********************************************************************
 * Update all derived state:
 */

void st_validate_state( struct st_context *st )
{
   struct st_state_flags *state = &st->dirty;
   GLuint i;

   /* The bitmap cache is immune to pixel unpack changes.
    * Note that GLUT makes several calls to glPixelStore for each
    * bitmap char it draws so this is an important check.
    */
   if (state->mesa & ~_NEW_PACKUNPACK)
      st_flush_bitmap_cache(st);

   check_program_state( st );

   st_manager_validate_framebuffers(st);

   if (state->st == 0)
      return;

   /*printf("%s %x/%x\n", __FUNCTION__, state->mesa, state->st);*/

#ifdef NDEBUG
   if (0) {
#else
   if (1) {
#endif
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct st_state_flags examined, prev;      
      memset(&examined, 0, sizeof(examined));
      prev = *state;

      for (i = 0; i < Elements(atoms); i++) {	 
	 const struct st_tracked_state *atom = atoms[i];
	 struct st_state_flags generated;
	 
	 /*printf("atom %s %x/%x\n", atom->name, atom->dirty.mesa, atom->dirty.st);*/

	 if (!(atom->dirty.mesa || atom->dirty.st) ||
	     !atom->update) {
	    printf("malformed atom %s\n", atom->name);
	    assert(0);
	 }

	 if (check_state(state, &atom->dirty)) {
	    atoms[i]->update( st );
	    /*printf("after: %x\n", atom->dirty.mesa);*/
	 }

	 accumulate_state(&examined, &atom->dirty);

	 /* generated = (prev ^ state)
	  * if (examined & generated)
	  *     fail;
	  */
	 xor_states(&generated, &prev, state);
	 assert(!check_state(&examined, &generated));
	 prev = *state;
      }
      /*printf("\n");*/

   }
   else {
      for (i = 0; i < Elements(atoms); i++) {	 
	 if (check_state(state, &atoms[i]->dirty))
	    atoms[i]->update( st );
      }
   }

   memset(state, 0, sizeof(*state));
}



