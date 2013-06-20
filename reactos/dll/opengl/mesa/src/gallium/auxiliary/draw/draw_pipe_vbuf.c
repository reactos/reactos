/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * \file
 * Vertex buffer drawing stage.
 * 
 * \author Jose Fonseca <jrfonsec@tungstengraphics.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */


#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw_vbuf.h"
#include "draw_private.h"
#include "draw_vertex.h"
#include "draw_pipe.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct vbuf_render *render;
   
   const struct vertex_info *vinfo;
   
   /** Vertex size in bytes */
   unsigned vertex_size;

   struct translate *translate;
   
   /* FIXME: we have no guarantee that 'unsigned' is 32bit */

   /** Vertices in hardware format */
   unsigned *vertices;
   unsigned *vertex_ptr;
   unsigned max_vertices;
   unsigned nr_vertices;
   
   /** Indices */
   ushort *indices;
   unsigned max_indices;
   unsigned nr_indices;

   /* Cache point size somewhere it's address won't change:
    */
   float point_size;

   struct translate_cache *cache;
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct vbuf_stage *
vbuf_stage( struct draw_stage *stage )
{
   assert(stage);
   return (struct vbuf_stage *)stage;
}


static void vbuf_flush_vertices( struct vbuf_stage *vbuf );
static void vbuf_alloc_vertices( struct vbuf_stage *vbuf );


static INLINE boolean 
overflow( void *map, void *ptr, unsigned bytes, unsigned bufsz )
{
   unsigned long used = (unsigned long) ((char *)ptr - (char *)map);
   return (used + bytes) > bufsz;
}


static INLINE void 
check_space( struct vbuf_stage *vbuf, unsigned nr )
{
   if (vbuf->nr_vertices + nr > vbuf->max_vertices ||
       vbuf->nr_indices + nr > vbuf->max_indices)
   {
      vbuf_flush_vertices( vbuf );
      vbuf_alloc_vertices( vbuf );
   }
}




/**
 * Extract the needed fields from post-transformed vertex and emit
 * a hardware(driver) vertex.
 * Recall that the vertices are constructed by the 'draw' module and
 * have a couple of slots at the beginning (1-dword header, 4-dword
 * clip pos) that we ignore here.  We only use the vertex->data[] fields.
 */
static INLINE ushort 
emit_vertex( struct vbuf_stage *vbuf,
             struct vertex_header *vertex )
{
   if (vertex->vertex_id == UNDEFINED_VERTEX_ID && vbuf->vertex_ptr) {
      /* Hmm - vertices are emitted one at a time - better make sure
       * set_buffer is efficient.  Consider a special one-shot mode for
       * translate.
       */
      /* Note: we really do want data[0] here, not data[pos]: 
       */
      vbuf->translate->set_buffer(vbuf->translate, 0, vertex->data[0], 0, ~0);
      vbuf->translate->run(vbuf->translate, 0, 1, 0, vbuf->vertex_ptr);

      if (0) draw_dump_emitted_vertex(vbuf->vinfo, (uint8_t *)vbuf->vertex_ptr);
      
      vbuf->vertex_ptr += vbuf->vertex_size/4;
      vertex->vertex_id = vbuf->nr_vertices++;
   }

   return (ushort)vertex->vertex_id;
}


static void 
vbuf_tri( struct draw_stage *stage,
          struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   check_space( vbuf, 3 );

   for (i = 0; i < 3; i++) {
      vbuf->indices[vbuf->nr_indices++] = emit_vertex( vbuf, prim->v[i] );
   }
}


static void 
vbuf_line( struct draw_stage *stage, 
           struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   check_space( vbuf, 2 );

   for (i = 0; i < 2; i++) {
      vbuf->indices[vbuf->nr_indices++] = emit_vertex( vbuf, prim->v[i] );
   }   
}


