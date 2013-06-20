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

/*
 * Functions for specifying the post-transformation vertex layout.
 *
 * Author:
 *    Brian Paul
 *    Keith Whitwell
 */


#include "draw/draw_private.h"
#include "draw/draw_vertex.h"


/**
 * Compute the size of a vertex, in dwords/floats, to update the
 * vinfo->size field.
 */
void
draw_compute_vertex_size(struct vertex_info *vinfo)
{
   uint i;

   vinfo->size = 0;
   for (i = 0; i < vinfo->num_attribs; i++)
      vinfo->size += draw_translate_vinfo_size(vinfo->attrib[i].emit);

   assert(vinfo->size % 4 == 0);
   /* in dwords */
   vinfo->size /= 4;
}


void
draw_dump_emitted_vertex(const struct vertex_info *vinfo, const uint8_t *data)
{
   unsigned i;

   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->attrib[i].emit) {
      case EMIT_OMIT:
         debug_printf("EMIT_OMIT:");
         break;
      case EMIT_1F:
         debug_printf("EMIT_1F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_1F_PSIZE:
         debug_printf("EMIT_1F_PSIZE:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_2F:
         debug_printf("EMIT_2F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_3F:
         debug_printf("EMIT_3F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         data += sizeof(float);
         break;
      case EMIT_4F:
         debug_printf("EMIT_4F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_4UB:
         debug_printf("EMIT_4UB:\t");
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         break;
      case EMIT_4UB_BGRA:
         debug_printf("EMIT_4UB_BGRA:\t");
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         break;
      default:
         assert(0);
      }
      debug_printf("\n");
   }
   debug_printf("\n");
}
