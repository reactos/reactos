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
 * Post-transform vertex format info.  The vertex_info struct is used by
 * the draw_vbuf code to emit hardware-specific vertex layouts into hw
 * vertex buffers.
 *
 * Author:
 *    Brian Paul
 */


#ifndef DRAW_VERTEX_H
#define DRAW_VERTEX_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_debug.h"


/**
 * Vertex attribute emit modes
 */
enum attrib_emit {
   EMIT_OMIT,      /**< don't emit the attribute */
   EMIT_1F,
   EMIT_1F_PSIZE,  /**< insert constant point size */
   EMIT_2F,
   EMIT_3F,
   EMIT_4F,
   EMIT_4UB, /**< is RGBA like the rest */
   EMIT_4UB_BGRA
};


/**
 * Attribute interpolation mode
 */
enum interp_mode {
   INTERP_NONE,      /**< never interpolate vertex header info */
   INTERP_POS,       /**< special case for frag position */
   INTERP_CONSTANT,
   INTERP_LINEAR,
   INTERP_PERSPECTIVE
};


/**
 * Information about hardware/rasterization vertex layout.
 */
struct vertex_info
{
   uint num_attribs;
   uint hwfmt[4];      /**< hardware format info for this format */
   uint size;          /**< total vertex size in dwords */
   
   /* Keep this small and at the end of the struct to allow quick
    * memcmp() comparisons.
    */
   struct {
      unsigned interp_mode:4;      /**< INTERP_x */
      unsigned emit:4;             /**< EMIT_x */
      unsigned src_index:8;          /**< map to post-xform attribs */
   } attrib[PIPE_MAX_SHADER_INPUTS];
};

static INLINE size_t
draw_vinfo_size( const struct vertex_info *a )
{
   return offsetof(const struct vertex_info, attrib[a->num_attribs]);
}

static INLINE int
draw_vinfo_compare( const struct vertex_info *a,
                    const struct vertex_info *b )
{
   size_t sizea = draw_vinfo_size( a );
   return memcmp( a, b, sizea );
}

static INLINE void
draw_vinfo_copy( struct vertex_info *dst,
                 const struct vertex_info *src )
{
   size_t size = draw_vinfo_size( src );
   memcpy( dst, src, size );
}



/**
 * Add another attribute to the given vertex_info object.
 * \param src_index  indicates which post-transformed vertex attrib slot
 *                   corresponds to this attribute.
 * \return slot in which the attribute was added
 */
static INLINE uint
draw_emit_vertex_attr(struct vertex_info *vinfo,
                      enum attrib_emit emit, 
                      enum interp_mode interp, /* only used by softpipe??? */
                      uint src_index)
{
   const uint n = vinfo->num_attribs;
   assert(n < PIPE_MAX_SHADER_INPUTS);
   vinfo->attrib[n].emit = emit;
   vinfo->attrib[n].interp_mode = interp;
   vinfo->attrib[n].src_index = src_index;
   vinfo->num_attribs++;
   return n;
}


extern void draw_compute_vertex_size(struct vertex_info *vinfo);

void draw_dump_emitted_vertex(const struct vertex_info *vinfo, 
                              const uint8_t *data);


static INLINE enum pipe_format draw_translate_vinfo_format(enum attrib_emit emit)
{
   switch (emit) {
   case EMIT_OMIT:
      return PIPE_FORMAT_NONE;
   case EMIT_1F:
   case EMIT_1F_PSIZE:
      return PIPE_FORMAT_R32_FLOAT;
   case EMIT_2F:
      return PIPE_FORMAT_R32G32_FLOAT;
   case EMIT_3F:
      return PIPE_FORMAT_R32G32B32_FLOAT;
   case EMIT_4F:
      return PIPE_FORMAT_R32G32B32A32_FLOAT;
   case EMIT_4UB:
      return PIPE_FORMAT_R8G8B8A8_UNORM;
   case EMIT_4UB_BGRA:
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   default:
      assert(!"unexpected format");
      return PIPE_FORMAT_NONE;
   }
}

static INLINE unsigned draw_translate_vinfo_size(enum attrib_emit emit)
{
   switch (emit) {
   case EMIT_OMIT:
      return 0;
   case EMIT_1F:
   case EMIT_1F_PSIZE:
      return 1 * sizeof(float);
   case EMIT_2F:
      return 2 * sizeof(float);
   case EMIT_3F:
      return 3 * sizeof(float);
   case EMIT_4F:
      return 4 * sizeof(float);
   case EMIT_4UB:
      return 4 * sizeof(unsigned char);
   case EMIT_4UB_BGRA:
      return 4 * sizeof(unsigned char);
   default:
      assert(!"unexpected format");
      return 0;
   }
}

#endif /* DRAW_VERTEX_H */
