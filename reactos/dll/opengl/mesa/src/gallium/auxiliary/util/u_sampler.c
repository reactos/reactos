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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "u_format.h"
#include "u_sampler.h"


/**
 * Initialize a pipe_sampler_view.  'view' is considered to have
 * uninitialized contents.
 */
static void
default_template(struct pipe_sampler_view *view,
                 const struct pipe_resource *texture,
                 enum pipe_format format,
                 unsigned expand_green_blue)
{
   memset(view, 0, sizeof(*view));

   /* XXX: Check if format is compatible with texture->format.
    */

   view->format = format;
   view->u.tex.first_level = 0;
   view->u.tex.last_level = texture->last_level;
   view->u.tex.first_layer = 0;
   view->u.tex.last_layer = texture->target == PIPE_TEXTURE_3D ?
                               texture->depth0 - 1 : texture->array_size - 1;
   view->swizzle_r = PIPE_SWIZZLE_RED;
   view->swizzle_g = PIPE_SWIZZLE_GREEN;
   view->swizzle_b = PIPE_SWIZZLE_BLUE;
   view->swizzle_a = PIPE_SWIZZLE_ALPHA;

   /* Override default green and blue component expansion to the requested
    * one.
    *
    * Gallium expands nonexistent components to (0,0,0,1), DX9 expands
    * to (1,1,1,1).  Since alpha is always expanded to 1, and red is
    * always present, we only really care about green and blue
    * components.
    *
    * To make it look less hackish, one would have to add
    * UTIL_FORMAT_SWIZZLE_EXPAND to indicate components for expansion
    * and then override without exceptions or favoring one component
    * over another.
    */
   if (format != PIPE_FORMAT_A8_UNORM) {
      const struct util_format_description *desc = util_format_description(format);

      assert(desc);
      if (desc) {
         if (desc->swizzle[1] == UTIL_FORMAT_SWIZZLE_0) {
            view->swizzle_g = expand_green_blue;
         }
         if (desc->swizzle[2] == UTIL_FORMAT_SWIZZLE_0) {
            view->swizzle_b = expand_green_blue;
         }
      }
   }
}

void
u_sampler_view_default_template(struct pipe_sampler_view *view,
                                const struct pipe_resource *texture,
                                enum pipe_format format)
{
   /* Expand to (0, 0, 0, 1) */
   default_template(view,
                    texture,
                    format,
                    PIPE_SWIZZLE_ZERO);
}

void
u_sampler_view_default_dx9_template(struct pipe_sampler_view *view,
                                    const struct pipe_resource *texture,
                                    enum pipe_format format)
{
   /* Expand to (1, 1, 1, 1) */
   default_template(view,
                    texture,
                    format,
                    PIPE_SWIZZLE_ONE);
}