static void 
vbuf_point( struct draw_stage *stage, 
            struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   check_space( vbuf, 1 );

   vbuf->indices[vbuf->nr_indices++] = emit_vertex( vbuf, prim->v[0] );
}




/**
 * Set the prim type for subsequent vertices.
 * This may result in a new vertex size.  The existing vbuffer (if any)
 * will be flushed if needed and a new one allocated.
 */
static void
vbuf_start_prim( struct vbuf_stage *vbuf, uint prim )
{
   struct translate_key hw_key;
   unsigned dst_offset;
   unsigned i;

   vbuf->render->set_primitive(vbuf->render, prim);

   /* Must do this after set_primitive() above:
    * 
    * XXX: need some state managment to track when this needs to be
    * recalculated.  The driver should tell us whether there was a
    * state change.
    */
   vbuf->vinfo = vbuf->render->get_vertex_info(vbuf->render);
   vbuf->vertex_size = vbuf->vinfo->size * sizeof(float);

   /* Translate from pipeline vertices to hw vertices.
    */
   dst_offset = 0;

   for (i = 0; i < vbuf->vinfo->num_attribs; i++) {
      unsigned emit_sz = 0;
      unsigned src_buffer = 0;
      enum pipe_format output_format;
      unsigned src_offset = (vbuf->vinfo->attrib[i].src_index * 4 * sizeof(float) );

      output_format = draw_translate_vinfo_format(vbuf->vinfo->attrib[i].emit);
      emit_sz = draw_translate_vinfo_size(vbuf->vinfo->attrib[i].emit);

      /* doesn't handle EMIT_OMIT */
      assert(emit_sz != 0);

      if (vbuf->vinfo->attrib[i].emit == EMIT_1F_PSIZE) {
	 src_buffer = 1;
	 src_offset = 0;
      }

      hw_key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
      hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      hw_key.element[i].input_buffer = src_buffer;
      hw_key.element[i].input_offset = src_offset;
      hw_key.element[i].instance_divisor = 0;
      hw_key.element[i].output_format = output_format;
      hw_key.element[i].output_offset = dst_offset;

      dst_offset += emit_sz;
   }

   hw_key.nr_elements = vbuf->vinfo->num_attribs;
   hw_key.output_stride = vbuf->vinfo->size * 4;

   /* Don't bother with caching at this stage:
    */
   if (!vbuf->translate ||
       translate_key_compare(&vbuf->translate->key, &hw_key) != 0) 
   {
      translate_key_sanitize(&hw_key);
      vbuf->translate = translate_cache_find(vbuf->cache, &hw_key);

      vbuf->translate->set_buffer(vbuf->translate, 1, &vbuf->point_size, 0, ~0);
   }

   vbuf->point_size = vbuf->stage.draw->rasterizer->point_size;

   /* Allocate new buffer?
    */
   assert(vbuf->vertices == NULL);
   vbuf_alloc_vertices(vbuf);
}


static void 
vbuf_first_tri( struct draw_stage *stage,
                struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_vertices( vbuf );
   vbuf_start_prim(vbuf, PIPE_PRIM_TRIANGLES);
   stage->tri = vbuf_tri;
   stage->tri( stage, prim );
}


static void 
vbuf_first_line( struct draw_stage *stage,
                 struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_vertices( vbuf );
   vbuf_start_prim(vbuf, PIPE_PRIM_LINES);
   stage->line = vbuf_line;
   stage->line( stage, prim );
}


static void 
vbuf_first_point( struct draw_stage *stage,
                  struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_vertices(vbuf);
   vbuf_start_prim(vbuf, PIPE_PRIM_POINTS);
   stage->point = vbuf_point;
   stage->point( stage, prim );
}



/**
 * Flush existing vertex buffer and allocate a new one.
 */
