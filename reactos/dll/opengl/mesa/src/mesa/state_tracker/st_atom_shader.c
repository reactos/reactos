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

/**
 * State validation for vertex/fragment shaders.
 * Note that we have to delay most vertex/fragment shader translation
 * until rendering time since the linkage between the vertex outputs and
 * fragment inputs can vary depending on the pairing of shaders.
 *
 * Authors:
 *   Brian Paul
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "program/program.h"

#include "pipe/p_context.h"

#include "util/u_simple_shaders.h"

#include "cso_cache/cso_context.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_program.h"


/**
 * Return pointer to a pass-through fragment shader.
 * This shader is used when a texture is missing/incomplete.
 */
static void *
get_passthrough_fs(struct st_context *st)
{
   if (!st->passthrough_fs) {
      st->passthrough_fs =
         util_make_fragment_passthrough_shader(st->pipe);
   }

   return st->passthrough_fs;
}


/**
 * Update fragment program state/atom.  This involves translating the
 * Mesa fragment program into a gallium fragment program and binding it.
 */
static void
update_fp( struct st_context *st )
{
   struct st_fragment_program *stfp;
   struct st_fp_variant_key key;

   assert(st->ctx->FragmentProgram._Current);
   stfp = st_fragment_program(st->ctx->FragmentProgram._Current);
   assert(stfp->Base.Base.Target == GL_FRAGMENT_PROGRAM_ARB);

   memset(&key, 0, sizeof(key));
   key.st = st;

   st->fp_variant = st_get_fp_variant(st, stfp, &key);

   st_reference_fragprog(st, &st->fp, stfp);

   if (st->missing_textures) {
      /* use a pass-through frag shader that uses no textures */
      void *fs = get_passthrough_fs(st);
      cso_set_fragment_shader_handle(st->cso_context, fs);
   }
   else {
      cso_set_fragment_shader_handle(st->cso_context,
                                     st->fp_variant->driver_shader);
   }
}


const struct st_tracked_state st_update_fp = {
   "st_update_fp",					/* name */
   {							/* dirty */
      0,						/* mesa */
      ST_NEW_FRAGMENT_PROGRAM                           /* st */
   },
   update_fp  					/* update */
};



/**
 * Update vertex program state/atom.  This involves translating the
 * Mesa vertex program into a gallium fragment program and binding it.
 */
static void
update_vp( struct st_context *st )
{
   struct st_vertex_program *stvp;
   struct st_vp_variant_key key;

   /* find active shader and params -- Should be covered by
    * ST_NEW_VERTEX_PROGRAM
    */
   assert(st->ctx->VertexProgram._Current);
   stvp = st_vertex_program(st->ctx->VertexProgram._Current);
   assert(stvp->Base.Base.Target == GL_VERTEX_PROGRAM_ARB);

   memset(&key, 0, sizeof key);
   key.st = st;  /* variants are per-context */

   /* When this is true, we will add an extra input to the vertex
    * shader translation (for edgeflags), an extra output with
    * edgeflag semantics, and extend the vertex shader to pass through
    * the input to the output.  We'll need to use similar logic to set
    * up the extra vertex_element input for edgeflags.
    * _NEW_POLYGON, ST_NEW_EDGEFLAGS_DATA
    */
   key.passthrough_edgeflags = (st->vertdata_edgeflags && (
                                st->ctx->Polygon.FrontMode != GL_FILL ||
                                st->ctx->Polygon.BackMode != GL_FILL));

   st->vp_variant = st_get_vp_variant(st, stvp, &key);

   st_reference_vertprog(st, &st->vp, stvp);

   cso_set_vertex_shader_handle(st->cso_context, 
                                st->vp_variant->driver_shader);

   st->vertex_result_to_slot = stvp->result_to_output;
}


const struct st_tracked_state st_update_vp = {
   "st_update_vp",					/* name */
   {							/* dirty */
      _NEW_POLYGON,					/* mesa */
      ST_NEW_VERTEX_PROGRAM | ST_NEW_EDGEFLAGS_DATA	/* st */
   },
   update_vp						/* update */
};



static void
update_gp( struct st_context *st )
{
   struct st_geometry_program *stgp;
   struct st_gp_variant_key key;

   if (!st->ctx->GeometryProgram._Current) {
      cso_set_geometry_shader_handle(st->cso_context, NULL);
      return;
   }

   stgp = st_geometry_program(st->ctx->GeometryProgram._Current);
   assert(stgp->Base.Base.Target == MESA_GEOMETRY_PROGRAM);

   memset(&key, 0, sizeof(key));
   key.st = st;

   st->gp_variant = st_get_gp_variant(st, stgp, &key);

   st_reference_geomprog(st, &st->gp, stgp);

   cso_set_geometry_shader_handle(st->cso_context,
                                  st->gp_variant->driver_shader);
}

const struct st_tracked_state st_update_gp = {
   "st_update_gp",			/* name */
   {					/* dirty */
      0,				/* mesa */
      ST_NEW_GEOMETRY_PROGRAM           /* st */
   },
   update_gp  				/* update */
};