static void 
vbuf_flush_vertices( struct vbuf_stage *vbuf )
{
   if(vbuf->vertices) {

      vbuf->render->unmap_vertices( vbuf->render, 0, vbuf->nr_vertices - 1 );

      if (vbuf->nr_indices) 
      {
         vbuf->render->draw_elements(vbuf->render, 
                                     vbuf->indices, 
                                     vbuf->nr_indices );
   
         vbuf->nr_indices = 0;
      }
     
      /* Reset temporary vertices ids */
      if(vbuf->nr_vertices)
	 draw_reset_vertex_ids( vbuf->stage.draw );
      
      /* Free the vertex buffer */
      vbuf->render->release_vertices( vbuf->render );

      vbuf->max_vertices = vbuf->nr_vertices = 0;
      vbuf->vertex_ptr = vbuf->vertices = NULL;
   }

   /* Reset point/line/tri function pointers.
    * If (for example) we transition from points to tris and back to points
    * again, we need to call the vbuf_first_point() function again to flush
    * the triangles before drawing more points.  This can happen when drawing
    * with front polygon mode = filled and back polygon mode = line or point.
    */
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
}
   

static void 
vbuf_alloc_vertices( struct vbuf_stage *vbuf )
{
   if (vbuf->vertex_ptr) {
      assert(!vbuf->nr_indices);
      assert(!vbuf->vertices);
   }
   
   /* Allocate a new vertex buffer */
   vbuf->max_vertices = vbuf->render->max_vertex_buffer_bytes / vbuf->vertex_size;

   if(vbuf->max_vertices >= UNDEFINED_VERTEX_ID)
      vbuf->max_vertices = UNDEFINED_VERTEX_ID - 1;

   /* Must always succeed -- driver gives us a
    * 'max_vertex_buffer_bytes' which it guarantees it can allocate,
    * and it will flush itself if necessary to do so.  If this does
    * fail, we are basically without usable hardware.
    */
   vbuf->render->allocate_vertices(vbuf->render,
                                   (ushort) vbuf->vertex_size,
                                   (ushort) vbuf->max_vertices);

   vbuf->vertices = (uint *) vbuf->render->map_vertices( vbuf->render );
   
   vbuf->vertex_ptr = vbuf->vertices;
}



static void 
vbuf_flush( struct draw_stage *stage, unsigned flags )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_vertices( vbuf );
}


static void 
vbuf_reset_stipple_counter( struct draw_stage *stage )
{
   /* XXX: Need to do something here for hardware with linestipple.
    */
   (void) stage;
}


static void vbuf_destroy( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if(vbuf->indices)
      align_free( vbuf->indices );
   
   if (vbuf->render)
      vbuf->render->destroy( vbuf->render );

   if (vbuf->cache)
      translate_cache_destroy(vbuf->cache);

   FREE( stage );
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *draw_vbuf_stage( struct draw_context *draw,
                                    struct vbuf_render *render )
{
   struct vbuf_stage *vbuf = CALLOC_STRUCT(vbuf_stage);
   if (vbuf == NULL)
      goto fail;
   
   vbuf->stage.draw = draw;
   vbuf->stage.name = "vbuf";
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
   vbuf->stage.flush = vbuf_flush;
   vbuf->stage.reset_stipple_counter = vbuf_reset_stipple_counter;
   vbuf->stage.destroy = vbuf_destroy;
   
   vbuf->render = render;
   vbuf->max_indices = MIN2(render->max_indices, UNDEFINED_VERTEX_ID-1);

   vbuf->indices = (ushort *) align_malloc( vbuf->max_indices * 
					    sizeof(vbuf->indices[0]), 
					    16 );
   if (!vbuf->indices)
      goto fail;

   vbuf->cache = translate_cache_create();
   if (!vbuf->cache) 
      goto fail;
      
   
   vbuf->vertices = NULL;
   vbuf->vertex_ptr = vbuf->vertices;
   
   return &vbuf->stage;

 fail:
   if (vbuf)
      vbuf_destroy(&vbuf->stage);
   
   return NULL;
}
